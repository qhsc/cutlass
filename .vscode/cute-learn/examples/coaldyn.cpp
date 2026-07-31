#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
{
    // 动态 stride：运行时 int
    auto dyn = make_layout(make_shape(2,3,4), make_stride(1,2,6));
    print("动态 dyn = "); print(dyn); print("\n");
    print("coalesce = "); print(coalesce(dyn)); print("   <- 合并结果?\n\n");

    // 静态 stride：编译期常量
    auto sta = make_layout(make_shape(_2{},_3{},_4{}), make_stride(_1{},_2{},_6{}));
    print("静态 sta = "); print(sta); print("\n");
    print("coalesce = "); print(coalesce(sta)); print("   <- 合并结果?\n");
}
