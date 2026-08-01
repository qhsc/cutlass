// 合并归档: layout.cpp, coord.cpp, hole.cpp
#include <cute/tensor.hpp>
#include <cstdio>
#include <set>

namespace ex_layout {

using namespace cute;
void run()
{
    // 文档里的 s2xh4: shape=(2,(2,2)), stride=(4,(2,1))
    auto s2xh4 = make_layout(make_shape (2, make_shape(2,2)),
                             make_stride(4, make_stride(2,1)));
    print("s2xh4 = "); print(s2xh4); print("\n");
    print("rank = %d, size = %d\n\n", int(rank(s2xh4)), int(size(s2xh4)));

    // print2D: 行 = mode0 (2 行), 列 = mode1 = (2,2) 摊平成 4 列
    print("print2D:\n");
    for (int i = 0; i < size<0>(s2xh4); ++i) {
        for (int j = 0; j < size<1>(s2xh4); ++j) {
            print("%4d ", int(s2xh4(i, j)));   // 用 (行, 列) 两维坐标，列坐标是 0..3
        }
        print("\n");
    }

    auto s2 = make_layout(make_shape(Int<4>{}, Int<2>{}), make_shape(2,1));
    for(int i=0; i<cute::size(s2); i++){
        print("%d -> %d\n", i, s2(i));
    }

    auto s3 = make_layout(make_shape(make_shape(2,2),2), make_stride(make_stride(4,1),2));
    print_layout(s3);
    print("\n");

    auto s4 = make_shape(3, make_shape(2,3));
    print(cute::idx2crd(cute::_16{}, s4)); // 1, 1, 2
    print("\n");
    print(cute::idx2crd(make_coord(1, cute::_5{}), s4)); // 1, 1, 2
}
} // namespace ex_layout

namespace ex_coord {

using namespace cute;
void run()
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
} // namespace ex_coord

namespace ex_hole {

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

void run()
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
} // namespace ex_hole

int main() {
  cute::print("\n##### layout #####\n");
  ex_layout::run();
  cute::print("\n##### coord #####\n");
  ex_coord::run();
  cute::print("\n##### hole #####\n");
  ex_hole::run();
  return 0;
}
