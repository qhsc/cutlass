// 合并归档: product.cpp, prod2d.cpp, prodtiler.cpp
#include <cute/tensor.hpp>
#include <cstdio>
#include <set>

namespace ex_product {

using namespace cute;
void run()
{
    // 文档 1-D 例子: A=(2,2):(4,1) 复制 6 份 (B=6:1)
    auto A = make_layout(make_shape(_2{},_2{}), make_stride(_4{},_1{}));
    auto B = Layout<_6,_1>{};
    print("A(要复制的tile) = "); print(A); print("   size=%d\n", int(size(A)));
    print("B(复制方式)     = "); print(B); print("\n\n");

    auto Astar = complement(A, size(A)*cosize(B));
    print("① A* = complement(A, 4*6=24) = "); print(Astar); print("\n");
    print("② A*∘B = "); print(composition(Astar, B)); print("\n");
    auto R = logical_product(A, B);
    print("③ A⊗B = "); print(R); print("   (文档: ((2,2),(2,3)):((4,1),(2,8)))\n\n");

    print("结果 mode0 = "); print(layout<0>(R)); print("  <- tile A 本身\n");
    print("结果 mode1 = "); print(layout<1>(R)); print("  <- 6 份复制怎么排\n\n");

    // ---- blocked vs raked product (更直观的 2-D 铺开) ----
    // A = 2x5 行主序 tile, 铺成 3x4 排列
    auto tile = make_layout(make_shape(_2{},_5{}), make_stride(_5{},_1{}));
    auto arr  = make_layout(make_shape(_3{},_4{}));  // 3x4 列主序排列
    print("=== blocked vs raked ===\n");
    print("tile A = 2x5 行主序,  排列 B = 3x4\n");
    print("blocked_product = "); print(blocked_product(tile, arr)); print("\n");
    print("raked_product   = "); print(raked_product(tile, arr)); print("\n");
}
} // namespace ex_product

namespace ex_prod2d {

using namespace cute;
void run()
{
    // tile A = 2x5 行主序;  排列 B = 3x4 列主序
    auto A = make_layout(make_shape(_2{},_5{}), make_stride(_5{},_1{}));
    auto B = make_layout(make_shape(_3{},_4{}), make_stride(_1{},_3{}));
    print("tile A = "); print(A); print("   (2x5 行主序)\n");
    print("排列 B = "); print(B); print("   (3x4 列主序)\n\n");

    // blocked_product 内部：对 A、B 逐 mode 做 1-D logical_product 再重组
    // mode0(行方向): A行=2:5, B行=3:1
    print("=== 逐 mode 看(rank-sensitive 重组前) ===\n");
    print("行方向: A0=2:5, B0=3:1\n");
    print("列方向: A1=5:1, B1=4:3\n\n");

    auto blk_logic = logical_product(A,B);
    print("blk_logic = "); print(blk_logic); print("\n");

    auto blk_zip = zipped_product(A,B);
    print("blk_zip = "); print(blk_zip); print("\n");

    auto blk_flat = flat_product(A,B);
    print("blk_flat = "); print(blk_flat); print("\n");

    auto blk_til = tiled_product(A,B);
    print("blk_til = "); print(blk_til); print("\n");

    auto blk = blocked_product(A, B);
    print("blocked_product = "); print(blk); print("\n");
    print("  形状 = "); print(shape(blk));
    print("   = ((TileRow,ArrRow),(TileCol,ArrCol)) 每维=(瓦片内, 瓦片阵列)\n\n");

    // 打印整个铺开结果的 offset 图 (最直观)
    print("=== 完整 offset 图 (行=6, 列=20) ===\n");
    auto R = blk;
    int nr = size<0>(R), nc = size<1>(R);
    print("尺寸 %d x %d\n", nr, nc);
    for (int i=0;i<nr;i++){
        print("  ");
        for (int j=0;j<nc;j++) print("%4d", int(R(i,j)));
        print("\n");
    }
}
} // namespace ex_prod2d

namespace ex_prodtiler {

using namespace cute;
void run()
{
    auto A = make_layout(make_shape(_2{},_5{}), make_stride(_5{},_1{}));

    // 情况1: tiler 是单个 layout (简单) -> logical 和 zipped 相同
    auto B1 = make_layout(make_shape(_3{},_4{}), make_stride(_1{},_3{}));
    print("=== 情况1: tiler 单 layout 3x4 ===\n");
    print("logical = "); print(shape(logical_product(A, B1))); print("\n");
    print("zipped  = "); print(shape(zipped_product (A, B1))); print("   <- 相同,tile_unzip 无事可做\n\n");

    // 情况2: tiler 是 by-mode <3, 4> (尖括号) -> logical 交错, zipped 收拢
    auto B2 = make_tile(Layout<_3,_1>{}, Layout<_4,_1>{});
    print("=== 情况2: tiler by-mode <3, 4> ===\n");
    print("logical = "); print(shape(logical_product(A, B2))); print("\n");
    print("zipped  = "); print(shape(zipped_product (A, B2))); print("   <- 不同了!\n");
}
} // namespace ex_prodtiler

namespace ex_prodtable {

using namespace cute;
// 复现文档那张「四变体表」——前提: 必须用尖括号 Tiler <TileM,TileN> (by-mode)!
// 若用单个 layout B, product 把 A/B 各当一整块, 得 ((整个A),(整个B)), 不逐方向配对, 对不上表。
// by-mode 才会对 A 的每个 mode 分别 product -> 逐方向交错 ((M,TileM),(N,TileN))。
void run()
{
    //  block A = (M,N) = (4,6),  Tiler = <TileM,TileN> = <2:1, 3:1>
    auto A     = Layout<Shape<_4,_6>, Stride<_1,_4>>{};
    auto Tiler = make_tile(Layout<_2,_1>{},    // TileM: M 方向复制 2 份
                           Layout<_3,_1>{});   // TileN: N 方向复制 3 份
    print("block A = (M,N)=(4,6),  Tiler = <2:1, 3:1> (尖括号 by-mode)\n\n");

    print("logical = "); print(logical_product(A, Tiler));
    print("   = ((M,TileM),(N,TileN))   逐方向交错\n");
    print("zipped  = "); print(zipped_product (A, Tiler));
    print("   = ((M,N),(TileM,TileN))   M/N 聚一起、Tile 聚一起\n");
    print("tiled   = "); print(tiled_product  (A, Tiler));
    print("   = ((M,N),TileM,TileN)     第二组拆开\n");
    print("flat    = "); print(flat_product   (A, Tiler));
    print("   = (M,N,TileM,TileN)       全拆平\n");
    print("\n对比: 单 layout B 时 logical=((整个A),(整个B)), 无逐方向配对 (见 prodtiler)\n");
}
} // namespace ex_prodtable

int main() {
  cute::print("\n##### product #####\n");
  ex_product::run();
  cute::print("\n##### prod2d #####\n");
  ex_prod2d::run();
  cute::print("\n##### prodtiler #####\n");
  ex_prodtiler::run();
  cute::print("\n##### prodtable (四变体表, 需尖括号 Tiler) #####\n");
  ex_prodtable::run();
  return 0;
}
