#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
{
    // 文档例子: (2,(1,6)):(1,(6,2)) -> 12:1
    auto a = make_layout(make_shape (_2{}, make_shape (_1{},_6{})),
                         make_stride(_1{}, make_stride(_6{},_2{})));
    print("a       = "); print(a); print("\n");
    print("coalesce= "); print(coalesce(a)); print("   <- 化简成 12:1\n\n");

    // 规则3: 连续可合并。 (2,4):(1,2) -> 8:1
    auto b = make_layout(make_shape(_2{},_4{}), make_stride(_1{},_2{}));
    print("b       = "); print(b); print("\n");
    print("coalesce= "); print(coalesce(b)); print("   <- d1=2=s0*d0=2*1，连续->合并 8:1\n\n");

    // 规则4: 不连续，无法合并。 (2,4):(1,5)
    auto c = make_layout(make_shape(_2{},_4{}), make_stride(_1{},_5{}));
    print("c       = "); print(c); print("\n");
    print("coalesce= "); print(coalesce(c)); print("   <- d1=5≠2，有空隙，保持两维\n\n");

    // 规则1/2: size-1 的维度被丢弃。 (4,1):(1,7) -> 4:1
    auto d = make_layout(make_shape(_4{},_1{}), make_stride(_1{},_7{}));
    print("d       = "); print(d); print("\n");
    print("coalesce= "); print(coalesce(d)); print("   <- size=1 的维度直接扔\n\n");

    // coalesce 不改变作为一维函数的值：逐点验证 b vs coalesce(b)
    print("验证 b 与 coalesce(b) 一维等价:\n  ");
    auto bc = coalesce(b);
    for (int i = 0; i < 8; ++i) print("b(%d)=%d/bc=%d  ", i, int(b(i)), int(bc(i)));
    print("\n");

    // test high dimension
    auto l3 = make_layout(make_shape(2,3,4), make_stride(1,2,6));
    print("l3 = "); print(l3); print("\n");
    print("-->"); print(coalesce(l3)); print("\n");
}
