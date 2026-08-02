// 0t_mma_atom.md 配套(Traits 层): ThrID / CLayout / ALayout / BLayout
// 以 Volta HMMA 8x8x4 (SM70) 为例, 全在 host 上 print 验证, 不需要 GPU 跑 MMA。
//
// 核心定义(务必按这个理解, 别用"值/线程沿哪轴"的结果论):
//   - 编码固定、与转置无关: A 的 (m,k)->m+k*M ; C 的 (m,n)->m+n*M ; B 的 (n,k)->n+k*N
//   - 转置(TN/NT)只改硬件图给的 ownership: (T,V)->(m,k)
//   - 求 layout = 找一个 layout 使 layout(T,V) 恰好 == 「(T,V) 拥有的 (m,k)」的编码 idx
//   验证 = 逐 (tid,vid) 求值, 反解回 (m,k) 看对不对。
#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;

// idx -> (行, 列), 列主编码 idx = 行 + 列*行数(rows)
static void decode(int idx, int rows, int& r, int& c) { r = idx % rows; c = idx / rows; }

// ---------------------------------------------------------------------------
namespace ex_thrid {
// ThrID: 逻辑线程号 [0,8) -> warp 物理线程 idx。输入是连续 0..7, 输出才是散的 {0..3,16..19}。
void run()
{
    auto ThrID = Layout<Shape<_4,_2>, Stride<_1,_16>>{};
    print("=== ThrID (逻辑 tid -> 物理 warp idx) ===\n");
    print("ThrID = "); print(ThrID); print("\n");
    print("logical: "); for (int t = 0; t < 8; ++t) print(" %d", t);       print("\n");
    print("physic : "); for (int t = 0; t < 8; ++t) print(" %d", ThrID(t)); print("\n");
    // 期望 physic = 0 1 2 3 16 17 18 19
}
} // namespace ex_thrid

// ---------------------------------------------------------------------------
namespace ex_clayout {
// CLayout (F32 累加器): (T8,V8) -> (m,n), 编码 m+n*8。四种转置下 C 恒不变。
void run()
{
    auto CLayout = Layout<Shape <Shape <_2, _2,_2>, Shape <_2,_2, _2>>,
                          Stride<Stride<_1,_16,_4>, Stride<_8,_2,_32>>>{};
    print("\n=== CLayout SM70_8x8_32b (T8,V8)->(m,n) ===\n");
    print("CLayout = "); print(CLayout); print("\n");

    // 线程维: 固定 V0, 走 T0..T7, 期望 (m,n) = (0,0)(1,0)(0,2)(1,2)(4,0)(5,0)(4,2)(5,2)
    print("固定 V0, T0..T7 -> (m,n):\n");
    for (int t = 0; t < 8; ++t) {
        int idx = CLayout(t, 0), m, n; decode(idx, 8, m, n);
        print("  T%d V0: idx=%2d -> (m,n)=(%d,%d)\n", t, idx, m, n);
    }
    // 值维: 固定 T0, 走 V0..V7, 期望 (0,0)(0,1)(2,0)(2,1)(0,4)(0,5)(2,4)(2,5)
    print("固定 T0, V0..V7 -> (m,n):\n");
    for (int v = 0; v < 8; ++v) {
        int idx = CLayout(0, v), m, n; decode(idx, 8, m, n);
        print("  T0 V%d: idx=%2d -> (m,n)=(%d,%d)\n", v, idx, m, n);
    }
}
} // namespace ex_clayout

// ---------------------------------------------------------------------------
namespace ex_ab {
// ALayout: (T8,V4)->(m,k), 编码 m+k*8。对比 TN(简单) vs NT(层次化)。
// 关键: 同一个「求 layout」定义, 只因 ownership 图不同, 抄出来的 layout 不同。
template <class L>
void dump_ab(const char* tag, L const& layout, char ax /*'m' or 'n'*/)
{
    print("%s = ", tag); print(layout); print("\n");
    // 每个逻辑线程持有 4 个值, 打印它拥有的 4 个 (轴, k)
    for (int t = 0; t < 8; ++t) {
        print("  T%d 拥有: ", t);
        for (int v = 0; v < 4; ++v) {
            int idx = layout(t, v), a, k; decode(idx, 8, a, k);
            print("(%c=%d,k=%d) ", ax, a, k);
        }
        print("\n");
    }
}

void run()
{
    print("\n=== A/B Layout: TN vs NT (T8,V4)->(m,k) ===\n");

    // TN: SM70_8x4_Row = (8,4):(1,8)  —— 每线程 = 一条 K 线
    auto A_TN = Layout<Shape<_8,_4>, Stride<_1,_8>>{};
    print("--- TN (A/B 都是这个) ---\n");
    dump_ab("A_TN", A_TN, 'm');

    // NT: SM70_8x4_Col = ((4,2),4):((8,4),1)  —— 每线程 = 一段 M
    auto A_NT = Layout<Shape <Shape<_4,_2>,_4>,
                       Stride<Stride<_8,_4>,_1>>{};
    print("--- NT (A/B 都是这个) ---\n");
    dump_ab("A_NT", A_NT, 'm');
    // 观察: TN 时 T0 拿 (m=0,k=0..3) 一条 K 线;
    //       NT 时 T0 拿 (m=0..3,k=0) 一段 M, T4 跳到 m=4 (来自 (4,2) 的 2)。
}
} // namespace ex_ab

// ---------------------------------------------------------------------------
int main()
{
    ex_thrid::run();
    ex_clayout::run();
    ex_ab::run();
    return 0;
}
