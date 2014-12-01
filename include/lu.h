#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

// 环境
struct lu;

enum lutype
{
	// tcp服务器
	lut_tcpserver,
	// tcp客户端
	lut_tcpclient,
};

#define LU_IP_SIZE 24

struct luconfig
{
    luconfig() : lum(&malloc), luf(&free), type(lut_tcpserver),
		port(8888), isreconnect(true), maxconnnum(1000),
		backlog(128), isnonblocking(true), sendbuff(1024*1024), recvbuff(1024*1024),
		waittimeout(1)
    {
		strcpy(ip, "127.0.0.1");
	}
	// 内存管理
    lumalloc lum;
	lufree luf;
	// 类型
	lutype type;
	// ip端口
	char ip[LU_IP_SIZE];
	uint16_t port;
	// 是否重连
	bool isreconnect;
	// 最大连接数
	int maxconnnum;
	// tcp参数
	int backlog;
	// 非阻塞
	bool isnonblocking;
	// 发送接收缓冲区
	int sendbuff;
	int recvbuff;
	// select超时时间
	int waittimeout;
};

// 初始化
LU_API void inilu();

// 申请回收
LU_API lu * newlu(luconfig * cfg = 0);
LU_API void dellu(lu * l);

// 心跳
LU_API void ticklu(lu * l);

