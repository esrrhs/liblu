# liblu

[<img src="https://img.shields.io/github/license/esrrhs/liblu">](https://github.com/esrrhs/liblu)
[<img src="https://img.shields.io/github/languages/top/esrrhs/liblu">](https://github.com/esrrhs/liblu)
[<img src="https://img.shields.io/github/actions/workflow/status/esrrhs/liblu/cmake.yml?branch=master">](https://github.com/esrrhs/liblu/actions)

> **面向游戏场景的轻量 C 风格 TCP 网络库：带帧转发、tick 驱动 IO，库内不再做加密 / 压缩。**

[English](README.md) | [中文说明](README_CN.md)

---

## 简介

**liblu** 是可嵌入的小型 TCP 框架，面向游戏服务器与工具。对外提供 C 风格 API（`newlu` / `ticklu` / `sendlu` / `dellu`），在单线程 tick 循环中驱动 IO，并原样转发长度前缀帧。

## 特性

* **TCP 服务端 / 客户端**：`lut_tcpserver` / `lut_tcpclient`，可配 backlog、linger、keepalive、nodelay 与缓冲区。
* **Tick 驱动 IO**：在主循环调用 `ticklu` — Linux 用 **epoll**，macOS / Windows（MinGW）用 **select**。
* **纯转发帧**：仅 `magic + size + payload`，库内无加密 / 压缩。
* **自定义分配器**：`lumalloc` / `lufree`。
* **连接级 userdata**：通过 `luuserdata` 挂宿主状态。

## 平台

| 平台 | IO | 工具链 |
|------|-----|--------|
| Linux | epoll | GCC / Clang |
| macOS | select | Apple Clang |
| Windows | select | MinGW-w64 |

## 前置依赖

* CMake (>= 3.12)
* C++11 编译器
* Windows：MinGW-w64（链接 `ws2_32`）

## 编译

```bash
./build.sh
```

或：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Debug：

```bash
./build.sh Debug
```

产物在 `build/bin/`：

* `liblu.a` — 静态库
* `lu_test` — 示例 + smoke 测试

## 快速开始

```cpp
#include "lu.h"

void on_conn_open(lu *l, int connid, luuserdata &userdata);
void on_conn_recv_packet(lu *l, int connid, const char *buff, size_t size, luuserdata &userdata);
void on_conn_close(lu *l, int connid, luuserdata &userdata, int reason);

int main() {
    inilu();

    luconfig cfg;
    cfg.type = lut_tcpserver; // 或 lut_tcpclient
    cfg.port = 8888;
    cfg.cco = on_conn_open;
    cfg.ccrp = on_conn_recv_packet;
    cfg.ccc = on_conn_close;

    lu *l = newlu(&cfg);
    while (/* running */) {
        ticklu(l);
    }
    dellu(l);
}
```

发送：

```cpp
sendlu(l, buffer, size, connid); // client 模式下 connid 可忽略
```

## 示例程序

```bash
./build/bin/lu_test server
./build/bin/lu_test client
./build/bin/lu_test smoke
```

默认地址：`127.0.0.1:8888`（见 [`include/lu.h`](include/lu.h) 中的 `luconfig`）。

## 线格式

| 字段 | 类型 | 说明 |
|------|------|------|
| magic | `uint8` | `0xCC` |
| size | `uint32` | payload 长度 |
| payload | `bytes` | 应用数据（原样转发） |

## API 一览

| API | 作用 |
|-----|------|
| `inilu()` | 平台 socket 初始化（Windows 上 Winsock） |
| `newlu(luconfig*)` | 创建 server / client |
| `ticklu(lu*)` | 驱动 accept / 读写 / 重连 |
| `sendlu(lu*, buf, size, connid)` | 组帧入队 |
| `dellu(lu*)` | 销毁 |
| `getlu_userdata` / `getlu_conn_userdata` | 宿主 userdata |

公共头文件：[`include/lu.h`](include/lu.h)。

## 许可

MIT — 见 [LICENSE](LICENSE)。
