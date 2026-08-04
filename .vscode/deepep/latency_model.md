# DeepEP Dispatch/Combine 耗时估算模型

本节给出在 **random 路由** 场景下，由 token 数量与有效带宽估算 DeepEP dispatch / combine 通信耗时的解析公式。

## 1. 符号定义

| 符号 | 含义 |
| :--- | :--- |
| $T$ | 每个 token 的输入数量（每 rank） |
| $R$ | rank 数量（GPU 总数） |
| $k$ | MoE top-k，每个 token 路由到的专家数 |
| $E$ | 专家总数 |
| $H$ | hidden 维度 |
| $BW$ | 有效数据带宽（GB/s） |
| $f$ | fan-out 因子，单个 token 平均落到的 rank 数 |

## 2. Fan-out 因子 $f$

一个 token 会被路由到 $k$ 个专家，专家均匀分布在 $R$ 张卡上。dispatch 按 **rank 去重**：同一 token 若有多个选中专家落在同一张卡，该卡只接收一份。因此需要计算 $k$ 次专家选择所覆盖的**不同 rank 数**的期望。

对任意一张固定的卡，单次专家选择「不落在该卡」的概率为 $\left(1 - \tfrac{1}{R}\right)$。当 $E \gg k$ 时，$k$ 次选择近似独立，则该卡接收到此 token 的概率为：

$$
P_{\text{hit}} = 1 - \left(1 - \frac{1}{R}\right)^{k}
$$

由期望的线性性，对 $R$ 张卡求和得到 fan-out 因子：

$$
\boxed{\,f = R\left[\,1 - \left(1 - \frac{1}{R}\right)^{k}\,\right]}
$$

于是每张卡平均接收的 token 数为：

$$
\text{recv\_num} \approx f \cdot T
$$

### 2.1 精确形式（超几何分布）

上式的近似之处在于 $\left(1 - \tfrac{1}{R}\right)^{k}$ 假设了 $k$ 次专家选择相互**独立**（放回采样）。但实际 top-k 是从 $E$ 个专家中**不放回**地取 $k$ 个，后一次选择依赖于前一次，因此应使用超几何分布严格推导。

**设定**。固定观察一张卡，其上有 $E/R$ 个专家，其余 $E - E/R$ 个专家不在该卡。「该卡收不到此 token」等价于：抽取的 $k$ 个专家**全部落在**那 $E - E/R$ 个「不在该卡」的专家之中。

**组合计数**。从 $E$ 个专家中取 $k$ 个的总方案数为 $\binom{E}{k}$；其中 $k$ 个全部取自 $E - E/R$ 个「不在该卡」专家的方案数为 $\binom{E - E/R}{k}$。二者之比即为收不到的概率：

$$
P_{\text{miss}} = \frac{\dbinom{E - E/R}{k}}{\dbinom{E}{k}}
$$

**展开为连乘**。将组合数按定义展开并逐项约分（这正是不放回采样的链式概率：第 $i$ 次抽取时，池中剩余 $E - i$ 个专家，其中 $E - E/R - i$ 个仍「不在该卡」）：

$$
P_{\text{miss}}
= \prod_{i=0}^{k-1}\frac{E - E/R - i}{E - i}
= \underbrace{\frac{E - E/R}{E}}_{\text{第 1 次}}\cdot\underbrace{\frac{E - E/R - 1}{E - 1}}_{\text{第 2 次}}\cdots\underbrace{\frac{E - E/R - (k-1)}{E - (k-1)}}_{\text{第 }k\text{ 次}}
$$

**得到精确 fan-out**：

$$
\boxed{\,f = R\left[\,1 - \prod_{i=0}^{k-1}\frac{E - E/R - i}{E - i}\,\right]}
$$

**与近似式的关系**。每个连乘项 $\dfrac{E - E/R - i}{E - i}$ 中，当 $E \gg k$（即 $i \ll E$）时分子分母的 $i$ 可忽略，每项退化为 $\dfrac{E - E/R}{E} = 1 - \dfrac{1}{R}$，连乘即塌缩回 $\left(1 - \tfrac{1}{R}\right)^{k}$，与第 2 节近似式一致。

**数值对比**（$R = 8,\ E = 256,\ k = 8$）：

| 方法 | $P_{\text{miss}}$ | $f$ |
| :--- | :---: | :---: |
| 近似 $\left(1-\tfrac1R\right)^k$（放回） | 0.34361 | 5.251 |
| 精确 超几何（不放回） | 0.33817 | 5.295 |

两者对 $f$ 的相对误差仅 **0.82%**，故工程估算使用第 2 节的闭式近似即可。

## 3. 传输字节数

每卡搬运的有效数据量由 `recv_num` 决定。dispatch 与 combine 的数据类型不同：

- **dispatch（FP8）**：每元素 1 字节数据 + 每 128 元素共享一个 FP32 scale。折算为相对 BF16 的字节系数：
  $$
  c_{\text{fp8}} = \frac{1 + 4/128}{2} \approx 0.5156
  $$
- **combine（BF16）**：每元素 2 字节，字节系数 $c = 1$。

对应搬运字节数：

$$
\text{Bytes}_{\text{dispatch}} = f \cdot T \cdot H \cdot 2 \cdot c_{\text{fp8}}
\qquad
\text{Bytes}_{\text{combine}} = f \cdot T \cdot H \cdot 2
$$

## 4. 耗时公式

耗时为搬运字节数除以有效带宽（$BW$ 单位 GB/s，故分母乘 $10^9$）：

$$
\boxed{\,T_{\text{dispatch}} = \frac{f \cdot T \cdot H \cdot 2 \cdot c_{\text{fp8}}}{BW \times 10^{9}}\,}
\qquad
\boxed{\,T_{\text{combine}} = \frac{f \cdot T \cdot H \cdot 2}{BW \times 10^{9}}\,}
$$

## 5. 代入示例

配置 $R = 8,\ k = 8,\ E = 256,\ H = 5120$，得 $f \approx 5.25$。化简后（输出单位 μs，$BW$ 单位 GB/s）：

$$
T_{\text{dispatch}}(\mu s) \approx \frac{27.7 \cdot T}{BW}
\qquad
T_{\text{combine}}(\mu s) \approx \frac{53.8 \cdot T}{BW}
$$

**验证**：取 $T = 512,\ BW = 220$，代入得 $T_{\text{dispatch}} \approx 64\ \mu s$，与实测 $64\ \mu s$ 吻合。

## 6. 适用范围与说明

- 公式为**纯传输模型**，假设 $BW$ 为有效数据带宽（goodput），未单独建模 kernel launch、barrier、notify 等固定开销。小 batch 时固定开销占比较大，若代入的 $BW$ 取自小 batch 实测值则可自洽。
- 常数 $f$、字节系数绑定具体配置。更换 $H$ 时线性项按比例缩放；更换 $R$ / $k$ 时需重算 $f$。
- $f$ 为期望值，单个 rank 存在约 ±10% 的负载抖动，公式预测的是跨 rank 平均耗时。
- 与实测对比（$T = 64 \sim 512$）：dispatch / combine 预测误差均在 **±3%** 以内，且随 batch 增大而减小。
