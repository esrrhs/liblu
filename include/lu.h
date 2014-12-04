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

// �û�����
union luuserdata
{
    void * _ptr;
    uint8_t _u8;
    int8_t _i8;
    uint16_t _u16;
    int16_t _i16;
    uint32_t u32;
    int32_t i32;
    uint64_t u64;
    int64_t i64;
    float _fl;
    double _dl;
};

// �ص�
typedef int (*cb_conn_open)(lu * l, int connid, luuserdata & userdata);
typedef int (*cb_conn_recv_packet)(lu * l, int connid, const char * buff, size_t size, luuserdata & userdata);
typedef int (*cb_conn_close)(lu * l, int connid, luuserdata & userdata);

struct luconfig
{
    luconfig() : lum(&malloc), luf(&free), type(lut_tcpserver),
		port(8888), isreconnect(true), maxconnnum(1000),
		backlog(128), linger(0), isnonblocking(true), sendbuff(1024*1024), recvbuff(1024*1024),
		socket_sendbuff(1024*256), socket_recvbuff(1024*256),
		waittimeout(1), cco(0), ccrp(0), ccc(0)
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
	int linger;
	// ������
	bool isnonblocking;
	// ���ͽ��ջ�����
	int sendbuff;
	int recvbuff;
	// socket���ͽ��ջ�������С
	int socket_sendbuff;
	int socket_recvbuff;
	// select��ʱʱ��
	int waittimeout;
	// �ص�����
	cb_conn_open cco;
	cb_conn_recv_packet ccrp;
	cb_conn_close ccc;
};

// ��ʼ��
LU_API void inilu();

// �������
LU_API lu * newlu(luconfig * cfg = 0);
LU_API void dellu(lu * l);

// ����
LU_API void ticklu(lu * l);
