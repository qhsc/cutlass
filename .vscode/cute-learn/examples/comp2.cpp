#include <cute/tensor.hpp>
#include <cstdio>

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

int main()
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
