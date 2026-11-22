# Design and debugging record

## 命名空间与类职责

系统核心库 `aim_oop_core` 严格划分为四个命名空间，声明位于 `include/`、实现位于 `src/`，保持单向依赖：

| 命名空间 | 对应文件 | 核心类型与函数 | 职责边界与依赖约束 |
|---|---|---|---|
| `warehouse` | `include/warehouse/domain.hpp`<br>`src/warehouse/domain.cpp` | `RobotState`、`Order`、`PickStation`、`Inventory` | 管理站点注册、查询、合格候选站筛选（按站点 ID 字典序升序）、原子扣减与占用（`reserve`）以及任务完成（`finish`）。不依赖 `dispatch`、`audit`、`app` 或 ROS 头文件。 |
| `dispatch` | `include/dispatch/policy.hpp`<br>`include/dispatch/scheduler.hpp`<br>`src/dispatch/policy.cpp`<br>`src/dispatch/scheduler.cpp` | `DispatchPolicy`、`make_policy`、`DispatchDecision`、`Scheduler` | `DispatchPolicy` 只接收已筛选候选站并返回选中站点 ID，不修改任何状态；`Scheduler` 串联合法性校验、候选站获取、策略选择、原子预留与审计写入（`submit` / `submit_all` / `complete`）。 |
| `audit` | `include/audit/trace.hpp`<br>`src/audit/trace.cpp` | `TraceEvent`、`Trace` | 按发生顺序记录成功事件（`DISPATCH O S`、`COMPLETE O S`），提供 `append`、`events`、`clear`；不校验业务规则，不依赖调度器。 |
| `app` | `include/app/cli.hpp`<br>`include/app/application.hpp`<br>`include/app/contracts.hpp`<br>`src/app/cli.cpp`<br>`src/app/main.cpp` | `normalize_sku`、`valid_station_id`、`valid_order_id`、`run`、`print_trace` | 负责 SKU 规范化（去首尾空白并转大写）、ID 合法性校验（拒绝空串和首尾空白）及 CLI 入口；不直接修改库存或机器人状态。 |

## 策略差异对比

两种调度策略均继承 `dispatch::DispatchPolicy`，在不修改 `Scheduler` 代码的前提下通过 `dispatch::make_policy(name)` 切换：

- **`priority` 策略**：从已按站点 ID 字典序升序排列的合格候选站列表中直接选择第一个站点（即站点 ID 字典序最小的合格候选站）。
- **`stock` 策略**：从合格候选站列表中选择当前可用库存最多的站点；若多个候选站库存并列最多，选择其中站点 ID 字典序较小的站点。

### 相同初始状态下的策略差异示例

- **初始状态**：`S1` 库存为 `2`（`idle`），`S2` 库存为 `5`（`idle`）。
- **输入订单**：`Order{"O1", "SKU-A", 1}`。
- **对比结果**：
  - 合格候选站列表为 `[S1(2), S2(5)]`。
  - `priority` 策略选择 `S1`（字典序更小）。
  - `stock` 策略选择 `S2`（库存 `5 > 2`）。

## 状态转换表

| 操作情形 | 目标站点库存变化 | 机器人状态变化 | 当前订单 ID 变化 | `audit::Trace` 变化 |
|---|---|---|---|---|
| 派单成功 | 选中站点库存减少订单数量（结果 `>= 0`） | 由 `idle` 变为 `busy` | 记录为当前订单 ID | 追加 `DISPATCH <order_id> <station_id>` |
| 派单失败 | 所有站点不变 | 所有站点不变 | 所有站点不变 | 不追加任何记录 |
| 完成成功 | 目标站点不变（库存不恢复） | 由 `busy` 变为 `idle` | 清空当前订单 ID | 追加 `COMPLETE <order_id> <station_id>` |
| 重复完成 / 错误完成 | 所有站点不变 | 所有站点不变 | 所有站点不变 | 不追加任何记录 |

## Q4 调试证据与修复前输出记录

- **复现输入**（`src/debug/main.cpp` 固定场景）：
  1. 初始库存：`S1=3`、`S2=5`，两台机器人均 `idle`。
  2. `submit("O1", 2)`：`S1` 接受，库存变为 `1`，`busy`。
  3. `submit("O2", 6)`：`S1` 处于 `busy` 被跳过；`S2` 库存为 `5 < 6`，应拒绝。
  4. `complete("S1")`：`S1` 恢复 `idle`，库存保持 `1`。
  5. `submit("O3", 1)`：`S1` 满足条件接受，库存变为 `0`。
- **修复前输出**：`S1 no -1 yes 0`（在执行修复前直接编译运行 `./build/debug_task` 实测所得，且原始版本保留于 Git 初始提交 `1e7fd18`）。
- **根因分析**：`LegacyDispatcher::submit` 在确认站点空闲后，先执行了 `item.second -= quantity`，随后才判断 `if (item.second < 0)` 并返回失败，且未拦截 `quantity <= 0` 的非法订单，导致拒绝 `O2` 时 `S2` 库存被错误修改为 `-1`。
- **修复前后逻辑差异**：
  - 修复前：循环内先执行 `item.second -= quantity`，再检查 `item.second < 0`。
  - 修复后：在循环前先拒绝 `quantity <= 0`；循环内找到第一个空闲站点后，先检查 `if (item.second < quantity)`，不足则立即返回 `{false, "", "insufficient stock"}`，仅在条件全部满足后才执行 `item.second -= quantity`、`busy_[station_id] = true` 与 `orders_[station_id] = order_id`，完整保留原有类结构、控制流与输出格式。
- **修复后输出**：`S1 no 5 yes 0`。

## 边界验证记录

以下记录均来自本地 `g++ -std=c++17 -Wall -Wextra -Wpedantic` 编译运行 `tests/public_tests.cpp` 与边界测试程序的实际结果：

| 边界场景 | 输入条件 | 预期结果 | 实际运行结果 |
|---|---|---|---|
| 1. 库存不足 | `S1` 库存 `3`（`idle`），提交 `reserve({"O1", "SKU", 4}, "S1")`；以及 `Scheduler` 下 `S1=1, S2=4, S3=4` 提交 `{"O2", "SKU-A", 10}` | 拒绝派单（返回 `false` / `accepted=false`），所有站点库存与 `robot_state` 不变，`Trace` 不追加记录 | 拒绝派单，`S1` 保持原库存与 `idle`，`Trace` 事件数保持不变（通过） |
| 2. 库存恰好等于订单数量 | `S1` 库存 `3`（`idle`），提交 `reserve({"O1", "SKU", 3}, "S1")`；以及 Q4 `S2=5` 时提交 `("O3", 5)` | 派单成功，`S1` 库存变为 `0` 且状态变为 `busy`；完成后库存保持 `0` 不恢复 | 派单成功，库存精确扣减为 `0`、`busy`，完成后库存仍为 `0`（通过） |
| 3. 忙碌站点被跳过 | `S1=2, S2=1`（`priority` 策略），先提交 `O1`（数量 `1`）占用 `S1`，紧接着提交 `O2`（数量 `1`） | `S1` 虽字典序更小但处于 `busy`，被跳过；`O2` 分配给 `S2` | `O2` 成功分配至 `S2`，`S1` 保持持有 `O1`（通过） |
| 4. 非法输入与重复完成 | 订单数量 `0` 或 `-5`、空或带首尾空白的 `station_id`/`order_id`/`sku`、重复注册站点、重复完成同一订单 | 派单/完成均被拒绝且无副作用；非法注册或重复注册抛出 `std::invalid_argument` 且不覆盖原状态 | 均按预期拒绝或抛异常，原有状态与 `Trace` 零污染（通过） |
