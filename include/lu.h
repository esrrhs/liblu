#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

// ����
struct lu;

enum lutype
{
	// tcp������
	lut_tcpserver,
	// tcp�ͻ���
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
	// �ڴ����
    lumalloc lum;
	lufree luf;
	// ����
	lutype type;
	// ip�˿�
	char ip[LU_IP_SIZE];
	uint16_t port;
	// �Ƿ�����
	bool isreconnect;
	// ���������
	int maxconnnum;
	// tcp����
	int backlog;
	// ������
	bool isnonblocking;
	// ���ͽ��ջ�����
	int sendbuff;
	int recvbuff;
	// select��ʱʱ��
	int waittimeout;
};

// ��ʼ��
LU_API void inilu();

// �������
LU_API lu * newlu(luconfig * cfg = 0);
LU_API void dellu(lu * l);

// ����
LU_API void ticklu(lu * l);

