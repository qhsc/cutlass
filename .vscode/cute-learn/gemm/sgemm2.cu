#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

#include <cute/tensor.hpp>

#include "cutlass/util/print_error.hpp"
#include "cutlass/util/GPU_Clock.hpp"
#include "cutlass/util/helper_cuda.hpp"

#define _PRINT_DEBUG

template<class ProblemShape, class CtaTiler,
    class TA, class AStride, class ASmemLayout, class TiledCopyA,
    class TB, class BStride, class BSmemLayout, class TiledCopyB,
    class TC, class CStride, class CSmemLayout, class TiledMma,
    class Alpha, class Beta>
__global__ static 
__launch_bounds__(cute::size_v<TiledMma>)
void gemm_device(
    ProblemShape shape_MNK, CtaTiler cta_tiler,
    TA const* A, AStride dA, ASmemLayout sA_layout, TiledCopyA copy_a,
    TB const* B, BStride dB, BSmemLayout sB_layout, TiledCopyB copy_b,
    TC *      C, CStride dC, CSmemLayout          , TiledMma   mma,
    Alpha alpha, Beta beta
){
    using namespace cute;
    
    // Matrix check
    CUTE_STATIC_ASSERT_V(rank(shape_MNK) == _3{});
    CUTE_STATIC_ASSERT_V(rank(cta_tiler) == _3{});
    CUTE_STATIC_ASSERT_V(congruent(select<0,2>(shape_MNK), dA));
    CUTE_STATIC_ASSERT_V(congruent(select<1,2>(shape_MNK), dB));
    CUTE_STATIC_ASSERT_V(congruent(select<0,1>(shape_MNK), dC));

    // Smem check
    static_assert(is_static_v<ASmemLayout>);
    static_assert(is_static_v<BSmemLayout>);
    static_assert(is_static_v<CSmemLayout>);
    CUTE_STATIC_ASSERT_V(size<0>(cta_tiler) == size<0>(ASmemLayout{}));
    CUTE_STATIC_ASSERT_V(size<0>(cta_tiler) == size<0>(CSmemLayout{}));
    CUTE_STATIC_ASSERT_V(size<1>(cta_tiler) == size<0>(BSmemLayout{}));
    CUTE_STATIC_ASSERT_V(size<1>(cta_tiler) == size<1>(CSmemLayout{}));
    CUTE_STATIC_ASSERT_V(size<2>(cta_tiler) == size<1>(ASmemLayout{}));
    CUTE_STATIC_ASSERT_V(size<2>(cta_tiler) == size<1>(BSmemLayout{}));

    // Thread check
    static_assert(size_v<TiledCopyA> == size_v<TiledMma>);
    static_assert(size_v<TiledCopyB> == size_v<TiledMma>);


    // Global Matrix
    Tensor mA = make_tensor(make_gmem_ptr(A), select<0,2>(shape_MNK), dA);
    Tensor mB = make_tensor(make_gmem_ptr(B), select<1,2>(shape_MNK), dB);
    Tensor mC = make_tensor(make_gmem_ptr(C), select<1,2>(shape_MNK), dC);

    // Tiled global matrix
    auto cta_coord = make_coord(blockIdx.x, blockIdx.y, _);
    Tensor gA = local_tile(mA, cta_tiler, cta_coord, Step<_1, X, _1>{}); // BM,BK,k
    Tensor gB = local_tile(mB, cta_tiler, cta_coord, Step<X, _1, _1>{}); // BN,BK,k
    Tensor gC = local_tile(mC, cta_tiler, cta_coord, Step<_1, _1, X>{}); // BM,BN

    // Shared memory
    __shared__ TA smemA[size_v<ASmemLayout>];
    __shared__ TB smemB[size_v<BSmemLayout>];
    Tensor sA = make_tensor(make_smem_ptr(smemA), sA_layout);
    Tensor sB = make_tensor(make_smem_ptr(smemB), sB_layout);


    // Copy slice
    ThrCopy thr_copy_a = copy_a.get_slice(threadIdx.x);
    Tensor tAgA = thr_copy_a.partition_S(gA); // CPY,CPY_M,CPY_K,k
    Tensor tAsA = thr_copy_a.partition_D(sA); // CPY,CPY_N,CPY_K
    // Allocate registers
    Tensor tArA = make_fragment_like(tAsA);

    ThrCopy thr_copy_b = copy_b.get_slice(threadIdx.x);
    // CPY 代表一条CopyAtom 内部一次搬的values
    // CPY_M,CPY_N,CPY_K 代表覆盖更大的 Tile，需要重复的次数
    // for (int k = 0; k < CPY_K; ++k) {
    //   for (int n = 0; n < CPY_N; ++n) {
    //     copy_b.call(
    //         tBgB(_, n, k),
    //         tBrB(_, n, k)
    //     );
    //   }
    // }
    Tensor tBgB = thr_copy_b.partition_S(gB); // CPY,CPY_N,CPY_K,k
    Tensor tBsB = thr_copy_b.partition_D(sB); // CPY,CPY_N,CPY_K
    // Allocate registers
    Tensor tBrB = make_fragment_like(tBsB);

    CUTE_STATIC_ASSERT_V(size<1>(tAgA) == size<1>(tAsA));
    CUTE_STATIC_ASSERT_V(size<2>(tAgA) == size<2>(tAsA));
    CUTE_STATIC_ASSERT_V(size<1>(tBgB) == size<1>(tBsB));
    CUTE_STATIC_ASSERT_V(size<2>(tBgB) == size<2>(tBsB));

    // Copy first k tile to register
    copy(copy_a, tAgA(_,_,_,0), tArA);
    copy(copy_b, tBgB(_,_,_,0), tBrB);

    // TiledMma
    ThrMMA thr_mma = mma.get_slice(threadIdx.x);
    // MMA 是一个MMA Atom 调用内部需要的value fragment
    // MMA_M,MMA_N,MMA_K 是当前线程为了覆盖更大的计算tile，
    // 沿着M.N.K 方向重复调用MMA atom 的次数
    // for (int k = 0; k < MMA_K; ++k) {
    //   for (int m = 0; m < MMA_M; ++m) {
    //     for (int n = 0; n < MMA_N; ++n) {
    //       mma.call(
    //           tCsA(_,m,k),
    //           tCsB(_,n,k),
    //           tCrC(_,m,n)
    //       );
    //     }
    //   }
    // }
    Tensor tCsA = thr_mma.partition_A(sA); // MMA,MMA_M,MMA_K
    Tensor tCsB = thr_mma.partition_B(sB); // MMA,MMA_N,MMA_K
    Tensor tCgC = thr_mma.partition_C(gC); // MMA,MMA_M,MMA_N
    // Allocator accumulator
    Tensor tCrC = thr_mma.make_fragment_C(tCgC);
    CUTE_STATIC_ASSERT_V(shape(tCgC) == shape(tCrC));
    CUTE_STATIC_ASSERT_V(size<1>(tCsA) == size<1>(tCgC));
    CUTE_STATIC_ASSERT_V(size<2>(tCsA) == size<2>(tCsB));
    CUTE_STATIC_ASSERT_V(size<1>(tCsB) == size<2>(tCgC));

    // Clear accumulators
    clear(tCrC);


#ifdef _PRINT_DEBUG
  if(thread0()) {
    print("  mA : "); print(  mA); print("\n");
    print("  gA : "); print(  gA); print("\n");
    print("  sA : "); print(  sA); print("\n");
    print("tAgA : "); print(tAgA); print("\n");
    print("tAsA : "); print(tAsA); print("\n");
    print("tArA : "); print(tArA); print("\n");
  }
#endif

#ifdef _PRINT_DEBUG
  if(thread0()) {
    print("  mB : "); print(  mB); print("\n");
    print("  gB : "); print(  gB); print("\n");
    print("  sB : "); print(  sB); print("\n");
    print("tBgB : "); print(tBgB); print("\n");
    print("tBsB : "); print(tBsB); print("\n");
    print("tBrB : "); print(tBrB); print("\n");
  }
#endif

#ifdef _PRINT_DEBUG
  if(thread0()) {
    print("  mC : "); print(  mC); print("\n");
    print("  gC : "); print(  gC); print("\n");
    print("tCsA : "); print(tCsA); print("\n");
    print("tCsB : "); print(tCsB); print("\n");
    print("tCgC : "); print(tCgC); print("\n");
    print("tCrC : "); print(tCrC); print("\n");
  }
#endif

#ifndef _PRINT_DEBUG
    // Mainloop
    auto K_TILE_MAX = size<3>(tAgA);
    for(int k_tile = 0; k_tile < K_TILE_MAX; ++k_tile){
        // 确保所有线程都已经完成shared memory 的使用
        __syncthreads();
        // from register to shared memory
        copy(tArA, tAsA);
        copy(tBrB, tBsB);
        // 确保所有线程都已经完成shared memory 的写入
        __syncthreads();

        // 从 global memory copy 数据到 register, k_tile
        int k_tile_next = (k_tile+1 < K_TILE_MAX) ? k_tile + 1 : k_tile;
        copy(copy_a, tAgA(_,_,_,k_tile_next), tArA);
        copy(copy_b, tBsB(_,_,_,k_tile_next), tBrB);

        gemm(mma, tCsA, tCsB, tCrC);
    }
#endif

    axpby(alpha, tCrC, beta, tCgC);
};

template<class TA, class TB, class TC,
        class Alpha, class Beta>
void gemm_nt(int m, int n, int k,
            Alpha alpha,
            TA const* A, int ldA,
            TB const* B, int ldB,
            Beta beta,
            TC      * C, int ldC,
            cudaStream_t stream)
{
    using namespace cute;

    auto M = int(m);
    auto N = int(n);
    auto K = int(k);
    auto prob_shape = make_shape(M,N,K);

    auto dA = make_stride(_1{}, ldA);
    auto dB = make_stride(_1{}, ldB);
    auto dC = make_stride(_1{}, ldC);

    auto bM = Int<128>{};
    auto bN = Int<128>{};
    auto bK = Int<8>{};

    auto cta_tiler = make_shape(bM, bN, bK);
    
    auto sA = make_layout(make_shape(bM, bK));
    auto sB = make_layout(make_shape(bN, bK));
    auto sC = make_layout(make_shape(bM, bN));

    TiledCopy copyA = make_tiled_copy(
        Copy_Atom<UniversalCopy<uint128_t>, TA>{},
        Layout<Shape<_32, _8>>{}, // thread_layout
        Layout<Shape<_4, _1>>{} // value_layout
    );
    TiledCopy copyB = make_tiled_copy(
        Copy_Atom<UniversalCopy<uint128_t>, TB>{},
        Layout<Shape<_32, _8>>{},
        Layout<Shape<_4, _1>>{}
    );

    TiledMMA mmaC = make_tiled_mma(
        UniversalFMA<TC,TA,TC>{},
        Layout<Shape<_16, _16, _1>>{}
    );

#ifdef _PRINT_DEBUG
    print(copyA);
    print(copyB);
    print(mmaC);
#endif

    dim3 dimBlock(size(mmaC));
    dim3 dimGrid(size(ceil_div(M, bM)),
        size(ceil_div(N, bN)));
    gemm_device<<<dimGrid, dimBlock, 0, stream>>>(
        prob_shape, cta_tiler,
        A, dA, sA, copyA,
        B, dB, sB, copyB,
        C, dC, sC, mmaC,
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
    TC     *  C, int ldC,
    cudaStream_t stream)
{
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
    auto bK = Int<8>{};
    auto cta_tiler = make_shape(bM, bN, bK);

    auto sA = make_layout(make_shape(bM, bK), 
                make_stride(_1{}, bM + _1{}));
    auto sB = make_layout(make_shape(bN, bK),
                make_stride(_1{}, bN + _1{}));
    auto sC = make_layout(make_shape(bM, bN));

    TiledCopy copyA = make_tiled_copy(Copy_Atom<UniversalCopy<TA>, TA>{},
                                    Layout<Shape<_32,_8>,Stride<_8,_1>>{}, // Thr layout 32x8 k-major
                                    Layout<Shape< _1,_1>>{});              // Val layout  1x1
    TiledCopy copyB = make_tiled_copy(Copy_Atom<UniversalCopy<TB>, TB>{},
                                    Layout<Shape<_32,_8>,Stride<_8,_1>>{}, // Thr layout 32x8 k-major
                                    Layout<Shape< _1,_1>>{});              // Val layout  1x1
    TiledMMA mmaC = make_tiled_mma(UniversalFMA<TA,TB,TC>{},
                            Layout<Shape<_16, _16, _1>>{});

    dim3 dimBlock(size(mmaC));
    dim3 dimGrid(size(ceil_div(M, bM)),
                size(ceil_div(N, bN)));
    gemm_device<<<dimGrid, dimBlock, 0, stream>>>
        (prob_shape, cta_tiler,
        A, dA, sA, copyA,
        B, dB, sB, copyB,
        C, dC, sC, mmaC,
        alpha, beta);
};


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


int main(int argc, char** argv)
{
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

  double gflops = (2.0*m*n*k) * 1e-9;

  const int timing_iterations = 100;
  GPU_Clock timer;

  int ldA = 0, ldB = 0, ldC = m;

  if (transA == 'N') {
    ldA = m;
  } else if (transA == 'T') {
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

  // Run once
  d_C = h_C;
  gemm(transA, transB, m, n, k,
       alpha,
       d_A.data().get(), ldA,
       d_B.data().get(), ldB,
       beta,
       d_C.data().get(), ldC);
  CUTE_CHECK_LAST();
  thrust::host_vector<TC> cute_result = d_C;

  return 0;

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
