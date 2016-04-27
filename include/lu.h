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

// 用户数据
union luuserdataparam
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
#define LU_MAX_USER_DATA_PARAM 4
struct luuserdata
{
    luuserdataparam params[LU_MAX_USER_DATA_PARAM];
};

// 回调 连接成功或者新连接连入
typedef void (*cb_conn_open)(lu * l, int connid, luuserdata & userdata);
// 回调 数据到达
typedef void(*cb_conn_recv_packet)(lu * l, int connid, const char * buff, size_t size, luuserdata & userdata);
// 回调 连接关闭
typedef void(*cb_conn_close)(lu * l, int connid, luuserdata & userdata, int reason);

struct luconfig
{
    luconfig() : lum(&malloc), luf(&free), type(lut_tcpserver),
		port(8888), maxconnnum(1000),
		backlog(128), linger(0), 
		iskeepalive(true), keepidle(60), keepinterval(5), keepcount(3),
		isnonblocking(true), isnodelay(true),
		sendbuff(1024*1024), recvbuff(1024*1024),
		socket_sendbuff(1024*256), socket_recvbuff(1024*256),
		waittimeout(1), cco(0), ccrp(0), ccc(0), 
		maxrecvpacket_perframe(10000), maxpacketlen(100*1024),
		isencrypt(true), iscompress(true)
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
	// 最大连接数
	int maxconnnum;
	// tcp参数
	int backlog;
	int linger;
	// 心跳
	bool iskeepalive;
	int keepidle;
	int keepinterval;
	int keepcount;
	// 非阻塞
	bool isnonblocking;
	// 无延迟
	bool isnodelay;
	// 发送接收缓冲区
	int sendbuff;
	int recvbuff;
	// socket发送接收缓冲区大小
	int socket_sendbuff;
	int socket_recvbuff;
	// select超时时间
	int waittimeout;
	// 回调函数
	cb_conn_open cco;
	cb_conn_recv_packet ccrp;
	cb_conn_close ccc;
	// 每帧处理消息数目
	int32_t maxrecvpacket_perframe;
	// 消息最大size
	uint32_t maxpacketlen;
	// 是否加密
	bool isencrypt;
	// 是否压缩
	bool iscompress;
};

// 初始化
LU_API void inilu();

// 申请回收
LU_API lu * newlu(luconfig * cfg = 0);
LU_API void dellu(lu * l);

// 心跳
LU_API void ticklu(lu * l);

// 类型
LU_API lutype gettypelu(lu * l);

// 发送消息
enum luerrortype
{
    luet_ok,
    // 类型错误
    luet_typeerr,
    // 链接无效
    luet_conninvalid,
    // 消息太大
    luet_msgtoobig,
    // 缓冲区满
    luet_sendbufffull,
    // 压缩失败
    luet_compressfail,
    // 加密失败
    luet_encryptfail,
};

// 发送消息
LU_API int sendlu(lu * l, char * buffer, size_t size, int connid = -1);


