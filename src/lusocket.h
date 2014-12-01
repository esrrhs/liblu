#pragma once

#include "lutypes.h"
#include "lu.h"
#include "circle_buffer.h"

struct lu;
struct lutcplink;
struct epoll_event;
typedef int (*cb_link_err)(lutcplink * ltl);
typedef int (*cb_link_in)(lutcplink * ltl);
typedef int (*cb_link_out)(lutcplink * ltl);
struct luselector
{
    void ini(lu * _l, size_t _len, int _waittime, 
        cb_link_err _cbe, 
        cb_link_in _cbi, 
        cb_link_out _cbo);
    void fini();
    bool add(lutcplink * ltl);
    bool del(lutcplink * ltl);
    bool select();
    
    lu * l;
    size_t len;
    int waittime;
    cb_link_err cbe;
    cb_link_in cbi;
    cb_link_out cbo;
#ifdef WIN32
#else
    int epollfd;
    epoll_event * events;
};
#endif

struct lutcplink
{
	socket_t s;
    circle_buffer sendbuff;
    circle_buffer recvbuff;
    size_t sendpacket;
    size_t recvpacket;
    size_t sendbytes;
    size_t recvbytes;
    size_t processtime;
    void clear();
};

struct lutcpserver
{
	lu * l;
	socket_t s;
	lutcplink * ltls;
	size_t ltlsnum;
	luselector ls;
};

struct lutcpclient
{
	lu * l;
	lutcplink ltl;
};

struct lu
{
	lumalloc lum;
	lufree luf;
	luconfig cfg;
	lutype type;
	lutcpserver * ts;
	lutcpclient * tc;
};

lutcpserver * newtcpserver(lu * l);
void deltcpserver(lutcpserver * lts);
lutcpclient * newtcpclient(lu * l);
void deltcpclient(lutcpclient * ltc);

void ticktcpserver(lutcpserver * lts);
void ticktcpclient(lutcpclient * ltc);

bool set_socket_nonblocking(socket_t s, bool on);
void close_socket(socket_t s);

