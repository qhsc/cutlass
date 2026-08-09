# CuTe Layout Algebra 计算参考

> 正规化参考手册。用 LaTeX 公式 + 分支/变体关系，精确描述 layout 代数的每个运算。
> 配套源码 `include/cute/layout.hpp`，文档 `media/docs/cpp/cute/02_layout_algebra.md`。
> 学习笔记(直觉版、踩坑记录)见 `memory.md`；本文件只放**定义、公式、结构关系**。

---

## 记号约定

| 记号 | 含义 |
| --- | --- |
| $L = S : D$ | Layout，shape $S$、stride $D$ |
| $L(c)$ | 把坐标/一维序号 $c$ 映射到内存偏移(layout 作为函数) |
| $\operatorname{size}(L)$ | 定义域大小 $=\prod_k s_k$ |
| $\operatorname{cosize}(L)$ | $L(\operatorname{size}-1)+1$ = 最大偏移 $+1$ |
| $A \circ B$ | composition(复合)，$(A\circ B)(c)=A(B(c))$ |
| $A^{*}$ | $\operatorname{complement}(A, M)$，A 关于目标大小 $M$ 的补 |
| $A \oslash B$ | logical_divide(切分) |
| $A \otimes B$ | logical_product(铺开) |
| $\langle t_0,t_1,\dots\rangle$ | Tiler(尖括号)，`make_tile` 构造，by-mode 逐维作用 |
| $(l_0,l_1,\dots)$ | 拼接的 sublayout(圆括号)，整体作用 |

**核心不变量**：layout 本质是函数 $\text{坐标}\mapsto\text{偏移}$。所有代数运算对该函数封闭(结果仍是 layout)。

---

## 0. 基础量

拆坐标(1-D 序号 $j\to$ 多维坐标)，**只依赖 shape**：

$$
j_0 = j \bmod s_0,\qquad
j_k = \left\lfloor \frac{j}{s_0 s_1 \cdots s_{k-1}} \right\rfloor \bmod s_k
$$

算偏移(坐标 $\cdot$ stride)：

$$
\operatorname{offset} = \sum_k j_k \, d_k
$$

$$
\operatorname{size}(S{:}D)=\prod_k s_k,\qquad
\operatorname{cosize}(S{:}D)=1+\sum_k (s_k-1)\,d_k
$$

$$
\text{compact} \iff \operatorname{size}=\operatorname{cosize}\quad(\text{无空洞})
$$

### 单射 / 满射 / 双射(layout 作为函数)

把 layout 看作函数 $L:\ [0,\operatorname{size})\to[0,\operatorname{cosize})$，$c\mapsto\text{offset}$：

$$
\begin{aligned}
&\textbf{单射 (injective):} && \forall\, c_1\ne c_2,\ L(c_1)\ne L(c_2) &&\text{(无坐标碰撞,不同坐标必落不同偏移)}\\
&\textbf{满射 (surjective):} && \forall\, o\in[0,\operatorname{cosize}),\ \exists\, c,\ L(c)=o &&\text{(值域填满整个陪域,无空洞)}\\
&\textbf{双射 (bijective):} && \text{单射} \wedge \text{满射} &&\text{(一一对应)}
\end{aligned}
$$

**与 size/cosize 的关系**：

$$
\begin{aligned}
&\text{单射} &&\iff \text{值域含 } \operatorname{size} \text{ 个不同偏移}\\
&\text{compact}\ (\operatorname{size}=\operatorname{cosize}) &&\iff L \text{ 是到 } [0,\operatorname{size}) \text{ 的双射}\quad(\text{无碰撞且无空洞})
\end{aligned}
$$

- $\operatorname{stride}=0$ 的维度 $\Rightarrow$ **非单射**(该维所有坐标塌到同一偏移，即广播)。
- 单射有空洞($\operatorname{size}<\operatorname{cosize}$，如 $4{:}2$)$\Rightarrow$ 无碰撞但非满射。

**为何重要**：complement 要求输入单射；反向 $\operatorname{idx2crd}$ 要求单射；$(A, A^{*})$ 拼接是双射(见 §3、§8)。

---

## 1. Coalesce(化简)

**作用**：保持一维函数值不变，合并相邻 mode，降到 $\operatorname{depth}\le 1$(分数约分)。

**后置条件**：$\operatorname{size}$ 不变；$\forall i,\ \text{result}(i)=L(i)$。

**规则**(flatten 后，对相邻 $s_0{:}d_0 \mathbin{{+}{+}} s_1{:}d_1$ 反复应用)：

$$
\begin{aligned}
&s_0{:}d_0 \mathbin{{+}{+}} 1{:}d_1 &&\Rightarrow\ s_0{:}d_0 &&\text{(丢弃 size=1)}\\
&1{:}d_0 \mathbin{{+}{+}} s_1{:}d_1 &&\Rightarrow\ s_1{:}d_1 &&\\
&s_0{:}d_0 \mathbin{{+}{+}} s_1{:}d_1 &&\Rightarrow\ (s_0 s_1){:}d_0 &&\text{当 } d_1 = s_0 d_0\ \text{(连续判据)}\\
&\text{否则} &&\Rightarrow\ (s_0,s_1){:}(d_0,d_1) &&\text{(有空隙)}
\end{aligned}
$$

**关键约束**：连续判据 $d_1 = s_0 d_0$ 在**编译期**求值。动态 stride 即使数值满足也**不合并**(结果 mode 数是类型，不能依赖运行时值)→ 需化简的维度用静态 `Int<>`。

**by-mode 变体**：$\operatorname{coalesce}(L, \text{trg\_profile})$ — 化简但**锁定结果的维度数**。

`trg_profile`(如 `Step<_1,_1>`)是一张模板：有几个 `_1` 就保留几个 mode。普通 coalesce 合到最简(可能塌维)，by-mode 版**只在每个 mode 内部合并，不跨 mode 合**，结果稳定为指定 rank。

$$
\begin{aligned}
L=(2,4){:}(1,2)\ (\text{连续}):\quad
&\operatorname{coalesce}(L) = 8{:}1 &&\text{(塌成 1-D)}\\
&\operatorname{coalesce}(L,\ \texttt{Step<\_1,\_1>}) = (2,4){:}(1,2) &&\text{(锁定 2-D,不跨 mode 合)}
\end{aligned}
$$

**动机**：下游代码常假定固定 rank(如 C 永远 2-D，用 $\operatorname{layout}\langle 1\rangle$ 取 N)。若 M/N 恰好连续，普通 coalesce 塌成 1-D 会让 $\operatorname{layout}\langle 1\rangle$ 越界。by-mode 版既清理内部冗余嵌套，又保住维度结构。

---

## 2. Composition(复合)—— 代数核心

**定义**：

$$
R = A \circ B,\qquad R(c) = A\big(B(c)\big)
$$

直观：B 决定定义域，A 决定去向。

**后置条件**：

- $\operatorname{compatible}(B, R)$
- $\forall\, i < \operatorname{size}(B),\quad R(i) = A(B(i))$

**分配律**(对 B 的 mode)：

$$
A \circ (B_0, B_1, \dots) = (A\circ B_0,\ A\circ B_1,\ \dots)
$$

故只需定义最简情形：

$$
A \circ (s{:}d) = \text{「从 A 中每隔 } d \text{ 取一个，共取 } s \text{ 个」}
$$

计算分两步，作用于 A 的 shape(**前缀因子分解**，非算术）：

$$
\begin{aligned}
\textbf{Step 1}\ &\ \text{shape}/d : &&\text{沿前缀劈掉因子 } d，\text{丢前缀留后缀；同时 stride } d_{\text{old}}\!\cdot\! d &&\text{(跳到步长 } d)\\
\textbf{Step 2}\ &\ \text{shape}\%s : &&\text{沿前缀凑出因子 } s，\text{留前缀其余置 1} &&\text{(截取长度 } s)
\end{aligned}
$$

shape 例：

$$
(6,2)/2=(3,2),\quad (6,2)\%2=(2,1),\quad (3,6,2,8)/9=(1,2,2,8),\quad (3,6,2,8)\%9=(3,3,1,1)
$$

含 stride 完整例(印证 stride 变化)：

$$
8{:}3 \ \circ\ 4{:}2 \ =\ 4{:}6\quad(\text{取 4 个，步长 } 2\Rightarrow\text{stride } 3\cdot2=6)
$$

**可除性条件**(编译期静态检查，不满足报错)：

- Step 1 需 **stride** divisibility
- Step 2 需 **shape** divisibility

**语义角色**：

- $A$ = 数据物理排布
- $B$ = 逻辑访问视角
- $R = A\circ B$ = 逻辑坐标 $\to$ 真实偏移

切子块 / 重排遍历 / 线程分配，数学上都是同一个 composition。

### 2.1 By-mode composition

Tiler 版(第二参 $\langle t_0,t_1\rangle$)：对 A 每个 mode 独立复合。

$$
\operatorname{composition}(A, \langle t_0, t_1\rangle) \equiv \operatorname{make\_layout}\big(\,A_0\circ t_0,\ A_1\circ t_1\,\big),\quad A_i = \operatorname{layout}\langle i\rangle(A)
$$

普通 composition 把 A 当一维函数整体；by-mode 对每个维度分别切(tiling 基础)。

---

## 3. Complement(补)—— divide/product 前置

**定义**：$R := \operatorname{complement}(A,M)$ = 为把 A 密集填满大小 $M$ 的空间，需额外添加的 mode 的 layout(下文用 $R$ 指代该结果)。

**前提**：A 单射(源码 Non-injective 检查)。

**精确性质**：$(A,\ R)$ 拼起来 $\operatorname{size}=\operatorname{cosize}=M$，是 $[0,M)$ 上双射(compact)。

**三性质(保证唯一)**：

$$
\begin{aligned}
&\text{(1) 有界：} && \operatorname{size}(R),\ \operatorname{cosize}(R)\le M\\
&\text{(2) 有序：} && \forall i,\ R(i) > R(i-1)\quad(\text{stride 严格递增}\Rightarrow\text{唯一})\\
&\text{(3) 不相交：} && \forall i,j,\ R(i)\ne A(j)
\end{aligned}
$$

**算法**(排序 + 累积扫描)，初始 $\text{cur}=1$：

$$
\begin{aligned}
&\text{filter A(去 size=1/stride=0),按 stride 升序排}\\
&\text{对每个 } s{:}d\ (\text{stride 升序}):\quad \text{产出 } \Big(\tfrac{d}{\text{cur}}\Big){:}\text{cur},\quad \text{cur} \leftarrow d\cdot s\\
&\text{收尾：产出 } \Big(\tfrac{M}{\text{cur}}\Big){:}\text{cur}\quad(\text{整体重复次数});\quad \text{最后 coalesce}
\end{aligned}
$$

($d/\text{cur}$ 与 $M/\text{cur}$ 必须整除)。两类效果：**填洞** / **重复平铺**。例：

$$
\begin{aligned}
\operatorname{complement}((2,2){:}(1,6),\,24) &= (3,2){:}(2,12)\\
\operatorname{complement}(4{:}2,\,24) &= (2,3){:}(1,8)\\
\operatorname{complement}(4{:}1,\,24) &= 6{:}4 \quad(\text{纯重复})\\
\operatorname{complement}(6{:}4,\,24) &= 4{:}1 \quad(\text{纯填洞})\\
\operatorname{complement}((4,6){:}(1,4),\,24) &= 1{:}0 \quad(\text{已满})
\end{aligned}
$$

---

## 4. Divide(切分)—— 切成「tile 内 + tile 间」

### 4.1 核心定义

$$
A \oslash B \ :=\ A \circ (B,\ B^{*}),\qquad B^{*}=\operatorname{complement}\big(B,\ \operatorname{shape}(\operatorname{coalesce}(A))\big)
$$

$B$ = tiler(tile 内部)，$B^{*}$ = 剩余(tile 之间)。$(B,B^{*})$ = 完整坐标系，$A\circ$ 映回内存。**无元素丢弃**。

**结果**：mode-0 $= A\circ B$(tile 本身)；mode-1 = 遍历各 tile。即 **divide = composition + 组织剩余**。

### 4.2 四变体(mode 重排，不改数据)

设 $\text{Layout}=(M,N,L,\dots)$，$\text{Tiler}=\langle T_M,T_N\rangle$，记 tile 分量 $T$、rest 分量 $R$：

| 变体 | 结果形状 |
| --- | --- |
| `logical_divide` | $((T_M,R_M),\,(T_N,R_N),\,L,\dots)$ |
| `zipped_divide`  | $((T_M,T_N),\,(R_M,R_N,L,\dots))$ ★最常用 |
| `tiled_divide`   | $((T_M,T_N),\,R_M,\,R_N,\,L,\dots)$ |
| `flat_divide`    | $(T_M,\,T_N,\,R_M,\,R_N,\,L,\dots)$ |

**源码关系**：

$$
\begin{aligned}
\text{zipped\_divide}(A,B) &= \operatorname{tile\_unzip}\big(\text{logical\_divide}(A,B),\ B\big)\\
\text{tiled\_divide}(A,B) &= \text{zipped\_divide 后拆第二 mode}\\
\text{flat\_divide}(A,B) &= \text{zipped\_divide 后拆两个 mode}
\end{aligned}
$$

**分量语义**：$T$ = 瓦片内部大小(块内)；$R$ = 瓦片个数(块间)。

### 4.3 使用约束

- **2-D divide 必须用 `make_tile`(Tiler)**，不能用 `make_layout`(拼接常非单射 $\to$ complement 报错)。
- Tiler 的 rank 可 $<$ layout 的 rank：未覆盖维度原样保留。
- 反向 `idx2crd` 要求 layout 单射(见 §8)。

---

## 5. Product(铺开)—— divide 的对偶

### 5.1 核心定义

$$
A \otimes B \ :=\ (A,\ A^{*}\circ B),\qquad A^{*}=\operatorname{complement}\big(A,\ \operatorname{size}(A)\cdot\operatorname{cosize}(B)\big)
$$

mode-0 $=A$ 原样；mode-1 $=A^{*}\circ B$：$A^{*}$ 是所有可用槽位，$\circ B$ 按 B 挑选排序($B$.size = 份数，$B$.stride = 顺序/间隔)。

### 5.2 divide / product 对偶

$$
\begin{aligned}
\text{divide:} &\quad A \oslash B = A \circ (B, B^{*}) &&\text{composition 作用于外层，选中放外}\\
\text{product:} &\quad A \otimes B = (A,\ A^{*}\circ B) &&\text{composition 作用于内层，A 原样拼上}
\end{aligned}
$$

1-D 结果同构：$(2,2){:}(4,1)\otimes 6{:}1$ 与对应 1-D divide 结果相同。

### 5.3 四变体(★仅在尖括号 Tiler 下成立)

设 $\text{block}=(M,N,L,\dots)$，$\text{Tiler}=\langle T_M,T_N\rangle$：

| 变体 | 结果形状 |
| --- | --- |
| `logical_product` | $((M,T_M),\,(N,T_N),\,L,\dots)$ |
| `zipped_product`  | $((M,N),\,(T_M,T_N,L,\dots))$ |
| `tiled_product`   | $((M,N),\,T_M,\,T_N,\,L,\dots)$ |
| `flat_product`    | $(M,\,N,\,T_M,\,T_N,\,L,\dots)$ |

源码关系与 divide 对偶($\operatorname{tile\_unzip}$ / 拆包)。

**⚠️ 前提**：上表要求 Tiler 是尖括号(by-mode)，对 A 每个 mode 分别 product。**若 B 是单个 layout(非 Tiler)**：

$$
\text{logical\_product}(A,\ B_{\text{single}}) = (\underbrace{A}_{\text{整块}},\ \underbrace{A^{*}\circ B}_{\text{整块}})
$$

例 $(2,2)\otimes(3,4)=((2,2),(3,4))$，**不逐方向配对**，且此时 $\text{zipped}=\text{logical}$。

### 5.4 Rank-sensitive：blocked / raked

友好接口：A(瓦片)、B(阵列)各普通 layout，自动 rank 对齐。**内部算法**，$R=\max(\operatorname{rank}A,\operatorname{rank}B)$：

$$
\begin{aligned}
\text{result} &= \text{logical\_product}\big(\operatorname{append}_R A,\ \operatorname{append}_R B\big)\\
\text{blocked} &= \operatorname{zip}\big(\text{result}\langle 0\rangle,\ \text{result}\langle 1\rangle\big)\\
\text{raked} &= \operatorname{zip}\big(\text{result}\langle 1\rangle,\ \text{result}\langle 0\rangle\big)\quad(\text{顺序反})
\end{aligned}
$$

**结果形状**(2-D)：

$$
\begin{aligned}
\text{blocked} &= ((T_{\text{row}},A_{\text{row}}),\,(T_{\text{col}},A_{\text{col}})) &&\text{瓦片成块(局部性)}\\
\text{raked} &= ((A_{\text{row}},T_{\text{row}}),\,(A_{\text{col}},T_{\text{col}})) &&\text{瓦片交错(合并访存)}
\end{aligned}
$$

**blocked/raked vs 四变体**：

| | 重排操作 | 改 stride? | 含义 |
| --- | --- | --- | --- |
| logical/zipped/tiled/flat | $\operatorname{tile\_unzip}$ / 拆包 | ✗ 否 | 只换括号 |
| blocked / raked | $\operatorname{zip}$(进组咬合) | ✓ 是 | 换数据物理排布 |

同源($\text{logical\_product}$ 打底)，粒度不同(分两堆 vs 咬合)，并列兄弟非特例。

---

## 6. Tensor view 操作：CTA tile 与线程 partition

这些操作只构造新的 **Tensor view**：改变 engine 的起点和 layout，不搬运数据。真正的数据搬运由后续 `copy(src,dst)` 完成。

### 6.1 共同底层：`zipped_divide` 后切一个 mode

标准 `zipped_divide` 接收一个数据 layout $T$ 和 tiler $B$：

$$
D=\operatorname{zipped\_divide}(T,B)
  =\big(\underbrace{\text{Tile}}_{\text{tile 内坐标}},
         \underbrace{\text{Rest}}_{\text{tile 编号}}\big)
$$

若 $B$ 是 Layout，它的 **shape 和 stride 都参与 divide**。若 $B$ 只是 Shape，则它会被解释成 compact tiler。

在 $D$ 上有两种基本切片：

| 基本操作 | 切片 | 固定 | 保留 |
| --- | --- | --- | --- |
| `inner_partition(T,B,q)` | $D(\_,q)$ | Rest 中的第 $q$ 个 tile | 一个完整 Tile |
| `outer_partition(T,B,p)` | $D(p,\_)$ | Tile 内第 $p$ 个位置 | 该位置跨所有 tile 的 Rest |

**1-D 最小例**：$T=16{:}1$，$B=4{:}1$：

$$
D(u,r)=T(u+4r)
$$

所以：

$$
\begin{aligned}
D(\_,2)&=[8,9,10,11] &&\text{第 2 个完整 tile}\\
D(2,\_)&=[2,6,10,14] &&\text{所有 tile 内的位置 2}
\end{aligned}
$$

`local_tile` 建立在 inner partition 上；`local_partition` 建立在 outer partition 上。但两者不只是切片方向不同，它们构造 divide tiler 的方式也不同。

### 6.2 `local_tile(T,B,q)`：选择一个 CTA tile

`local_tile` 将调用者给出的完整 tiler $B$ 原样交给 `zipped_divide`，再固定 Rest 坐标 $q$：

$$
\boxed{
\operatorname{local\_tile}(T,B,q)
=\operatorname{zipped\_divide}(T,B)(\_,q)
}
$$

这里：

- $B$ 决定 tile 内怎样取数据；
- $q$ 是 tile 编号，不是 tile 内元素坐标；
- 输出保留 Tile mode。

当 $B$ 是 Shape / Tile tuple、对 $T$ 的各 mode 分别使用 compact tiler
$B=(b_0,\ldots,b_{r-1})$ 时：

$$
\operatorname{local\_tile}(T,B,q)(u)
=T(u_0+b_0q_0,\ldots,u_{r-1}+b_{r-1}q_{r-1})
$$

这里必须区分两类长得相似、代数含义却不同的参数：

| 第二参数 | `logical_divide` 的行为 |
| --- | --- |
| Shape / Tile tuple，如 `make_shape(_32,_8)` | 对 $T$ 的对应 mode 分别 divide |
| 单个 Layout，如 `Layout<Shape<_32,_8>,Stride<_1,_32>>` | 把它作为一个整体 tiler，与 $T$ 的整个逻辑域 composition |

所以“一个 Layout 的 shape 是 tuple”不等于“它是一个 by-mode tiler tuple”。源码也对应两个不同重载：`logical_divide(Layout,Tuple)` 使用 `transform_layout` 逐 mode 处理；`logical_divide(Layout,Layout)` 直接计算整体 composition。

非 compact tiler 必须按真正的 Layout Algebra 计算。例如：

$$
\operatorname{zipped\_divide}(16{:}1,4{:}2)
=(4,(2,2)):(2,(1,8))
$$

于是：

$$
\begin{aligned}
\operatorname{local\_tile}(T,4{:}2,0)&=[0,2,4,6]\\
\operatorname{local\_tile}(T,4{:}2,1)&=[1,3,5,7]
\end{aligned}
$$

这说明 $B$ 的 stride $2$ 确实进入了数据 divide，而不是被忽略。

**投影版本**：`local_tile(T,B,q,Step)` 先按 `Step` 选出与 $T$ 对应的 $B,q$ mode，再执行上述运算。例如 `Step<_1,X,_1>` 选择第 $0,2$ mode，跳过第 $1$ mode。

**SGEMM A 的实例**：

$$
\begin{aligned}
mA &: (M,K):(d_M,d_K)\\
B&=(128,128,8),\qquad q=(\texttt{blockIdx.x},\texttt{blockIdx.y},\_)\\
gA&=\operatorname{local\_tile}(mA,B,q,\texttt{Step<\_1,X,\_1>})
\end{aligned}
$$

投影后使用 $(128,8)$ 和 $(\texttt{blockIdx.x},\_)$：

$$
\begin{aligned}
gA &: (128,8,K/8):(d_M,d_K,8d_K)\\
gA(m,k,k_t)&=mA(m+128\,\texttt{blockIdx.x},\ k+8k_t)
\end{aligned}
$$

M 的 tile 编号被固定进 view 起点；K 的 Rest 坐标 $k_t$ 被保留，供 mainloop 遍历。

### 6.3 `local_partition(T,P,t)`：选择一个 worker fragment

这里 $P$ 不是数据 tiler，而是 ThreadLayout：

$$
P:\ \text{worker coordinate}\longrightarrow\text{physical worker id}
$$

`local_partition` 分三步：

$$
\begin{aligned}
B_P&=\operatorname{product\_each}(\operatorname{shape}(P))
&&\text{构造 compact 数据 tiler}\\
p&=P^{-1}(t)
&&\text{将物理 worker id 反查为 worker 坐标}\\
\operatorname{local\_partition}(T,P,t)
&=\operatorname{zipped\_divide}(T,B_P)(p,\_)
&&\text{固定 Tile，保留 Rest}
\end{aligned}
$$

因此精确定义是：

$$
\boxed{
\operatorname{local\_partition}(T,P,t)
=\operatorname{zipped\_divide}
 \big(T,\operatorname{product\_each}(\operatorname{shape}(P))\big)
 \big(P^{-1}(t),\_\big)
}
$$

源码中的 $P^{-1}(t)$ 写作 `P.get_flat_coord(t)`。教程里的 ThreadLayout 是 compact 双射，因此每个 `threadIdx.x` 都能唯一反查到一个 worker 坐标。

`product_each` 只乘平每个顶层 mode 的内部层次，保留顶层 rank：

$$
\operatorname{product\_each}(2,(3,4),5)=(2,12,5)
$$

它不是总乘积 $2\times3\times4\times5=120$。这样层次化的 ThreadLayout shape 才能转换成“每个数据 mode 有多少个 worker”的普通 tiler。

对 2-D CTA Tensor $T=(M,K)$ 和 $\operatorname{shape}(P)=(P_M,P_K)$，若 $p=P^{-1}(t)=(p_M,p_K)$：

$$
\begin{aligned}
T_t &: (M/P_M,K/P_K)\\
T_t(i,j)&=T(p_M+P_Mi,p_K+P_Kj)
\end{aligned}
$$

也就是说，ThreadLayout 的 shape 决定怎样分数据，ThreadLayout 的 stride 决定哪个物理线程取得哪一份。

**SGEMM A 的实例**：

$$
tA=(32,8):(1,32),\qquad
(p_M,p_K)=tA^{-1}(t)=(t\bmod32,\lfloor t/32\rfloor)
$$

对 $gA=(128,8,K/8):(1,5120,40960)$：

$$
\begin{aligned}
B_{tA}&=(32,8)\Longleftrightarrow\langle32{:}1,8{:}1\rangle\\
tAgA&:(4,1,K/8):(32,0,40960)\\
tAgA(i,0,k_t)&=gA(p_M+32i,p_K,k_t)
\end{aligned}
$$

`tAsA = local_partition(sA,tA,t)` 产生对应的 smem fragment view，因而 `copy(tAgA(_,_,k_t),tAsA)` 让每个线程搬运自己的 4 个 A 元素。

### 6.4 两个接口的精确对比与坐标空间

$$
\underbrace{mA}_{\text{完整 gmem Tensor}}
\xrightarrow{\ \operatorname{local\_tile}\ }
\underbrace{gA}_{\text{一个 CTA tile}}
\xrightarrow{\ \operatorname{local\_partition}\ }
\underbrace{tAgA}_{\text{一个 thread fragment}}
$$

| | `local_tile(T,B,q)` | `local_partition(T,P,t)` |
| --- | --- | --- |
| 输入 layout 的语义 | $B$ 是数据 tiler | $P$ 是 worker-id layout |
| 实际 divide tiler | 完整 $B$（shape+stride） | $\operatorname{product\_each}(\operatorname{shape}(P))$ |
| 选择坐标 | 直接使用 $q$ | 先算 $P^{-1}(t)$ |
| slice | $D(\_,q)$ | $D(P^{-1}(t),\_)$ |
| 输出 | 一个完整 tile | 一个 worker fragment |

这里有三个不能混用的坐标空间：

| 对象 | 坐标空间 | stride 的含义 |
| --- | --- | --- |
| 数据 tiler $B$ | Tensor 逻辑数据坐标 | tile 内怎样采样数据；直接进入 divide |
| tile 坐标 $q$ | divide 后的 Rest / tile-grid 坐标 | 选择哪个 tile |
| ThreadLayout $P$ | worker coordinate $\to$ physical id | 只用于从 $t$ 反查 worker 坐标，不进入 divide |

这就是 stride 混用风险的统一结论：

- `local_tile` 中，$B.stride$ **会进入** `zipped_divide`，所以 $B$ 必须真的是数据 tiler。
- `local_partition` 中，$P.stride$ **不会进入** `zipped_divide`，只参与 $P^{-1}(t)$。
- BlockLayout / ThreadLayout 的 id stride 都不能被当成 Tensor 数据采样 stride。

例如 $tA=(32,8):(1,32)$ 中的 $32$ 表示 K-worker 坐标加 1 时，物理 tid 加 32：

$$
tA^{-1}(0)=(0,0),\qquad
tA^{-1}(32)=(0,1),\qquad
tA^{-1}(64)=(0,2)
$$

它不表示数据 K 坐标跳 32。数据 K-mode 做的是：

$$
8{:}5120\ \oslash\ 8{:}1
$$

而不是 $8{:}5120\oslash8{:}32$。

如果 CTA 网格由 $Q:\text{tile-grid coordinate}\to\text{block id}$ 编码，也应先算 $q=Q^{-1}(\texttt{blockIdx.x})$，再把 $q$ 交给 `local_tile`；不能把 $Q.stride$ 塞进数据 tiler $B$。

**代码验证**（`examples/12_local_tile_partition.cpp`）：

- $T=16{:}1$ 使用 $B=4{:}2$ 时，两个 local tile 分别取 `[0,2,4,6]` 和 `[1,3,5,7]`，证明 $B.stride$ 进入 divide。
- $P_1=(2,4):(1,2)$ 与 $P_2=(2,4):(4,1)$ 得到相同的 divide layout，但 `tid=1` 反查出的 worker 坐标不同，证明 $P.stride$ 只进入 $P^{-1}(t)$。

### 6.5 Tensor 的限制

$$
\begin{aligned}
\text{开放：}&\quad \text{composition},\ \text{logical/zipped/tiled/flat\_divide}\\
\text{禁止：}&\quad \text{所有 } \_\text{product}
\end{aligned}
$$

原因：product 扩大 codomain $\to$ Tensor 会访问超出原边界的内存。divide = 已有数据里切分(安全)；product = 凭空造更大布局(仅纯 Layout 有意义)。

---

## 7. 统一视角(全代数结构)

$$
\underbrace{\text{composition}}_{\text{核心}}
\ \Longrightarrow\
\begin{cases}
\text{divide} = A\circ(B,B^{*}) & \text{(切)}\\[2pt]
\text{product} = (A,\,A^{*}\circ B) & \text{(铺)}
\end{cases}
\ \xrightarrow{\text{mode 重排}}\
\begin{cases}
\text{logical/zipped/tiled/flat}\\[2pt]
\text{blocked/raked (zip 改排布)}
\end{cases}
$$

**两条定理性观察**：

1. **变体 = 核心运算 + mode 重排**。$\text{logical}\_*$ 产出原始两组 mode，其余变体只重排，不改数据映射。

2. **两种「变形」性质不同**：
   - **flatten**(去括号)：$((2,3),(4,2)) \leftrightarrow (2,3,4,2)$，逐点**完全不改函数**。
   - **permute**(重排 mode，如 logical $\leftrightarrow$ zipped)：遍历顺序/坐标配对变，但 codomain(offset 集合)不变。
   - 故「底层同一」**对集合成立，对函数(带顺序)不成立**。GPU 中顺序影响 coalescing / bank conflict。

---

## 8. Compatible 与坐标转换

### 8.1 Compatible(弱偏序)

$A \text{ compatible } B$(记 $A\le B$)$\iff \operatorname{size}(A)=\operatorname{size}(B)$ 且 B 是 A 的更细划分。

$$
\begin{aligned}
&\text{自反：} && A\le A\\
&\text{反对称：} && A\le B \wedge B\le A \Rightarrow A=B\\
&\text{传递：} && A\le B \wedge B\le C \Rightarrow A\le C\\
&\text{偏(partial):} && \exists\ \text{不可比 shape}\ (\text{如 } ((2,3),4)\ \text{vs}\ ((2,2),(3,2)))
\end{aligned}
$$

是 composition / divide / tiling 的类型约束。

### 8.2 crd2idx / idx2crd

两种「数字」：$\text{index}$(列主序序号，只依赖 shape)、$\text{offset}$(坐标点乘 stride，依赖 stride)，二者不等 $\text{index}\ne\text{offset}$。

四个重载(互为逆)：

$$
\begin{aligned}
\operatorname{crd2idx}(c,s) &\longleftrightarrow \operatorname{idx2crd}(i,s) &&\text{坐标}\leftrightarrow\text{index(纯 shape)}\\
\operatorname{crd2idx}(c,s,d) &\longleftrightarrow \operatorname{idx2crd}(i,s,d) &&\text{坐标}\leftrightarrow\text{offset(带 stride)}\\
L(\text{coord}) &\equiv \operatorname{crd2idx}(\text{coord}, \text{shape}, \text{stride})
\end{aligned}
$$

**反向 `idx2crd` 成立条件 = 单射**(compact 是充分条件)：

| layout 性质 | 反向行为 |
| --- | --- |
| compact $(\operatorname{size}=\operatorname{cosize})$ | $[0,\operatorname{size})$ 全部正确 |
| 单射有空洞 $(\operatorname{size}<\operatorname{cosize})$ | 落合法 offset 对，落空洞静默给错 |
| 非单射有碰撞 | 一律静默给错 |
| 广播 $(\text{stride}=0)$ | 除零崩溃(SIGFPE) |

正向 $\operatorname{crd2idx}$ 对所有情况 OK。

---

## 附录：速查

$$
\begin{array}{ll}
\textbf{composition} & R(c)=A(B(c)),\quad A\circ(s{:}d)=\text{每隔 }d\text{ 取 }s\text{ 个}\\
\textbf{complement} & (A,A^{*})\ \text{填满}\ [0,M)\ \text{双射；排序+累积扫描}\\
\textbf{divide} & A\oslash B = A\circ(B,B^{*}),\quad B^{*}=\operatorname{complement}(B,\operatorname{shape}(\operatorname{coalesce}A))\\
\textbf{product} & A\otimes B = (A,A^{*}\circ B),\quad A^{*}=\operatorname{complement}(A,\operatorname{size}A\cdot\operatorname{cosize}B)
\end{array}
$$

$$
\begin{array}{ll}
\text{divide 变体} & \text{logical}\to\text{zipped}\to\text{tiled}\to\text{flat}\ (\text{mode 重排})\\
\text{product 变体} & \text{同构(需尖括号 Tiler);单 layout 时} = (\text{整块 }A,\ \text{整块复制})\\
\text{blocked/raked} & \text{rank-sensitive,}\operatorname{zip}\text{ 咬合(改 stride);blocked 成块 / raked 交错}\\
\text{编译期原则} & \text{能证明的才做，证不了保守/报错}
\end{array}
$$
