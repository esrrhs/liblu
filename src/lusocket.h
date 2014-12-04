#pragma once

#include "lutypes.h"
#include "lu.h"
#include "circle_buffer.h"
#include "lustack.h"

struct lu;
struct lutcplink;
struct epoll_event;
typedef int (*cb_link_err)(lutcplink * ltl);
typedef int (*cb_link_in)(lutcplink * ltl);
typedef int (*cb_link_out)(lutcplink * ltl);
typedef int (*cb_link_close)(lutcplink * ltl);
// window下仅供调试
struct luselector
{
    void ini(lu * _l, size_t _len, int _waittime, 
        cb_link_err _cbe, 
        cb_link_in _cbi, 
        cb_link_out _cbo,
        cb_link_close _cbc);
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
    cb_link_close cbc;
#ifdef WIN32
	typedef std::map<socket_t, lutcplink *> lutcplinkmap;
	lutcplinkmap * ltlmap;
#else
    int epollfd;
	epoll_event * events;
#endif
};

struct lutcplink
{
    void ini(lu * _l, int _index);
    void fini();
    void clear();
    
	lu * l;
	int index;
	socket_t s;
    circle_buffer sendbuff;
    circle_buffer recvbuff;
    size_t sendpacket;
    size_t recvpacket;
    size_t sendbytes;
    size_t recvbytes;
    size_t processtime;
	char ip[LU_IP_SIZE];
	uint16_t port;
	char peerip[LU_IP_SIZE];
	uint16_t peerport;
	luuserdata userdata;
};

struct lutcpserver
{
    lutcplink * alloc_tcplink();
    void dealloc_tcplink(lutcplink * ltl);

	lu * l;
	socket_t s;
	lutcplink * ltls;
	size_t ltlsnum;
	lustack<int> ltlsfreeindex;
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

int on_tcpserver_err(lutcplink * ltl);
int on_tcpserver_in(lutcplink * ltl);
int on_tcpserver_out(lutcplink * ltl);
int on_tcpserver_accept(lutcplink * ltl);
int on_tcpserver_close(lutcplink * ltl);

bool set_socket_nonblocking(socket_t s, bool on);
bool set_socket_linger(socket_t s, uint32_t lingertime);
void close_socket(socket_t s);

