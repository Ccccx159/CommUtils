# CommUtils

面向 Linux 的轻量 C/C++ 通用工具库，使用 C++11 和 CMake 3.15 及以上版本。
提供日志、时间、文件/JSON 读写、进程资源监控、线程性能追踪、崩溃诊断和 C 环形缓冲区。
当前依赖 `/proc`、pthread、Linux syscall，不支持直接在 Windows 原生环境构建。

## 构建、测试和安装

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCOMMUTILS_BUILD_TEST_HO=ON
cmake --build build -j4
ctest --test-dir build --output-on-failure
cmake --install build --prefix /your/install/path
```

- `COMMUTILS_WITH_SPDLOG=ON`（默认）：使用 spdlog；关闭时使用自带的同步控制台日志。
- `COMMUTILS_BUILD_TEST_HO=ON`：构建测试，包括各公共头文件的独立编译检查。
- `COMMUTILS_SANITIZE_ADDRESS=ON`：开启 ASan/UBSan；可同时设置 `COMMUTILS_SANITIZE_LEAK=ON` 检查泄漏。
- `COMMUTILS_INSTALL=OFF`：作为子项目使用时可关闭安装规则。
- 原有 `platform` 参数保留，默认 `linux64`。配置阶段不自动调用 SVN 下载脚本；交叉编译需自行传入 CMake toolchain。

安装后在下游项目中使用：

```cmake
find_package(commutils CONFIG REQUIRED)
add_executable(app main.cc)
target_link_libraries(app PRIVATE commutils::commutils)
```

通过 `-DCMAKE_PREFIX_PATH=/your/install/path` 指定安装位置。依赖、线程库和日志配置会随导出目标传递。
各模块可单独包含，也可通过 `#include <commutils/commutils.h>` 引入全部工具。

## 分析线程 CPU 占用

Tracer 使用 Linux **线程 CPU 时钟**采集 CPU 耗时，使用单调时钟采集墙钟耗时。
它适合分析已打点接口：不能自动采样调用栈，也不会自动发现未打点的热点函数。

在目标线程的可疑接口入口添加作用域事件，正常返回、提前返回和抛异常均会自动结束：

```cpp
#include <commutils/tracer_.h>

void EncodeFrame() {
  Tracer::ScopedEvent event("EncodeFrame", {{"frame_id", 42}}, "video");
  // 待分析的编码调用
}

void WorkerLoop() {
  for (int i = 0; i < 100; ++i) {
    Tracer::ScopedEvent event("WorkerIteration");
    EncodeFrame();
  }
}
```

完成采集时，先停止并等待工作线程退出，再调用 `Tracer::instance().stop()`。
停止会排空已接收的事件、关闭 JSON 文件；随后新增事件会被忽略。不支持停止后重新启动。
默认文件名为 `Tracer_Report_<PID>.json`，可导入 Perfetto 或 Chrome Trace 查看时间线。
也可创建 `Tracer tracer("report.json")`，用 `Tracer::ScopedEvent event(tracer, "接口名")` 指定实例。
Tracer 必须比作用域事件和访问它的线程存活更久；作用域事件必须在同一线程创建和销毁。

每个结束事件的 `args` 包含：

| 字段 | 含义 |
| --- | --- |
| `cpu_time_us` | 接口执行期间消耗的线程 CPU 时间，微秒 |
| `wall_time_us` | 接口执行期间经过的墙钟时间，微秒 |
| `dutycycle` | CPU 时间 / 墙钟时间 × 100 |
| `preempted` | 非自愿上下文切换次数 |
| `voluntary` | 自愿上下文切换次数 |

所有输出时间戳均为微秒，保留小数精度。手工 `begin/end` 支持同线程同名嵌套，按后进先出匹配。
没有起始事件的结束事件标记为 `unmatched_begin`，不计入接口汇总。

按线程和接口汇总，优先查看累计 CPU 时间最大的接口：

```bash
python3 scripts/summarize_trace.py Tracer_Report_1234.json --tid 1235 --limit 20
```

累计 CPU 高、调用次数多的接口可能是高频热点；墙钟耗时高但 CPU 时间低，通常主要在等待。
统计包含子调用时间，父子接口或同名递归的时间不能直接相加。可从线程主循环逐层打点缩小范围。
Tracer 自身有时钟采集、同步和 JSON 输出开销；应按接口或批次打点，避免在每次极短操作上打点。
事件队列未限制容量，生产长期超过写出速度会增加内存占用，建议缩短采集窗口。

完全不知道热点位置时，先带调试符号构建，使用采样工具定位：

```bash
top -H -p <PID>
perf record -t <TID> -g -- sleep 10
perf report
```

得到可疑调用栈后，再使用 Tracer 比较具体接口的 CPU 时间和调用频率。

## 进程资源监控

```cpp
#include <commutils/guest_agent_.h>

GuestAgent agent(GuestAgentExternalInfo("agent.csv", nullptr, nullptr, 1, "stat"));
// 执行业务；可调用 agent.ReadAgentFile() 获取快照。
agent.Stop();  // 可重复调用；析构时也会停止并等待线程退出。
```

采样周期必须为正整数秒。`stat` 模式按进程存活时间计算平均 CPU 使用率，启动初期可能为 0；
它不是相邻两次采样的瞬时使用率。`top` 模式执行一次固定英文 locale 的 batch 快照，依赖系统的 `top` 命令；
命令失败或输出不受支持时返回 `0.00%`。优先使用无需启动外部进程的 `stat` 模式。

外部自定义回调仍接收 `GuestAgentExternalInfo*`，用户参数保存在 `arg_`。
**旧版无限循环回调必须改为协作退出**，否则 `Stop()` 和析构会一直等待：

```cpp
void* Monitor(void* arg) {
  auto* info = static_cast<GuestAgentExternalInfo*>(arg);
  while (!info->StopRequested()) {
    // 自定义采样，使用 info->arg_ 获取用户数据。
    if (info->WaitForStop(1000)) break;  // 可中断等待，单位毫秒。
  }
  return nullptr;
}
```

内置监控线程的等待可立即中断；正在运行的外部 `top` 命令需结束后才能退出。
由拥有实例的线程调用 `Stop()`，不要在监控回调内部调用它。外部回调自行负责文件读写同步；
`ReadAgentFile()` 只与内置采样器共享文件锁。
单例接口 `GetGuestAgentInstance()` 保留，仅首次调用的配置生效。

## 崩溃诊断

包含头文件不会自动注册信号处理器。需要时显式启用：

```cpp
#include <commutils/backtrace_.h>
Backtrace::GetBacktraceInstance();
```

SIGSEGV 处理器仅使用异步信号安全操作向 stderr 输出故障地址，然后恢复默认行为并终止进程，
由系统的 core dump 保存完整崩溃现场（可按环境配置 `ulimit -c unlimited`）。
信号处理器内不执行日志、堆分配、栈展开或 shell 命令，避免二次故障和死锁。
需要在正常执行上下文输出调用栈时，调用 `Backtrace::DumpStack()`。

## 其他模块约定

- `DateTime::GetSTime/GetMsTime/GetUsTime` 返回单调时钟计数，适合算时间差，不是 Unix 时间戳。
- `FileOperate::MkdirsByPath` 支持无斜杠相对路径和多层目录，拒绝空路径和已存在的普通文件。
- `FileOperate::WriteTo` 遵守传入的打开模式，例如 `std::ios::out | std::ios::app` 用于追加。
- C 环形缓冲区需链接 `commutils`；push 成功转移数据所有权，pop 成功后由调用者释放。
  满时覆盖最旧未读数据，释放回调默认 `free()`。不接管内存时需显式传入不释放的回调。
  销毁前必须确保所有访问线程已退出。
