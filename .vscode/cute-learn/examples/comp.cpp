#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
{
    // 文档主例子: A=(6,2):(8,2), B=(4,3):(3,1)
    auto A = make_layout(make_shape(_6{},_2{}), make_stride(_8{},_2{}));
    auto B = make_layout(make_shape(_4{},_3{}), make_stride(_3{},_1{}));
    auto R = composition(A, B);
    print("A = "); print(A); print("\n");
    print("B = "); print(B); print("\n");
    // (6,2):(8,2) 4:3, 3:1
    // (6,2)/3 -> (2,2), (8*3,2)
    // (2,2) % 4 -> (2,2).  (2,2):(24,2)
    // (6,2)/1 -> (6,2), (8,2)
    // (6,2)*3 -> (3,1)  (3,1):(8,2) -> 3:8
    // ((2,2),3):((24,2),8)
    print("R = A o B = "); print(R); print("   (文档说应为 ((2,2),3):((24,2),8))\n\n");

    // 逐点验证 R(c) == A(B(c))
    print("逐点验证 R(c) == A(B(c)):\n");
    bool ok = true;
    for (int c = 0; c < 12; ++c) {
        int lhs = int(R(c));
        int rhs = int(A(B(c)));
        if (lhs != rhs) ok = false;
        print("  R(%2d)=%2d  A(B(%2d))=%2d  %s\n", c, lhs, c, rhs, lhs==rhs?"":"<<DIFF");
    }
    print(ok ? "\n全部一致：R 确实是复合函数 A∘B\n" : "\n有不一致!\n");

    // 简单情形: A o (s:d) = 从 A 每隔 d 取 s 个。A=(6,2):(8,2)，取 s=3、步长 d=2
    print("\n简单情形 A=(6,2):(8,2) o 3:2 （从A每隔2取3个，size=12 可整除）:\n");
    auto B2 = make_layout(_3{}, _2{});
    auto R2 = composition(A, B2);
    print("  R2 = A o (3:2) = "); print(R2); print("\n");
    print("  逐点: ");
    for (int c = 0; c < 3; ++c) print("R2(%d)=%d[A(%d)=%d] ", c, int(R2(c)), 2*c, int(A(2*c)));
    print("\n");

    auto a3 = make_layout(make_shape(_12{}, make_shape(_4{},_8{})), 
                        make_stride(Int<59>{}, make_stride(Int<13>{}, _1{})));
    print("a3 "); print(a3);
    auto tiler = make_tile(Layout<_3, _4>{}, Layout<_8, _2>{});
    print("\n tiler"); print(tiler);
    auto comp1 = composition(a3, tiler);
    print("\ncomp "); print(comp1);

    auto tiler2 = make_shape(_3{}, _8{});
    auto comp2 = composition(a3, tiler2);
    print("\ncomp "); print(comp2);

    print("\n");
    
}
