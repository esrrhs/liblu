#include "lusocket.h"
#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <execinfo.h>
#include <signal.h>
#include <exception>
#include <setjmp.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/syscall.h> 
#include <sys/reg.h>
#include <sys/user.h>
#endif

lutcpserver * newtcpserver(lu * l)
{
	lutcpserver * ts = (lutcpserver *)safelumalloc(l, sizeof(lutcpserver));
	memset(ts, 0, sizeof(lutcpserver));
	ts->l = l;
	ts->s = -1;

	LULOG("start tcp server %s %d", l->cfg.ip, l->cfg.port);

	// create
	ts->s = ::socket(AF_INET, SOCK_STREAM, 0);
	if (ts->s == -1)
	{
		LUERR("create socket error");
		safelufree(l, ts);
		return 0;
	}

	// bind
	sockaddr_in _sockaddr;
	memset(&_sockaddr, 0, sizeof(_sockaddr));
	_sockaddr.sin_family = AF_INET;
	_sockaddr.sin_port = htons(l->cfg.port);
	if (strlen(l->cfg.ip) > 0)
	{
		_sockaddr.sin_addr.s_addr = inet_addr(l->cfg.ip);
	}
	else
	{
		_sockaddr.sin_addr.s_addr = inet_addr(INADDR_ANY);
	}
	int32_t ret = ::bind(ts->s, (const sockaddr *)&_sockaddr, sizeof(_sockaddr));
	if (ret != 0)
	{
		LUERR("bind socket error");
		safelufree(l, ts);
		return 0;
	}

	// listen
	ret = ::listen(ts->s, l->cfg.backlog);
	if (ret != 0)
	{
		LUERR("listen socket error");
		safelufree(l, ts);
		return 0;
	}

	// 非阻塞
	if (!set_socket_nonblocking(ts->s, l->cfg.isnonblocking))
	{
		LUERR("set nonblocking socket error");
		safelufree(l, ts);
		return 0;
	}

    // 建立link
    ts->ltlsnum = l->cfg.maxconnnum + 1;
    ts->ltls = (lutcplink *)safelumalloc(l, sizeof(lutcplink) * ts->ltlsnum);
	memset(ts->ltls, 0, sizeof(sizeof(lutcplink) * ts->ltlsnum));
    for (int i = 0; i < (int)ts->ltlsnum; i++)
    {
        ts->ltls[i].clear();
    }

    // 建立selector
    ts->ls.ini(l, ts->ltlsnum, l->cfg.waittimeout, 0, 0, 0);

	return ts;
}

void deltcpserver(lutcpserver * lts)
{
	close_socket(lts->s);
	lts->l->cfg.luf(lts);
}

lutcpclient * newtcpclient(lu * l)
{
	lutcpclient * tc = (lutcpclient *)l->lum(sizeof(lutcpclient));
	memset(tc, 0, sizeof(lutcpclient));

	return tc;
}

void deltcpclient(lutcpclient * ltc)
{

}

bool set_socket_nonblocking(socket_t s, bool on)
{
#if defined(WIN32)
	return ioctlsocket(s, FIONBIO, (u_long *)&on) == 0;
#else
	int32_t opts;
	opts = fcntl(s, F_GETFL, 0);

	if (opts < 0)
	{
		return false;
	}

	if (on)
		opts = (opts | O_NONBLOCK);
	else
		opts = (opts & ~O_NONBLOCK);

	fcntl(s, F_SETFL, opts);
	return opts < 0 ? false : true;
#endif
}

void close_socket(socket_t s)
{
	if (s != -1)
	{
#if defined(WIN32)
		::closesocket(s);
#else
		::close(s);
#endif
	}
}

void ticktcpserver(lutcpserver * lts)
{
    
}

void ticktcpclient(lutcpclient * ltc)
{
}

void lutcplink::clear()
{
    s = -1;
    sendbuff.clear();
    recvbuff.clear();
    sendpacket = 0;
    recvpacket = 0;
    sendbytes = 0;
    recvbytes = 0;
    processtime = 0;
}

void luselector::ini(lu * _l, size_t _len, int _waittime, 
        cb_link_err _cbe, 
        cb_link_in _cbi, 
        cb_link_out _cbo)
{
    l = _l;
    len = _len;
    waittime = _waittime;
    cbe = _cbe;
    cbi = _cbi;
    cbo = _cbo;
#ifdef WIN32
#else
    epollfd = ::epoll_create(len);
    events = (epoll_event *)safelumalloc(l, sizeof(epoll_event) * len);
#endif
}

void luselector::fini()
{
#ifdef WIN32
#else
    ::close(epollfd);
    safelufree(l, events);
    epollfd = -1;
    events = 0;
#endif
}

bool luselector::add(lutcplink * ltl)
{
#ifdef WIN32
#else
	epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLERR;
	ev.data.ptr = ltl;
	if (::epoll_ctl(epollfd, EPOLL_CTL_ADD, ltl->s, &ev) != 0) 
	{
	    assert(0);
	    return false;
	}
    return true;
#endif
}

bool luselector::del(lutcplink * ltl)
{
#ifdef WIN32
#else
    ::close(ltl->s);
	if (::epoll_ctl(epollfd, EPOLL_CTL_DEL, ltl->s, 0) == -1) 
	{
	    assert(0);
		return false;
	}
	return true;
#endif
}

bool luselector::select()
{   
#ifdef WIN32
#else
    int ret = ::epoll_wait(epollfd, events, len, waittime);
	if (ret < 0)
	{
		if(EINTR == errno)
		{
			return true;
		}
		assert(0);
		return false;
	}

    for(int i = 0; i < ret; i++)
	{
	    epoll_event & ev = events[i];
	    lutcplink * ltl = (lutcplink *)ev.data.ptr;
	    if(EPOLLERR & ev.events)
	    {
	        if (cbe)
	        {
	            cbe(ltl);
	        }
	        del(ltl);
	    }
	    if(EPOLLIN & ev.events)
	    {
	        if (cbi && cbi(ltl) != 0)
	        {
	            del(ltl);
	        }
	    }
	    if(EPOLLOUT & ev.events)
	    {
	        if (cbo && cbo(ltl) != 0)
	        {
	            del(ltl);
	        }
	    }
    }
    
	return true;
#endif
}


