// 合并归档: comp2.cpp, comp3.cpp
#include <cute/tensor.hpp>
#include <cstdio>
#include <set>

namespace ex_comp2 {

using namespace cute;

template <class A, class R>
void show(const char* tag, A a, R r) {
    print("%s\n", tag);
    print("  A          = "); print(a); print("\n");
    print("  complement = "); print(r); print("\n");
    // 拼起来看是否覆盖 [0, cosize) 且唯一
    auto both = make_layout(a, r);
    print("  (A, comp)  = "); print(both); print("   cosize=%d\n\n", int(cosize(both)));
}

void run()
{
    // 文档例子逐个验证 (cotarget = 24)
    // 1) complement(4:1, 24) = 6:4  —— 4:1 稀疏重复 6 次
    show("① complement(4:1, 24)  期望 6:4  (4:1 被重复6次填满)",
         Layout<_4,_1>{}, complement(Layout<_4,_1>{}, _24{}));

    // 2) complement(6:4, 24) = 4:1  —— 填 6:4 的"洞"
    show("② complement(6:4, 24)  期望 4:1  (填6:4留下的洞)",
         Layout<_6,_4>{}, complement(Layout<_6,_4>{}, _24{}));

    // 3) complement((4,6):(1,4), 24) = 1:0  —— 已铺满，无需补
    show("③ complement((4,6):(1,4), 24)  期望 1:0  (已满，无需补)",
         Layout<Shape<_4,_6>,Stride<_1,_4>>{},
         complement(Layout<Shape<_4,_6>,Stride<_1,_4>>{}, _24{}));

    // 4) complement(4:2, 24) = (2,3):(1,8)  —— 先填洞2:1，再重复3次
    show("④ complement(4:2, 24)  期望 (2,3):(1,8)",
         Layout<_4,_2>{}, complement(Layout<_4,_2>{}, _24{}));
}
} // namespace ex_comp2

namespace ex_comp3 {

using namespace cute;
void run()
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
} // namespace ex_comp3

int main() {
  cute::print("\n##### comp2 #####\n");
  ex_comp2::run();
  cute::print("\n##### comp3 #####\n");
  ex_comp3::run();
  return 0;
}
