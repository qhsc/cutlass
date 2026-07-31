#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
{
    // ================= 场景1：切子块 / tiling =================
    // 数据 A：8x8 列主序矩阵 (物理排布)
    auto A = make_layout(make_shape(_8{},_8{}), make_stride(_1{},_8{}));
    print("=== 场景1: 从 8x8 矩阵切一个 4x4 左上角 tile ===\n");
    // B: 想取 4x4 子块，行取前4(步长1)、列取前4(步长1) —— 用 by-mode tiler
    auto tile = composition(A, make_tile(Layout<_4,_1>{}, Layout<_4,_1>{}));
    print("A(8x8列主序) = "); print(A); print("\n");
    print("4x4 tile     = "); print(tile); print("\n");
    print("tile 每格的内存 offset:\n");
    for (int i=0;i<4;i++){ print("  "); for(int j=0;j<4;j++) print("%3d ", int(tile(i,j))); print("\n"); }
    print("  -> 就是 A 左上角 4x4，offset 全部落在原矩阵真实位置\n\n");

    // ================= 场景2：改变遍历顺序 / 重排 =================
    // 数据 A2：8 元素连续向量 (物理: 0,1,2,...,7)
    auto A2 = make_layout(_8{}, _1{});
    print("=== 场景2: 把连续 8 元素按 (4,2):(2,1) 交织顺序访问 ===\n");
    // B2: 交织访问模式——先跳偶数再跳奇数
    auto B2 = make_layout(make_shape(_4{},_2{}), make_stride(_2{},_1{}));
    auto reorder = composition(A2, B2);
    print("A2(连续)     = "); print(A2); print("\n");
    print("reorder      = "); print(reorder); print("\n  访问序: ");
    for (int i=0;i<8;i++) print("%d ", int(reorder(i)));
    print("\n  -> 0 2 4 6 1 3 5 7，遍历顺序被重排(向量化/避bank conflict常用)\n\n");

    // ================= 场景3：线程 <-> 数据分配 (partition) =================
    // 数据 A3：16 元素连续向量
    auto A3 = make_layout(_16{}, _1{});
    print("=== 场景3: 16 个数据分给 4 个线程，每线程拿 4 个 ===\n");
    // 线程布局：4 个线程，stride=4 (线程t负责 t, t+4, t+8, t+12)
    auto thr = make_layout(_4{}, _4{});
    auto thr_data = composition(A3, thr);
    print("每个线程负责的 4 个元素(以线程0为例，取该线程的起点):\n");
    // 用 composition 得到线程维度的起点，再看每线程 stride
    for (int t=0;t<4;t++) {
        print("  线程%d 起点 offset = %d\n", t, int(thr_data(t)));
    }
    print("  -> 线程t 从 offset t 开始，配合内层 stride 取 t,t+4,t+8,t+12\n");
}
