#include <cstdlib>
#include <cstdio>
#include <cassert>

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

#include <cute/tensor.hpp>

#include "cutlass/util/print_error.hpp"
#include "cutlass/util/GPU_Clock.hpp"
#include "cutlass/util/helper_cuda.hpp"

// #define _PRINT_DBEUG

template <class ProblemShape, class CtaTiler,
          class TA, class AStride, class ASmemLayout, class AThreadLayout,
          class TB, class BStride, class BSmemLayout, class BThreadLayout,
          class TC, class CStride, class CSmemLayout, class CThreadLayout,
          class Alpha, class Beta>
__global__ static 
__launch_bounds__(decltype(cute::size(CThreadLayout{}))::value) 
void gemm_device(
    ProblemShape shape_MNK, CtaTiler cta_tiler,
    TA const* A, AStride dA, ASmemLayout sA_layout, AThreadLayout tA,
    TB const* B, BStride dB, BSmemLayout sB_layout, BThreadLayout tB,
    TC *      C, CStride dC, CSmemLayout          , CThreadLayout tC,
    Alpha alpha, Beta beta
){
    using namespace cute;

    // ASmemLayout、BSmemLayout 决定每个 CTA 暂存 A/B tile 的共享内存布局。
    // 这个示例不在共享内存中暂存 C；CSmemLayout 只用于编译期检查 C tile 的 shape。
    // AThreadLayout、BThreadLayout 划分 gmem -> smem 的搬运任务；
    // CThreadLayout 划分 smem 上的计算任务以及 gmem C 的写回任务。

    // Preconditions
    // 教学版没有边界 predicate：运行时还要求 M、N、K 分别被 BLK_M、BLK_N、BLK_K 整除，
    // 否则边缘 CTA 或最后一个 K tile 会发生越界访问。
    CUTE_STATIC_ASSERT_V(rank(shape_MNK) == _3{});
    CUTE_STATIC_ASSERT_V(rank(cta_tiler) == _3{});

    // 检查 stride 和shape 是否结构上相容
    // 忽略所有叶子上的具体数字，只比较两者的“嵌套树形结构（profile）”是否完全相同。
    CUTE_STATIC_ASSERT_V(congruent(select<0,2>(shape_MNK), dA));
    CUTE_STATIC_ASSERT_V(congruent(select<1,2>(shape_MNK), dB));
    CUTE_STATIC_ASSERT_V(congruent(select<0,1>(shape_MNK), dC));

    // 检查静态 layout 的 shape 是否和 cta_tiler 对应。
    // is_static 是编译期 type trait；这里没有运行期判断。
    static_assert(is_static<ASmemLayout>::value);
    static_assert(is_static<BSmemLayout>::value);
    static_assert(is_static<CSmemLayout>::value);
    // BLK_M check
    CUTE_STATIC_ASSERT_V(size<0>(cta_tiler) == size<0>(ASmemLayout{}));
    CUTE_STATIC_ASSERT_V(size<0>(cta_tiler) == size<0>(CSmemLayout{}));
    // BLK_N check
    CUTE_STATIC_ASSERT_V(size<1>(cta_tiler) == size<0>(BSmemLayout{}));
    CUTE_STATIC_ASSERT_V(size<1>(cta_tiler) == size<1>(CSmemLayout{}));
    // BLK_K check
    CUTE_STATIC_ASSERT_V(size<2>(cta_tiler) == size<1>(ASmemLayout{}));
    CUTE_STATIC_ASSERT_V(size<2>(cta_tiler) == size<1>(BSmemLayout{}));

    // 检查 Thread layout是否能整除 CTA 的tile
    static_assert(is_static<AThreadLayout>::value);
    static_assert(is_static<BThreadLayout>::value);
    static_assert(is_static<CThreadLayout>::value);
    // 线程数应该要相等，因为无论ABC 都由这一组线程来处理
    CUTE_STATIC_ASSERT_V(size(tA) == size(tB));
    CUTE_STATIC_ASSERT_V(size(tA) == size(tC));
    // 线程layout 应该要能整除 BLOCK layout
    CUTE_STATIC_ASSERT_V(size<0>(cta_tiler) % size<0>(tA) == _0{});
    CUTE_STATIC_ASSERT_V(size<0>(cta_tiler) % size<0>(tC) == _0{});
    CUTE_STATIC_ASSERT_V(size<1>(cta_tiler) % size<0>(tB) == _0{});
    CUTE_STATIC_ASSERT_V(size<1>(cta_tiler) % size<1>(tC) == _0{});
    CUTE_STATIC_ASSERT_V(size<2>(cta_tiler) % size<1>(tA) == _0{});
    CUTE_STATIC_ASSERT_V(size<2>(cta_tiler) % size<1>(tB) == _0{});


    // ---
    // 构建Tensors
    // ---

    // 全局的Matrix
    Tensor mA = make_tensor(make_gmem_ptr(A), select<0,2>(shape_MNK), dA);
    Tensor mB = make_tensor(make_gmem_ptr(B), select<1,2>(shape_MNK), dB);
    Tensor mC = make_tensor(make_gmem_ptr(C), select<0,1>(shape_MNK), dC);

    // 每个 CTA 负责一个 BM x BN 的 C tile。
    // gA/gB 是包含全部 K tiles 的 3-D view，gC 是当前 CTA 的 2-D C view。
    auto cta_coord = make_coord(blockIdx.x, blockIdx.y, _);
    // local_tile(T,B,q) = zipped_divide(T,B)(_,q)：q 固定 Rest mode，保留 Tile mode。
    // 当前实例中的 cta_tiler 是 Shape tuple，因此会对选中的 Tensor modes 做 by-mode divide。
    Tensor gA = local_tile(mA, cta_tiler, cta_coord, Step<_1, X, _1>{}); // BM, BK, k_tiles
    Tensor gB = local_tile(mB, cta_tiler, cta_coord, Step<X, _1, _1>{});
    Tensor gC = local_tile(mC, cta_tiler, cta_coord, Step<_1, _1, X>{}); // BM, BN

    // 分配当前一次 K tile 迭代所需的 shared memory。
    // cosize_v<Layout> 是编译期 variable template，可作为静态数组长度。
    __shared__ TA smemA[cosize_v<ASmemLayout>];
    __shared__ TB smemB[cosize_v<BSmemLayout>];
    Tensor sA = make_tensor(make_smem_ptr(smemA), sA_layout); // BM,BK
    Tensor sB = make_tensor(make_smem_ptr(smemB), sB_layout);


    // ---
    // 切分 copy 任务
    // ---

    // 用 tA 划分 gA -> sA 的搬运任务。
    // local_partition 不会把完整 tA 当作数据 tiler：
    //   B = product_each(shape(tA))             用于数据的 by-mode divide
    //   p = tA.get_flat_coord(threadIdx.x)       用 tA 的 stride 反查 worker 坐标
    //   result = zipped_divide(gA, B)(p, _)
    // (BM,BK,k_tiles) / (T_M,T_K) -> (FRAG_M,FRAG_K,k_tiles)
    Tensor tAgA = local_partition(gA, tA, threadIdx.x); // THR_M, THR_K, k_tiles
    Tensor tAsA = local_partition(sA, tA, threadIdx.x); // THR_M, THR_K
    CUTE_STATIC_ASSERT_V(size<0>(tAgA) == size<0>(tAsA)); // THR_M
    CUTE_STATIC_ASSERT_V(size<1>(tAgA) == size<1>(tAsA)); // THR_K

    // 用 tB 划分 gB -> sB 的搬运任务，过程与 A 相同。
    Tensor tBgB = local_partition(gB, tB, threadIdx.x); // THR_N, THR_K, k_tiles
    Tensor tBsB = local_partition(sB, tB, threadIdx.x); // THR_N, THR_K
    CUTE_STATIC_ASSERT_V(size<0>(tBgB) == size<0>(tBsB)); // THR_N
    CUTE_STATIC_ASSERT_V(size<1>(tBgB) == size<1>(tBsB)); // THR_K


    // --- 
    // 切分计算任务
    // ---

    // 预先用 tC 构造每个线程计算 C 时读取的 sA/sB view，
    // 以及该线程对应的 gC 输出 view；这些操作只构造 view，不搬运数据。
    Tensor tCsA = local_partition(sA, tC, threadIdx.x, Step<_1, X>{}); // THR_M, BK
    Tensor tCsB = local_partition(sB, tC, threadIdx.x, Step<X, _1>{}); // THR_N, BK
    Tensor tCgC = local_partition(gC, tC, threadIdx.x, Step<_1, _1>{}); // THR_M, THR_N

    // C 的累加寄存器
    Tensor tCrC = make_tensor_like(tCgC);

    CUTE_STATIC_ASSERT_V(size<0>(tCrC) == size<0>(tCsA));
    CUTE_STATIC_ASSERT_V(size<0>(tCrC) == size<0>(tCgC));
    CUTE_STATIC_ASSERT_V(size<1>(tCrC) == size<0>(tCsB));
    CUTE_STATIC_ASSERT_V(size<1>(tCrC) == size<1>(tCgC));
    CUTE_STATIC_ASSERT_V(size<1>(tCsA) == size<1>(tCsB));

    clear(tCrC);

#ifdef _PRINT_DBEUG
    if(thread0()) {
        print("     mA: "); print(mA); print("\n");
        print("     gA: "); print(gA); print("\n");
        print("     sA: "); print(sA); print("\n");
        print("   tAgA: "); print(tAgA); print("\n");
        print("   tAsA: "); print(tAsA); print("\n");
    }
#endif

#ifdef _PRINT_DBEUG
    if(thread0()){
        print("     mB: "); print(mB); print("\n");
        print("     gB: "); print(gB); print("\n");
        print("     sB: "); print(sB); print("\n");
        print("   tBgB: "); print(tBgB); print("\n");
        print("   tBsB: "); print(tBsB); print("\n");
    }
#endif


#ifdef _PRINT_DBEUG
    if(thread0()){
        print("     mC: "); print(mC); print("\n");
        print("     gC: "); print(gC); print("\n");
        print("   tCsA: "); print(tCsA); print("\n");
        print("   tCsB: "); print(tCsB); print("\n");
        print("   tCgC: "); print(tCgC); print("\n");
        print("   tCrC: "); print(tCrC); print("\n");
    }
#endif

#ifndef _PRINT_DBEUG

    auto K_TILE_MAX = size<2>(tAgA);

    for(int k_tile = 0; k_tile < K_TILE_MAX; ++k_tile){
        // copy from global mem to smem
        copy(tAgA(_, _, k_tile), tAsA);
        copy(tBgB(_, _, k_tile), tBsB);

        // copy 可能使用 cp.async；以下两个操作的作用域仍是当前线程。
        cp_async_fence();   // 当前线程提交之前发出的 cp.async group（不阻塞）
        cp_async_wait<0>(); // 当前线程等待自己提交的全部 cp.async group
        __syncthreads();    // CTA barrier：确保所有线程都已完成对 smem 的写入

        // THR_M, THR_K, THR_N,THR_K -> THR_M,THR_N
        gemm(tCsA, tCsB, tCrC);

        // CTA barrier：确保所有线程读完当前 smem tile，才能在下一轮覆盖它。
        __syncthreads();
    }
#endif


    // ---
    // EPILOGUE
    // ---

    axpby(alpha, tCrC, beta, tCgC);
};

template<class TA, class TB, class TC,
         class Alpha, class Beta> 
void gemm_nt(int m, int n, int k,
        Alpha alpha,
        TA const* A, int ldA,
        TB const* B, int ldB,
        Beta beta,
        TC *C, int ldC,
        cudaStream_t stream
){
    using namespace cute;

    // global matrix
    auto M = int(m);
    auto N = int(n);
    auto K = int(k);
    auto prob_shape = make_shape(M, N, K);
    auto dA = make_stride(_1{}, ldA);
    auto dB = make_stride(_1{}, ldB);
    auto dC = make_stride(_1{}, ldC);

    // cta tiling
    auto bM = Int<128>{};
    auto bN = Int<128>{};
    auto bK = Int<8>{};
    auto cta_tiler = make_shape(bM, bN, bK);

    // smem layout
    auto sA = make_layout(make_shape(bM, bK));
    auto sB = make_layout(make_shape(bN, bK));
    auto sC = make_layout(make_shape(bM, bN));


    // thread layout
    auto tA = make_layout(make_shape(Int<32>{}, Int<8>{}));
    auto tB = make_layout(make_shape(Int<32>{}, Int<8>{}));
    auto tC = make_layout(make_shape(Int<16>{}, Int<16>{}));

    dim3 block(size(tC));
    dim3 grid(size(ceil_div(M, bM)), size(ceil_div(N, bN)));
    gemm_device<<<grid, block, 0, stream>>>(
        prob_shape, cta_tiler,
        A, dA, sA, tA, 
        B, dB, sB, tB, 
        C, dC, sC, tC,
        alpha, beta 
    );
};

template<class TA, class TB, class TC,
        class Alpha, class Beta>
void gemm_tn(int m, int n, int k,
    Alpha alpha,
    TA const* A, int ldA,
    TB const* B, int ldB,
    Beta beta,
    TC *C, int ldC,
    cudaStream_t stream
){
    using namespace cute;

    auto M = int(m);
    auto N = int(n);
    auto K = int(k);
    auto prob_shape = make_shape(M, N, K);
    auto dA = make_stride(ldA, _1{});
    auto dB = make_stride(ldB, _1{});
    auto dC = make_stride(_1{}, ldC);

    auto bM = Int<128>{};
    auto bN = Int<128>{};
    auto bK = Int<  8>{};
    auto cta_tiler = make_shape(bM, bN, bK); // (BLK_M, BLK_N, BLK_K)

    auto sA = make_layout(make_shape(bM, bK), LayoutRight{});
    auto sB = make_layout(make_shape(bN, bK), LayoutRight{});
    auto sC = make_layout(make_shape(bM, bN));

    auto tA = make_layout(make_shape(Int<32>{}, Int< 8>{}), LayoutRight{});  // (m,k) -> thr_idx; k-major
    auto tB = make_layout(make_shape(Int<32>{}, Int< 8>{}), LayoutRight{});  // (n,k) -> thr_idx; k-major
    auto tC = make_layout(make_shape(Int<16>{}, Int<16>{}));                 // (m,n) -> thr_idx; m-major

    dim3 dimBlock(size(tC));
    dim3 dimGrid(size(ceil_div(M, bM)),
                size(ceil_div(N, bN)));
    gemm_device<<<dimGrid, dimBlock, 0, stream>>>
        (prob_shape, cta_tiler,
        A, dA, sA, tA,
        B, dB, sB, tB,
        C, dC, sC, tC,
        alpha, beta);
}

template <class TA, class TB, class TC,
          class Alpha, class Beta>
void
gemm(char transA, char transB, int m, int n, int k,
     Alpha alpha,
     TA const* A, int ldA,
     TB const* B, int ldB,
     Beta beta,
     TC      * C, int ldC,
     cudaStream_t stream = 0)
{
    if (transA == 'N' && transB == 'T') {
        return gemm_nt(m, n, k, alpha, A, ldA, B, ldB, beta, C, ldC, stream);
    } else 
    if (transA == 'T' && transB == 'N') {
        return gemm_tn(m, n, k, alpha, A, ldA, B, ldB, beta, C, ldC, stream);
    }
    assert(false && "Not implemented");
}

int main(int argc, char** argv) {
    int m = 5120;
    if (argc >= 2)
        sscanf(argv[1], "%d", &m);

    int n = 5120;
    if (argc >= 3)
        sscanf(argv[2], "%d", &n);

    int k = 4096;
    if (argc >= 4)
        sscanf(argv[3], "%d", &k);

    char transA = 'N';
    if (argc >= 5)
        sscanf(argv[4], "%c", &transA);

    char transB = 'T';
    if (argc >= 6)
        sscanf(argv[5], "%c", &transB);

    using TA = float;
    using TB = float;
    using TC = float;
    using TI = float;

    TI alpha = 1.0;
    TI beta  = 0.0;

    std::cout << "M = " << m << std::endl;
    std::cout << "N = " << n << std::endl;
    std::cout << "K = " << k << std::endl;
    std::cout << "C = A^" << transA << " B^" << transB << std::endl;

    cute::device_init(0);

    thrust::host_vector<TA> h_A(m*k);
    thrust::host_vector<TB> h_B(n*k);
    thrust::host_vector<TC> h_C(m*n);

    for (int j = 0; j < m*k; ++j) h_A[j] = static_cast<TA>( 2*(rand() / double(RAND_MAX)) - 1 );
    for (int j = 0; j < n*k; ++j) h_B[j] = static_cast<TB>( 2*(rand() / double(RAND_MAX)) - 1 );
    for (int j = 0; j < m*n; ++j) h_C[j] = static_cast<TC>(-1);

    thrust::device_vector<TA> d_A = h_A;
    thrust::device_vector<TB> d_B = h_B;
    thrust::device_vector<TC> d_C = h_C;

    int ldA = 0, ldB = 0, ldC = m;

    if (transA == 'N') {
        ldA = m;
    } else if (transA == 'T'){
        ldA = k;
    } else {
        assert(false);
    }

    if (transB == 'N') {
        ldB = k;
    } else if (transB == 'T') {
        ldB = n;
    } else {
        assert(false);
    }

    d_C = h_C;
    gemm(transA, transB, m, n, k,
        alpha,
        d_A.data().get(), ldA,
        d_B.data().get(), ldB,
        beta,
        d_C.data().get(), ldC);
    CUTE_CHECK_LAST();
    // 将结果取回便于调试；当前示例没有与 CPU/cuBLAS reference 做数值比较。
    thrust::host_vector<TC> cute_result = d_C;

    double gflops = (2.0*m*n*k) * 1e-9;
    const int timing_iterations = 100;
    GPU_Clock timer;

    // Timing iterations
    timer.start();
    for (int i = 0; i < timing_iterations; ++i) {
        gemm(transA, transB, m, n, k,
            alpha,
            d_A.data().get(), ldA,
            d_B.data().get(), ldB,
            beta,
            d_C.data().get(), ldC);
    }
    double cute_time = timer.seconds() / timing_iterations;
    CUTE_CHECK_LAST();
    printf("CUTE_GEMM:     [%6.1f]GFlop/s  (%6.4f)ms\n", gflops / cute_time, cute_time*1000);
    return 0;
}
