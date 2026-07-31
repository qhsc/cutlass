#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;

// 检验 idx2crd 是否是 crd2idx 的真逆：反向后再正向，看能否回到原 offset
template <class S, class D>
void check(const char* tag, S shape, D stride, int off)
{
    auto crd = idx2crd(off, shape, stride);
    int  back = int(crd2idx(crd, shape, stride));
    print("   %s off=%d -> idx2crd=", tag, off); print(crd);
    print(" -> crd2idx=%d  %s\n", back, back==off ? "[往返一致]" : "[!! 不一致]");
}

int main()
{
    // A：有【空洞】—— 单射但非 compact。shape=(4), stride=(2)
    // 合法 offset 只有 0,2,4,6；1,3,5 是空洞
    auto sa = make_shape(4); auto da = make_stride(2);
    auto A = make_layout(sa, da);
    print("A 空洞: "); print(A); print("   size=%d cosize=%d\n", int(size(A)), int(cosize(A)));
    check("合法", sa, da, 4);   // 4 真实映射到 (coord 2)
    check("空洞", sa, da, 3);   // 3 没有坐标映射到
    check("空洞", sa, da, 5);

    // B：有【碰撞】—— 非单射。shape=(8,4), stride=(1,1)
    print("\n");
    auto sb = make_shape(8,4); auto db = make_stride(1,1);
    auto B = make_layout(sb, db);
    print("B 碰撞: "); print(B); print("   size=%d cosize=%d\n", int(size(B)), int(cosize(B)));
    check("", sb, db, 3);
    check("", sb, db, 6);
}
