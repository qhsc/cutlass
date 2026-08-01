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

- 仓库根:`/cpfs/user/baiheng/code/cutlass`
- 练习目录:`.vscode/cute-learn/`
- CUDA:`/usr/local/cuda`(nvcc 12.9),纯 layout 代码不需要 GPU 即可编译运行。
- 已配好:
  - `.vscode/c_cpp_properties.json`(include path + `__CUDACC__`,IntelliSense 用)
  - `.vscode/cute-learn/Makefile`
  - `.vscode/tasks.json`

**练习文件组织(已压缩归档)**:所有示例按主题合并进 `.vscode/cute-learn/examples/`
下 7 个文件, 编号即学习顺序:`01_layout` `02_tuple` `03_coalesce`
`04_composition` `05_complement` `06_divide`
`07_product`。每个原示例在文件内独立 namespace(`ex_xxx::run()`),末尾 main 顺序调用。

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
- [x] 见过第一张 layout 图:`(8,4):(1,8)`
      = 列主序映射:偏移 = 行×stride[0] + 列×stride[1] = 行×1 + 列×8
- [ ] **进行中的练习**:把 `.vscode/cute-learn/examples/01_layout.cpp`。
- [ ] 尚未正式开讲 01_layout.md 的完整内容

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
    - `tiled_divide`: `((TileM,TileN),RestM,RestN,L)`;`flat_divide`:
      `(TileM,TileN,RestM,RestN,L)`。
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
    | **outer** | `tiled(idx, make_coord(_,_))`   | rest 编号 | 「给每线程它在各 tile 的落点」细粒度→thread | `local_partition` |
  - **为何方向相反**:inner 保 tile 内容、遍历 tile 编号(分给 block);outer 固定线程、遍历它在各 tile 的落点(分给 thread)。
  - 实测:`tiled(mc(_,_),(1,2))` 与 `local_tile(T,tiler,(1,2))` **地址完全相同**
    → 证实 local_tile =
    inner_partition 别名。inner 块起点 offset 用 zipped 后 mode 的 stride 算(`1*4+2*64=132`)与实测指针偏移吻合。
  - `local_partition(T, Layout, Idx)` =
    rank-sensitive 的 outer 包装:用 Layout 的逆把 Idx 转成 Coord, 再按 Layout 顶层 shape 造 Tiler
    → 可指定行主/列主/任意的线程排布来划分。
  - **两级划分**:GEMM 先 `local_tile` 把大矩阵分给 CTA,再 `local_partition`
    把 tile 分给线程。
  - **TV-partition(thread-value)**:造一个 TV-layout 把 (线程id, 值id)→目标数据坐标,`composition(A, tv_layout)`
    变形后 slice 线程维 → 每线程拿到它那几个值(按 TV 规定的形状/顺序)。MMA 里线程拿寄存器片段用此。
  - 练习:`.vscode/cute-learn/examples/08_tensor.cpp`。

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

## 待办 / 下次从这里继续

已完成:00 + 01 大半(tuple 底层、部分静态、坐标机制、size/cosize、层次化 shape),
02:compatible、crd2idx/idx2crd、coalesce、composition(含 by-mode + 用途)、complement、logical_divide(1-D/2-D 拆解 + 四变体 +
make_tile 坑)、logical_product(含 blocked/raked) —— 02_layout_algebra 主体完成。
**03_tensor.md 已学完**:Tensor=Engine+Layout、owning/nonowning、tagging、三种访问、slice(`_`
规则)、partition(inner/outer=local_tile/local_partition、TV-partition)、Tensor 只 divide 不 product。练习
`08_tensor.cpp`。
**04_algorithms.md 已学完**:copy(类型 dispatch→cp.async/向量化,不转置,需同步)、gemm(V/M/N/K 记号、mode 数 dispatch、B=(N,K) 约定)、axpby/fill/clear 工具。练习
`09_algorithms.cpp`(RC=0)。 **环境重要事实**:有 GPU!8× NVIDIA
L20Z(报告为 SM90,132
SM)。sgemm 能真编译真跑。sgemm_1 编译命令:`cd examples/cute/tutorial && nvcc -I ../../../include -I ../../../tools/util/include -std=c++17 -arch=sm_89 --expt-relaxed-constexpr -o /tmp/sgemm_1 sgemm_1.cu`;默认 5120³ 跑出 ~31.7
TFLOP/s。

**进行中:0t_mma_atom.md(按文档编号顺序学 MMA,用户要求)**。已学:四层抽象框架(Operation/Traits/Atom/
TiledMMA)、CLayout/ALayout/BLayout 灵魂概念、Operation 层详解 + SM90 vs
SM100 对比(FP8 例)。 **约定:后续 MMA 学习都做 SM90 vs
SM100 对比(用户明确要求)。** 下一步:

1. **继续 0t_mma_atom.md:Traits 层**——ThrID /
   CLayout/ALayout/BLayout 怎么把线程↔数据映射编码成 layout (0t 文档 Volta HMMA
   8x8x4 的手算 stride 推导是最佳教材)。仍做 SM90 vs SM100 对比。
2. 然后 TiledMMA(make_tiled_mma 拼 atom)。
3. 再进 `0x_gemm_tutorial.md` +
   `sgemm_1.cu`(把 MMA/layout/partition/copy/gemm 全串进真实 GEMM)。sgemm_1 用默认 FMA;骨架吃透后把 gemm 换成 MMA_Atom。然后 sgemm_2(pipeline)。
4. 之后按需:TMA(`0z_tma_tensors.md`)/ predication(`0y_predication.md`)。
5. 保持打印驱动 + 手算先行 + 原理/用途双轨;一次学习量别太大(用户要求小步走)。
