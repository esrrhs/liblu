# liblu

[<img src="https://img.shields.io/github/license/esrrhs/liblu">](https://github.com/esrrhs/liblu)
[<img src="https://img.shields.io/github/languages/top/esrrhs/liblu">](https://github.com/esrrhs/liblu)
[<img src="https://img.shields.io/github/actions/workflow/status/esrrhs/liblu/cmake.yml?branch=master">](https://github.com/esrrhs/liblu/actions)

> **A lightweight C-style TCP networking library for games: framed packets, tick-driven I/O, no built-in encrypt/compress.**

[English](README.md) | [Chinese](README_CN.md)

---

## Overview

**liblu** is a small embeddable TCP framework for game servers and tools. It exposes a C-style API (`newlu` / `ticklu` / `sendlu` / `dellu`), drives I/O from a single-threaded tick loop, and forwards length-prefixed frames as-is.

## Features

* **TCP server & client**: `lut_tcpserver` / `lut_tcpclient` with backlog, linger, keepalive, nodelay, and buffer sizes.
* **Tick-driven I/O**: call `ticklu` from your main loop — **epoll** on Linux, **select** on macOS and Windows (MinGW).
* **Plain framed packets**: `magic + size + payload` only; no encrypt / compress in the library.
* **Custom allocator**: plug in `lumalloc` / `lufree`.
* **Per-connection userdata**: attach host state via `luuserdata`.

## Platforms

| Platform | I/O | Toolchain |
|----------|-----|-----------|
| Linux | epoll | GCC / Clang |
| macOS | select | Apple Clang |
| Windows | select | MinGW-w64 |

## Prerequisites

* CMake (>= 3.12)
* C++11 compiler
* On Windows: MinGW-w64 (links `ws2_32`)

## Build

```bash
./build.sh
```

Or:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Debug:

```bash
./build.sh Debug
```

Artifacts in `build/bin/`:

* `liblu.a` — static library
* `lu_test` — example + smoke test

## Quick Start

```cpp
#include "lu.h"

void on_conn_open(lu *l, int connid, luuserdata &userdata);
void on_conn_recv_packet(lu *l, int connid, const char *buff, size_t size, luuserdata &userdata);
void on_conn_close(lu *l, int connid, luuserdata &userdata, int reason);

int main() {
    inilu();

    luconfig cfg;
    cfg.type = lut_tcpserver; // or lut_tcpclient
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

Send:

```cpp
sendlu(l, buffer, size, connid); // connid ignored in client mode
```

## Example binary

```bash
./build/bin/lu_test server
./build/bin/lu_test client
./build/bin/lu_test smoke
```

Default address: `127.0.0.1:8888` (see `luconfig` in [`include/lu.h`](include/lu.h)).

## Wire format

Each packet on the wire:

| Field | Type | Notes |
|-------|------|-------|
| magic | `uint8` | `0xCC` |
| size | `uint32` | payload length |
| payload | `bytes` | opaque application data |

## API sketch

| API | Role |
|-----|------|
| `inilu()` | Platform socket init (Winsock on Windows) |
| `newlu(luconfig*)` | Create server or client |
| `ticklu(lu*)` | Drive accept / read / write / reconnect |
| `sendlu(lu*, buf, size, connid)` | Frame and enqueue |
| `dellu(lu*)` | Tear down |
| `getlu_userdata` / `getlu_conn_userdata` | Host userdata |

Public header: [`include/lu.h`](include/lu.h).

## License

MIT — see [LICENSE](LICENSE).
