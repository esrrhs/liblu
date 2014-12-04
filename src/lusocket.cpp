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
        ts->ltls[i].ini(l, i);
    }

    // 建立selector
    ts->ls.ini(l, ts->ltlsnum, l->cfg.waittimeout, on_tcpserver_err, on_tcpserver_in, on_tcpserver_out, on_tcpserver_close);

    // 把server的listen加进去
    ts->ltls[0].s = ts->s;
    ts->ls.add(&ts->ltls[0]);

    // 建立index
    ts->ltlsfreeindex.ini(l, ts->ltlsnum);
    for (int i = 1; i < (int)ts->ltlsnum; i++)
    {
        ts->ltlsfreeindex.push(i);
    }

	return ts;
}

void deltcpserver(lutcpserver * lts)
{
    lts->ls.fini();
    for (int i = 0; i < (int)lts->ltlsnum; i++)
    {
        lts->ltls[i].fini();
    }
	safelufree(lts->l, lts->ltls);
	close_socket(lts->s);
	safelufree(lts->l, lts);
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

bool set_socket_linger(socket_t s, uint32_t lingertime)
{
	linger so_linger;
	so_linger.l_onoff = true;
	so_linger.l_linger = lingertime;
	return ::setsockopt(s, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) == 0;
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
    lts->ls.select();
}

void ticktcpclient(lutcpclient * ltc)
{
}

void lutcplink::ini(lu * _l, int _index)
{
    l = _l;
    index = _index;
    s = -1;
    sendbuff.ini(_l, _l->cfg.sendbuff);
    recvbuff.ini(_l, _l->cfg.recvbuff);
    sendpacket = 0;
    recvpacket = 0;
    sendbytes = 0;
    recvbytes = 0;
    processtime = 0;
}

void lutcplink::fini()
{
    close_socket(s);
    sendbuff.fini();
    recvbuff.fini();
}

void lutcplink::clear()
{
    close_socket(s);
    s = -1;
    sendbuff.clear();
    recvbuff.clear();
    sendpacket = 0;
    recvpacket = 0;
    sendbytes = 0;
    recvbytes = 0;
    processtime = 0;
    ip[0] = 0;
    port = 0;
    peerip[0] = 0;
    peerport = 0;
    userdata.u64 = 0;
}

void luselector::ini(lu * _l, size_t _len, int _waittime, 
        cb_link_err _cbe, 
        cb_link_in _cbi, 
        cb_link_out _cbo,
        cb_link_close _cbc)
{
    l = _l;
    len = _len;
    waittime = _waittime;
    cbe = _cbe;
    cbi = _cbi;
    cbo = _cbo;
    cbc = _cbc;
#ifdef WIN32
	ltlmap = (lutcplinkmap *)safelumalloc(l, sizeof(lutcplinkmap));
	new (ltlmap)lutcplinkmap();
#else
    epollfd = ::epoll_create(len);
    events = (epoll_event *)safelumalloc(l, sizeof(epoll_event) * len);
#endif
}

void luselector::fini()
{
#ifdef WIN32
	ltlmap->~lutcplinkmap();
	safelufree(l, ltlmap);
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
	return true;
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
	return true;
#else
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
	return true;
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
	        cbe(ltl);
	        del(ltl);
	        cbc(ltl);
	    }
	    if(EPOLLIN & ev.events)
	    {
	        if (cbi(ltl) != 0)
	        {
	            del(ltl);
	            cbc(ltl);
	        }
	    }
	    if(EPOLLOUT & ev.events)
	    {
	        if (cbo(ltl) != 0)
	        {
	            del(ltl);
	            cbc(ltl);
	        }
	    }
    }
    
	return true;
#endif
}

lutcplink * lutcpserver::alloc_tcplink()
{
    int index = 0;
    if (!ltlsfreeindex.pop(index))
    {
        return 0;
    }

    ltls[index].clear();
    
    return &ltls[index];
}

void lutcpserver::dealloc_tcplink(lutcplink * ltl)
{
    ltl->clear();
    ltlsfreeindex.push(ltl->index);
}

int on_tcpserver_err(lutcplink * ltl)
{
    LULOG("on_tcpserver_err %d from %s:%u", ltl->s, ltl->peerip, ltl->peerport);

    return -1;
}

int on_tcpserver_in(lutcplink * ltl)
{
    LULOG("on_tcpserver_in %d from %s:%u", ltl->s, ltl->peerip, ltl->peerport);
    
    lu * l = ltl->l;
    lutcpserver * lts = l->ts;
    
    // accept
    if (lts->s == ltl->s)
    {
        on_tcpserver_accept(ltl);
        return 0;
    }

    // 写入到输入缓冲区
	if (ltl->recvbuff.full())
	{
        LULOG("on_tcpserver_in recv buff is full(%u) %d from %s:%u", ltl->recvbuff.size(), ltl->s, ltl->peerip, ltl->peerport);
		return 0;
	}

	int len = ::recv(ltl->s, ltl->recvbuff.get_write_line_buffer(), ltl->recvbuff.get_write_line_size(), 0);

    LULOG("on_tcpserver_in recv size(%d) %d from %s:%u", len, ltl->s, ltl->peerip, ltl->peerport);
        
	if (len == 0 || len < 0)
	{
		if (GET_NET_ERROR != NET_BLOCK_ERROR && GET_NET_ERROR != NET_BLOCK_ERROR_EX)
		{
            LULOG("on_tcpserver_in socket error %d from %s:%u", ltl->s, ltl->peerip, ltl->peerport);
		    return -1;
		}
	}
	else
	{
		ltl->recvbuff.skip_write(len);
	}

	// 解包
    
    return 0;
}

int on_tcpserver_accept(lutcplink * ltl)
{
	sockaddr_in _sockaddr;
	memset(&_sockaddr, 0, sizeof(_sockaddr));
	socklen_t size = sizeof(_sockaddr);
    socket_t s = ::accept(ltl->s, (sockaddr*)&_sockaddr, &size);
    if (s == -1)
    {
        return -1;
    }

    // 创建新连接
    lu * l = ltl->l;
    lutcpserver * lts = l->ts;
    lutcplink * newltl = lts->alloc_tcplink();

    newltl->s = s;
	strcpy(newltl->peerip, inet_ntoa(_sockaddr.sin_addr));
	newltl->peerport = htons(_sockaddr.sin_port);

    ::setsockopt(s, SOL_SOCKET, SO_RCVBUF, &l->cfg.socket_recvbuff, sizeof(int));
    ::setsockopt(s, SOL_SOCKET, SO_SNDBUF, &l->cfg.socket_sendbuff, sizeof(int));   

    set_socket_nonblocking(s, l->cfg.isnonblocking);
    set_socket_linger(s, l->cfg.linger);

	sockaddr_in _local_sockaddr;
	memset(&_local_sockaddr, 0, sizeof(_local_sockaddr));
	size = sizeof(_local_sockaddr);
	if (::getsockname(s, (sockaddr *)&_local_sockaddr, &size) == 0)
	{
		strcpy(newltl->ip, inet_ntoa(_local_sockaddr.sin_addr));
		newltl->port = htons(_local_sockaddr.sin_port);
	}
	
    // 加入到selector
    lts->ls.add(newltl);

    // 上层的回调
    if (l->cfg.cco)
    {
        l->cfg.cco(l, newltl->index, newltl->userdata);
    }

    LULOG("accept new link from %s:%u", newltl->peerip, newltl->peerport);
    
    return 0;
}

int on_tcpserver_out(lutcplink * ltl)
{    
    if (ltl->sendbuff.empty())
    {
    	return true;
    }
    
    LULOG("on_tcpserver_out %d to %s:%u", ltl->s, ltl->peerip, ltl->peerport);

    int len = ::send(ltl->s, 
    	ltl->sendbuff.get_read_line_buffer(), 
    	ltl->sendbuff.get_read_line_size(), 0);

    LULOG("on_tcpserver_out send size(%d) %d to %s:%u", len, ltl->s, ltl->peerip, ltl->peerport);
    
    if (len == 0 || len < 0)
    {
    	if (GET_NET_ERROR != NET_BLOCK_ERROR && GET_NET_ERROR != NET_BLOCK_ERROR_EX)
    	{
            LULOG("on_tcpserver_out socket error %d to %s:%u", ltl->s, ltl->peerip, ltl->peerport);
    		return -1;
    	}
    }
    else
    {
    	ltl->sendbuff.skip_read(len);
    }

    return 0;
}

int on_tcpserver_close(lutcplink * ltl)
{
    LULOG("on_tcpserver_close %d from %s:%u", ltl->s, ltl->peerip, ltl->peerport);

    lu * l = ltl->l;
    lutcpserver * lts = l->ts;

    lts->dealloc_tcplink(ltl);
    
    // 上层的回调
    if (l->cfg.ccc)
    {
        l->cfg.ccc(l, ltl->index, ltl->userdata);
    }
    
    return 0;
}

