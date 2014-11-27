#pragma once

#include <stdint.h>
#include <stdlib.h>

#define LU_VERSION "1.0"
#define LU_VERSION_NUM 100
#define LU_AUTHOR "esrrhs@163.com"

// 错误号
enum eluerror
{
    elu_ok = 0,
};

typedef void * (*lumalloc)(size_t size);
typedef void (*lufree)(void *ptr);

#define LU_API extern "C"

// 服务器环境
struct lusvr;

// 客户端环境
struct lucl;

struct lusvrconfig
{
    lusvrconfig() : lum(&malloc), luf(&free)
        {}
    lumalloc lum;
    lufree luf;	// 内存管理
};

struct luclconfig
{
    luclconfig() : lum(&malloc), luf(&free)
        {}
    lumalloc lum;
    lufree luf;	// 内存管理
};

// 申请回收
LU_API lucl * newlusvr(lusvrconfig * cfg = 0);
LU_API void dellusvr(lucl * ls);

LU_API lusvr * newlucl(luclconfig * cfg = 0);
LU_API void dellucl(lusvr * ls);
