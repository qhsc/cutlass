// 0t_mma_atom.md 配套(TiledMMA): 印证 make_tiled_mma 内部如何把 atom 铺成大 tile。
// 用 Volta SM70_8x8x4_F32F16F16F32_NT 为例(atom 8 线程, 物理 {0-3,16-19})。
//
// 核心链条(源码 mma_atom.hpp):
//   TiledMMA 内部 = tiled_product(AtomThrID, AtomLayoutMNK)  -> ThrLayoutVMNK (4维 V,M,N,K)
//   tiled_product = logical_product 再 mode 重排;logical_product(A,B) = (A, complement(A)∘B)
//   complement 负责算出 atom 线程留下的"空缺", ∘B 把复制品填进去 -> 不重不漏铺满线程空间。
#include <cute/tensor.hpp>
#include <cute/atom/mma_atom.hpp>
#include <cute/arch/mma_sm70.hpp>
#include <cute/atom/mma_traits_sm70.hpp>
#include <cstdio>

using namespace cute;

template <class L>
void dump_1d(const char* tag, L const& l) {
    print("%s = ", tag); print(l); print("  size=%d\n", int(size(l)));
}

// ---------------------------------------------------------------------------
namespace ex_expand {
// 印证: 8 个散线程 {0-3,16-19} 如何被 tiled_product 铺成连续 32。
void run()
{
    print("=== 1) tiled_product 把 8 线程铺成 32 ===\n");
    using Op = SM70_8x8x4_F32F16F16F32_NT;
    auto atom_thrid = typename MMA_Traits<Op>::ThrID{};   // 直接取 CuTe 定义的 ThrID(=SM70_QuadPair)
    dump_1d("AtomThrID", atom_thrid);
    print("  atom 8 线程物理: ");
    for (int i=0;i<8;++i) print("%d ", atom_thrid(i));
    print("(缺 4-15, 20-31)\n\n");

    auto atom_layout = Layout<Shape<_2,_2>, Stride<_2,_1>>{};   // 参数②: 2x2 atom 排布
    dump_1d("AtomLayoutMN", atom_layout);

    // (a) 手动 logical_product: 看 (A, A*∘B) 原始结构
    auto lp = logical_product(atom_thrid, atom_layout);
    dump_1d("logical_product ", lp);
    // (b) complement 单独看: atom 线程留下的空缺怎么排
    auto comp = complement(atom_thrid, Int<32>{});
    dump_1d("complement(atom,32)", comp);
    // (c) tiled_product: TiledMMA 真正用的
    auto tp = tiled_product(atom_thrid, atom_layout);
    dump_1d("tiled_product   ", tp);

    print("  tiled_product 逐个求值 (= 32 个物理线程):\n");
    for (int i=0;i<int(size(tp));++i) { print("%3d", tp(i)); if (i%8==7) print("\n"); }
    print("  -> 四象限各是原 atom 一份拷贝, 偏移 +0/+4/+8/+12 正好填满空缺\n\n");
}
} // namespace ex_expand

// ---------------------------------------------------------------------------
namespace ex_tmma {
// 印证: make_tiled_mma 造出的 TiledMMA, 看它的 ThrLayoutVMNK 和 tile 尺寸。
void run()
{
    print("=== 2) make_tiled_mma 的三种配置对比 ===\n");
    using Op = SM70_8x8x4_F32F16F16F32_NT;

    // (a) 1x1x1: 单 atom, 原始 8x8x4
    auto mma1 = make_tiled_mma(Op{}, Layout<Shape<_1,_1,_1>>{});
    print("--- 1x1x1 (单 atom) ---\n");
    print("  ThrLayoutVMNK = "); print(mma1.get_thr_layout_vmnk()); print("\n");
    print("  线程数 size = %d\n\n", int(size(mma1.get_thr_layout_vmnk())));

    // (b) 2x2 atom: 32 线程, 16x16x4
    auto mma2 = make_tiled_mma(Op{}, Layout<Shape<_2,_2,_1>>{});
    print("--- 2x2x1 atom (加线程) ---\n");
    print("  ThrLayoutVMNK = "); print(mma2.get_thr_layout_vmnk()); print("\n");
    print("  线程数 size = %d  (8 atom内 x 4 份 = 32 = 一个 warp)\n\n",
          int(size(mma2.get_thr_layout_vmnk())));

    // (c) 2x2 atom + Tile<32,32,4>: 线程仍 32, 但每线程多拿值 (加值不加线程)
    auto mma3 = make_tiled_mma(Op{}, Layout<Shape<_2,_2,_1>>{}, Tile<_32,_32,_4>{});
    print("--- 2x2x1 atom + Tile<32,32,4> (加值) ---\n");
    print("  ThrLayoutVMNK = "); print(mma3.get_thr_layout_vmnk()); print("\n");
    print("  线程数 size = %d  (仍 32! tile 撑大靠加值, 非加线程)\n\n",
          int(size(mma3.get_thr_layout_vmnk())));
}
} // namespace ex_tmma

// ---------------------------------------------------------------------------
namespace ex_ctv {
// 印证: TiledMMA 的 C 分区 layout (thr,val)->(m,n), 以及某线程负责哪些 C 元素。
void run()
{
    print("=== 3) TiledMMA 的 C partition: (thr_idx,val)->(m,n) ===\n");
    using Op = SM70_8x8x4_F32F16F16F32_NT;
    auto mma = make_tiled_mma(Op{}, Layout<Shape<_2,_2,_1>>{});   // 16x16x4

    auto ctv = mma.get_layoutC_TV();   // (thr,val) -> C 内一维 idx
    print("LayoutC_TV = "); print(ctv); print("\n");
    int M = 16;  // 16x16 C, 编码 m + n*16
    for (int t : {0, 1, 4, 8, 12}) {     // 抽查几个物理线程
        print("  线程 %2d 负责 C 的 (m,n): ", t);
        int nval = size<1>(ctv);
        for (int v=0; v<nval; ++v) {
            int idx = ctv(t, v); int m = idx % M, n = idx / M;
            print("(%d,%d) ", m, n);
        }
        print("\n");
    }
    print("  -> 线程0/4/8/12 落在 C 不同象限, 印证 2x2 atom 复制铺满 16x16\n");
}
} // namespace ex_ctv

// ---------------------------------------------------------------------------
int main()
{
    ex_expand::run();
    ex_tmma::run();
    ex_ctv::run();
    return 0;
}
