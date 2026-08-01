// 合并归档: divide.cpp, divstep.cpp, divide3d.cpp, tilerest.cpp, tilerstride.cpp, zipped.cpp, zipeq.cpp, flatperm.cpp, allvariants.cpp
#include <cute/tensor.hpp>
#include <cstdio>
#include <set>

namespace ex_divide {

using namespace cute;
void run()
{
    // 文档 1-D 例子: A=(4,2,3):(2,1,8), tiler B=4:2
    auto A = make_layout(make_shape(_4{},_2{},_3{}), make_stride(_2{},_1{},_8{}));
    auto B = Layout<_4,_2>{};

    print("A = "); print(A); print("   size=%d\n", int(size(A)));
    print("B(tiler) = "); print(B); print("\n\n");

    // 分解三步
    auto Bstar = complement(B, size(A));
    print("① B* = complement(B, 24) = "); print(Bstar); print("\n");
    auto BB = make_layout(B, Bstar);
    print("② (B, B*) = "); print(BB); print("\n");
    auto R = logical_divide(A, B);
    print("③ A ⊘ B = "); print(R); print("   (文档: ((2,2),(2,3)):((4,1),(2,8)))\n\n");

    // 结果两个 mode 的含义
    print("结果 rank=%d: mode0=tile内, mode1=tile间\n", int(rank(R)));
    print("  layout<0>(R) = "); print(layout<0>(R)); print("   <- 一个 tile 内部的布局\n");
    print("  layout<1>(R) = "); print(layout<1>(R)); print("   <- 遍历各个 tile\n\n");

    // 逐点：第0个tile的4个元素 vs 直接 A∘B
    print("验证 mode0 == A∘B (tile 本身):\n  ");
    auto AB = composition(A, B);
    for (int i=0;i<4;i++) print("R(%d,0)=%d [A∘B(%d)=%d]  ", i, int(R(i,0)), i, int(AB(i)));
    print("\n\n");

    // 展示所有 6 个 tile 的起点(mode1 遍历 tile)
    print("6 个 tile 的起点 offset (mode1):\n  ");
    for (int t=0;t<6;t++) print("tile%d起点=R(0,%d)=%d  ", t, t, int(R(0,t)));
    print("\n");

    // 2-D divide: 用 make_tile（Tiler，尖括号）而不是 make_layout（拼接）
    auto la = make_layout(make_shape(_9{}, make_shape(_4{}, _8{})),
                          make_stride(Int<59>{}, make_stride(Int<13>{}, _1{})));
    auto tb = make_tile(Layout<_3,_3>{},
                        Layout<Shape<_2,_4>, Stride<_1,_8>>{});
    print("\n\nlayout A = "); print(la);
    print("\ntiler  B = "); print(tb);
    auto R2 = logical_divide(la, tb);
    print("\nlogical_divide(A,B) = "); print(R2);
    print("\n");
}
} // namespace ex_divide

namespace ex_divstep {

using namespace cute;
void run()
{
    // ---- mode-0: A0=9:59, tiler t0=3:3 ----
    auto A0 = Layout<_9,Int<59>>{};
    auto t0 = Layout<_3,_3>{};
    print("=== mode-0: A0=9:59 ⊘ 3:3 ===\n");
    print("  B* = complement(3:3, 9) = "); print(complement(t0, size(A0))); print("\n");
    print("  A0 ∘ B (tile) = "); print(composition(A0, t0)); print("\n");
    print("  logical_divide = "); print(logical_divide(A0, t0)); print("\n\n");

    // ---- mode-1: A1=(4,8):(13,1), tiler t1=(2,4):(1,8) ----
    auto A1 = Layout<Shape<_4,_8>, Stride<Int<13>,_1>>{};
    auto t1 = Layout<Shape<_2,_4>, Stride<_1,_8>>{};
    print("=== mode-1: A1=(4,8):(13,1) ⊘ (2,4):(1,8) ===\n");
    print("  size(A1) = %d\n", int(size(A1)));
    print("  B* = complement((2,4):(1,8), 32) = "); print(complement(t1, size(A1))); print("\n");
    print("  A1 ∘ B (tile) = "); print(composition(A1, t1)); print("\n");
    print("  A1 ∘ B* (rest) = "); print(composition(A1, complement(t1, size(A1)))); print("\n");
    print("  logical_divide = "); print(logical_divide(A1, t1)); print("\n\n");

    // ---- 合起来 by-mode ----
    auto la = make_layout(make_shape(_9{}, make_shape(_4{},_8{})),
                          make_stride(Int<59>{}, make_stride(Int<13>{},_1{})));
    auto tb = make_tile(t0, t1);
    print("=== 合并 by-mode ===\n  R = "); print(logical_divide(la, tb)); print("\n");
}
} // namespace ex_divstep

namespace ex_divide3d {

using namespace cute;
void run()
{
    // 三维 A: (6, 8, 4) 列主序. stride = (1, 6, 48)
    auto A = make_layout(make_shape(_6{}, _8{}, _4{}),
                         make_stride(_1{}, _6{}, _48{}));
    print("A = "); print(A); print("   (6 x 8 x 4 列主序)\n\n");

    // ---- 情况1: tiler 给满 3 维 <2, 4, 2> ----
    auto B3 = make_tile(Layout<_2,_1>{}, Layout<_4,_1>{}, Layout<_2,_1>{});
    print("=== tiler 3 维 <2:1, 4:1, 2:1> (每维都切) ===\n");
    print("logical = "); print(logical_divide(A, B3));
    print("   = ((TileM,RestM),(TileN,RestN),(TileL,RestL))\n");
    print("zipped  = "); print(zipped_divide(A, B3));
    print("   = ((TileM,TileN,TileL),(RestM,RestN,RestL))\n\n");

    // ---- 情况2: tiler 只给 2 维 <2, 4> (第3维不切) ----
    auto B2 = make_tile(Layout<_2,_1>{}, Layout<_4,_1>{});
    print("=== tiler 只 2 维 <2:1, 4:1> (第3维 L 原样保留) ===\n");
    print("logical = "); print(logical_divide(A, B2));
    print("   前两维被切成(Tile,Rest), 第3维 L=4 原样跟在后面\n");
    print("zipped  = "); print(zipped_divide(A, B2));
    print("   = ((TileM,TileN),(RestM,RestN,L)) — L 归到 rest 那一堆\n");
}
} // namespace ex_divide3d

namespace ex_tilerest {

using namespace cute;
void run()
{
    // A: 6x8 列主序矩阵。offset = 行 + 列*6
    auto A = make_layout(make_shape(_6{}, _8{}), make_stride(_1{}, _6{}));
    // Tiler: 每块 2 行 x 4 列
    auto B = make_tile(Layout<_2,_1>{}, Layout<_4,_1>{});

    print("矩阵 A = 6行 x 8列 (列主序, offset=行+列*6)\n");
    print("瓦片大小 = 2行 x 4列\n\n");

    auto ld = logical_divide(A, B);
    print("logical_divide 形状 = "); print(ld);
    
    print("  = ((TileM,RestM),(TileN,RestN))\n");
    print("  TileM=2 (每块2行)  RestM=3 (行方向共3排,因为6/2=3)\n");
    print("  TileN=4 (每块4列)  RestN=2 (列方向共2排,因为8/4=2)\n");
    print("  => 共 3x2 = 6 个瓦片，每个 2x4\n\n");

    auto zd = zipped_divide(A, B);
    print("zipped_divide 形状 = "); print(shape(zd));
    print("  = ((TileM,TileN),(RestM,RestN)) = ((2,4),(3,2))\n");
    print("  mode0=(2,4)=一整个瓦片   mode1=(3,2)=瓦片阵列(3排x2列)\n\n");

    // 画出每个瓦片左上角的 offset
    print("=== 6 个瓦片各自左上角在 A 里的 offset ===\n");
    print("     列瓦片0   列瓦片1\n");
    for (int rm = 0; rm < 3; ++rm) {          // RestM: 行方向第几排瓦片
        print("行瓦片%d: ", rm);
        for (int rn = 0; rn < 2; ++rn) {      // RestN: 列方向第几排瓦片
            // zd(瓦片内坐标0, 瓦片编号(rm,rn)) = 该瓦片左上角 offset
            int off = int(zd(0, make_coord(rm, rn)));
            print("  %3d   ", off);
        }
        print("\n");
    }

    print("\n=== 取出 行瓦片1,列瓦片1 这个瓦片的全部 8 个元素 ===\n");
    // mode0 遍历瓦片内部(2x4=8个), mode1 固定选 (1,1) 这个瓦片
    for (int i = 0; i < 2; ++i) {              // 瓦片内 行
        print("  ");
        for (int j = 0; j < 4; ++j) {          // 瓦片内 列
            int off = int(zd(make_coord(i,j), make_coord(1,1)));
            print("%3d ", off);
        }
        print("\n");
    }
}
} // namespace ex_tilerest

namespace ex_tilerstride {

using namespace cute;

template <class TB>
void demo(const char* tag, TB B) {
    auto A = make_layout(make_shape(_6{}, _8{}), make_stride(_1{}, _6{})); // 6x8列主序
    auto zd = zipped_divide(A, B);
    print("%s\n", tag);
    print("  取 (0,0) 这个瓦片的 8 个元素 offset:\n");
    for (int i=0;i<2;i++){ print("    "); for(int j=0;j<4;j++) print("%3d ", int(zd(make_coord(i,j), 0))); print("\n"); }
    print("\n");
}

void run()
{
    print("矩阵 6x8 列主序: offset = 行 + 列*6。瓦片 2行x4列。\n\n");

    // 列方向 stride=1: 连续取第 0,1,2,3 列
    demo("① 列tiler = 4:1  (每隔1列取,连续)",
         make_tile(Layout<_2,_1>{}, Layout<_4,_1>{}));

    // 列方向 stride=2: 跳着取第 0,2,4,6 列
    demo("② 列tiler = 4:2  (每隔2列取,交错)",
         make_tile(Layout<_2,_1>{}, Layout<_4,_2>{}));

    // 行方向 stride=2: 跳着取第 0,2,4 行... 行只有6，2行stride3更明显
    demo("③ 行tiler = 2:3  (行方向每隔3行取)",
         make_tile(Layout<_2,_3>{}, Layout<_4,_1>{}));
}
} // namespace ex_tilerstride

namespace ex_zipped {

using namespace cute;
void run()
{
    // A: (9, 32) 简单列主序，好看 offset
    auto A = make_layout(make_shape(_9{}, _32{}), make_stride(_1{}, _9{}));
    // Tiler: mode0 取 3、mode1 取 8 （都连续）
    auto B = make_tile(Layout<_3,_1>{}, Layout<_8,_1>{});

    print("A = "); print(A); print("   (9x32 列主序)\n");
    print("Tiler = <3:1, 8:1>  => tile 是 3x8\n\n");

    auto ld = logical_divide(A, B);
    auto zd = zipped_divide(A, B);
    auto td = tiled_divide(A, B);
    auto fd = flat_divide(A, B);

    print("logical_divide = "); print(ld); print("\n");
    print("  形状 ((TileM,RestM),(TileN,RestN)) = "); print(shape(ld)); print("\n\n");
    print("zipped_divide  = "); print(zd); print("\n");
    print("  形状 ((TileM,TileN),(RestM,RestN)) = "); print(shape(zd)); print("\n\n");
    print("tiled_divide   = "); print(td); print("  形状 "); print(shape(td)); print("\n");
    print("flat_divide    = "); print(fd); print("  形状 "); print(shape(fd)); print("\n\n");

    // zipped 的杀手锏：tile 可索引
    print("=== zipped: tile 可索引 ===\n");
    print("layout<0>(zd) = tile 本身布局 = "); print(layout<0>(zd)); print("\n");
    print("第0个tile起点 zd(0, 0) = %d\n", int(zd(0, 0)));
    print("第1个tile起点 zd(0, 1) = %d\n", int(zd(0, 1)));
    print("第(1,2)个tile起点 zd(0, (1,2)) = %d\n", int(zd(0, make_coord(1,2))));
    print("验证 layout<0>(zd) == composition(A,B): ");
    print(composition(A,B)); print("\n");
}
} // namespace ex_zipped

namespace ex_zipeq {

using namespace cute;
void run()
{
    auto A = make_layout(make_shape(_6{}, _8{}), make_stride(_1{}, _6{}));
    auto B = make_tile(Layout<_2,_1>{}, Layout<_4,_1>{});

    auto ld = logical_divide(A, B);   // ((2,3),(4,2))
    auto zd = zipped_divide(A, B);    // ((2,4),(3,2))
    print("logical = "); print(ld); print("\n");
    print("zipped  = "); print(zd); print("\n\n");

    // 二者 size 相同，都是 48 个元素
    print("size(ld)=%d  size(zd)=%d\n\n", int(size(ld)), int(size(zd)));

    // 核心：把同一个 tile 元素，两种方式各自定位，offset 完全一样
    // 目标：行瓦片1、列瓦片0 这个瓦片里的 (行1,列2) 元素
    // logical 坐标: ((TileM=1,RestM=1),(TileN=2,RestN=0))
    int off_ld = int(ld(make_coord(1,1), make_coord(2,0)));
    // zipped 坐标: ((TileM=1,TileN=2),(RestM=1,RestN=0))
    int off_zd = int(zd(make_coord(1,2), make_coord(1,0)));
    print("同一元素两种坐标:\n");
    print("  logical ld((1,1),(2,0)) = %d\n", off_ld);
    print("  zipped  zd((1,2),(1,0)) = %d\n", off_zd);
    print("  %s\n", off_ld==off_zd ? "-> 相同 offset，确认是同一份数据的不同 mode 排列" : "不同!");
}
} // namespace ex_zipeq

namespace ex_flatperm {

using namespace cute;

template <class L> void codomain(const char* tag, L l) {
    std::set<int> s;
    for (int i=0;i<size(l);i++) s.insert(int(l(i)));
    print("  %s 碰到的 offset 集合大小=%d, 前几个: ", tag, (int)s.size());
    int c=0; for(int v: s){ if(c++<8) print("%d ", v);} print("...\n");
}

void run()
{
    auto A = make_layout(make_shape(_6{}, _8{}), make_stride(_1{}, _6{}));
    auto B = make_tile(Layout<_2,_1>{}, Layout<_4,_1>{});
    auto ld = logical_divide(A, B);
    auto zd = zipped_divide(A, B);

    // ---- 1) flatten 去括号：是否改变 1-D 函数? ----
    print("=== 1) flatten(去括号) 是否改变函数 ===\n");
    auto ldf = flatten(ld);
    print("logical         = "); print(ld);  print("\n");
    print("flatten(logical)= "); print(ldf); print("\n");
    bool same = true;
    for (int i=0;i<size(ld);i++) if (int(ld(i))!=int(ldf(i))) same=false;
    print("  逐点 ld(i)==flatten(ld)(i) ? %s  <- 去括号不改函数\n\n", same?"全部相同":"有不同");

    // ---- 2) permute 重排：改变函数，但保住 offset 集合 ----
    print("=== 2) logical vs zipped (互为 permute) ===\n");
    print("flatten(logical)= "); print(flatten(ld)); print("\n");
    print("flatten(zipped) = "); print(flatten(zd)); print("\n");
    print("  遍历顺序(前8个 1-D):\n");
    print("    logical: "); for(int i=0;i<8;i++) print("%d ", int(ld(i))); print("\n");
    print("    zipped : "); for(int i=0;i<8;i++) print("%d ", int(zd(i))); print("\n");
    codomain("logical", ld);
    codomain("zipped ", zd);
    print("  -> 遍历顺序不同(函数不同)，但碰到的 offset 集合相同\n");
}
} // namespace ex_flatperm

namespace ex_allvariants {

using namespace cute;
void run()
{
    auto A = make_layout(make_shape(_6{},_8{}), make_stride(_1{},_6{}));  // 6x8
    auto Bd = make_tile(Layout<_2,_1>{}, Layout<_4,_1>{});                // divide tiler 2x4

    print("========== DIVIDE (A=6x8, tiler=<2,4>) ==========\n");
    print("logical_divide = "); print(shape(logical_divide(A, Bd))); print("\n");
    print("zipped_divide  = "); print(shape(zipped_divide (A, Bd))); print("\n\n");

    auto blk = make_layout(make_shape(_2{},_5{}), make_stride(_5{},_1{}));// tile 2x5
    auto arr = make_layout(make_shape(_3{},_4{}), make_stride(_1{},_3{}));// 排列 3x4
    print("========== PRODUCT (tile=2x5, 排列=3x4) ==========\n");
    print("logical_product = "); print(shape(logical_product(blk, arr))); print("\n");
    print("zipped_product  = "); print(shape(zipped_product (blk, arr))); print("\n");
    print("blocked_product = "); print(shape(blocked_product(blk, arr))); print("\n");
    print("raked_product   = "); print(shape(raked_product  (blk, arr))); print("\n");
}
} // namespace ex_allvariants

int main() {
  cute::print("\n##### divide #####\n");
  ex_divide::run();
  cute::print("\n##### divstep #####\n");
  ex_divstep::run();
  cute::print("\n##### divide3d #####\n");
  ex_divide3d::run();
  cute::print("\n##### tilerest #####\n");
  ex_tilerest::run();
  cute::print("\n##### tilerstride #####\n");
  ex_tilerstride::run();
  cute::print("\n##### zipped #####\n");
  ex_zipped::run();
  cute::print("\n##### zipeq #####\n");
  ex_zipeq::run();
  cute::print("\n##### flatperm #####\n");
  ex_flatperm::run();
  cute::print("\n##### allvariants #####\n");
  ex_allvariants::run();
  return 0;
}
