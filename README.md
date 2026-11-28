# AIM 2627 C++ OOP Coursework 02

仓库拣货机器人任务分配系统。全部任务规格见 [Problem.pdf](Problem.pdf)，本 README 只负责本地启动。

```bash
cmake -S . -B build
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/debug_task
```

## 修改范围

- 可以修改：`include/`、`src/`、`README.md`、`docs/`。
- 不要修改：`tests/`、`tools/`、`.github/`、`CMakeLists.txt`、`Problem.pdf`。
- `src/debug/main.cpp` 是 Q4 的独立调试任务；不要通过绕开缺陷的输出方式得分。

## ROS 2 演示

ROS 2 部分在 `ros2_ws/` 中独立构建，复用 `include/` 和 `src/` 的核心实现。课程使用 ROS 2 Jazzy，演示不依赖硬件。命令见题面 Q2。

## 文档交付

在 `docs/README.md` 和 `docs/DESIGN.md` 中完成 Q5。初始文件只是提示骨架，正式要求以题面为准。

## Agent 会话归档

允许并鼓励合理使用 AI 辅助开发，但必须保留可核对的过程记录。参照 Python CW 与 C++ Assessment 01，本作业使用 Agent Session Commit（AgentLedger）v0.1.5。

在个人学生仓库根目录配置，不要在教师完整目录或其他项目中安装：

```bash
python -m venv .venv-agent-session-commit
source .venv-agent-session-commit/bin/activate
python -m pip install --index-url https://pypi.org/simple 'agent-session-commit[pre-commit]==0.1.5'
agent-session-commit install --pre-commit
agent-session-commit status
```

每次正常 `git commit` 后，post-commit hook 会把新增 AI 会话打包到 `.agent-sessions/bundles/` 并 amend 到同一提交。`.agent-sessions/bundles/` 必须提交；`.agent-sessions/source/` 与 `.venv-agent-session-commit/` 已忽略。没有使用 AI 时，在 README 末尾明确声明并提供开发记录。

提交前检查：

```bash
git rev-parse HEAD
git show --stat HEAD -- .agent-sessions/
```

## AI 使用与开发过程（你来写）

本作业在开发过程中使用 Antigravity（AI 编程助手）辅助阅读题面规格、定位 Q4 缺陷、补充边界验证。

- **使用范围**：
  - Q1：`src/warehouse/domain.cpp`、`src/dispatch/policy.cpp`、`src/dispatch/scheduler.cpp`、`src/audit/trace.cpp`、`src/app/cli.cpp` 的边界校验；
  - Q4：`src/debug/main.cpp` 中 `LegacyDispatcher::submit` 修复；
  - Q5：`docs/README.md`、`docs/DESIGN.md` 文档编写与本地编译测试核对。
- **未采用或受限说明**：
  - `ros2_ws/` 在模板中已提供完整节点接入框架且不在 CI 白名单修改范围内，因此保持原样未修改；
  - 本机为 Windows 环境（使用 MinGW `g++ 8.1.0` 以 `-std=c++17 -Wall -Wextra -Wpedantic` 编译验证），未在本地安装 `cmake` 或 Ubuntu 24.04 + ROS 2 Jazzy，ROS 2 运行时闭环将由推送后的远程流程或 ROS 2 环境进一步验证。
- **结果验证**：
  - `public_tests` 输出 `PUBLIC 2/2`；`core_demo` 正常退出；
  - `debug_task` 修复前实测输出 `S1 no -1 yes 0`，修复后实测输出 `S1 no 5 yes 0`；
  - `python tools/validate_structure.py` 输出 `Structure OK`；
  - 独立边界测试覆盖零库存、负库存、非法 ID/SKU/数量、重复注册、平局策略、忙碌跳过、重复/错误完成，全部通过。
- **会话归档位置**：
  - 仓库已配置 `agent-session-commit` v0.1.5（`custom` agent 模式，源目录 `.agent-sessions/source/`），每次 `git commit` 后由 `post-commit` hook 自动打包并随提交保存至 `.agent-sessions/bundles/`。
