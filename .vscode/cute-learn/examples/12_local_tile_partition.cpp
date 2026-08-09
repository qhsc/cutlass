// 用代码验证 local_tile / local_partition 对 tiler shape、stride 的真实处理。
#include <cute/tensor.hpp>

using namespace cute;

template <class Tensor>
void dump_1d_values(char const* name, Tensor const& t)
{
  print("%s layout = ", name);
  print(layout(t));
  print("  values: ");
  for (int i = 0; i < int(size(t)); ++i) {
    print("%d ", int(t(i)));
  }
  print("\n");
}

int main()
{
  print("=== 1) local_tile: Shape tiler vs Layout tiler ===\n");

  int data1[16];
  for (int i = 0; i < 16; ++i) {
    data1[i] = i;
  }
  int* ptr1 = data1;
  auto t1 = make_tensor(ptr1, make_layout(_16{}));

  // Shape _4 等价于 compact tiler 4:1。
  auto compact_tile_1 = local_tile(t1, _4{}, _1{});
  dump_1d_values("local_tile(T, Shape<_4>, q=1)", compact_tile_1);

  // Layout<4,2> 的 stride=2 直接进入 zipped_divide，表示隔 2 采样。
  auto strided_tiler = Layout<_4,_2>{};
  auto strided_tile_0 = local_tile(t1, strided_tiler, _0{});
  auto strided_tile_1 = local_tile(t1, strided_tiler, _1{});
  dump_1d_values("local_tile(T, Layout<4,2>, q=0)", strided_tile_0);
  dump_1d_values("local_tile(T, Layout<4,2>, q=1)", strided_tile_1);
  print("zipped_divide(Layout<16,1>, Layout<4,2>) = ");
  print(zipped_divide(t1.layout(), strided_tiler));
  print("\n");

  print("\n=== 1b) misuse ThreadLayout as local_tile data tiler ===\n");
  auto ga_layout = make_layout(make_shape(_128{}, _8{}, Int<512>{}),
                               make_stride(_1{}, Int<5120>{}, Int<40960>{}));
  auto ga = make_tensor(static_cast<int*>(nullptr), ga_layout);
  auto ta = make_layout(make_shape(_32{}, _8{}),
                        make_stride(_1{}, _32{}));
  auto wrong_divide = zipped_divide(ga, ta);
  auto wrong_tile = local_tile(ga, ta, _0{});
  print("gA layout = "); print(ga_layout); print("\n");
  print("tA used as one Layout tiler = "); print(ta); print("\n");
  print("zipped_divide(gA,tA) layout = "); print(layout(wrong_divide)); print("\n");
  print("local_tile(gA,tA,0) layout = "); print(layout(wrong_tile)); print("\n");

  print("\n=== 2) local_partition: same shape, different ThreadLayout stride ===\n");

  int data2[64];
  for (int i = 0; i < 64; ++i) {
    data2[i] = i;
  }
  int* ptr2 = data2;
  auto t2 = make_tensor(ptr2, make_layout(make_shape(_8{}, _8{})));

  // 两者 worker 网格 shape 都是 (2,4)，但物理 tid 编号顺序不同。
  auto p_mmajor = Layout<Shape<_2,_4>, Stride<_1,_2>>{};
  auto p_kmajor = Layout<Shape<_2,_4>, Stride<_4,_1>>{};

  print("P_mmajor = "); print(p_mmajor); print("\n");
  print("P_kmajor = "); print(p_kmajor); print("\n");
  print("product_each(shape(P_mmajor)) = "); print(product_each(shape(p_mmajor))); print("\n");
  print("product_each(shape(P_kmajor)) = "); print(product_each(shape(p_kmajor))); print("\n");

  auto tiled_m = zipped_divide(t2, product_each(shape(p_mmajor)));
  auto tiled_k = zipped_divide(t2, product_each(shape(p_kmajor)));
  print("zipped_divide by P_mmajor.shape = "); print(layout(tiled_m)); print("\n");
  print("zipped_divide by P_kmajor.shape = "); print(layout(tiled_k)); print("\n");

  constexpr int tid = 1;
  auto coord_m = p_mmajor.get_flat_coord(Int<tid>{});
  auto coord_k = p_kmajor.get_flat_coord(Int<tid>{});
  print("P_mmajor.get_flat_coord(1) = "); print(coord_m); print("\n");
  print("P_kmajor.get_flat_coord(1) = "); print(coord_k); print("\n");

  auto part_m = local_partition(t2, p_mmajor, Int<tid>{});
  auto part_k = local_partition(t2, p_kmajor, Int<tid>{});
  dump_1d_values("local_partition(T,P_mmajor,1)", part_m);
  dump_1d_values("local_partition(T,P_kmajor,1)", part_k);

  return 0;
}
