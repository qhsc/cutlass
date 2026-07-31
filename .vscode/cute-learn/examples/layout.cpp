#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;
int main()
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
