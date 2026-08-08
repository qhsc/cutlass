# 3.5 动态调度：Tomasulo 算法与例子（Dynamic Scheduling: Examples and the Algorithm）

> 本节把 Tomasulo 视为一套“每周期规则”。无需先记住每个硬件端口；先掌握 issue、等待、wakeup、execute、writeback/commit 的状态转换。

## 1. 一条指令的生命周期

```text
Fetch/Decode → Issue 到 RS/Load Buffer → 等待 operand ready
            → Execute → Write result / wakeup → In-order commit（现代 CPU）
```

关键区别：**Issue 不等于 Execute。** 指令可以已经进入 reservation station，却因 operand tag 未完成而等待；这时后续独立指令仍可 issue 和 execute。

## 2. 每周期的简化规则

### Issue

对一条新指令：

1. 目标 RS/load buffer 是否有空位？没有则前端 stall。
2. 为目的寄存器分配一个新的 tag（现代 CPU 可理解为 physical register + ROB entry）。
3. 对每个源寄存器：若值已 ready，复制到 `Vj/Vk`；否则记录它的 producer `Qj/Qk`。
4. 更新 rename map：后续读取该 architectural register 的指令应等待新 tag。

### Wakeup 与 select

功能单元广播 `(tag, value)` 后，所有 RS 比较自己的 `Qj/Qk`：匹配者接收 value、清除 Q。两操作数都 ready 的 RS 进入 ready 集合；若 ready 指令多于执行单元，scheduler 选择其中一条/几条执行。这叫 **wakeup-select**。

### Execute 与 writeback

执行单元按自身 latency 运行。完成后结果写到 physical register 或经结果总线广播；消费者被唤醒。现代 CPU 的 result writeback 与 ROB commit 分离：前者只是“算完”，后者才是“正式生效”。

## 3. 逐步例子

假设：每 cycle 最多 issue 一条；load latency 为 3 cycle、ADD/SUB 为 1 cycle；有足够的 RS，结果总线每 cycle 最多广播一个结果。指令：

```asm
I1: LD   R1, 0(R2)
I2: LD   R3, 0(R4)
I3: ADD  R5, R1, R3
I4: SUB  R6, R7, R8
I5: MUL  R9, R5, R10
```

### Cycle 1–5：issue 阶段

| cycle | 新 issue | RS 中的关键状态 | 能执行的指令 |
|---|---|---|---|
| 1 | I1，tag=L1 | `R1 → L1` | I1 开始 load |
| 2 | I2，tag=L2 | `R3 → L2` | I2 开始 load |
| 3 | I3，tag=A1 | `Qj=L1, Qk=L2` | 尚不能执行 |
| 4 | I4，tag=A2 | `Vj=R7, Vk=R8` | I4 立即执行 |
| 5 | I5，tag=M1 | `Qj=A1, Vk=R10` | 尚不能执行 |

注意 I4 的执行没有等待 I1/I2/I3；这就是乱序执行。I4 虽更年轻，却完全独立。

### 之后的 wakeup 链

假设 I1 先返回：

```text
L1 广播 (L1, value1)
→ I3: Qj 清除，得到 value1；但仍等 L2
```

I2 返回后：

```text
L2 广播 (L2, value3)
→ I3: 两操作数 ready → A1 执行
```

I3 完成后：

```text
A1 广播 (A1, value5)
→ I5: Qj 清除 → M1 执行
```

真实依赖链为 `I1/I2 → I3 → I5`，因此 I5 不能早于 I3；I4 不在链上，能抢先完成。

## 4. 同周期完成的资源冲突

若 I1 和 I2 同周期完成、但只有一条结果广播路径：

```text
cycle t:    I1 ready, I2 ready
cycle t:    只广播 I1；I2 等到 t+1
```

I3 要等两个输入，因此还要等 I2。这个例子说明，动态调度并不消灭资源瓶颈；CDB/结果端口、RS 数量、load queue、执行单元、ROB 容量都可能限制 IPC。

## 5. Store 与 load 的简化规则

store 有地址和数据两个依赖：地址决定写哪里，数据决定写什么。它可先进入 store queue，等两者齐备；真正写入 memory 通常延后到可安全 commit 时。

更年轻 load 若地址与更早 store 相同，应从 store queue forwarding 最新值；若地址尚未知，硬件可能等待或预测它们不冲突并投机执行 load。预测错则要 replay load 及其依赖者。

这也是现代 CPU 中最难的动态调度部分：寄存器 tag 很明确，内存别名要在运行时才完全知道。

## 6. 如何读性能计数器

看到低 IPC 时，可按资源链条定位：

```text
RS/ROB 满       → instruction window 不够或长延迟太多
load queue 满   → memory 请求积压
execution port 忙 → 算术/地址生成结构瓶颈
branch recovery → 错误路径频繁 flush
retire 停顿      → 头部老指令等待 cache miss/exception
```

这些不是所有 CPU 都公开的同名指标，但 `top-down` CPU profiler 通常能给出 frontend、bad speculation、backend memory/core-bound、retiring 等相近分类。

## 7. 自测

1. 为什么 I4 可以在 I1/I2/I3 未完成时先执行，却不能先 commit？
2. `Qj=L1` 在 I1 广播后发生什么变化？
3. 两个结果同周期完成但只有一条 CDB，会如何影响消费者？
4. store queue 为什么不能简单在 issue 后立刻写 memory？

## 下一节

第 3.6 节将把 branch prediction 与动态调度结合，讨论硬件投机如何沿预测路径执行、又如何在预测错误时精确恢复。
