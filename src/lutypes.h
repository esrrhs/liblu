#pragma once

#include <string>
#include <list>
#include <vector>
#include <map>
#include <stdint.h>
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
#define LULOG(...) lulog("[DEBUG] ", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define LUERR(...) lulog("[ERROR] ", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
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

struct lu;
void * safelumalloc(lu * l, size_t len);
void safelufree(lu * l, void * p);

