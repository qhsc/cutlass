#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
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
