#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
{
    // 一个层次化 tuple：外层 3 个元素，其中第 1 个自己又是 tuple
    auto s = make_shape(2, make_shape(3, 4), 5);
    print("s = "); print(s); print("\n\n");

    // rank: 最外层有几个元素（不往里看）
    print("rank(s)        = %d   <- 外层元素个数 (2, (3,4), 5) 共 3 个\n", int(rank(s)));
    print("rank<1>(s)     = %d   <- 第1个元素 (3,4) 的 rank\n", int(rank<1>(s)));

    // depth: 嵌套层数（标量=0，一层tuple=1，tuple套tuple=2...）
    print("depth(s)       = %d   <- 最深处嵌套 2 层\n", int(depth(s)));
    print("depth<0>(s)    = %d   <- 第0个元素是标量 2，深度 0\n", int(depth<0>(s)));
    print("depth<1>(s)    = %d   <- 第1个元素 (3,4) 深度 1\n\n", int(depth<1>(s)));

    // get<N>: 取第 N 个元素（可嵌套 get<1> 再 get<0>）
    print("get<1>(s)      = "); print(get<1>(s)); print("   <- 取出 (3,4)\n");
    print("get<0>(get<1>(s)) = "); print(get<0>(get<1>(s))); print("      <- 再取里面的 3\n\n");

    // take<B,E>: 切出 [B, E) 区间的子 tuple（左闭右开）
    print("take<0,2>(s)   = "); print(take<0,2>(s)); print("   <- 取前两个元素\n");
    print("take<1,3>(s)   = "); print(take<1,3>(s)); print(" <- 取后两个元素\n");
}
