// 04_algorithms.md 配套: copy / fill / clear / axpby / gemm 语义
// 注: dispatch 到 cp.async/MMA 等硬件指令需 GPU; host 上验证的是"朴素语义"。
#include <cute/tensor.hpp>
#include <cute/algorithm/copy.hpp>
#include <cute/algorithm/fill.hpp>
#include <cute/algorithm/clear.hpp>
#include <cute/algorithm/axpby.hpp>
#include <cute/algorithm/gemm.hpp>
#include <cstdio>

using namespace cute;

// 逻辑 dump: 按逻辑线性坐标 t(i) 读, i in [0,size)
template <class T>
void dump(const char* tag, T const& t) {
    print("%s", tag);
    for (int i = 0; i < size(t); ++i) print(" %.0f", double(t(i)));
    print("\n");
}

// 物理 dump: 绕过 layout, 直接按内存地址 data()[j] 读, j in [0,cosize)
template <class T>
void dump_phys(const char* tag, T const& t) {
    print("%s", tag);
    for (int j = 0; j < cosize(t.layout()); ++j) print(" %.0f", double(t.data()[j]));
    print("\n");
}

// ---------------------------------------------------------------------------
namespace ex_copy {
// copy: 按 1-D 线性坐标 dst(i)=src(i) 遍历(列主序)。类型不同 -> dispatch 不同指令。
void run()
{
    Tensor src = make_tensor<float>(Shape<_2,_3>{});   // (_2,_3) owning
    Tensor dst = make_tensor<float>(Shape<_2,_3>{});
    for (int i = 0; i < size(src); ++i) src(i) = float(i + 1);

    copy(src, dst);                                     // 两参:默认实现
    dump("src :", src);
    dump("dst :", dst);   // 应与 src 相同

    // 即使 src/dst layout 不同(列主 vs 行主), copy 按【逻辑坐标】对应搬,不做转置
    Tensor dst_row = make_tensor<float>(Shape<_2,_3>{}, LayoutRight{});
    copy(src, dst_row);
    print("dst_row layout = "); print(dst_row.layout()); print("\n");
    dump     ("dst_row(逻辑 t(i)) :", dst_row);   // 逻辑读: 仍 1..6 (差别被抵消)
    dump_phys("src    (物理 data):", src);        // 列主: 逻辑序==物理序 1 2 3 4 5 6
    dump_phys("dst_row(物理 data):", dst_row);    // 行主: 1 3 5 2 4 6 (逻辑序被 stride 打散)
}
} // namespace ex_copy

// ---------------------------------------------------------------------------
namespace ex_fillclear {
void run()
{
    Tensor t = make_tensor<float>(Shape<_2,_4>{});
    fill(t, 7.0f);   dump("fill 7 :", t);
    clear(t);        dump("clear  :", t);   // 全 0, 常用于累加器初始化
}
} // namespace ex_fillclear

// ---------------------------------------------------------------------------
namespace ex_axpby {
// y = alpha*x + beta*y  (GEMM epilogue: C = alpha*(A@B) + beta*C)
void run()
{
    Tensor x = make_tensor<float>(Shape<_4>{});
    Tensor y = make_tensor<float>(Shape<_4>{});
    for (int i = 0; i < 4; ++i) { x(i) = float(i + 1); y(i) = 10.0f; }

    axpby(2.0f, x, 3.0f, y);   // y = 2*x + 3*y = 2*(1..4)+30
    dump("y=2x+3y:", y);       // 期望 32 34 36 38
}
} // namespace ex_axpby

// ---------------------------------------------------------------------------
namespace ex_gemm {
// gemm(A,B,C): C += A*B, 按 mode 数 dispatch。这里验证 (M,K)x(N,K)=>(M,N)。
// 注意 CuTe 约定: A=(M,K) B=(N,K)(B 也是 N 在前 K 在后!), C=(M,N)。
void run()
{
    // C(2x2) += A(2x3) * B(2x3)^T_over_K
    Tensor A = make_tensor<float>(Shape<_2,_3>{});   // (M=2, K=3)
    Tensor B = make_tensor<float>(Shape<_2,_3>{});   // (N=2, K=3)
    Tensor C = make_tensor<float>(Shape<_2,_2>{});   // (M=2, N=2)
    for (int i = 0; i < size(A); ++i) A(i) = float(i + 1);   // 列主: A(m,k)
    for (int i = 0; i < size(B); ++i) B(i) = float(i + 1);
    clear(C);                                        // 累加器清 0

    gemm(A, B, C);   // C(m,n) += sum_k A(m,k)*B(n,k)

    print("A = "); print(A.layout()); print("\n"); dump("A(线性):", A);
    print("B = "); print(B.layout()); print("\n"); dump("B(线性):", B);
    dump("C=A*B^T:", C);

    // 手算核对 C(m,n)=sum_k A(m,k)B(n,k):
    // A 列主 (2,3): A(0,0)=1 A(1,0)=2 A(0,1)=3 A(1,1)=4 A(0,2)=5 A(1,2)=6
    // C(0,0)=1*1+3*3+5*5=35; C(1,0)=2*1+4*3+6*5=44; 对称 C(0,1)=44 C(1,1)=2*2+4*4+6*6=56
    print("手算期望 C(线性,列主)= 35 44 44 56\n");
}
} // namespace ex_gemm

// ---------------------------------------------------------------------------
int main() {
    print("\n##### copy #####\n");        ex_copy::run();
    print("\n##### fill/clear #####\n");  ex_fillclear::run();
    print("\n##### axpby #####\n");       ex_axpby::run();
    print("\n##### gemm #####\n");        ex_gemm::run();
    return 0;
}
