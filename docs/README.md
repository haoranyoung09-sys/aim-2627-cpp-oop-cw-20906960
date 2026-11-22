# Operation and verification

本项目为仓库拣货机器人任务分配系统（AIM 2627 C++ OOP Coursework 02）。本文档提供从仓库根目录出发的可复现构建、测试、调试与 ROS 2 演示说明。

## 本地构建与测试

在支持 C++17 与 CMake 3.16+ 的环境中，从仓库根目录执行以下命令：

```bash
cmake -S . -B build
cmake --build build
./build/public_tests
./build/core_demo
./build/debug_task
python3 tools/validate_structure.py
```

也可以使用仓库提供的一键检查脚本：

```bash
tools/check.sh
```

预期关键输出：

- `./build/public_tests` 输出 `PUBLIC 2/2`，退出码为 `0`。
- `./build/core_demo` 输出 `AIM OOP CW2 1.0`，退出码为 `0`。
- `python3 tools/validate_structure.py` 输出 `Structure OK`，退出码为 `0`。

## Q4 调试程序运行说明

从仓库根目录构建并运行独立调试目标：

```bash
cmake -S . -B build
cmake --build build --target debug_task
./build/debug_task
```

- 修复前初始错误输出：`S1 no -1 yes 0`（拒绝 `O2` 时先扣减了 `S2` 的库存，导致剩余库存变为 `-1`）。
- 修复后目标输出：`S1 no 5 yes 0`（拒绝 `O2` 时无副作用，`S2` 保持库存 `5`）。

## ROS 2 构建与运行

在 Ubuntu 24.04 + ROS 2 Jazzy 环境中，从仓库根目录执行：

```bash
source /opt/ros/jazzy/setup.bash
cd ros2_ws
colcon build --symlink-install
source install/setup.bash
ros2 launch warehouse_dispatch warehouse_demo.launch.py
```

### 预期输出对照

`warehouse_simulator` 每 300 ms 向 `/warehouse_events` 发布一条事件，`warehouse_dispatcher` 调用同一套核心库（`warehouse_core`）执行调度并向 `/dispatch_results` 发布结果，全部处理完成后两个节点自动退出：

| 顺序 | 输入事件（`/warehouse_events`） | 预期输出（`/dispatch_results` 与终端） |
|---|---|---|
| 1 | `ORDER O1 SKU-A 1` | `ACCEPT O1 S1` |
| 2 | `ORDER O2 SKU-A 1` | `ACCEPT O2 S2` |
| 3 | `ORDER O3 SKU-A 2` | `REJECT O3` |
| 4 | `COMPLETE O1` | `COMPLETE O1` |
| 5 | `ORDER O4 SKU-A 3` | `REJECT O4` |
| 6 | `WAIT 1` | `WAITED 1` |

## 故障排查

1. **找不到 `ros2` 或 `colcon` 命令 / 找不到 `rclcpp` 包**
   - 原因：未在当前终端加载 ROS 2 Jazzy 基础环境。
   - 排查与解决：
     ```bash
     ls /opt/ros/jazzy/setup.bash
     source /opt/ros/jazzy/setup.bash
     which ros2 colcon
     ```
     若已编译工作区但找不到 `warehouse_dispatch` 包，确认已在 `ros2_ws/` 下执行 `source install/setup.bash`。

2. **`ros2 launch` 结束后未自动退出或有节点残留**
   - 原因：某节点未收到最后一条 `WAIT 1` 消息（例如 `dispatcher` 启动慢于 `simulator` 首条消息，或节点进程卡在 spin 中）。
   - 排查与解决：
     ```bash
     ps aux | grep -E "simulator|dispatcher"
     ros2 node list
     ros2 topic echo /dispatch_results
     ```
     检查 `simulator_node.cpp` 是否完整发送第 6 条事件 `WAIT 1` 后调用 `rclcpp::shutdown()`，以及 `dispatcher_node.cpp` 在收到 `WAIT` 指令后是否发布 `WAITED 1` 并调用 `rclcpp::shutdown()`。

3. **Windows / MinGW 本地无 `cmake` 时的编译验证**
   - 若本机仅安装了 `g++`（C++17）而未配置 `cmake`，可直接在仓库根目录使用 `g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude` 编译对应 `.cpp` 文件进行本地单元测试与边界验证，CI 环境仍使用 `cmake` 完整构建。
