#pragma once

#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/epoll.h>
#endif
#endif

#if defined(__linux__)
#define LU_USE_EPOLL 1
#else
#define LU_USE_SELECT 1
#endif

#ifdef _DEBUG
#define LULOG(...)
#define LUERR(...)
#else
#define LULOG(...)
#define LUERR(...)
#endif

void lulog(const char * header, const char * file, const char * func, int pos, const char *fmt, ...);

#if defined(_WIN32)
typedef SOCKET socket_t;
#define LU_INVALID_SOCKET INVALID_SOCKET
#else
typedef int socket_t;
#define LU_INVALID_SOCKET (-1)
#endif

#define LU_SOCKET_VALID(s) ((s) != LU_INVALID_SOCKET)

#if defined(_WIN32)
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
