#include "lu.h"
#include <iostream>
#include <stdio.h>
#ifndef WIN32
#include "gperftools/profiler.h"
#endif

int loopnum = 0;
int loopmax = 1000000;

int on_conn_open(lu * l, int connid, luuserdata & userdata)
{
    printf("on_conn_open %d\n", connid);
    if (gettypelu(l) == lut_tcpserver)
    {
        char s[100];
        sprintf(s, "welcome %d", connid);
        sendlu(l, s, strlen(s) + 1, connid);
    }
    return 0;
}

int on_conn_recv_packet(lu * l, int connid, const char * buff, size_t size, luuserdata & userdata)
{
#ifdef _DEBUG
    printf("on_conn_recv_packet %d %d : %s\n", connid, (int)size, buff);
#endif
    char s[100];
    sprintf(s, "hello i am connid %d", connid);
    sendlu(l, s, strlen(s) + 1, connid);
    loopnum++;
    return 0;
}

int on_conn_close(lu * l, int connid, luuserdata & userdata)
{
    printf("on_conn_close %d\n", connid);
    loopnum = loopmax;
    return 0;
}

int main(int argc, char *argv[])
{
	if (argc <= 1)
	{
		std::cout<<"need arg: [server or client]"<<std::endl;
		return 0;
	}

    luconfig cfg;
    cfg.cco = on_conn_open;
    cfg.ccrp = on_conn_recv_packet;
    cfg.ccc = on_conn_close;
	
    std::string name = argv[1];
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
		std::cout<<"need arg: [server or client]"<<std::endl;
		return 0;
    }

	inilu();
	lu * l = newlu(&cfg);
    if (!l)
    {
		std::cout<<"new lu fail"<<std::endl;
		return 0;
    }
    
#ifndef WIN32
#ifndef _DEBUG
	ProfilerStart(((std::string)"test_" + name + ".prof").c_str());
#endif
#endif
#ifdef _DEBUG
    while (1)
#else
    while (loopnum < loopmax)
#endif
	{
	    ticklu(l);
	}
#ifndef WIN32
#ifndef _DEBUG
	ProfilerStop();
#endif
#endif
	
	dellu(l);
	
	std::cout<<"finish"<<std::endl;
	char c;
	std::cin >> c;
	return 0;
}
