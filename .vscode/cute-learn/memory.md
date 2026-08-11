# CuTe 学习交接文件 (memory.md)

> 用途:对话上下文太长时,新开对话对 Claude 说「load 这个 memory.md,我们继续」,
> Claude 读完即可无缝接上学习进度,不用重新对齐。每次学完一块,让 Claude 更新本文件的「当前进度」和「已掌握」。

---

## 学习者画像

- **目标**:熟练使用 CuTe 和 CUTLASS。当前聚焦 CuTe。
- **路线**:C++ 模板版(不是 Python DSL)。
- **C++ 底子**:日常 C++ 没问题,但模板元编程 / SFINAE 一般,长模板报错会发怵。
- **因此的教学约定**:
  - 学 layout 阶段优先用**运行时值**
    `make_layout(make_shape(8,4), make_stride(1,8))`, 而非编译期
    `Int<8>{}`——报错短、能 print。概念通了再换编译期版本。
  - 模板报错先看**第一行和最后一行**,中间展开先跳过。
  - 模板元编程**按需补**,不专门啃。只需看懂 `Int<N>{}` / `make_tuple` / `auto`
    返回值。

## 核心学习方法(每次都遵守)

1. **打印驱动**:每个概念都写 host `main()` 用 `print` / `print_layout`
   打印出来验证,不只看文档。
2. **手算先行**:先在纸上预测结果,再编译对答案。对不上的地方就是理解漏洞。
3. **一个概念一个文件**:layout.cpp / algebra.cpp / tensor.cpp
   ... 攒起来,不互相覆盖。
4. 慢在 Layout 就是快——CuTe 难点 90% 在 Layout 代数(composition / product /
   divide)。
5. **原理 + 实用性双轨(最终目标,务必贯彻)**:学习者的目标是「非常熟悉每一个 CuTe特性」,不满足于「知道原理/是什么」。每讲一个特性,除了讲清原理,**必须补上实用性质**:
   - 它能拿来做什么?什么真实场景会有意使用它?对应什么硬件/性能动机?
   - 有哪些非显然的用法(如「非单射 → 广播」这种一开始想不到的)。记「已掌握」时,原理和用途都要写下来,不能只留原理。
6. **留档取舍(学习者偏好)**:只记真正非显然的原理、设计哲学、容易再次困惑的点。基础工具函数(如 rank/depth/take/get 这类 API)**不留档**,需要时现查即可,不写进「已掌握」。

---

## 环境(已全部跑通)

- 仓库根:`/home/rain/code/cutlass`
- 练习目录:`.vscode/cute-learn/`
- 当前 GPU：NVIDIA GeForce RTX 5080（程序实测报告 SM120、84 SM）；编译 GPU
  示例使用 `-arch=sm_120`。旧的 L20Z/SM89 记录已经过时。
- CUDA:`/usr/local/cuda`；纯 layout 代码不需要 GPU 即可编译运行。
- 已配好:
  - `.vscode/c_cpp_properties.json`(include path + `__CUDACC__`,IntelliSense 用)
  - `.vscode/cute-learn/Makefile`
  - `.vscode/tasks.json`

**练习文件组织**：示例按主题放在 `.vscode/cute-learn/examples/`，编号即学习
顺序；基础代数从 `01_layout` 到 `07_product`，后续已增加 Tensor、Algorithms、
MMA 与 `12_local_tile_partition.cpp` 等验证文件。

**编译运行(在 `examples/` 目录)**:

- `make run F=06`(编号前缀简写,产物进 `build/`);`make run F=06_divide`
  全名也行。
- `make all` 编译全部;`make clean` 清 `build/`。VSCode 里打开某 .cpp 按
  `Ctrl+Shift+B`。
- ⚠️ 坑:namespace 内示例函数用 `void run()`,不能 `int main()`
  无 return——否则 UB 会 SIGILL(RC=132)。

---

## 官方材料位置(仓库内,按学习顺序)

| 顺序 | 文件                                                                                 |
| ---- | ------------------------------------------------------------------------------------ |
| 1    | `media/docs/cpp/cute/00_quickstart.md`                                               |
| 2    | `media/docs/cpp/cute/01_layout.md`                                                   |
| 3    | `media/docs/cpp/cute/02_layout_algebra.md` ⚠️最难,预计花 2-3 倍时间                  |
| 4    | `media/docs/cpp/cute/03_tensor.md`                                                   |
| 5    | `media/docs/cpp/cute/04_algorithms.md`                                               |
| 6    | `examples/cute/tutorial/sgemm_1.cu` → `sgemm_2.cu`                                   |
| 后续 | `0t_mma_atom.md` / `0x_gemm_tutorial.md` / `0y_predication.md` / `0z_tma_tensors.md` |

## 计划节奏(可调整)

- 第 1 周:00 + 01,能手算 layout(coord)→偏移,能画图
- 第 2-3 周:02 layout 代数(最难)
- 第 4 周:03 tensor + 04 algorithms(local_tile / local_partition / copy / gemm)
- 第 5 周:读懂 sgemm_1/2,自己改 tile 跑通
- 之后:MMA / TMA / predication 按需

---

## 当前进度

- [x] 方向确认、路线定案、环境搭好、编译链路跑通
- [x] Layout 基础、Tuple/静态表示、Tensor、Algorithms 与 Layout Algebra 主体
- [ ] MMA Atom / Traits 已完成大量源码与打印验证，TiledMMA 进阶部分仍在继续
- [x] **`sgemm1` 已学完**：独立学习文件为
      `.vscode/cute-learn/gemm/sgemm1.cu`，注释已逐项复核；SM120 编译产物放在
      `.vscode/cute-learn/build/sgemm1`
- [x] **`sgemm2` 已学完**：独立学习文件为
      `.vscode/cute-learn/gemm/sgemm2.cu`；已掌握 TiledCopy/TiledMMA partition、
      gmem→rmem→smem 寄存器预取流水及相关 layout algebra
- [ ] **下一步路线已改定**：先学 `media/docs/cpp/cute/0y_predication.md`，再学
      `media/docs/cpp/cute/0z_tma_tensors.md`；Hopper → Blackwell GEMM 教程整体顺延，
      **不学习 `sgemm_sm70.cu` / `sgemm_sm80.cu`**

## 已掌握的概念

> 记录原则：每条同时保留原理与【用途】；只收录非显然、容易再次困惑，或会影响实际使用的知识。

### Layout 基础

- **Layout = Shape + Stride**;layout(坐标) → 内存偏移 这个映射是 CuTe 的本质。
  - 【用途】所有 CuTe 数据访问的基石;把「逻辑索引」和「物理内存排布」解耦, 换 layout 就能换访存模式而不改算法代码。

- **列主序 vs 行主序**:stride 小的维度变化快。`(8,4):(1,8)`
  列主序(同列内存连续); `(8,4):(4,1)` 行主序(同行内存连续)。
  - 【用途】匹配数据在内存里的真实排布 / 决定访存是否合并(coalesced); 选错主序会让 warp 访存不连续,直接掉性能。

- **Layout 允许非单射(non-injective)**:多个坐标映射到同一偏移是合法的,CuTe 不禁止。
  - CuTe 只检查**结构匹配**(shape 与 stride 的 rank 要一致),**不检查**是否一一对应。
  - "是否单射"是使用者的语义责任:存数据通常要单射(否则互相覆盖);广播/复用则故意非单射。
  - 例:`(8,4):(8,8)` 有坐标碰撞但合法;`stride=0`
    是故意广播(该维坐标不影响偏移)。
  - 本质原因:Layout 数学上就是"坐标→整数"的函数,函数本就可以不是单射。
  - 真·非法只有结构对不上(如 shape 2 维、stride 给 3 个)→ 编译报错。
  - 【用途】**广播**:`stride=0`
    让一份数据被多个坐标复用(如 bias 向量按行广播到矩阵、A/B 矩阵在某维度对所有线程共享),省内存、省重复加载。

### Tuple 与编译期表示

- **cute::tuple**:CuTe 自造的 `std::tuple`
  替代品,是 Shape/Stride/Layout 的底层容器。 `make_shape`/`make_stride`
  返回的都是它。源码:`include/cute/container/tuple.hpp`。四个关键区别(各对应一个 GPU 硬约束):
  1. host+device 都能用(std::tuple 成员没标 `__device__`)。
  2. 参数须 semiregular(可默认构造+可拷贝),禁引用类型 → 保证能安全拷进显存。
  3. **standard-layout**:ABI 跨 host/device 一致 →
     Layout 可**直接当 kernel 参数**传,不错位。
  4. **ESO(空结构优化)**:元素若是空类型(编译期常量)则**零存储**。
  - **`Int<8>{}`(=`_8{}`) vs
    `8`**:前者是编译期常量(类型即值,零存储,可被编译器优化, 如地址计算/循环展开在编译期完成);后者是运行时 int(占内存,值运行时才知)。实测 sizeof:`(8,4)`=8B,`(_8,_4)`=1B(ESO),`(8,_4)`=4B(只有动态部分占空间)。
  - 【用途】固定维度(tile 大小如 128)用 `Int<128>{}`
    → 零开销+可优化; 只有运行时才知的维度(如矩阵 M/N/K)才用普通 int。CuTe
    kernel 里大量 `_128{}`/`_64{}` 即此。
  - **tuple 可嵌套**
    → 这是「层次化 shape」的来源:`make_shape(make_shape(2,4),4)` 的 rank=2
    (外层 2 个元素,首元素本身是 tuple)。是 CuTe 表达分块/多级 tiling 的基础。
  - **Shape 与 Stride 本质同类型**(都是 tuple),`Shape`/`Stride`
    只是语义标签; 操作 tuple 的工具对两者通用。
  - 练习文件:`.vscode/cute-learn/examples/02_tuple.cpp`。

#### 部分静态 tuple 的实现

- **部分静态 tuple(动/静态混合)的实现原理**——三块机制咬合:
  1. **`Int<4>` = `C<4>`
     是空类型**(`include/cute/numeric/integral_constant.hpp`): 值 `4` 用
     `static constexpr value` 编码在**模板参数/类型**里,无非静态数据成员 →
     sizeof=1。数字成了「类型身份」的一部分,`C<4>` 与 `C<8>`
     是不同类型;编译器直接把它当立即数编进指令。
  2. **ESO 逐元素检测 `is_empty` 并选特化**:tuple 递归成
     `first_ + rest_`(head+tail), 每层判断当前元素空不空 → 空的选「不建成员」特化(构造函数收下参数但丢弃), 非空的选「建 first_ 成员」特化。所以动态元素存、静态元素不存,共处一个 tuple。
  3. **`get<N>` 取值**:空类型 `return {}` 凭空重建(无数据,等价);非空才去内存捞。
  - 整条链(is_empty 判断/特化选择/if
    constexpr)**全在编译期**,运行时只剩几条 int 赋值 → 零开销。

- **构造调用栈实例:`make_shape(8, Int<4>{}, 2)`**(完整展开,记录于此备查):
  - `Ts...` 推导为 `{int, C<4>, int}`;`Shape<...>` 就是
    `tuple<int,C<4>,int>`(Shape 是 tuple 别名)。
  - `make_shape` → `return {8, C<4>{}, 2}` 列表初始化 tuple;tuple 继承
    `ESO_t`,构造函数转发给基类。
  - 递归下降,每层剥一个元素:
    - 第1层 `ESO<false,false,int,C<4>,int>`:`first_{8}` 建 int 存 8,rest 传
      `{C<4>{},2}`。
    - 第2层
      `ESO<true,false,C<4>,int>`:First 空 → 构造函数**收下 C<4>{} 但不建成员**(静态零存储瞬间),rest 传
      `{2}`。
    - 第3层 `ESO<false,true,int>`:`first_{2}` 建 int 存 2,递归到底。
  - 最终内存 = `[int(8)][int(2)]` = 8B,`C<4>` 不在内存里(活在类型中)。
  - 洞察:构造过程本身就是在「head+tail 递归链」上逐节点决策建不建成员,与 tuple 的递归定义哲学一一对应。

#### 编译期递归范式

- **CuTe 编译期递归的两种通用范式**(贯穿全库:depth/rank/size/shape/product/layout 代数都用):
  1. **`is_tuple` 分流 + `apply` 摊包 + 包展开递归**(如
     `depth`,`include/cute/int_tuple.hpp`): 标量=base
     case,tuple=`1+max(子递归...)`;`is_tuple` 编译期判枝叶,
     `cute::apply(t, lambda)` 把异构 tuple 摊成参数包
     `v...`(因异构不能 for 循环遍历), `depth(v)...`
     包展开触发子递归。全 constexpr → 结果是 `Int<N>` 编译期常量,运行时零成本。
  2. **变参 `get<I0,I1,...>` = 剥最左索引 + 递归 + 重载消解终止**:
     `get<I0,I1,Is...>(t) = get<I1,Is...>(get<I0>(t))`,每次剥一个索引钻一层(路径导航); 终止不靠 if,靠重载匹配:剩 1 索引→单索引 get 取值停;标量的
     `get<0>`
     返回自身 (CuTe 定义 rank(标量)=1,把标量当单元素 tuple,让标量/tuple 走同一套递归)。所有
     `func<Is...>`(rank/shape/size<Is...>)都先用变参 get 定位子结构,再对它运算。
  - 洞察:吃透这两个最小样本(depth + 变参 get),后面复杂 layout 代数都能一眼看穿骨架。

### 索引、大小与坐标

- **Layout 本质 = 函数「坐标 → 内存偏移」,可直接调用**:`a(3,2)`
  就是调用它算偏移。

- **size / cosize / range 辨析(核心,易混,直接决定内存分配)**:用数学三概念对应:
  - **domain(定义域)=
    `size`**:所有合法输入坐标数 = 各 shape 之积。实用:**线程/元素个数**(要处理多少数据)。
  - **codomain(陪域)= `cosize`**:输出可能落入的整个区间 `[0, max]` 的大小 =
    `a(size-1)+1` = 最大偏移+1。实用:**底层至少要分配多少个元素**(buffer 大小)。
  - **range(值域)**:实际被映射到的偏移集合(那 size 个真实地址)。
  - 关键:文档特意用 **codomain 而非 range** 定义 cosize ——
    cosize 按**最大偏移**算,
    **不管中间有没有空洞**(没被映射到的地址也算进去)。因为你会用最大偏移去写数据,
    buffer 必须够到最大偏移,哪怕中间的格子永远不碰。cosize 回答「最远写到哪」,不是「用了几格」。
  - 例:`(8,4):(2,16)` → size=32,
    cosize=63(max=7*2+3*16=62,+1)。中间大量地址空洞。 `(8,4):(1,8)` →
    size=cosize=32(无空洞)。
  - **compact(紧致)判定:`size == cosize`
    ⟺ 无空洞、数据紧密**。【用途】compact 才能整块 memcpy;开 smem
    buffer 按 cosize 开(否则最大偏移越界); 空洞常是故意的(规避 bank conflict
    / 从大 tensor 切下的非连续 sub-tile)。
  - 练习文件:`.vscode/cute-learn/examples/01_layout.cpp`。

- **一维坐标 → 多维坐标怎么拆(索引的总机制)**:两步,分开看就不乱。
  1. **拆坐标**:把一维 `j` 按 shape 拆成 `(j0,j1,...)`,规则 =
     **像拆十进制的个位十位, 但每位的进制换成对应维的 shape**。第一维是"个位",变化最快。公式:`j0=j%s0`,`j1=j/s0%s1`,`j2=j/(s0*s1)%s2`...
     **这一步只看 shape,和 stride 无关**。
  2. **算偏移**:拆出的 `(j0,j1,...)` 和 stride 点积 →
     `off = j0*d0+j1*d1+...`。stride 只在这步用。
  - 记牢:拆分永远规规矩矩(只依赖 shape);偏移看起来"乱序"是 stride 造成的,不是拆分。例:shape
    `(2,2)` 下 j=0,1,2,3 永远拆成 (0,0)(1,0)(0,1)(1,1); 配 stride
    (2,1) 得偏移 0,2,1,3(交错来自 stride),配 (1,2) 得 0,1,2,3。
  - 这也是「一维坐标能索引多维 layout」`a(9)==a(1,1)`
    的原理;层次化 mode 的交错同理。
  - 练习文件:`.vscode/cute-learn/examples/01_layout.cpp`。

### Layout 代数

#### Compatible 与坐标变换

- **compatible(兼容)—— layout 代数的地基概念(一开始难懂,务必记牢)**: 文档
  `02_layout_algebra.md` / `01_layout.md:349`。
  - **定义(大白话)**:「A compatible B」读作 **A ≤
    B,即 B 是 A 的更细划分**。成立条件:① `size(A)==size(B)`(总数相同)②
    B 是把 A 的某些维度再切细得到的。方向性:A compatible
    B 意思是「B 比 A 更细(或相等)」。
  - 例:`24`✓`(4,6)`✓`((2,2),6)`✓`((2,2),(3,2))`(一路细分,总数都=24); `24` ✗
    `32`(总数不同);`((2,3),4)` ✗ `((2,2),(3,2))`(都=24 但结构对不上,不可比)。
  - **它是 Shapes 上的弱偏序(weak partial order)**,即满足三性质(类比整数 `≤`):
    - 自反:A 兼容 A(≤ 自己)。
    - 反对称:A 兼容 B 且 B 兼容 A ⟹ A==B(不可能互相比对方细)。
    - 传递:A 兼容 B 且 B 兼容 C ⟹ A 兼容 C(细分可接力)。
  - **"偏(partial)"= 有些 shape 谁也不兼容谁(不可比)**,如上面 `((2,3),4)` vs
    `((2,2),(3,2))`。对比整数 `≤`
    是全序(任意两个可比);兼容是偏序(有的可比有的不可比)。
  - **"弱(weak)"= 允许相等**(对应 `≤` 而非 `<`),因为自反性把「相等」也算进序里。
  - 【用途】是 layout 代数的类型约束:composition / logical_divide /
    tiling 等要求 shape 兼容, 否则无意义或编译报错(文档里的
    `@post compatible(...)`)。切 tile 本质就是把粗 shape 细化, 兼容性保证「切完能对应上坐标、能拼回去」。

- **crd2idx / idx2crd(坐标 ↔ 数字,源码
  `include/cute/stride.hpp`)**:名字看似不对称,其实完全对称——各有 2-arg 和 3-arg 两个重载,分别互为逆。**先分清两种"数字":**
  - **index(一维序号)**:坐标按列主序从 0 数到 size-1 的序号,**只跟 shape 有关**。
  - **offset(内存偏移)**:坐标乘 stride 的真实内存位置,**跟 stride 有关**。index≠offset。
  - 四个重载:
    - `crd2idx(c, s)` ⟷ `idx2crd(i, s)`:坐标 ↔
      **index**(纯 shape,就是前面「拆/合坐标」)。
    - `crd2idx(c, s, d)` ⟷ `idx2crd(i, s, d)`:坐标 ↔ **offset**(带 stride)。
  - 例 `(4,2):(2,1)` 坐标 (2,1):crd2idx
    2-arg=6(index),3-arg=5(offset),两者不同。
  - 名字读法:`crd2idx`=coord→数字,`idx2crd`=数字→coord;给 2 参是 index、给 3 参是 offset。
  - `layout(coord)` 直接调用 layout ≡ 走 3-arg `crd2idx(coord,shape,stride)`。
  - 【反向 idx2crd 的成立条件——实测过的坑,重点】源码注释说"only works for
    compact", 但真实边界是**单射(injective)**;compact 只是 CuTe 采用的"好检查且足够强"的充分条件 (保证
    `[0,size)` 每个 index 都合法、都对)。分级:
    - **compact(size==cosize)**:`[0,size)` 全部正确。安全区。
    - **单射但有空洞(size<cosize,如 `(4):(2)`)**:落在**合法 offset**
      上→对(往返一致); 落在**空洞 offset**(如 3,5)上→静默给错坐标(往返不一致)。
    - **非单射有碰撞(如
      `(8,4):(1,1)`)**:一律静默给错(`idx2crd(6)=(6,2)`→回来是8≠6)。
    - **广播(stride=0,如 `(8,4):(1,0)`)**:反向执行 `(idx/0)%s` → **除零崩溃**
      (实测运行时 SIGFPE
      / 退出码 136;stride 是运行时值才崩到运行时,编译期 0 则编译期出错)。
    - 正向 `crd2idx` 对以上所有情况都 OK(多坐标同 offset 正是广播/碰撞的本意)。
  - 守则:调 3-arg `idx2crd` 前确保 layout 单射(最好 compact);CuTe
    **不检查**,后果自负。实践中反向只出现在 tiling 拆循环变量这种天然 compact 场景。
  - 练习文件:`.vscode/cute-learn/examples/01_layout.cpp`。

#### Coalesce 与 composition

- **coalesce(合并/化简)—— layout 代数第一个工具,composition 的前置**: 文档
  `02_layout_algebra.md`。
  - **作用**:在**不改变 layout 作为「一维函数」值**的前提下,把 mode 合并、简化成更少维度、更浅 depth(相当于分数「约分」)。post-condition:`size`
    不变、`depth<=1`、逐点 `result(i)==layout(i)`。例:`(2,(1,6)):(1,(6,2))` →
    `12:1`。
  - **规则**(flatten 后对相邻两 mode `s0:d0 ++ s1:d1` 反复应用):
    1. `s0:d0 ++ 1:d1 => s0:d0`;2.
       `1:d0 ++ s1:d1 => s1:d1`(**size=1 的维度直接扔,stride 无所谓**)。
    2. `s0:d0 ++ s1:(s0*d0) => (s0*s1):d0`(**连续判据
       `d1==s0*d0`:第二维步长恰好接上第一维走满一圈→无缝→合并**)。
    3. 否则保持两维 `(s0,s1):(d0,d1)`(有空隙,合不了)。
  - 【用途】① 性能:动态 stride 时更少 mode
    = 更少运行时地址计算指令。② 规范形式:代数运算后清理冗余嵌套,便于判等/喂给下一步。③
    by-mode 变体
    `coalesce(layout, trg_profile)`:化简但保住指定维度结构(如保持 2D
    / 保留 M/N/K 分界)。
  - **⚠️ 关键坑:合并只在「编译期可证连续」时发生**(实测):
    - 静态 `(_2,_3,_4):(_1,_2,_6)` → 合并成 `24:1`; 动态
      `(2,3,4):(1,2,6)`(同样数字!)→ **不合并**,原样返回。
    - 原因:合并判据 `d1==s0*d0`
      在**编译期**求值,且 coalesce 结果的 mode 数/shape 是**类型**、必须编译期定死。动态 int 的相等运行时才知 →
      CuTe **保守不合并**(类型不能依赖运行时值)。
    - 【守则】编译期固定的维度/stride 尽量用 `Int<>`
      静态类型——不只为零存储([[cute-tuple]]), 更为让 coalesce/composition 等代数能真正化简/优化。CuTe
      kernel 里 stride 大量静态正为此。
    - 通用原理:CuTe 代数「编译期能证明的才做,证不了一律保守」。
  - 练习文件:`.vscode/cute-learn/examples/03_coalesce.cpp`。

- **composition(复合)—— layout 代数的核心,几乎所有高层操作都靠它**:文档
  `02_layout_algebra.md`。
  - **概念 = 复合函数**:`R = A o B` 定义为
    `R(c) = A(B(c))`。先用 B 把坐标映射成 index, 再喂给 A。**B 决定定义域,A 决定去向**。结果 R 仍是一个 layout(代数封闭), 且
    `compatible(B, R)`(R 的定义域=B 的定义域)。post-condition:`for all i<size(B), R(i)==A(B(i))`。
  - **核心机制**:composition 对 B 的各 mode 可分配(`A o (B0,B1)=(A o B0, A o B1)`), 所以只需搞懂最简情形
    **`A o (s:d)`**,大白话 =
    **「从 A 里,每隔 d 个取一个,共取 s 个」**。两步计算(都作用在 A 的 shape 上):
    1. `shape / d`:跳到步长 d —— **沿前缀劈掉 d 的因子,丢前缀、保留后缀**。例
       `(6,2)/2=>(3,2)`,`(6,2)/6=>(1,2)`,`(3,6,2,8)/9=>(1,2,2,8)`(吃掉3、再从6劈3)。
    2. `shape % s`:截取 s 个 —— **沿前缀凑出 s 的因子,留前缀、其余置1**。例
       `(6,2)%2=>(2,1)`,`(3,6,2,8)%9=>(3,3,1,1)`(3×3,第二个3从6劈出)。stride 相应被 divide 的残数缩放。
  - **⚠️ 关键认知(自己推翻了文档用词):`/` 和 `%`
    都不是算术运算,是一对镜像的「shape 前缀因子分解」**: `/d` 丢前缀留后缀、`%s`
    留前缀丢后缀。只是**借用整数
    `(x/d)%s`「跳步长/截长度」的记号和直觉**。验证:纯算术下 `6%2=0`,但这里
    `(6,2)%2=>(2,1)` 第一维是 2 不是 0
    → 显然非取模。为什么敢借记号:一维退化情形(`n:1` 纯向量)它们确实退回算术
    `n/d`、`n%s`; 多维是把一维算术**推广**成结构分解(又一个「把 layout 当一维函数再推广」的例子)。
  - **可除性条件(divisibility)——不满足会编译期报错**: 步骤1 需 stride
    divisibility、步骤2 需 shape
    divisibility,CuTe 编译期静态检查。实测:`A=(6,2):(8,2) o 4:2` → 编译失败
    `static assertion failed: Shape Divisibility Condition` (因 4 无法沿 `(6,2)`
    前缀整齐凑出)。改成 `o 3:2` 则合法。→
    composition 不是任意两 layout 都能做,CuTe 编译期帮你挡非法组合。
  - 练习文件:`.vscode/cute-learn/examples/04_composition.cpp`。

- **composition 到底干嘛用(一句话:给一块数据换坐标系)**: 角色:**A=数据物理排布(东西在哪),B=你想要的访问视角(逻辑意图),R=A∘B 把意图翻译成真实内存 offset**。你不用手算地址,`R(你的坐标)`
  直接给对的 offset。CuTe 文档:几乎每个高层操作都靠它。三大场景(本质是同一件事,练习文件
  `.vscode/cute-learn/examples/04_composition.cpp` 全部验证过):
  1. **切子块 / tiling**:B="取哪个子块" → R 给子块每格在**原内存**的 offset。例
     `(8,8):(1,8) o <4:1,4:1> = (4,4):(1,8)`,切出左上角 4×4,stride 仍是原矩阵的 (tile 记得自己从大矩阵切来,坐标映射回原数据)。GEMM 切 block/thread
     tile 即此。
  2. **改变遍历顺序 / 重排**:B="按什么顺序走" →
     R 让你照常用线性坐标遍历,实际访问被重排。例
     `8:1 o (4,2):(2,1) = (4,2):(2,1)`,访问序变
     `0 2 4 6 1 3 5 7`(先偶后奇)。重排逻辑被 layout 吸收,循环代码不用改。向量化加载 / 避 bank
     conflict 用此。
  3. **线程↔数据分配 (partition)**:B="哪个线程" →
     R 给该线程负责的数据 offset。例
     `16:1 o 4:4`,线程 0/1/2/3 起点 offset=0/4/8/12。`local_partition`
     核心。(真实 partition 用二维 tiler 同时给「线程维」和「每线程数据维」,这里是最简形式。)
  - 共同骨架:**B 逻辑意图 + A 物理现实 →
    R=A∘B 翻译成地址**。切块/重排/分线程表面三件事, 数学上同一个 composition。所以 logical_divide/product、local_tile/partition 全是它的包装。
  - 练习文件:`.vscode/cute-learn/examples/04_composition.cpp`。

#### By-mode composition

- **by-mode composition(逐模式复合)—— tiling/分块的基础**:文档
  `02_layout_algebra.md:286`。
  - **普通 composition**:第二参 B 是单个 layout,把 A
    **当一维函数**整体复合,不管 A 的多维结构。
  - **by-mode**:第二参是 **Tiler**(`make_tile(t0,t1,...)` 造,记号 `<t0,t1,...>`
    尖括号), **对 A 的每个 mode 分别独立做 composition**,互不干扰。等价定义:
    `composition(a, make_tile(t0,t1)) ≡ make_layout(composition(layout<0>(a),t0), composition(layout<1>(a),t1))`。
  - **作用**:对多维 layout 的**每个维度分别施加不同的切法**(而非拍平混在一起)。文档:「对列方向取一个子布局、对行方向取另一个」。
  - **手算实例**(练习验证过):`a=(12,(4,8)):(59,(13,1)) o <3:4, 8:2>`:
    - mode0: `12:59 o 3:2... ` → `12:59 o 3:4` = 从 12:59 每隔4取3 →
      `3:236`(59*4=236)。
    - mode1: `(4,8):(13,1) o 8:2` → /2:`(2,8):(26,1)`,%8:`(2,4)` →
      `(2,4):(26,1)`。
    - 合并 = `(3,(2,4)):(236,(26,1))`(与实测一致)。
  - **记号区分**:`<A,B,...>`(尖,Tiler)=分而治之逐 mode 作用;`(A,B,...)`(圆)=sublayout 拼接。外观像,含义反。
  - 【用途】**tiling 的台阶**:GEMM 把大矩阵 `(M,N)`
    按 M 维、N 维分别切 tile 全靠它; 是通往 logical_divide /
    local_tile 的基础。普通 composition 做不到(会把 M/N 拍平)。

#### Complement 与 divide

- **complement(补)—— logical_divide/product 的前置**:文档
  `02_layout_algebra.md:338`, 源码 `include/cute/layout.hpp:1166`(注释:"just a
  sort and a fold")。
  - **是什么(两个等价视角)**:
    - 描述视角(文档):A
      **没碰到的「剩余」元素**的布局。composition 里 B 从 A「选中」tile,
      complement 描述**没被选中的那些**怎么排。
    - **操作视角(学习者总结,做 tiling 更顺手)**:complement =
      **为了把 A 密集填满 size=M 的空间, 需要额外增加的那些 mode 的 layout**。即「A 上再拼哪些维度,才能不重不漏铺满 M」。
    - 两者是同一事两面:「剩余的位置」正好由「额外的 mode」来编排填入。
    - 精确化:`(A, complement(A,M))` 拼起来
      **size==cosize==M 且是 [0,M) 上的双射**(即 compact, 不重不漏)。**前提:A 必须单射**;A 若非单射 complement 编译报错(源码 Non-injective 检查)。
    - 印证:`complement((4,6):(1,4),24)=1:0` —— A 已铺满,不需额外 mode,故平凡
      `1:0`。
  - **三性质(保证唯一)**:① 有界:size/cosize ≤
    size(M)。② 有序:stride 正且**递增**(故唯一)。③ 不相交:A 与 R 的 codomain 不重叠,R 精确填 A 没占的位置。
  - **两种补法**:① **填洞**(A 元素间有空隙→补进去);②
    **重复/平铺**(A 整体没铺满 M→平铺够)。
  - **手算算法**(排序+累积扫描):
    1. filter A(扔 size=1/stride=0 mode)→ 各 mode 按 **stride 升序** 排。设
       `current=1`。
    2. 对每个 mode `s:d`(stride 从小到大):产出补 mode **shape=`d/current`,
       stride=`current`**; 然后更新
       **`current = d*s`**。(`d/current`=当前占用点到该 stride 之间的洞大小。)
    3. 收尾:产出最后补 mode **shape=`M/current`,
       stride=`current`**(=整体重复次数)。
    4. `coalesce` 结果(扔掉 size=1 的平凡 mode)。
    - `d/current` 和 `M/current`
      必须整除,否则非法(源码 static_assert，A 非单射会报错)。
  - **逐步手算实例 `complement((2,2):(1,6), 24) = (3,2):(2,12)`**: 排序后
    `[2:1, 2:6]`,current=1。
    - mode `2:1`:补 shape=1/1=1(平凡丢),current=1*2=2。
    - mode `2:6`:补 shape=6/2=3 stride=2 →`3:2`(填 offset
      2..5 的洞),current=6*2=12。
    - 收尾:补 shape=24/12=2 stride=12 →`2:12`(整体重复2次)。
    - coalesce → `(3,2):(2,12)`。验证:A 占{0,1,6,7},comp 占{0,2,4,12,14,16},
      A 平铺到 comp 每点 → 恰好密铺 0..23 无重复。
    - 其它:`4:2→(2,3):(1,8)`;`4:1→6:4`(纯重复);`6:4→4:1`(纯填洞);`(4,6):(1,4)→1:0`(已满)。
  - **为何是 divide 前置**:`logical_divide: A⊘B := A∘(B, B*)`,`B*=complement(B, size(A))`。B=tile 内部、B*=tile 之间,合起来把 A 干净切成「tile 维 +
    tile 间维」。
  - 练习文件:`.vscode/cute-learn/examples/05_complement.cpp`。

- **logical_divide(切分)—— tiling/partition 的核心**:文档
  `02_layout_algebra.md:383`。
  - **是什么**:把 layout A **切成两层:tile 内 +
    tile 间**。定义:`A⊘B := A∘(B, B*)`,
    `B*=complement(B, size(A))`。B=tiler(一个 tile 内部长啥样),B*=剩余(tile 之间怎么排)。
    `(B,B*)` = 「tile 内+tile 间」完整坐标系,再 `A∘`
    映回真实内存。**没有元素被丢弃**。
  - **结果两个 mode**:mode-0 = tile 本身(**恰等于 `A∘B` 的 composition 结果**);
    mode-1 = 遍历各 tile(靠 complement 组织)。**divide =
    composition + 把剩余也组织好**。
  - **1-D 实例**:`A=(4,2,3):(2,1,8) ⊘ B=4:2`:B*=complement(4:2,24)=(2,3):(1,8),
    (B,B*)=(4,(2,3)):(2,(1,8)),A∘(B,B*)=`((2,2),(2,3)):((4,1),(2,8))`。mode0
    `(2,2):(4,1)`=tile(与 A∘B 一致);mode1
    `(2,3):(2,8)`=6 个 tile 起点 0,2,8,10,16,18。
  - **⚠️ 2-D divide 必须用 `make_tile`(Tiler,尖括号)不能用
    `make_layout`(拼接)**(实测踩坑):
    - 错:`make_layout(la, lb)` 把两子 layout 焊成一个大 layout,常**非单射**
      →对它(或手动整体)求 complement 时编译报错
      `Non-injective Layout detected in complement`。
    - 对:`make_tile(t0,t1)` 得 Tiler,divide **对 A 每个 mode 分别做 1-D
      divide**(各自 complement 单射合法)。
    - 别手动对多维 tiler 求 complement——by-mode 内部逐 mode 算,你只管
      `logical_divide(A, tiler)`。
  - **2-D 逐步拆解实例**(复杂,完整记录;练习
    `.vscode/cute-learn/examples/06_divide.cpp`。
    `A=(9,(4,8)):(59,(13,1)) ⊘ <3:3, (2,4):(1,8)>` → 两条独立 1-D divide 再拼:
    - **mode-0**:`9:59 ⊘ 3:3`。B*=complement(3:3,9)=`3:1`;(B,B*)=(3,3):(3,1);
      A0∘: tile `9:59∘3:3=3:177`(59*3), rest `9:59∘3:1=3:59` →
      `(3,3):(177,59)`。
    - **mode-1**:`(4,8):(13,1) ⊘ (2,4):(1,8)`。B*=complement((2,4):(1,8),32)=`4:2`;
      tile `(2,4):(13,2)`, rest `(2,2):(26,1)` →
      `((2,4),(2,2)):((13,2),(26,1))`。
    - 合并 `R=((3,3),((2,4),(2,2))):((177,59),((13,2),(26,1)))`。
    - **读法 `((TileM,RestM),(TileN,RestN))`**:每维劈成(tile内,
      tile间)。行9→(tile 3, 共3 tile)、列32→(tile 8, 共4 tile)
      ⇒ 切成 3×8 的 tile、共 3×4=12 个 tile。
  - **四种变体**(重排 mode 方便切 tile;`Layout=(M,N,L)`,`Tiler=<TileM,TileN>`):
    - `logical_divide`: `((TileM,RestM),(TileN,RestN),L)` 原始,tile/rest 交错。
    - `zipped_divide`: `((TileM,TileN),(RestM,RestN,L))`
      **★最常用**,tile 聚一起、rest 聚一起。
    - `tiled_divide`: `((TileM,TileN),RestM,RestN,L)`。
    - `flat_divide`: `(TileM,TileN,RestM,RestN,L)`。
    - zipped 让 tile 可索引:`zd(0,3)`=第3个tile起点,`zd(0,make_coord(1,2))`=第(1,2)个,
      `layout<0>(zd)`=tile 本身布局(恒定)。
  - 【用途】**GEMM 的心脏**:`local_tile` 底层就是 zipped_divide。大矩阵
    `(M,N) ⊘ <128,128>` →
    `((128,128),(M/128,N/128))`,mode-1 用 blockIdx 索引 → 每个线程块拿到自己那块 tile。
  - 练习文件:`.vscode/cute-learn/examples/06_divide.cpp`。

- **TileM/RestM/TileN/RestN 的含义(直观版)**:切 tile 后每个方向拆成两个数:
  **Tile=一个瓦片内部有多大(块内)**,**Rest=这样的瓦片有几个(块间/剩余重复次数)**;
  M/N/L 只是标"哪个方向"。例:6行×8列 切 2×4 瓦片 → 行:TileM=2,RestM=3(6/2); 列:TileN=4,RestN=2(8/4)
  ⇒ 共 3×2=6 个瓦片。Tile=放大镜(块内),Rest=地图(第几块)。练习:`.vscode/cute-learn/examples/06_divide.cpp`。

- **tiler 里 Layout 的 stride 决定「瓦片内部怎么采样」**:`make_tile(Layout<2,1>, Layout<4,1>)`
  的两个 `1` 是块内 stride。**stride=1=瓦片内取连续元素**(普通紧凑块,99% 情况);
  **stride>1=块内跳着取**(交错/采样瓦片,如 `4:2`
  取第0,2,4,6列)。用途:交错分配给线程(相邻线程取交错数据)以合并访存/避 bank
  conflict。另:**tiler 的 rank 可 < layout 的 rank**——未被覆盖的维度原样保留:
  logical 让它裸跟在后面 `((TileM,RestM),(TileN,RestN),L)`;zipped 归入 rest 组
  `((TileM,TileN),(RestM,RestN,L))`。GEMM 只切 M/N、batch 维 L 不切即用此。练习:`.vscode/cute-learn/examples/06_divide.cpp`。

- **logical vs
  zipped 的本质(自己推出的洞察)= 同一个 divide 的不同 mode 排列**: 四个分量(TileM/RestM/TileN/RestN,各是 size:stride)完全相同,只是**分组/排列不同**——
  logical 按**方向**分组 `((TileM,RestM),(TileN,RestN))`(保留 M/N 语义);
  zipped 按**角色**分组
  `((TileM,TileN),(RestM,RestN))`(把「一整个瓦片」拎成 mode-0,便于索引块)。实测:同一元素两种坐标给出**相同 offset**(ld((1,1),(2,0))==zd((1,2),(1,0))==15)。
  - **两种「变形」性质不同**(关键区分):
    - **flatten(去括号)**:`((2,3),(4,2))`↔`(2,3,4,2)`
      逐点**完全不改函数**,括号纯是显示分组。
    - **permute(重排 mode,如 logical↔zipped)**:遍历顺序变、坐标→元素配对变, 但**碰到的 offset 集合(codomain)不变**。类比洗牌:牌不变(集合),抽牌顺序变。
  - 所以「底层是同一个东西」**对集合成立,对函数(带顺序)不成立**。GPU 里顺序决定一切:相邻坐标碰哪个地址 → 影响 coalescing/bank
    conflict; copy 的第 i 个对第 i 个 →
    permute 改配对。故 CuTe 保留多种排列,顺序/分组是有用信息。
  - **layout 不只是「一堆元素」,而是「带顺序、带结构的访问方式」**:去括号随便,换顺序有意义。
  - 练习:`.vscode/cute-learn/examples/06_divide.cpp`。

#### Product 与统一视角

- **logical_product(铺开)—— divide 的对偶**:文档 `02_layout_algebra.md`。
  - **是什么**:divide 是「把大 layout 切成 tile」,product 是「把 tile
    A 复制铺开成大 layout」。定义:`A⊗B := (A, A*∘B)`,`A*=complement(A, size(A)*cosize(B))`。
  - **公式逐块拆解(自己推出的理解)**:
    - mode-0 = **A 原样保留**(product 是扩充 A,当然留着 A)。
    - 要拼的是 A 的补 A*,但**不能直接拼 A***:A* 只是「A 能重复的**所有可用槽位**」(满格, 最多 cosize(B) 份);**必须先
      `A*∘B`**——用 B 去 composition,即**从所有槽位里按 B 挑选并排序**
      (B.size=要几份、B.stride=按什么顺序/间隔)。即「按 B 的方式扩充」。
    - `(A, A*∘B)` = 瓦片 + 它的实际排布。
  - **与 divide 的对偶对称**:
    - divide
      `A⊘B = A∘(B,B*)`:B=tiler,composition 作用在**外层**(整个 A∘坐标系),选中的放外面。
    - product
      `A⊗B = (A, A*∘B)`:A=tile,composition 作用在**内层**(只 A*∘B,A 原样拼上),补出来的去复合。
  - **1-D 结果与 divide 同构**:`A=(2,2):(4,1) ⊗ B=6:1` →
    `((2,2),(2,3)):((4,1),(2,8))`, 和 1-D
    divide 例子结果**完全相同**。同一「tile+排布」两层结构,divide 从大切来、product 从小铺来。此例 B=6:1 正好把 6 槽位全按序选中,故
    `A*∘B==A*`;若 B=(4,2):(2,1) 则只选8个且换序 → 瓦片重排。
  - **blocked_product vs
    raked_product**(实用形式,直接说「A 按 B 铺开」,A/B 独立直观; by-mode
    tiler 做 product **不推荐**,因 tiler 需精确知道 A 的 shape/stride,反直觉):
    - 两者
      **rank-sensitive**:让 A、B 同 rank,product 后把同类 mode(列配列、行配行)重组。
    - **blocked**:瓦片**成块聚集**(贴瓷砖,每块完整挨一块)。tile 2x5 铺成 3x4 →
      `((2,3),(5,4))`。线程视角:每线程拿**连续一块** [T0 T0][T1 T1]...
      → 数据局部性(如寄存器分块)。blocked 会顺手 coalesce mode-0。
    - **raked**:瓦片**交错/耙开**(cyclic 循环分布),tile 元素与排布交织。→
      `((3,2),(4,5))`。线程视角:线程**交错**拿 [T0 T1 T2 T3][T0 T1 T2 T3]...
      → 相邻线程读相邻内存 = **合并访存**(如全局内存加载)。
    - 区别在重组顺序:blocked 列mode=A列then B列;raked 列mode=B列then A列。
  - 【用途】GEMM/CuTe 里**把数据分配给线程**的两种经典模式:连续块=blocked,交错访存=raked。
  - 练习:`.vscode/cute-learn/examples/07_product.cpp`。

- **product 四变体(与 divide 四变体对偶;源码 `layout.hpp`)。★大坑:这张表只在「尖括号 Tiler(by-mode)」下成立!** 设 block=(M,N,L,...)、Tiler=`<TileM,TileN>`(尖括号):
  - `logical_product` → `((M,TileM),(N,TileN),L,...)`(逐方向交错)
  - `zipped_product`  → `((M,N),(TileM,TileN,L,...))`(★最常用,M/N 聚一起、Tile 聚一起)
  - `tiled_product`   → `((M,N),TileM,TileN,L,...)`(zipped 拆第二组)
  - `flat_product`    → `(M,N,TileM,TileN,L,...)`(两组都拆平)
  - **为何要尖括号**:by-mode Tiler 对 A 的**每个 mode 分别** product,才产生 `((M,TileM),(N,TileN))` 的逐方向交错。**若 B 是单个 layout**(非 Tiler),product 把 A/B 各当**一整块** → `logical=((整个A),(整个B))`(如 `(2,2)⊗(3,4)=((2,2),(3,4))`),**不逐方向配对,对不上此表**;且此时 `zipped==logical`(tile_unzip 对单块无事可做)。实测见 `07_product.cpp`(ex_prodtable 尖括号复现表 / ex_prodtiler 单块对比)。
  - 关系同 divide:zipped=logical 重排;tiled=zipped 拆第二 mode;flat=拆两组(源码 `flat_product`)。**TiledMMA 用的正是 `tiled_product(AtomThrID, AtomLayoutMNK)`**,产物 `((原atom),复制M,复制N,复制K)` 便于摊平索引复制维。
  - **注意与 blocked/raked 的区别**:那张 `((M,TileM),(N,TileN))` 逐方向配对,blocked/raked 也有类似形状——但 blocked/raked 用 `zip`(进组咬合,**改 stride/改数据排布**、rank-sensitive、A/B 独立友好),四变体用 `tile_unzip`/拆包(**不改 stride,只换括号**)。同源(都 logical_product 打底)但粒度不同,是并列兄弟非特例。

- **blocked_product 的内部计算 3 步(源码 `layout.hpp:1726`,只有 3 行)**:
  1. **rank 对齐**:`R=max(rank(block),rank(tiler))`,`append<R>`
     补齐不足的维(rank-sensitive 来源)。
  2. **logical_product**(B 是普通 layout → by-mode),结果按角色分两大组:
     `get<0>`=全部 tile 内(=block 本身)、`get<1>`=全部阵列。此时结构
     `(所有tile内, 所有阵列)`。
  3. **`zip(get<0>, get<1>)`**:把两组按维度**交错咬合**成
     `((tile,阵列),(tile,阵列),...)` (每 mode = 某方向的 (瓦片内,阵列))。
  - **raked 唯一区别**:`zip(get<1>, get<0>)`
    顺序反 → 阵列放前 → 瓦片交错而非成块。
  - 实例:A=(2,5):(5,1), B=(3,4):(1,3)
    → 步1后 get0=(2,5):(5,1),get1=(3,4):(10,30);
    blocked=`((2,3),(5,4)):((5,10),(1,30))`,raked=`((3,2),(4,5)):((10,5),(30,1))`。
  - **tiler 方式做 product =
    blocked 的等价但反直觉写法**:`logical_product(A, <3:5,4:6>)`
    结果与 blocked 相同,但 tiler 里的 stride(5=A行stride、6=瓦片列跨度)**编码了对 A 内部结构的依赖**, 必须精通 A 才写得对 → 文档不推荐。实战永远用
    `blocked/raked_product(tile, 阵列)`(A/B 独立)。

- **★统一视角(提纲挈领,自己悟出的)——整个 layout 代数的结构**:
  - **所有 divide/product 变体 = 两段式:核心运算 +
    mode 重排**。core(logical_divide/logical_product 产出原始两组 mode)→ 重排(permute/zip)成方便形态。源码印证:zipped_divide=重排 logical_divide;blocked_product=zip(get0,get1);raked=zip(get1,get0)。变体**不产生新数据,只换 mode 排列**(呼应 flatten/permute:同数据不同排列)。
  - **zip 本质 = 在「维度 × 角色」2×2 分组表上转置**: 表格 行=M/N维、列=Tile/Rest角色。**logical=按行读**(TileM,RestM 一组)、
    **zipped=按列读**(TileM,TileN 一组)→ 正是转置(按行读↔按列读)。但只转
    **mode 分组结构**, 不动底层数据(offset 映射/codomain 不变),是「坐标系描述」的转置,非数据转置。
  - **全代数塌缩成一棵树**:根=**composition**(唯一真核心);两个对偶=**divide(切)/product(铺)**
    (= composition +
    complement 的两种组合);其余 logical/zipped/tiled/flat、blocked/raked
    **全是对结果的 mode 重排**。→ 记住 composition +
    complement + 「重排」三件事即可推导全部。

### Tensor

- **Tensor = Engine(数据指针) + Layout(坐标→offset)**:文档 `03_tensor.md`。
  - **本质一行**:`tensor(coord) ≡ data()[ layout()(coord) ]`。Layout 是「地图」(coord→offset),
    Engine 是「地皮」(持有随机访问迭代器,负责 `ptr[offset]`
    取真实元素)。前面 layout 的硬骨头啃完, tensor 就是给 layout 配个指针。实测
    `v(2,1)==A[layout(2,1)]` 完全吻合。
  - 【用途】算法只跟 Tensor 打交道 → 同时拿到「形状/访问方式」和「数据」,且**完全不关心数据在gmem/smem/rmem**——换内存空间算法代码一行不改。这是 CuTe 写泛型 kernel 的根基。
  - **owning vs nonowning(关键区别)**:
    - **nonowning(视图)**:`make_tensor(ptr, layout)`,像裸指针,拷贝不拷数据,layout 静态/动态都行。典型 =
      gmem/smem 视图。函数传参要用引用/const 引用(传值可能触发深拷)。
    - **owning(拥有)**:`make_tensor<T>(static_layout)`,像
      `std::array`,拷贝深拷、析构释放, **layout 必须全静态**(shape+stride 都
      `Int<>`)。典型 = rmem(寄存器)。 **为何必须全静态**:底层是
      `T arr[N]`,N 要编译期常量;CUDA kernel 里不做动态分配(非性能操作)。
      `make_tensor_like(t)` = 造个同 value_type/shape、尽量同 stride 序的 owning
      rmem tensor。
    - 【用途】`make_tensor<float>(Shape<_4,_8>{})`
      = 在寄存器开 4×8 临时 buffer;GEMM 每线程的累加器 C 即此。
  - **指针 tagging(`make_gmem_ptr`/`make_smem_ptr`)**:给迭代器贴「内存空间」标签,写进 tensor 类型。不贴也能跑,但贴了 CuTe 才能**编译期 dispatch 到最快 copy**(如
    `cp.async`/TMA 硬性要求 src=gmem、dst=smem)并**校验没接错内存**。
- **⚠️ 坑(实测)**:`make_tensor` 传**裸数组** `float A[64]`
  会被当数组类型报错 (`array must be initialized with a brace-enclosed initializer`)→ 传**指针**
    `float* p=A; make_tensor(p,...)`。
  - 练习:`.vscode/cute-learn/examples/08_tensor.cpp`。

- **⚠️ C++ 变量名不可取 `_`（CuTe slice sentinel 遮蔽坑）**：`cute::_` 是真正的 Underscore 对象，用于 `make_coord(..., _)` / Tensor slice 表示“保留该 mode”。若在同一作用域把函数参数也命名为 `_`，局部变量会遮蔽 `cute::_`；例如把未使用的 `CSmemLayout` 参数写成 `CSmemLayout _` 后，`make_coord(blockIdx.x, blockIdx.y, _)` 的第 3 项竟变成一个 C-smem Layout，而非 Underscore，`local_tile` 随后尝试拿 Layout 当坐标运算并爆出深层 `Layout * int` / `make_tensor undefined` 模板错误。守则：未使用参数保持**未命名**或取 `unused_sC_layout`；必要时显式写 `cute::_`。

- **Tensor 三种访问 + slice(切子张量)**:文档 `03_tensor.md`。
  - **访问**:`t(变参坐标)` / `t[make_coord(...)]` /
    `t[i]`(线性,按 layout 顺序遍历)三者等价通向同一元素。
  - **slice = 用 `_`(Underscore,= Matlab 的 `:`)抽子张量**,做两件事:
    ① 给了**具体坐标**的维求值,offset 累加进指针 → 新指针指向子张量起点; ② 给了
    `_` 的维**保留其 layout** → 组成新 layout。**结果 rank == 坐标里 `_`
    的个数**。
  - **⚠️ 易混:`_` vs `make_coord(_,_)` 决定 rank**(实测):对层次维 `(_3,2)`,
    `T(_,5)`→`((_3,2))`(单个 `_`,整个子维当一个 mode,rank1);
    `T(make_coord(_,_),5)`→`(_3,2)`(两个 `_`
    分别保留,rank2)。**元素相同、指针地址相同,但 rank/shape 不同**。
  - 【用途】`gmem(_, j)`=取第 j 列一整列;`copy(gmem(_,j), rmem)`=把一列搬进寄存器。切某行/列/tile 的通用手段。

- **Partition(划分)= tiling(zipped_divide)+ slice —— GEMM 命脉**:文档
  `03_tensor.md`。
  - 先 `tiled = zipped_divide(T, tiler)` →
    `((tile内),(tile编号))`,再按方向 slice:
    |           | 切法                            | 保留      | 语义                                        | 别名              |
    | --------- | ------------------------------- | --------- | ------------------------------------------- | ----------------- |
    | **inner** | `tiled(make_coord(_,_), coord)` | tile 内容 | 「给我第(bx,by)块整块」粗粒度→CTA           | `local_tile`      |
    | **outer** | `tiled(thread_coord, make_coord(_,_))` | Rest fragment | 「固定每 tile 内的线程 lane，保留所有重复」细粒度→thread | `local_partition` |
  - **为何方向相反**:inner 固定 Rest/tile 编号、保 Tile 内容(分给 block);outer 固定 Tile/线程坐标、保 Rest fragment(分给 thread)。
  - **★统一洞察（源码+代码实测后的精确版）**:`local_tile` 与
    `local_partition` 都是 `zipped_divide + slice`，但不只差 slice 位置，还差
    **divide tiler 的构造方式**：
    - `local_tile(T,B,q)`：将完整 $B$（shape+stride）原样传给
      `zipped_divide`，再做 `D(_,q)`；$B$ 是数据坐标空间中的 tiler，其 stride
      会真实影响数据采样。
    - `local_partition(T,P,t)`：先用
      `product_each(shape(P))` 构造 compact 数据 tiler，再用
      `P.get_flat_coord(t)`（数学上记作 $P^{-1}(t)$）反查 worker 坐标，最后做
      `D(P^{-1}(t),_)`。$P.stride$ 属于 physical-thread-id 空间，只参与反查，
      **不进入 divide**。
    - 还要区分 Shape/Tile tuple 与单个 Layout：前者触发 by-mode divide；即使
      shape 是 tuple，单个 Layout 仍会作为整体 tiler 与 Tensor 的整个逻辑域
      composition。`local_tile(gA,tA,0)` 的代码实验验证了这一点。
  - 实测:`tiled(mc(_,_),(1,2))` 与 `local_tile(T,tiler,(1,2))` **地址完全相同**
    → 证实 local_tile =
    inner_partition 别名。inner 块起点 offset 用 zipped 后 mode 的 stride 算(`1*4+2*64=132`)与实测指针偏移吻合。
  - `local_partition(T,Layout,Idx)` 是 outer 包装：用 Layout 的逆把 Idx 转成
    worker Coord，再按 `product_each(shape(Layout))` 造数据 Tiler。改变
    ThreadLayout stride 会改变“物理线程领取哪份 fragment”，不会改变 divide
    layout 或每份 fragment 的 shape。
  - **两级划分**:GEMM 先 `local_tile` 把大矩阵分给 CTA,再 `local_partition`
    把 tile 分给线程。
  - **TV-partition(thread-value)**:造一个 TV-layout 把 (线程id, 值id)→目标数据坐标,`composition(A, tv_layout)`
    变形后 slice 线程维 → 每线程拿到它那几个值(按 TV 规定的形状/顺序)。MMA 里线程拿寄存器片段用此。
  - 练习:`.vscode/cute-learn/examples/08_tensor.cpp`、
    `.vscode/cute-learn/examples/12_local_tile_partition.cpp`；完整标准说明见
    `.vscode/cute-learn/algebra_reference.md` 第 6 章。

- **⚠️ 设计约束:Tensor 只能 divide,不能 product**:文档 `03_tensor.md`。
  - `composition / logical_divide / zipped_divide / tiled_divide / flat_divide`
    对 Tensor 开放, 但 `_product`
    **不开放**。原因:product 会**扩大 codomain**(值域范围变大)→
    tensor 要访问远超原边界的内存,危险。divide=「在已有数据里切分」(安全);product=「凭空造更大布局」(只对纯 Layout 有意义)。

### Algorithms

- **copy —— 靠「类型 dispatch」选硬件指令(核心思想)**:文档
  `04_algorithms.md`, 源码 `include/cute/algorithm/copy.hpp`。
  - **朴素语义**:`for i<size(dst): dst(i)=src(i)`,按 1-D 线性坐标(列主序)搬。
    **⚠️
    copy 不做转置**:src 第 i 个逻辑元素→dst 第 i 个逻辑元素,各自物理 offset 由各自 layout 定。实测:dst 行主
    `(_2,_3):(_3,_1)`,copy 后线性序仍 1..6(逻辑对逻辑)。
  - **两个重载**:`copy(src,dst)`(按类型自动选实现)/
    `copy(copy_atom,src,dst)`(手动指定硬件指令)。
  - **为何类型能决定实现**:tensor 类型里编码了 ①数据类型 ②内存空间标签(gmem/smem)③静态 layout。CuTe 编译期看这三样 dispatch 到:(a) 专用指令(src=gmem&dst=smem→`cp.async`
    异步拷贝); (b) 向量化(静态 layout 且能证连续→4×`ld.b32`
    合成 1×`ld.b128`);(c) 校验指令对 src/dst 合法。→ **这是 03 章 tensor
    tagging 的兑现**:当时贴的 gmem/smem 标签在此换成硬件加速路径。
  - **⚠️ copy 可能异步/多线程**,用结果前要同步:block 内
    `__syncthreads()`;`cp.async` 要异步同步。这是 GEMM
    pipeline(拷贝与计算重叠)的基础。
  - `copy_if(pred, src, dst)`:多一个同 shape 的谓词 tensor,谓词非零才拷 → 边界 predication(见
    `0y_predication.md`)。
  - 练习:`.vscode/cute-learn/examples/09_algorithms.cpp`。

- **gemm —— 按「mode 个数」dispatch + V/M/N/K 记号(核心)**:文档
  `04_algorithms.md`, 源码 `include/cute/algorithm/gemm.hpp`。
  - `gemm(A,B,C)` 做
    `C += A*B`,**具体做什么取决于三个 tensor 各有几个 mode**。维度语义用字母标:
    - **V** = vector(独立元素,若出现**永远最左/最内**)
    - **M/N** = 结果 C 的行/列
    - **K** = reduction 维(被 sum 消掉,若出现**永远最右/最外**)
  - **5 种形态,高维递归拆成低维**(记号 `A × B => C`):
    - `(V)×(V)=>(V)` 逐元素积 → FMA/MMA 硬件指令(最底)
    - `(M)×(N)=>(M,N)` 外积 → 形态(V=1)
    - `(M,K)×(N,K)=>(M,N)` **矩阵乘** → 对每个 K 做外积
    - `(V,M)×(V,N)=>(V,M,N)` 批量外积 → 对每 M,N 做逐元素积
    - `(V,M,K)×(V,N,K)=>(V,M,N)` 批量矩阵乘 → 对每 K 做批量外积
  - **⚠️ 易栽约定:CuTe gemm 里 B 是 `(N,K)` 不是
    `(K,N)`**——N 在前 K 在后,因为「K 永远最右」。所以它算
    `C(m,n) += Σ_k A(m,k)·B(n,k)`(K 对 K 收缩)= 数学上的
    `A·Bᵀ`。写 kernel 时 B 的 layout 按此摆。实测:A=B=(_2,_3) 列主填 1..6,`gemm(A,B,C)`
    → C=`35 44 44 56`,手算 `Σ_k A(m,k)B(n,k)` 吻合。
  - 和 copy 一样可选 `MMA_Atom` 参数覆盖默认 FMA → 换 tensor core 的 MMA(见
    `0t_mma_atom.md`)。
  - 【用途】一个 gemm 函数靠 mode 数递归下降:大矩阵乘一路拆到底层一条 FMA/MMA。GEMM
    kernel 的计算核心。
  - 练习:`.vscode/cute-learn/examples/09_algorithms.cpp`。

- **GEMM 三件工具(一句话,不细留)**:`axpby(α,x,β,y)`→`y=αx+βy`(GEMM epilogue
  `C=α(A·B)+βC`);
  `fill(t,v)`→全填标量;`clear(t)`→全填 0(累加器 C 计算前必清)。练习
  `09_algorithms.cpp`。

### GEMM Tutorial（`sgemm1.cu`，已完成）

- **学习文件**：`.vscode/cute-learn/gemm/sgemm1.cu`。已从接口到 epilogue
  逐行学习并复核注释；只修改 `.vscode/cute-learn/` 内的学习材料，不改仓库外部
  教程源码。编译产物统一放进 `.vscode/cute-learn/build/`，不提交 Git。
- **完整骨架**：host 选择 NT/TN stride → 构造完整 gmem Tensor →
  `local_tile` 分给 CTA → `local_partition(tA/tB)` 分配 gmem→smem copy →
  `local_partition(tC)` 构造计算/写回 view → K-tile mainloop → `axpby` epilogue。
- **统一矩阵语义**：kernel 始终把 A/B/C 看作 `(M,K)`、`(N,K)`、`(M,N)`，
  计算 $C(m,n)=\sum_k A(m,k)B(n,k)$。NT/TN 只改变 stride，不改变 mode
  位置；`ldA/ldB` 是底层二维存储相邻主维之间的 leading dimension。
- **CTA 切块**：`cta_tiler=(128,128,8)`，
  `cta_coord=(blockIdx.x,blockIdx.y,_)`；得到
  `gA=(128,8,k_tiles)`、`gB=(128,8,k_tiles)`、`gC=(128,128)`。
  K 的 Rest mode 保留给 mainloop 遍历。
- **copy partition**：`tA=tB=(32,8):(1,32)` 有 256 个 worker。
  `local_partition` 使用 compact 数据 tiler `(32:1,8:1)`，不使用
  ThreadLayout 的 `(1,32)` 作为数据 stride。对实际
  `gA=(128,8,512):(1,5120,40960)`，每线程得到
  `tAgA=(4,1,512):(32,0,40960)`；`tAsA` 具有对应 fragment shape，故每轮
  每线程搬 4 个 A 元素。B 同理。
- **compute partition**：`tC=(16,16):(1,16)` 将 128×128 C tile 分成每线程
  8×8 accumulator；投影后的 `tCsA=(8,8)`、`tCsB=(8,8)` 提供该线程计算所需
  的 A 行/B 行数据，`make_tensor_like(tCgC)` 创建寄存器累加器。
- **同步语义**：`cp_async_fence` 提交当前线程的潜在 async group；
  `cp_async_wait<0>` 只等待当前线程自己的 group；第一个 `__syncthreads()` 保证
  CTA 全体写完 smem 后再读，第二个保证全体读完后下一轮才能覆盖 smem。
- **epilogue**：`axpby(alpha,tCrC,beta,tCgC)` 写出
  $C=\alpha AB^T+\beta C$；当 beta=0 时实现会避免读取旧 C。
- **教学版限制**：无边界 predicate，要求 M/N/K 分别被 128/128/8 整除；
  单缓冲、默认 `UniversalFMA`，尚无 Tensor Core MMA 或 copy/compute pipeline；
  当前程序取回结果但没有 CPU/cuBLAS reference 数值比较。
- **易错点已经掌握**：未使用的 `CSmemLayout` 不是 C 的 smem buffer；参数不能
  命名 `_` 以免遮蔽 `cute::_`；三个 ThreadLayout 的 size 必须相等，因为实际
  kernel block 只有同一组物理线程。

### GEMM Tutorial（`sgemm2.cu`，已完成）

- **相对 sgemm1 的核心升级**：裸 ThreadLayout 被 `TiledCopy` / `TiledMMA`
  取代。二者都把「最小 Atom 做什么」与「Atom 如何在线程/数据上铺开」分层；
  `get_slice(threadIdx.x)` 固定物理线程，`partition_S/D/A/B/C` 直接生成该线程的
  source、destination 与 MMA fragment。【用途】今后只换架构专属 Copy/MMA Atom，
  CTA partition 和 mainloop 的上层骨架仍可复用。
- **fragment 最左 mode 的语义**：`(CPY,CPY_M,CPY_K)` 中 `CPY` 是一次
  Copy Atom 吃掉的完整 value fragment；`(MMA,MMA_M/N/K)` 中 `MMA` 是一次
  MMA Atom 所需的线程局部 A/B/C value fragment。它们不是新的矩阵数学维度；
  后面的 M/N/K mode 才是固定线程后在 Atom 外层的重复次数。当前
  `UniversalFMA` 是 1×1×1，故 MMA mode size=1；真正 Tensor Core atom 可让
  A/B/C 的 MMA mode 大小各不相同。
- **TiledCopy 的 layout algebra 已沿源码推导**：
  `layout_mn=raked_product(thr_layout,val_layout)`，
  `layout_tv=right_inverse(layout_mn).with_shape((size(thr),size(val)))`，
  `tiler=product_each(shape(layout_mn))`。当前 NT 配置
  `thr=(32,8):(1,32)`、`val=(4,1):(1,0)`，完整结果为
  `layout_mn=((4,32),(1,8)):((256,1),(0,32))`；继而
  `Tiler_MN=(128,8)`、`TiledLayout_TV=(256,4):(4,1)`。
  - `logical_product(thr,val)=((32,8),(4,1)):((1,32),(256,0))`；
    raked 再做 `zip(get<1>,get<0>)`，所以每个数据 mode 都是 Value 在内、Thread
    在外。`layout_mn` 的 codomain 编码为 `tid + 256*vid`，故四个叶 stride 是
    `(256,1,0,32)`；不能只算 shape 而忽略 stride。
  - 【用途】256 线程每线程一个连续 `4×1` float fragment，合起来恰好覆盖
    `128×8` A/B tile；相邻线程/线程内 value 都连续，服务于合并且 128-bit 的
    gmem load。
- **Universal 操作的定位**：`UniversalCopy<S,D>` 本质是 `dst=src`；以
  `uint128_t` 为打包类型、float 为 Copy Atom 内部类型时，一次处理 4 个 float。
  `UniversalFMA<D,A,B,C>` 是架构无关的 1-thread、1×1×1 标量
  `d=a*b+c`，不是 Tensor Core。当前 TiledMMA 把它沿 MN 铺成 16×16=256
  线程，面对 128×128×8 CTA tile 时每线程外层重复 M/N/K 各 8 次。
- **流水线语义**：循环前预取 tile0 到 `tArA/tBrB`；稳态中先把当前 rmem tile
  写入单份 smem，再预取下一 tile 到同一份 rmem，随后从 smem 计算当前 tile。
  因此同一时刻是「smem=当前、rmem=下一块」；这是 gmem→rmem→smem 的软件
  预取流水，不是 `cp.async`。循环首个 `__syncthreads()` 防止覆盖尚未读完的旧
  smem，第二个保证新 smem 写完再计算；尾轮重复读取最后 tile 以免越界，结果不再使用。
- **官方注释存在已确认的陈旧索引**：加入最左 `CPY/MMA` mode 后，手写展开注释
  没有把旧二维 mode 编号右移。copy 正确循环应遍历 `size<1/2>(tArA)` 的
  CPY_M/CPY_K；gemm 应遍历 `size<1>(tCrC)`、`size<2>(tCrC)`、
  `size<2>(tCsA)` 的 MMA_M/N/K。真正的 `copy(...)` / `gemm(...)` 实现无误，
  只是教程伪代码注释错误。

### MMA Atom

- **MMA atom 四层抽象(0t_mma_atom.md 的骨架,先记框架)**:CuTe 驯服 tensor
  core 的方式。
  - **背景矛盾**:tensor core
    MMA 是**架构专属的裸 PTX 指令**(Volta/Ampere/Hopper/Blackwell 各一套), 且规定「哪个线程必须持有矩阵哪几个元素」(硬件写死的诡异分布)。想写泛型代码就得抽象掉它。
  - **CuTe 解法**:用 `Layout`
    把「线程↔数据」这个别扭映射编码成 layout 对象 → 硬件怪异分布变成可计算/可组合。
  - **四层塔**:`Operation(裸PTX) → MMA_Traits(元信息) → MMA_Atom(可用单元) → TiledMMA(拼大)`:
    - **Operation**:PTX 指令的 struct 包装。只有 ①4 个寄存器别名
      `DRegisters/ARegisters/BRegisters/CRegisters`
      (每线程给指令喂几个/什么类型的寄存器)②一个 `fma()` 静态函数(内含
      `asm volatile` 裸 PTX)。
      **不含 layout/tensor,依赖极少**——只忠实翻译硬件指令,越薄越好。
    - **MMA_Traits<Op>**:这条指令的元信息——`ValTypeA/B/C/D`(逻辑类型)、`Shape_MNK`(逻辑形状)、
      **`ThrID`**(逻辑线程 id→warp 内线程 idx 的映射)、**`ALayout/BLayout/CLayout`**(核心!见下)。
    - **MMA_Atom** = Operation +
      Traits 合体,一个可用的最小 MMA 单元,能造 fragment、作用于 tensor。
    - **TiledMMA** =
      `make_tiled_mma(atom, atom_layout, tiler)`,把小 atom 复制/交错成大 tile 供 GEMM 分区。
  - **★灵魂概念 CLayout/ALayout/BLayout**:`(logical_thr_id, logical_val_id) → 矩阵 (m,n)/(m,k)/(n,k) 坐标`。回答「第 T 号线程持有的第 V 个值,对应矩阵哪个位置」。硬件规定的别扭对应关系被转录成一个 layout 的 shape+stride
    → 线程↔数据分布变成可组合的数学对象。**这就是 CuTe 驯服 tensor
    core 的全部秘密**。
  - **命名规则**(会读即可)`SM70_8x8x4_F32F16F16F32_NT`:架构_MxNxK_(D,A,B,C 类型)_(A/B 转置)。类型从左读对应
    `D=C+A*B`;NT=A 列主(M-major)、B 行主(N-major)。
  - **在 GEMM 里的位置**:MMA atom 只替换 sgemm 三段(分块/mainloop
    copy+gemm/epilogue)里 mainloop 的 **gemm 计算**(把默认 FMA 换成 tensor core
    MMA),数据编排骨架不动。sgemm_1 用默认 FMA(不带 MMA_Atom)。
  - 源码:Operation 在 `include/cute/arch/mma_*.hpp`;Traits 在
    `include/cute/atom/mma_traits_*.hpp`。

- **Operation 层详解 + SM90 vs SM100 对比(以 FP8 E4M3 输入/F32 累加 GEMM 为例)**:
  - **最小范例（非 Tensor Core）**：`SM100_2x1x1_F32F32F32F32`（`arch/mma_sm100.hpp`）其实是 FFMA2（一条 SIMD 指令算 2 个 FMA）。A/C/D=`float2`（真数据），B=`float`（通过 `make_float2(b0,b0)` 广播到两个 lane）。名字中的 `2x1x1` 即 M×N×K：N=1，却靠广播 B 计算两个 M。由此可见，Operation 层只含寄存器别名、`fma()` 与宏保护。
  - **对比对象**：SM90 `MMA_64x64x32_F32E4M3E4M3_SS_TN`（`mma_sm90_gmma.hpp:13613`）与 SM100 `SM100_MMA_F8F6F4_SS`（`mma_sm100_umma.hpp:1214`）。两者均为 D=C+A·B，A/B=FP8（E4M3），C=F32，且 A/B 来自 smem（`_SS`）。FP8 MMA 的累加器只有 F16/F32，**没有 BF16 累加**。
  - **① 组织方式（最大差异）**：SM90 是**枚举式**，一个 struct 对应一条具体指令，形状 64x64x32、类型与转置 TN 全固化在类型名中，因而文件上万行、包含成百上千个 struct。SM100 是**模板参数化**：`template<a_type,b_type,c_type, int M,int N,Major a_major,b_major>` 覆盖一大类指令；`F8F6F4` 加 PTX `kind::f8f6f4` 统一覆盖 8/6/4 位窄浮点，约 2000 行即可。**SM100 的核心转变是：枚举 → 参数化。**
  - **② 寄存器别名**：两者的 A/B 都是 `uint64_t[1]`（smem 描述符，不是真数据），这是 Tensor Core 指令与普通 SIMD 的分水岭。关键差异在 C 累加器：SM90 的 `CRegisters=float[32]`，每线程在**寄存器**中持有 32 个 float；SM100 的 `CRegisters=uint32_t[1]`，它是一个 tmem 地址，累加器转移到 Blackwell 新增的 tmem。两者的 `DRegisters=void`，输出都不经寄存器返回。
  - **③ `fma()` 签名**：SM90 需要逐个列出 32 个 `float& d00..d31`，代码很长；SM100 只传 `uint32_t tmem_c`（累加器地址）等 5 个标量。
  - **④ PTX 与发射方式**：SM90 的 `wgmma.mma_async.sync.aligned.m64n64k32.f32.e4m3.e4m3 {%0..%31},...` 由一个 warpgroup 的 **128 个线程协作**发射，结果落入 32 个寄存器。SM100 的 `if(elect_one_sync()){ tcgen05.mma.cta_group::1.kind::f8f6f4 [%0],... }` 由**单线程 elect** 发射整个 CTA 级 MMA；累加器 `[%0]` 是 tmem 地址，方括号表示访存。
  - **⑤ 不变的壳**：两者均使用 4 个 `?Registers` 别名、一个含裸 PTX 的 `fma()`，以及 `#if defined(..._ENABLED)/#else CUTE_INVALID_CONTROL_PATH` 宏保护。因此接口形状恒定，上层代码不受硬件剧变影响。
  - **演进动机**：Tensor Core 越强，累加器越大，寄存器越不够用。Blackwell 引入 tmem 专放累加器，并让单线程驱动整个 CTA 的 MMA；Operation 层抽象让这种并行和存储模型的变化对上层透明。
  - **一句话演进表**：累加器：寄存器 → tmem；发射：128 线程 → 单线程 elect；组织：枚举 → 模板参数化；FP8：专名 E4M3 → 统一 `kind::f8f6f4`；寄存器压力：大 → 释放。
  - **数据源后缀 `_SS`/`_RS`/`_TS`（两字母=A 源+B 源）**：
    - **S=Shared**：smem 描述符，`uint64_t[1]`；**R=Register**：寄存器真数据，`uint32_t[4]`；**T=Tmem**：tmem 地址，`uint32_t[1]`，SM100 新增。第一字母代表 A 源，第二字母代表 B 源。
    - **SM90 有 SS 和 RS，无 T**；**SM100 有 SS 和 TS，无 RS**。也就是 A 从 smem 读取两代都有，A 从寄存器仅 SM90 有，A 从 tmem 仅 SM100 有。
    - **RS → TS 是同一需求的两代实现**：GEMM 中 A 常是上一步输出或片上暂存，不必绕回 smem。SM90 用寄存器（RS），SM100 用 tmem（TS），因为累加器和 MMA 输出已迁入 tmem。
    - **PTX 证据**：SM100 TS 为 `tcgen05.mma [%0], [%1], %2`，其中 A=`[%1]` 带方括号，表示 tmem 访存；SS 为 `[%0], %1, %2`，其中 A 是不带方括号的描述符。TS 的 `fma()` 首参是 `uint32_t tmem_a`，SS 是 `uint64_t desc_a`。
    - 这与“累加器从寄存器迁到 tmem”处于同一演进链路：Blackwell 系统性地将大块矩阵数据从寄存器迁移到 tmem。
    - 源码：SM90 `mma_sm90_gmma.hpp`（SS:13613，RS:13672）；SM100 `mma_sm100_umma.hpp`（SS:1214，TS:1292）。

### MMA Traits 层:ThrID / ABC Layout(进行中)

- **Traits 层的使命(一句话)**:把硬件强制规定的「线程↔数据」别扭分布,**转录成一个 CuTe layout 的 shape+stride**。一旦变 layout,别扭分布就成了可组合/可计算的数学对象——这是 CuTe 驯服 tensor core 的全部秘密。三主角:`ThrID`(逻辑线程号→warp 物理 idx)、`CLayout`(逻辑(tid,vid)→C 的 (m,n))、`ALayout/BLayout`(→A 的 (m,k)/B 的 (n,k))。

- **★ThrID 的方向 + 输入域(最易绕晕,亲历踩坑)**:以 Volta HMMA 8x8x4 为例,`ThrID = Layout<Shape<_4,_2>,Stride<_1,_16>>`。
  - **输入域永远是连续 [0,8)**(逻辑线程号),size=8。散的集合 `{0,1,2,3,16,17,18,19}` 是它的**输出(range)不是输入**。拆坐标只看 shape(4,2):逻辑 0~3→(0..3,0)→物理 0~3;逻辑 4~7→(0..3,1)、第二维 stride=16→物理 16~19。**「散」是 stride 造出来的,输入本身不散**(呼应早学的「拆分只依赖 shape,乱序来自 stride」)。
  - 想要反方向 `物理 idx→逻辑号`(以散集合为输入域)= `right_inverse(ThrID)`;ThrID 单射故有逆。kernel 里线程手上是 `threadIdx`(物理号),要反查逻辑号才用逆。**存进 Traits 的永远是正方向**(输入连续、shape/stride 干净好组合)。
  - **为何需要 ThrID**:CLayout 用的是**逻辑线程号**(连续 0~7),硬件用物理号(不连续),ThrID 是二者的桥。但注意:**ThrID 和 CLayout 各自独立、互不调用**——CLayout 处理逻辑 4~7 时不查 ThrID 的 16~19,它用自己的 stride 直接算。两套「别扭」分开编码:物理号的别扭只活在 ThrID,数据位置的别扭只活在 CLayout 的 stride。

- **★一个 atom 只管 8 个线程,32 个线程由 TiledMMA 铺(关键认知)**:一个 HMMA 8x8x4 atom 天生只需一个 quadpair(8 线程=QP0=`{0,1,2,3,16,17,18,19}`)。warp 的另外 24 线程属于 QP1/2/3,**不由 ThrID/Traits 管**。`make_tiled_mma` 把这一个 atom 的模式**复制 4 份**盖满整个 warp(拼成 16x16x4,四象限各一个 QP,复制遵循 `(2,2):(2,1)` 的 atom 排布)。**ThrID/CLayout 只描述单个 atom 内部的最小单元;铺满 warp / 更大 tile 是上层 TiledMMA 的事**。
  - warp 拆 QP:QP0`{0-3,16-19}` QP1`{4-7,20-23}` QP2`{8-11,24-27}` QP3`{12-15,28-31}`。

- **★CLayout/ThrID 的输入都是逻辑线程号,使用时「物理→逻辑」自动转(源码印证)**:CLayout 的输入是逻辑 tid 0~7,**不是物理 `threadIdx`**。真实调用链:`物理 threadIdx --right_inverse(ThrID)--> 逻辑 tid --CLayout--> (m,n)`。源码 `include/cute/atom/mma_atom.hpp:405` 有个变量就叫 `thridx_2_thrid`(= `composition(..., right_inverse(...))`),`get_layoutC_TV()` 里 `thrfrg_C(ref_C).compose(thridx_2_thrid, _)` 把这步接上。
  - **但代码上你不手写这步转换**:GEMM kernel 里只写 `auto thr_mma = tiled_mma.get_slice(threadIdx.x); auto tCgC = thr_mma.partition_C(gC);`。`get_slice(threadIdx.x)`(mma_atom.hpp:359)收**物理号**,内部 `get_flat_coord` + `thridx_2_thrid` 自动走完「物理→逻辑→(m,n)」,直接吐给你「本线程负责 C 的哪几个元素」。转换被 CuTe 封装,对你透明。
  - 分层记:**Traits 层(CLayout/ThrID)= 纯逻辑蓝图**(输入输出全逻辑量);**使用层(get_slice/partition_C)= 入口收物理 threadIdx,CuTe 用 right_inverse(ThrID) 自动翻逻辑**。你只管传 `threadIdx.x`。

- **★A/B Layout 与转置后缀(TN/NT/NN/TT)—— 本质:改的是 thread→data 的 ownership**:
  - **逻辑形状恒定**:A 永远 (M,K)、B 永远 (N,K)(gemm 铁律「K 永远最右」)。转置后缀**只改 ALayout/BLayout(哪个线程持有哪个元素),不碰逻辑形状,更不碰 C**(源码 `mma_traits_sm70.hpp`:四种转置 CLayout 全是 `SM70_8x8_32b`,只 A/B 在 `_Row`/`_Col` 切换;第一字母管 A、第二字母管 B)。
  - **ownership 是硬件 PTX 指令写死的,CuTe 只「照文档图如实转录」成 layout,不推导**。TN/NT 是两条不同指令,排布本就不同——**没有一条规则能推所有转置**(「谁连续沿谁铺」只在 TN 恰好成立,NT 硬套就错)。要写 ALayout/BLayout 就照图抄。
  - **★求 ALayout/BLayout 的正确定义(学习者纠正,务必按这个理解,别用「沿哪轴」的结果论)**:
    - **编码固定、与转置无关**:A 的 (m,k)→`m+k*M`、B 的 (n,k)→`n+k*N`(列主编码)。四种转置都用它。
    - **转置只改硬件图给的 ownership** `(T,V)→(m,k)`。
    - **求 layout = 找一个 layout 使 `layout(T,V)` 恰好等于「(T,V) 拥有的那个 (m,k)」按上式编码出的一维 idx**。即 `ALayout = ownership映射(T,V)→(m,k) ∘ 固定编码(m,k)→m+k*M`。验证=逐 `(tid,vid)` 求值对照编码 idx(文档结尾那句)。
    - TN 抄出恰好 `(8,4):(1,8)`、NT 恰好 `((4,2),4):((8,4),1)`,同一定义、只因 ownership 图不同而不同。**「值沿 K/M、线程沿 M/K」只是观察到的结果,不是定义,且 NT 线程维是 `(4,2)`(T0-3 沿 K、T4 跳回 m=4),不能简单说「沿 K」**。
  - 命名层(仅解释内存 major 动机,**别拿来推 ownership**):T/N 相对自然形状 A=[M,K]、B=[K,N](B 是 KN!)转不转;TN=双 K 连续=硬件规范形/文档「简单基准」。

- **★为什么叫「TN」+ 为什么它是规范形(命名动机,人类易读版)**:
  - **T/N 来自 BLAS**(几十年的 gemm 标准):N=No-transpose(不转)、T=Transpose(转)。两字母第一个说 A、第二个说 B,所以 TN = 「A 转、B 不转」。
  - **转不转相对「自然形状 + 列主序」**:A[M,K] 列主→M 连续,转了→K 连续;B[K,N] 列主→K 连续,不转→仍 K 连续。**故 TN = A、B 双双 K 连续**。
  - **为什么硬件偏爱 TN**:K 是收缩维(`C=Σ_k A·B`),每线程沿 K 乘加。K 连续→线程要的那几个值在内存连成一条,一次向量化 load 喂给 tensor core 最顺。所以「双 K 连续」被做成主/优化路径,恰好对应 BLAS 的 TN,文档也拿它当简单基准。
  - **小坑**:PTX 里 CuTe 的 `_TN` 写成 `.row.col.`(逻辑名 vs 物理名:A 转置≡A 从列主变行主)。同一件事两种叫法,能对上号即可,记 CuTe 那套 T/N 为主。

- **CLayout 手算(Volta HMMA 8x8x4,F32 累加器)**:`(T8,V8)→(m,n)`,(m,n) 用列主序编码成一维 index `m+n*M`(M=8)。8 线程各持 8 值 → shape 两 mode 都是 8,但**单一 stride 描述不了跳跃序列,须把 8 层次化拆成 (2,2,2) 每子维一个 stride**。
  - 线程维(固定 V0,走 T0→T7,编码值 0,1,16,17,4,5,20,21):T0→T1=+1、T0→T2=+16、T0→T4=+4 → `Shape<_2,_2,_2>,Stride<_1,_16,_4>`。
  - 值维(固定 T0,走 V0→V7,编码值 0,8,2,10,32,40,34,42):V0→V1=+8、V0→V2=+2、V0→V4=+32 → `Shape<_2,_2,_2>,Stride<_8,_2,_32>`。
  - 合体 `CLayout = Layout<Shape<Shape<_2,_2,_2>,Shape<_2,_2,_2>>, Stride<Stride<_1,_16,_4>,Stride<_8,_2,_32>>>`。求值:给 (tid,vid),各自拆 (2,2,2) 坐标点积求和得编码,反解 m+8n。
  - **F16 累加器则简单得多**:每行 (m,:) 由单线程持有 → `CLayout=Layout<Shape<_8,_8>,Stride<_1,_8>>`。
  - 【用途】回答「第 T 号线程持的第 V 个值落在 C 矩阵哪个 (m,n)」,是造 fragment、拼 TiledMMA、GEMM 分区的地基。
  - 练习文件:`.vscode/cute-learn/examples/10_mma_traits.cpp`(host print,RC=0)。三块:ThrID(输入连续 0..7→输出散 0-3,16-19)、CLayout(逐 T/V 求值反解 (m,n),对上文档表)、A/B TN vs NT(逐线程打印 ownership:TN 每线程一条 K 线、NT 每线程一段 M 且 T4 跳 m=4)。全部与手算/文档吻合。

### MMA:TiledMMA(把 atom 铺成大 tile)

- **已完成第一小步：`make_tiled_mma` 参数①②与「加 atom = 加线程」**（`0t_mma_atom.md:436-466`，练习 `11_tiled_mma.cpp` 已重新编译 RC=0）。
  - `make_tiled_mma(atom, AtomLayoutMNK, PermutationMNK)`：① atom 是硬件最小 MMA；② `AtomLayoutMNK` 指定 atom 沿 M/N/K 的复制位置，因此决定新增哪些线程；③第三参才是最终逻辑 tile 的尺寸/排列，若它比 atom 覆盖范围大，扩大的是每线程的 value fragment，而非线程数。
  - 单 atom 用 `(1,1,1)`：8 个 Volta quadpair 线程、逻辑 tile `8x8x4`。2x2x1 atom 用文档的 n-major `Layout<(2,2,1):(2,1,0)>`：4 个 atom × 8 线程 = 32 线程、逻辑 tile `16x16x4`。其 `ThrLayoutVMNK=((4,2),2,2,1):((1,16),8,4,0)`：M-copy 使物理线程号加 8（T8），N-copy 加 4（T4），两个都加为 T12；四份 atom 正好填满一个 warp 的四个 quadpair。
  - **为何能从散线程拼满 warp**：`tiled_product(AtomThrID, AtomLayoutMNK)` 先由 `complement((4,2):(1,16),32)=4:4` 找到 quadpair 留出的空位，再按 atom layout 将复制品放入。因此并非手写「T4/T8」规则，而是 layout 代数自动把 atom 的线程空缺补齐。
  - 【用途】第二参是设计“谁执行哪块 atom”的线程编址旋钮；2x2 atom 是将 HMMA atom 组成一个 warp 级 16x16x4 MMA 的标准做法。先区分它和第三参，才能避免误以为“tile 变大就需要更多线程”。
  - **练习修正**：`11_tiled_mma.cpp` 曾把文档的 n-major atom 排列写成默认列主序；现显式写为 `(2,2,1):(2,1,0)`，使 T4/T8 所在 C 象限与文档一致。

- **TiledMMA = 对 atom 做 product**(呼应统一视角:product 铺开)。`make_tiled_mma(Atom, AtomLayoutMNK, PermutationMNK)` 三参:
  - **① Atom**:用哪条硬件指令(最小单元,如 Volta 8 线程/8x8x4)。
  - **② AtomLayoutMNK**(rank-3,对应 M/N/K):atom 沿 MNK 各**复制几份、线程怎么编号**。**加 atom = 加线程**。
  - **③ PermutationMNK**(默认 `Tile<_,_,_>` 不排):atom 铺开后同线程数据在某维不连续,用它 scatter 重排成连续(方便设计 smem/寄存器)。**撑大 tile = 加值(每线程多拿),不加线程**。
- **★核心:内部就是 `tiled_product(AtomThrID, AtomLayoutMNK)` → `ThrLayoutVMNK`(4维 V,M,N,K)**(源码 `mma_atom.hpp:225,231`)。
  - 公式链:`tiled_product` = `logical_product` 再 mode 拆包重排;`logical_product(A,B) = (A, complement(A)∘B)`(A 原样 + 补集按 B 铺开)。
  - **散线程能拼成连续的原理**:Volta atom 8 线程 `{0-3,16-19}`(缺 4-15,20-31),**`complement(atom,32)` 算出空缺布局**,`∘B` 把 3 份拷贝按偏移 +4/+8/+12 填进去 → 不重不漏铺满 0-31。那些 stride 不是手算的,是 complement 算的。Volta ThrID 设计成「隔一半」的散布正是为可平铺性。
  - `ThrLayoutVMNK` 四维:V=atom 内线程、M/N=沿 M/N 复制的 atom、K=沿 K。是「物理线程→(哪个 atom,atom 内几号)」的地图,`get_slice(threadIdx)` 靠它分数据(上一轮 `thridx_2_thrid` 的 right_inverse 就在它上面做)。
- **★A/B/C TV-layout 怎么随 TiledMMA 扩展(`thrfrg_C/A/B`,源码 `mma_atom.hpp:252`)**:ThrLayoutVMNK 只管线程铺开;A/B/C 的完整分区是另一条流水线,**核心=复用单 atom 的 CLayout/ALayout/BLayout(「砖」),外面套 divide 铺满整个 tile(「按图纸铺砖」)**。以 C 为例,输入整块 C layout `(M,N)`,4 步:
  1. **Permutation 重排**:`logical_divide(C, <permM,permN>)`——参数③ 起作用处,默认恒等。
  2. **按 atom 尺寸切块**:`zipped_divide(., <AtomM,AtomN>)` → `((AtomM,AtomN),(RestM,RestN))`,把大 tile 分解成「一个 atom 块 + 块排布」。
  3. **★(m,n)→(thr,val)**:`.compose(AtomLayoutC_TV, _)` → `((ThrV,FrgV),(RestM,RestN))`。**单 atom 的 CLayout 在此被复用**,把块内 (m,n) 翻译成 (线程,值)。
  4. **块排布归线程维**:`zipped_divide(., <_,<ThrM,ThrN>>)` → `((ThrV,(ThrM,ThrN)),(FrgV,(RestM,RestN)))`。因 RestM/RestN 个 atom = 不同线程组,故从值维挪到线程维。
  - 产物结构 `((ThrV,(ThrM,ThrN)),(FrgV,(RestM,RestN)))`:线程部分=atom 内线程×沿MN复制的atom=完整线程编址;值部分 FrgV=atom 内每线程值、(RestM,RestN)=参数③撑大 tile 时每线程多拿的值。再套 `thridx_2_thrid`(right_inverse)得最终 `(物理thr,val)→(m,n)`。
  - A/B 同构:C 用 (M,N)+CLayout;A 用 (M,K)+ALayout(取 VMNK 的 `<1>`M `<3>`K,A 不沿 N 分);B 用 (N,K)+BLayout。
  - **实测变形链(练习 `11_tiled_mma.cpp` ex_thrfrg,2x2 TiledMMA,C=(16,16))**:`(16,16)` →Step2 `((8,8),(2,2))`(切 atom 块+块排布)→Step3 `(((2,2,2),(2,2,2)),(2,2))`(**compose 后 (8,8) 块变成单 atom CLayout 原样嵌入**——即 10_mma_traits 手算的那个 `((2,2,2),(2,2,2))`,「砖」被复用的字面证据)→Step4 `(((2,2,2),(2,2)),((2,2,2),(1,1)))`(块排布拆进线程/值两侧;值侧 (1,1) stride 0 = 没加值)。
- **实测印证(练习 `11_tiled_mma.cpp` RC=0)**:
  - `tiled_product((4,2):(1,16), (2,2):(2,1))` = `((4,2),2,2):((1,16),8,4)`,求值得 0-31 全出现,四象限偏移 +0/+4/+8/+12。`complement(atom,32)=4:4`。
  - 1x1x1→8线程 VMNK `((4,2),1,1,1):(...,0,0,0)`;2x2x1→32线程 `((4,2),2,2,1):((1,16),4,8,0)`;**2x2 + `Tile<32,32,4>` 的 VMNK 与 2x2 完全相同**→证实「加值不改 VMNK」。
  - C partition:线程 0/4/8/12 落 16x16 的四象限(+0/+8m/+8n/+8m8n),印证 2x2 atom 铺满 C。线程0 的 8 个 (m,n) 与单 atom CLayout 手算一致。

### MMA Traits:Hopper GMMA + SM70/90/100 演进

- **Hopper GMMA vs Volta HMMA(规模巨变)**:协作单位 8 线程(quadpair)→ **128 线程(warpgroup=4 warp)**;形状 8x8x4 → **64xNx16**;同步 → **异步 `wgmma.mma_async`**;A/B 来源寄存器 → **直接吃 smem 整块**。
- **ThrID = `Layout<_128>`**:128 连续线程,逻辑号=物理号,无 Volta 的 quadpair 跳跃(Volta 用 stride=16 是因 QP 是 warp 子集)。
- **CLayout(core matrix 分层搭建,方法同 Volta:观察硬件图→每个重复方向加一个 (子shape:子stride))**:从 8x8 core matrix 起,沿 M 堆 8 次、下一 core matrix 回 T0V2、4 warp 沿 M 重复 → 64x8 基元;64xN 再沿 N 铺 N/8 次。`SM90_64x128` = `((_4,_8,_4),(_2,_2,_16)):((_128,_1,_16),(_64,_8,_512))`(源码 `mma_traits_sm90_gmma.hpp:434`)。记方法别记数字。
- **★A/B Layout = stride=0 广播(最大哲学转变)**:`ABLayout<M,K> = (_128,(M,K)):(_0,(_1,M))`(源码:465)。**线程维 stride=0 → 128 线程全映到 (0,0)**。因 GMMA 描述符建在整块 smem tile 上,每线程看到整块、硬件自己去 smem 取,不按线程切。Volta ALayout=线程↔数据精细分配;Hopper=整块 smem 描述+线程维广播(呼应早学的 stride=0 广播)。
- **★SM70→90→100 演进主线:MMA「线程参与度」8→128→1**:
  - ThrID:`(4,2):(1,16)` → `_128` → **`_1`**(SM100 单线程 elect 驱动整个 CTA)。
  - A/B 来源:寄存器精细切 → smem 整块(线程维 stride=0 广播)→ tmem/smem(**连线程维都塌成 shape=1**)。
  - CLayout:层次化 → core-matrix 更复杂 → **平凡 `(_1,(M,N))`**(源码 `mma_traits_sm100.hpp:180`)。
  - 本质:线程不再参与数据切分(累加器搬进 tmem 由硬件管)→ traits 从「精细描述线程分布」退化到「根本不用描述」。与 Operation 层演进(累加器 寄存器→tmem、发射 128线程→单线程 elect)同源,这是它在 Traits 层的镜像。

## 待办 / 下次从这里继续

已完成:00 + 01 大半(tuple 底层、部分静态、坐标机制、size/cosize、层次化 shape),
02:compatible、crd2idx/idx2crd、coalesce、composition(含 by-mode + 用途)、complement、logical_divide(1-D/2-D 拆解 + 四变体 +
make_tile 坑)、logical_product(含 blocked/raked) —— 02_layout_algebra 主体完成。
**03_tensor.md 已学完**:Tensor=Engine+Layout、owning/nonowning、tagging、三种访问、slice(`_`
规则)、partition(inner/outer=local_tile/local_partition、TV-partition)、Tensor 只 divide 不 product。练习
`08_tensor.cpp`。
**04_algorithms.md 已学完**:copy(类型 dispatch→cp.async/向量化,不转置,需同步)、gemm(V/M/N/K 记号、mode 数 dispatch、B=(N,K) 约定)、axpby/fill/clear 工具。练习
`09_algorithms.cpp`(RC=0)。 **当前硬件事实**：RTX 5080，程序报告 SM120、84
SM；GPU 示例使用 `-arch=sm_120`，所有编译产物放到
`.vscode/cute-learn/build/`。旧的 L20Z/SM89 数据不得继续使用。

**`sgemm1` 已学完**：见上方「GEMM Tutorial（已完成）」；本阶段重点包括完整
CTA/thread 两级 partition、gmem→smem copy、同步、默认 FMA mainloop 与 epilogue。

**`sgemm2` 已学完**：见上方对应小节；本阶段重点是 TiledCopy/TiledMMA 的 Atom +
铺开分层、CPY/MMA fragment mode、`raked_product → right_inverse → product_each`
构造链，以及 gmem→rmem→smem 的单级预取流水。

**`0t_mma_atom.md` 阶段进度保留（当前暂停）**。已学:四层抽象框架(Operation/Traits/Atom/
TiledMMA)、CLayout/ALayout/BLayout 灵魂概念、Operation 层详解 + SM90 vs
SM100 对比(FP8 例)。**★Volta 一节全部学完 + print 验证过(练习 `10_mma_traits.cpp` RC=0)**:ThrID(方向/输入域连续 0-7 输出散 / 与 CLayout 独立 / 物理→逻辑用 right_inverse 且被 get_slice 封装)、atom 只管 8 线程 TiledMMA 铺满 32、CLayout 手算(Volta 8x8x4 F32/F16)、A/B Layout(TN vs NT = 改 thread→data ownership、求 layout 的正确定义=ownership∘固定编码、命名来自 BLAS + 双 K 连续动机)——详见上「MMA Traits 层」小节各条。 **约定:后续 MMA 学习都做 SM90 vs
SM100 对比(用户明确要求)。**架构主线现改为 SM90 → SM100；SM70/SM80 的既有
Traits/Atom 知识保留，但不再学习 `examples/cute/tutorial/sgemm_sm70.cu` 与
`sgemm_sm80.cu`。** 当前先暂停 GEMM/进阶 MMA 主线，下一步：

1. **先学 predication**：阅读 `media/docs/cpp/cute/0y_predication.md`，掌握恒等
   coordinate tensor、边界谓词的构造，以及 `copy_if` 如何让非整除 M/N/K 的 tile
   安全访问；保持手算 + 打印验证。
2. **再学 TMA tensor**：阅读 `media/docs/cpp/cute/0z_tma_tensors.md`，掌握 TMA
   descriptor/tensor、gmem↔smem tile 搬运及其 layout/坐标语义，为 Hopper 教程做准备。
3. **GEMM 教程整体顺延**：完成 0y/0z 后，先读
   `examples/cute/tutorial/hopper/wgmma_sm90.cu` → `wgmma_tma_sm90.cu`；随后按顺序读
   `examples/cute/tutorial/blackwell/01_mma_sm100.cu`、`02_mma_tma_sm100.cu`、
   `03_mma_tma_multicast_sm100.cu`、`04_mma_tma_2sm_sm100.cu`、
   `05_mma_tma_epi_sm100.cu`，重点追踪 UMMA/tmem、TMA、multicast、双 SM 与 epilogue
   如何逐层加入。
4. 保持打印驱动 + 手算先行 + 原理/用途双轨；一次学习量别太大（用户要求小步走）。
