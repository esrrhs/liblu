#pragma once

#include <stdint.h>
#include <stdlib.h>

#define LU_VERSION "1.0"
#define LU_VERSION_NUM 100
#define LU_AUTHOR "esrrhs@163.com"

// �����
enum eluerror
{
    elu_ok = 0,
};

typedef void * (*lumalloc)(size_t size);
typedef void (*lufree)(void *ptr);

#define LU_API extern "C"

// ����������
struct lusvr;

// �ͻ��˻���
struct lucl;

struct lusvrconfig
{
    lusvrconfig() : lum(&malloc), luf(&free)
        {}
    lumalloc lum;
    lufree luf;	// �ڴ����
};

struct luclconfig
{
    luclconfig() : lum(&malloc), luf(&free)
        {}
    lumalloc lum;
    lufree luf;	// �ڴ����
};

// �������
LU_API lucl * newlusvr(lusvrconfig * cfg = 0);
LU_API void dellusvr(lucl * ls);

LU_API lusvr * newlucl(luclconfig * cfg = 0);
LU_API void dellucl(lusvr * ls);
