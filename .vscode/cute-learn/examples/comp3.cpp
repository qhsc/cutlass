#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
{
    // 多 mode 例子，验证"按 stride 排序 + 累积"算法
    // 文档: complement((2,2):(1,6), 24) = (3,2):(2,12)
    print("① complement((2,2):(1,6), 24) 期望 (3,2):(2,12)\n");
    auto A1 = Layout<Shape<_2,_2>,Stride<_1,_6>>{};
    print("  A          = "); print(A1); print("\n");
    print("  complement = "); print(complement(A1, _24{})); print("\n");
    print("  手算: 排序后 stride 升序 1,6; current=1\n");
    print("        mode 1:1 -> 补shape=1/1=1(丢), current=1*2=2\n");
    print("        mode 2:6 -> 补shape=6/2=3 stride=2 -> 3:2, current=6*2=12\n");
    print("        收尾 -> 补shape=24/12=2 stride=12 -> 2:12\n");
    print("        合并 (3,2):(2,12)\n\n");

    // 文档: complement((2,4):(1,6), 24) = 3:2
    print("② complement((2,4):(1,6), 24) 期望 3:2\n");
    auto A2 = Layout<Shape<_2,_4>,Stride<_1,_6>>{};
    print("  A          = "); print(A2); print("\n");
    print("  complement = "); print(complement(A2, _24{})); print("\n");
}
