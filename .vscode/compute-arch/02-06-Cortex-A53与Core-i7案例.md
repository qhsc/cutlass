# 2.6 案例：ARM Cortex-A53 与 Intel Core i7-6700 的存储层次

> 原节：*Memory Hierarchies in the ARM Cortex-A53 and Intel Core i7 6700*。两个处理器都使用多级 cache、TLB 与 DRAM，却面向相反约束：A53 优先能效、面积与可预测性；Skylake i7 优先单线程性能与隐藏延迟的能力。

## 1. 先避免一个误区：核与具体 SoC 不相同

**Cortex-A53** 是 Arm 授权的 CPU core IP。芯片厂可选择 core 数、L1 容量、cluster 共享 L2 容量、DRAM controller 和互连；因此“A53 的 cache 是多少”必须带具体 SoC。A53 支持可配置 L1，L2 也可在一定范围内配置。

**Core i7-6700** 是 Intel 第六代 Core（Skylake-S）的一个具体桌面 SKU：4 core、8 thread、8 MB Intel Smart Cache、最高 4.0 GHz，支持 DDR4-2133 或 DDR3L-1600。它的数字是具体产品属性，而非所有 Skylake 的绝对规则。

## 2. 两种设计哲学

| 维度 | Cortex-A53 | Core i7-6700（Skylake） |
|---|---|---|
| 主目标 | performance per watt、低面积、移动/嵌入式 SoC | 高单线程性能、桌面通用计算 |
| 执行方式 | in-order，较简单 | out-of-order，激进预测与调度 |
| 延迟应对 | 尽量避免长路径和高能耗 | 用大窗口、预取、MLP 隐藏延迟 |
| cache 取向 | 较小的私有 L1、cluster 共享 L2 | 私有 L1/L2 + 多 core 共享 LLC |
| 功耗代价 | 严格受限 | 可用较高功耗换性能 |

in-order 的含义是：一条 load 若长时间等待，后续依赖工作很难绕过它。因此 A53 更依赖良好局部性与低延迟。out-of-order core 可在 cache miss 时执行其他已经就绪的独立指令，配合 nonblocking cache 增加 MLP；但这要付出 reorder buffer、调度器、预测器和更多 SRAM/逻辑的面积与能耗。

## 3. Cortex-A53：小而规整的 cluster

A53 常以 1–4 个 core 组成一个 cluster。每个 core 有独立的 L1 instruction/data cache；cluster 内 core 共享 L2。L2 既是容量更大的第二层，也在 core 间数据共享时充当更近的汇合点。

```text
A53 cluster
  core 0: L1I + L1D ─┐
  core 1: L1I + L1D ─┼→ shared L2 → SoC interconnect → DRAM
  ...                ┘
```

这种结构的好处是 L1 小、近、低能耗，shared L2 以较低成本增加 cluster 工作集容量；代价是多个 core 同时流式访问时共享 L2/DRAM 带宽会竞争。对嵌入式软件，连续访问、减少 pointer chasing、控制工作集和避免无谓共享依旧重要。

Armv8-A 的 MMU/TLB 与 cache 一样是性能路径的一部分：小页和大范围随机访问会引入 TLB miss/page-table walk；大页可提高 reach，但要与碎片和实时性权衡。

## 4. Core i7-6700：用层次与动态执行换单线程性能

i7-6700 的典型层次可概念化为：

```text
每个 core：L1I + L1D → private L2
                              │
4 个 core ─────────────────→ shared 8 MB LLC (Intel Smart Cache)
                              │
                         DDR3L / DDR4 controller → DRAM
```

L1 追求极短 hit path；private L2 扩大每 core 的低延迟工作集；共享 LLC 既作为到 DRAM 前的最后缓存，也让 core 间共享数据有共同位置。8 MB Smart Cache 的共享性质带来容量弹性：一个 core 若闲置，其他 core 可多使用其容量；同时也带来多线程间的 capacity/bandwidth 干扰。

Skylake 的 out-of-order 执行、硬件 prefetch、较强分支预测和多个 outstanding memory request 意味着：对有独立工作的软件，单个 L2/LLC miss 的一部分等待可被隐藏。对串行 pointer chasing 或频繁 TLB/page walk，这些机制仍难以消除关键路径延迟。

## 5. 同一个 workload 的不同瓶颈

### 顺序扫描

两者都能从 cache line、硬件预取和 DRAM burst 中受益。i7 通常能维持更多未完成请求，更容易接近 DRAM 带宽；A53 则以较低能耗完成任务，若足够满足吞吐目标，其性能/瓦可能更优。

### 分支多、依赖强的查询

i7 可利用更激进的预测和 out-of-order 执行隐藏部分非关键等待；但依赖链最终仍由 latency 限制。A53 的简单 in-order pipeline 对这类负载更敏感，优化数据布局与减少随机访问尤为重要。

### 多线程共享队列

两者都会遇到 false sharing 与 coherence 流量。i7 的更高 core 性能不消除共享 cache line 的 ping-pong；A53 cluster 的 shared L2 也不使错误同步变正确。应通过分片、padding、批量提交和合适 atomic/fence 解决。

## 6. 将案例迁移到 AI Infra

GPU 与 i7 一样，会用大量并行 work 隐藏 memory latency；GPU 与 A53 一样，也极其在意能效和数据移动成本。二者都不是完整类比，但有助于建立判断：

- 计算能否在等待期间切换到独立工作？对应 OOO/MLP/warp scheduling。
- 数据是私有、cluster 共享，还是全芯片共享？对应 register/shared memory/L2/HBM。
- 增加 cache/并发是在减少 miss，还是只把排队挪到了下一层？

对模型服务，CPU 常承担 tokenization、调度、网络和 KV metadata；CPU cache/TLB/NUMA 的失配会使 GPU 空闲。不要只 profile GPU kernel，也要观察 host 的 page fault、LLC miss、CPU run queue 和 PCIe/NVLink copy。

## 7. 本节结论

1. 存储层次服务于整体核心设计：in-order/低功耗与 OOO/高性能会选择不同的 cache 和并发度。
2. A53 的“可配置 core IP”与 i7-6700 的“具体 SKU”不能用同一方式背参数。
3. 更大的共享 cache 有容量与共享优势，也有干扰和一致性成本。
4. MLP 能隐藏部分 latency；串行依赖和错误的数据布局仍会暴露底层延迟。

## 自测

1. 为什么不能仅根据“i7 的 LLC 有 8 MB”断言所有程序都比 A53 快？
2. 若 A53 cluster 中两个 core 高频修改同一 cache line，会发生什么？
3. 对 CPU 侧 AI preprocessing，什么迹象说明瓶颈不是 GPU，而是 CPU 的 cache/TLB/NUMA？

参考：[Intel i7-6700 官方规格](https://www.intel.com/content/www/us/en/products/sku/88196/intel-core-i76700-processor-8m-cache-up-to-4-00-ghz/specifications.html)、[Arm Cortex-A 系列对比表](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Cortex-A%20R%20M%20datasheets/Arm%20Cortex-A%20Comparison%20Table_v4.pdf)。
