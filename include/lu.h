#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

#define LU_VERSION "1.1"
#define LU_VERSION_NUM 110
#define LU_AUTHOR "esrrhs@163.com"

enum eluerror
{
    elu_ok = 0,
};

typedef void * (*lumalloc)(size_t size);
typedef void (*lufree)(void *ptr);

#define LU_API extern "C"

struct lu;

enum lutype
{
    lut_tcpserver,
    lut_tcpclient,
};

#define LU_IP_SIZE 64

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

typedef void (*cb_conn_open)(lu * l, int connid, luuserdata & userdata);
typedef void (*cb_conn_recv_packet)(lu * l, int connid, const char * buff, size_t size, luuserdata & userdata);
typedef void (*cb_conn_close)(lu * l, int connid, luuserdata & userdata, int reason);

struct luconfig
{
    luconfig()
        : lum(&malloc)
        , luf(&free)
        , type(lut_tcpserver)
        , port(8888)
        , maxconnnum(1000)
        , backlog(128)
        , linger(0)
        , iskeepalive(true)
        , keepidle(60)
        , keepinterval(5)
        , keepcount(3)
        , isnonblocking(true)
        , isnodelay(true)
        , sendbuff(1024 * 1024)
        , recvbuff(1024 * 1024)
        , socket_sendbuff(1024 * 256)
        , socket_recvbuff(1024 * 256)
        , waittimeout(1)
        , cco(0)
        , ccrp(0)
        , ccc(0)
        , maxrecvpacket_perframe(10000)
        , maxpacketlen(100 * 1024)
    {
        strcpy(ip, "127.0.0.1");
        memset(&userdata, 0, sizeof(userdata));
    }

    lumalloc lum;
    lufree luf;
    lutype type;
    char ip[LU_IP_SIZE];
    uint16_t port;
    int maxconnnum;
    int backlog;
    int linger;
    bool iskeepalive;
    int keepidle;
    int keepinterval;
    int keepcount;
    bool isnonblocking;
    bool isnodelay;
    int sendbuff;
    int recvbuff;
    int socket_sendbuff;
    int socket_recvbuff;
    int waittimeout;
    cb_conn_open cco;
    cb_conn_recv_packet ccrp;
    cb_conn_close ccc;
    int32_t maxrecvpacket_perframe;
    uint32_t maxpacketlen;
    luuserdata userdata;
};

LU_API void inilu();

LU_API lu * newlu(luconfig * cfg = 0);
LU_API void dellu(lu * l);

LU_API void ticklu(lu * l);

LU_API lutype gettypelu(lu * l);

enum luerrortype
{
    luet_ok,
    luet_typeerr,
    luet_conninvalid,
    luet_msgtoobig,
    luet_sendbufffull,
};

LU_API int sendlu(lu * l, const char * buffer, size_t size, int connid = -1);

LU_API luuserdata * getlu_userdata(lu * l);
LU_API luuserdata * getlu_conn_userdata(lu * l, int connid = -1);
