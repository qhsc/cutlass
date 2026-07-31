#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
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
