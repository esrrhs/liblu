#pragma once

#include <string>
#include <list>
#include <vector>
#include <map>
#ifdef WIN32
#if _MSC_VER > 1500
#include <stdint.h>
#else
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#undef INT8_MIN  
#undef INT16_MIN 
#undef INT32_MIN 
#undef INT64_MIN 
#undef INT8_MAX  
#undef INT16_MAX 
#undef INT32_MAX 
#undef INT64_MAX 
#undef UINT8_MAX 
#undef UINT16_MAX
#undef UINT32_MAX
#undef UINT64_MAX
#define INT8_MIN         (-127i8 - 1)
#define INT16_MIN        (-32767i16 - 1)
#define INT32_MIN        (-2147483647i32 - 1)
#define INT64_MIN        (-9223372036854775807i64 - 1)
#define INT8_MAX         127i8
#define INT16_MAX        32767i16
#define INT32_MAX        2147483647i32
#define INT64_MAX        9223372036854775807i64
#define UINT8_MAX        0xffui8
#define UINT16_MAX       0xffffui16
#define UINT32_MAX       0xffffffffui32
#define UINT64_MAX       0xffffffffffffffffui64
#endif
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <typeinfo>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <assert.h>
#include <map>

#ifdef _DEBUG
#define LULOG(...) //lulog("[DEBUG] ", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define LUERR(...) //lulog("[ERROR] ", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#else
#define LULOG(...)
#define LUERR(...)
#endif

void lulog(const char * header, const char * file, const char * func, int pos, const char *fmt, ...);

typedef	int socket_t;
#if defined(WIN32)
typedef int socklen_t;
#else
#endif

// socket
#ifdef WIN32
	#define GET_NET_ERROR WSAGetLastError()
	#define NET_BLOCK_ERROR WSAEWOULDBLOCK
	#define NET_BLOCK_ERROR_EX WSAEWOULDBLOCK
	#define NET_INTR_ERROR WSAEINTR
#else
	#define GET_NET_ERROR errno
	#define NET_BLOCK_ERROR EWOULDBLOCK
	#define NET_BLOCK_ERROR_EX EAGAIN
	#define NET_INTR_ERROR EINTR
#endif

#define LUMIN(a, b) ((a) < (b) ? (a) : (b))
#define LUMAX(a, b) ((a) > (b) ? (a) : (b))

template <typename T>
void luswap(T & left, T & right)
{
    T tmp = left;
    left = right;
    right = tmp;
}

static void luswap(const char * left, char * right)
{
	const char * tmp = left;
	left = right;
	right = (char *)tmp;
}

struct lu;
void * safelumalloc(lu * l, size_t len);
void safelufree(lu * l, void * p);

