#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
{
    // 非单射 layout: shape=(8,4), stride=(1,1) —— 没有 0，但坐标会碰撞
    auto shape  = make_shape(8, 4);
    auto stride = make_stride(1, 1);
    auto L = make_layout(shape, stride);
    print("L = "); print(L); print("\n");
    print("size = %d, cosize = %d  (cosize<size 说明非单射/有碰撞)\n\n",
          int(size(L)), int(cosize(L)));

    // 正向：看碰撞。(0,3) 和 (3,0) 都 -> 3
    print("① 正向 crd2idx：\n");
    print("   (3,0)->%d   (0,3)->%d   (1,2)->%d   (2,1)->%d  <- 全是 3，多坐标碰撞\n\n",
          int(crd2idx(make_coord(3,0),shape,stride)),
          int(crd2idx(make_coord(0,3),shape,stride)),
          int(crd2idx(make_coord(1,2),shape,stride)),
          
          int(crd2idx(make_coord(2,1),shape,stride)));

    // 反向：不除零，不崩，但结果是【错的】（不是真正的逆）
    print("② 反向 idx2crd(3, (8,4), (1,1)) = ");
    auto crd = idx2crd(3, shape, stride);
    print(crd);
    print("\n   验证 crd2idx(该坐标) = %d\n", int(crd2idx(crd, shape, stride)));
    print("   -> 每维各算 (3/1)%%8=3, (3/1)%%4=3 得 (3,3)，但 (3,3) 其实映射到 ");
    print("%d，不是 3！反向失效但静默不报错。\n", int(crd2idx(make_coord(3,3),shape,stride)));
}
