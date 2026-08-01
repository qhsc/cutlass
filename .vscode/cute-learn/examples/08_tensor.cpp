// 03_tensor.md 配套: 构造 / 访问 / slice / partition
#include <cute/tensor.hpp>
#include <cstdio>

using namespace cute;

// ---------------------------------------------------------------------------
namespace ex_make {
// Tensor = Engine(数据指针) + Layout(坐标->offset)
void run()
{
    static float A[64];
    for (int i = 0; i < 64; ++i) A[i] = float(i);
    float* p = A;   // 传指针; 传裸数组 float[64] 会被当成数组类型报错

    // nonowning: 视图, 不拷数据; layout 静态/动态都行
    Tensor v_static = make_tensor(p, make_layout(Int<8>{}));        // _8:_1
    Tensor v_dyn    = make_tensor(p, make_shape(8, 4));             // (8,4) 动态
    Tensor v_gmem   = make_tensor(make_gmem_ptr(p), make_shape(Int<4>{}, Int<8>{})); // 打 gmem 标签
    Tensor v_gmem2  = make_tensor(p, make_layout(make_shape(4,16), make_stride(1,4)));

    print("nonowning static : "); print(v_static); print("\n");
    print("nonowning dyn    : "); print(v_dyn);    print("\n");
    print("nonowning gmem   : "); print(v_gmem);   print("\n");
    print("nonowning gmem2  : "); print(v_gmem2);  print("\n");


    // owning: 像 std::array, 必须全静态 layout (寄存器 buffer)
    Tensor r = make_tensor<float>(Shape<_4,_8>{});                  // (_4,_8):(_1,_4)
    Tensor r_like = make_tensor_like(r);                            // 同 shape/序
    print("owning rmem      : "); print(r);        print("\n");
    print("owning like      : "); print(r_like);   print("\n");

    // 本质: tensor(coord) == data()[ layout()(coord) ]
    auto lay = v_dyn.layout();
    print("check tensor(2,1)=%.0f  vs  data[layout(2,1)=%d]=%.0f\n",
          v_dyn(2,1), int(lay(make_coord(2,1))), A[lay(make_coord(2,1))]);
}
} // namespace ex_make

// ---------------------------------------------------------------------------
namespace ex_access {
// 三种访问: op()(变参坐标) / op[](coord) / 线性 op[](i)
void run()
{
    static float buf[13*20];
    float* p = buf;
    Tensor B = make_tensor(p, make_shape(13, 20));   // (13,20) 列主序
    Tensor C = make_tensor(p, make_layout(make_shape(20,13), LayoutRight{}));

    // 变参 op(): 自然坐标
    B(0,0) = 3.0f;  B(1,0) = 5.0f;  B(0,1) = 7.0f;
    print("B(0,0)=%.0f B(1,0)=%.0f B(0,1)=%.0f\n", B(0,0), B(1,0), B(0,1));

    // op[](coord)
    print("B[(1,0)]=%.0f\n", B[make_coord(1,0)]);

    // 线性 op[](i): 按 layout 顺序遍历
    print("linear B[0..2]= %.0f %.0f %.0f  (列主序: 第0列先走)\n", B[0], B[1], B[13]);
    print("C[(0,1)]=%.0f C[(1,0)] = %.0f\n", C(0,1), C(1,0));
}
} // namespace ex_access

// ---------------------------------------------------------------------------
namespace ex_slice {
// slice: 给具体坐标的维求值累加进指针, 给 _ 的维保留 layout
// 结果 rank == 坐标里 _ 的个数
void run()
{
    static float A[400];
    for (int i = 0; i < 400; ++i) A[i] = float(i);
    float* p = A;

    // ((_3,2),(2,_5,_2)):((4,1),(_2,13,100))
    Tensor T = make_tensor(p,
        make_shape (make_shape (Int<3>{}, 2), make_shape (       2, Int<5>{}, Int<2>{})),
        make_stride(make_stride(       4, 1), make_stride(Int<2>{},      13,     100)));
    print("T                = "); print(T); print("\n");

    Tensor B = T(2, _);                          // 第0维定=2, 第1维保留 -> (2,5,2)
    print("T(2,_)           = "); print(B); print("\n");

    Tensor C = T(_, 5);                           // -> ((_3,2))  rank1(一个_)
    print("T(_,5)           = "); print(C); print("\n");

    Tensor D = T(make_coord(_,_), 5);             // 两个_ 分别保留 -> (_3,2) rank2
    print("T(mc(_,_),5)     = "); print(D); print("  (同 C 元素, 但 rank/shape 不同)\n");

    Tensor F = T(make_coord(2,_), make_coord(_,3,_)); // 混合 -> (2,2,_2)
    print("T(mc(2,_),mc(_,3,_)) = "); print(F); print("\n");
}
} // namespace ex_slice

// ---------------------------------------------------------------------------
namespace ex_partition {
// partition = tiling(zipped_divide) + slice
//   inner (local_tile):     保留 tile内容, 按 block 编号切 -> 分给 threadgroup
//   outer (local_partition): 保留 rest编号, 按 thread 索引切 -> 分给 thread
void run()
{
    static float A[8*24];
    for (int i = 0; i < 8*24; ++i) A[i] = float(i);
    float* p = A;

    Tensor T = make_tensor(p, make_shape(8, 24));    // (8,24)
    auto tiler = Shape<_4,_8>{};                       // 4x8 tile

    Tensor tiled = zipped_divide(T, tiler);           // ((_4,_8),(2,3))
    print("zipped_divide    = "); print(tiled); print("\n");

    // inner: 取第 (1,2) 块整块 -> (_4,_8)
    Tensor blk = tiled(make_coord(_,_), make_coord(1, 2));
    print("inner (blk 1,2)  = "); print(blk); print("   [local_tile]\n");

    // outer: 第 5 号线程在每个 tile 里的落点 -> (2,3)
    Tensor thr = tiled(5, make_coord(_,_));
    print("outer (thr 5)    = "); print(thr); print("   [local_partition]\n");

    // local_tile 直接一步到位 (等价 inner)
    Tensor blk2 = local_tile(T, tiler, make_coord(1, 2));
    print("local_tile(1,2)  = "); print(blk2); print("\n");
}
} // namespace ex_partition

// ---------------------------------------------------------------------------
int main() {
    print("\n##### make (construct) #####\n");   ex_make::run();
    print("\n##### access #####\n");             ex_access::run();
    print("\n##### slice #####\n");              ex_slice::run();
    print("\n##### partition #####\n");          ex_partition::run();
    return 0;
}
