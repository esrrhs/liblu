#include "lu.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#if defined(LIBLU_USE_GPERFTOOLS) && !defined(_WIN32)
#include "gperftools/profiler.h"
#endif

namespace {

int loopnum = 0;
int loopmax = 1000000;

void on_conn_open(lu * l, int connid, luuserdata & userdata)
{
    (void)userdata;
    printf("on_conn_open %d\n", connid);
    if (gettypelu(l) == lut_tcpserver)
    {
        char s[100];
        sprintf(s, "welcome %d", connid);
        sendlu(l, s, strlen(s) + 1, connid);
    }
}

void on_conn_recv_packet(lu * l, int connid, const char * buff, size_t size, luuserdata & userdata)
{
    (void)buff;
    (void)size;
    (void)userdata;
#ifdef _DEBUG
    printf("on_conn_recv_packet %d %d : %s\n", connid, (int)size, buff);
#endif
    char s[100];
    sprintf(s, "hello i am connid %d", connid);
    sendlu(l, s, strlen(s) + 1, connid);
    loopnum++;
}

void on_conn_close(lu * l, int connid, luuserdata & userdata, int reason)
{
    (void)l;
    (void)userdata;
    printf("on_conn_close %d %d %s\n", connid, reason, strerror(reason));
    loopnum = loopmax;
}

int run_smoke()
{
    inilu();

    luconfig cfg;
    cfg.cco = on_conn_open;
    cfg.ccrp = on_conn_recv_packet;
    cfg.ccc = on_conn_close;
    cfg.type = lut_tcpserver;
    cfg.port = 0;
    strcpy(cfg.ip, "127.0.0.1");

    lu * l = newlu(&cfg);
    if (!l)
    {
        std::cerr << "smoke: newlu failed\n";
        return 1;
    }

    for (int i = 0; i < 10; ++i)
    {
        ticklu(l);
    }

    dellu(l);
    std::cout << "smoke ok\n";
    return 0;
}

int run_role(const std::string & name)
{
    luconfig cfg;
    cfg.cco = on_conn_open;
    cfg.ccrp = on_conn_recv_packet;
    cfg.ccc = on_conn_close;

    if (name == "server")
    {
        cfg.type = lut_tcpserver;
    }
    else if (name == "client")
    {
        cfg.type = lut_tcpclient;
    }
    else
    {
        std::cout << "need arg: [server|client|smoke]\n";
        return 1;
    }

    inilu();
    lu * l = newlu(&cfg);
    if (!l)
    {
        std::cout << "new lu fail\n";
        return 1;
    }

#if defined(LIBLU_USE_GPERFTOOLS) && !defined(_WIN32) && !defined(_DEBUG)
    ProfilerStart(("test_" + name + ".prof").c_str());
#endif

#ifdef _DEBUG
    while (1)
#else
    while (loopnum < loopmax)
#endif
    {
        ticklu(l);
    }

#if defined(LIBLU_USE_GPERFTOOLS) && !defined(_WIN32) && !defined(_DEBUG)
    ProfilerStop();
#endif

    dellu(l);
    std::cout << "finish\n";
    return 0;
}

} // namespace

int main(int argc, char * argv[])
{
    if (argc <= 1)
    {
        std::cout << "need arg: [server|client|smoke]\n";
        return 1;
    }

    std::string name = argv[1];
    if (name == "smoke")
    {
        return run_smoke();
    }
    return run_role(name);
}
