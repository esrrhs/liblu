#include "lusocket.h"
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
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
#define UINT8_MAX	0xff
#define UINT16_MAX	0xffff
#define UINT32_MAX	0xffffffff
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
		LUERR("create socket error %s:%u", l->cfg.ip, l->cfg.port);
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
		LUERR("bind socket error %s:%u", l->cfg.ip, l->cfg.port);
		safelufree(l, ts);
		return 0;
	}

	// listen
	ret = ::listen(ts->s, l->cfg.backlog);
	if (ret != 0)
	{
		LUERR("listen socket error %s:%u", l->cfg.ip, l->cfg.port);
		safelufree(l, ts);
		return 0;
	}

	// 非阻塞
	if (!set_socket_nonblocking(ts->s, l->cfg.isnonblocking))
	{
		LUERR("set nonblocking socket error %s:%u", l->cfg.ip, l->cfg.port);
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
    for (int i = (int)ts->ltlsnum - 1; i >= 1; i--)
    {
        ts->ltlsfreeindex.push(i);
    }

    // 建立解包缓冲区
    l->recvpacketbuffer = (char*)safelumalloc(l, l->cfg.maxpacketlen);
    l->recvpacketbufferex = (char*)safelumalloc(l, l->cfg.maxpacketlen);
    
	LULOG("start tcp server ok %s %d", l->cfg.ip, l->cfg.port);
	
	return ts;
}

void deltcpserver(lutcpserver * lts)
{
    lu * l = lts->l;
	safelufree(l, l->recvpacketbuffer);
	safelufree(l, l->recvpacketbufferex);
    lts->ltlsfreeindex.fini();
    lts->ls.fini();
    for (int i = 0; i < (int)lts->ltlsnum; i++)
    {
        lts->ltls[i].fini();
    }
	safelufree(l, lts->ltls);
	close_socket(lts->s);
	safelufree(l, lts);
}

bool lutcpclient::reconnect()
{
    lu * l = ltl.l;
    
	LULOG("reconnect tcp client %s %d", l->cfg.ip, l->cfg.port);

    close_socket(ltl.s);
    ltl.s = -1;
    
	strcpy(ltl.peerip, l->cfg.ip);
	ltl.peerport = l->cfg.port;

	// create
	socket_t s = ::socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
	{
		LUERR("create socket error %s:%u", l->cfg.ip, l->cfg.port);
		return false;
	}	
    ltl.s = s;

	// connect
	sockaddr_in _sockaddr;
	memset(&_sockaddr, 0, sizeof(_sockaddr));
	_sockaddr.sin_family = AF_INET;
	_sockaddr.sin_port = htons(l->cfg.port);
	_sockaddr.sin_addr.s_addr = inet_addr(l->cfg.ip);
	int ret = ::connect(s, (const sockaddr *)&_sockaddr, sizeof(_sockaddr));
	if (ret != 0)
	{
		LUERR("connect error %s:%u", l->cfg.ip, l->cfg.port);
        close_socket(ltl.s);
        ltl.s = -1;
		return false;
	}

	// 缓冲区
    ::setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char *)&l->cfg.socket_recvbuff, sizeof(int));
	::setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char *)&l->cfg.socket_sendbuff, sizeof(int));

	// 非阻塞
    set_socket_nonblocking(s, l->cfg.isnonblocking);

	// linger
    set_socket_linger(s, l->cfg.linger);

	// 获取自己的信息
	sockaddr_in _local_sockaddr;
	memset(&_local_sockaddr, 0, sizeof(_local_sockaddr));
	socklen_t size = sizeof(_local_sockaddr);
	if (::getsockname(s, (sockaddr *)&_local_sockaddr, &size) == 0)
	{
		strcpy(ltl.ip, inet_ntoa(_local_sockaddr.sin_addr));
		ltl.port = htons(_local_sockaddr.sin_port);
	}

	LULOG("reconnect tcp client ok %s %d", l->cfg.ip, l->cfg.port);
	
	ls.add(&ltl);
	    
    // 上层的回调
    if (l->cfg.cco)
    {
        l->cfg.cco(l, ltl.index, ltl.userdata);
    }
    
    return 0;
}

lutcpclient * newtcpclient(lu * l)
{
	LULOG("start tcp client %s %d", l->cfg.ip, l->cfg.port);
	
	lutcpclient * tc = (lutcpclient *)l->lum(sizeof(lutcpclient));
	memset(tc, 0, sizeof(lutcpclient));
    tc->ltl.ini(l, 0);

    // 建立selector
    tc->ls.ini(l, 1, l->cfg.waittimeout, on_tcpclient_err, on_tcpclient_in, on_tcpclient_out, on_tcpclient_close);
    
    // 建立解包缓冲区
    l->recvpacketbuffer = (char*)safelumalloc(l, l->cfg.maxpacketlen);
    l->recvpacketbufferex = (char*)safelumalloc(l, l->cfg.maxpacketlen);
    
	LULOG("start tcp client ok %s %d", l->cfg.ip, l->cfg.port);
	
	return tc;
}

void deltcpclient(lutcpclient * ltc)
{
    lu * l = ltc->ltl.l;
	safelufree(l, l->recvpacketbuffer);
	safelufree(l, l->recvpacketbufferex);
    ltc->ltl.fini();
    ltc->ls.fini();
	safelufree(l, ltc);
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
	return ::setsockopt(s, SOL_SOCKET, SO_LINGER, (const char *)&so_linger, sizeof(so_linger)) == 0;
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
    recv_tcpserver_packet(lts);
}

void recv_tcpserver_packet(lutcpserver * lts)
{
    lu * l = lts->l;
    int maxvalidnum = lts->ltlsnum - lts->ltlsfreeindex.size() - 1;
    int validnum = 0;
    for (int i = 1; i < (int)lts->ltlsnum && validnum < maxvalidnum; i++)
    {   
        lutcplink & ltl = lts->ltls[i];
        if (ltl.s > 0)
        {
            for (int j = 0; j < (int)l->cfg.maxrecvpacket_perframe; j++)
            {
                LULOG("recv_tcpserver_packet %d from %s:%u", ltl.s, ltl.peerip, ltl.peerport);
                if (unpack_packet(l, &ltl.recvbuff, ltl.index, ltl.userdata) != 0)
                {
                    break;
                }
            }
            validnum++;
        }
    }
}

int unpack_packet(lu * l, circle_buffer * cb, int connid, luuserdata & userdata)
{    
    if (cb->empty())
    {
        return -1;
    }

    LULOG("unpack_packet from %d size(%d)", connid, (int)cb->size());
                
    // 找出正确包头
    msgheader head;
    head.magic = 0;
    while (1)
    {
        cb->store();
        if (!cb->read((char*)&head, sizeof(head)))
        {
            LULOG("unpack_packet from %d not enough head size(%d)", connid, (int)cb->size());
            return -1;
        }

        if (head.magic == HEAD_MAGIC)
        {
            break;
        }

        assert(0);
        cb->skip_read(1);
        LUERR("unpack_packet from %d wrong head %u size(%d)", connid, head.magic, (int)cb->size());
    }

    // 超过最大大小，说明包有问题，跳过他
    if (head.size >= l->cfg.maxpacketlen)
    {
        LUERR("unpack_packet from %d max size %d size(%d)", connid, (int)head.size, (int)l->cfg.maxpacketlen);
        cb->restore();
        cb->skip_read(1);
        assert(0);
        return -1;
    }

    // size不够，等下次
    if (!cb->read(l->recvpacketbuffer, head.size))
    {
        LUERR("unpack_packet from %d not enough size %d size(%d)", connid, (int)head.size, (int)cb->size());
        cb->restore();
        return -1;
    }

    LULOG("unpack_packet start %d size(%d)", connid, (int)head.size);
    
    char * buffer = l->recvpacketbuffer;
    size_t datasize = head.size;
    char * destbuffer = l->recvpacketbufferex;
    size_t destdatasize = 0;
    if (IS_ENCRYPT(head.flag))
    {
        if (!decrypt_packet(buffer, datasize, destbuffer, l->cfg.maxpacketlen, destdatasize))
        {
            LUERR("unpack_packet decrypt %d err size(%d)", connid, (int)datasize);
            assert(0);
            return -1;
        }
        LULOG("unpack_packet decrypt %d new size(%d) oldsize(%d)", connid, (int)destdatasize, (int)datasize);
        luswap(buffer, destbuffer);
        luswap(datasize, destdatasize);
    }

    if (IS_COMPRESS(head.flag))
    {
        if (!decompress_packet(buffer, datasize, destbuffer, l->cfg.maxpacketlen, destdatasize))
        {
            LUERR("unpack_packet decompress %d err size(%d)", connid, (int)datasize);
            assert(0);
            return -1;
        }
        LULOG("unpack_packet decompress %d new size(%d) oldsize(%d)", connid, (int)destdatasize, (int)datasize);
        luswap(buffer, destbuffer);
        luswap(datasize, destdatasize);
    }

    if (l->cfg.ccrp)
    {
        LULOG("unpack_packet cb_conn_recv_packet %d size(%d)", connid, (int)datasize);
        l->cfg.ccrp(l, connid, buffer, datasize, userdata);
    }
    
    return 0;
}

void ticktcpclient(lutcpclient * ltc)
{
    if (ltc->ltl.s < 0)
    {
        ltc->reconnect();
    }
    ltc->ls.select();
    recv_tcpclient_packet(ltc);
}

void recv_tcpclient_packet(lutcpclient * ltc)
{
    lu * l = ltc->ltl.l;
    lutcplink & ltl = ltc->ltl;
    if (ltl.s > 0)
    {
        for (int j = 0; j < (int)l->cfg.maxrecvpacket_perframe; j++)
        {
            LULOG("recv_tcpclient_packet %d from %s:%u", ltl.s, ltl.peerip, ltl.peerport);
            if (unpack_packet(l, &ltl.recvbuff, ltl.index, ltl.userdata) != 0)
            {
                break;
            }
        }
    }
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
    memset(&userdata, 0, sizeof(userdata));
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
	(*ltlmap)[ltl->s] = ltl;
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
	(*ltlmap).erase(ltl->s);
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
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	socket_t maxfd;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	maxfd = -1;

	for (lutcplinkmap::iterator it = (*ltlmap).begin(); it != (*ltlmap).end(); it++)
	{
		socket_t s = it->first;
		FD_SET(s, &readfds);
		FD_SET(s, &writefds);
		FD_SET(s, &exceptfds);
		maxfd = LUMAX(maxfd, s + 1);
	}

	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int ret = ::select(maxfd + 1, 
		&readfds,
		&writefds,
		&exceptfds,
		&timeout);

	if (ret < 0)
	{
		return false;
	}
	
	std::vector<lutcplink *> delvec;
	for (lutcplinkmap::iterator it = (*ltlmap).begin(); it != (*ltlmap).end(); it++)
	{
		socket_t s = it->first;
		lutcplink * ltl = it->second;
		if (FD_ISSET(s, &exceptfds) != 0)
		{
			cbe(ltl);
			delvec.push_back(ltl);
			cbc(ltl);
			continue;
		}
		if (FD_ISSET(s, &readfds) != 0)
		{
			if (cbi(ltl) != 0)
			{
				delvec.push_back(ltl);
				cbc(ltl);
				continue;
			}
		}
		if (FD_ISSET(s, &writefds) != 0)
		{
			if (cbo(ltl) != 0)
			{
				delvec.push_back(ltl);
				cbc(ltl);
				continue;
			}
		}
	}

	for (int i = 0; i < (int)delvec.size(); i++)
	{
		del(delvec[i]);
	}

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
			continue;
	    }
	    if(EPOLLIN & ev.events)
	    {
	        if (cbi(ltl) != 0)
	        {
	            del(ltl);
				cbc(ltl);
				continue;
	        }
	    }
	    if(EPOLLOUT & ev.events)
	    {
	        if (cbo(ltl) != 0)
	        {
	            del(ltl);
				cbc(ltl);
				continue;
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

    ::setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char *)&l->cfg.socket_recvbuff, sizeof(int));
	::setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char *)&l->cfg.socket_sendbuff, sizeof(int));

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
    	return 0;
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

    // 上层的回调
    if (l->cfg.ccc)
    {
        l->cfg.ccc(l, ltl->index, ltl->userdata);
    }
    
    lts->dealloc_tcplink(ltl);
    
    return 0;
}

const int encrypt_key[] = {0x77073096UL, 0xee0e612cUL, 0x990951baUL, 0x076dc419UL,
    0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL, 0x0edb8832UL, 0x79dcb8a4UL,
    0xe0d5e91eUL, 0x97d2d988UL, 0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL,
    0x90bf1d91UL, 0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
    0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL, 0x136c9856UL,
    0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL, 0x14015c4fUL, 0x63066cd9UL};

bool encrypt_packet(char * buffer, size_t size, char * obuffer, size_t omaxsize, size_t & osize)
{
	for (int i = 0; i < (int)size; i++)
	{
		obuffer[i] = buffer[size - 1 - i] ^ ((char*)encrypt_key)[i % sizeof(encrypt_key)];
	}
	osize = size;
    return true;
}

bool decrypt_packet(char * buffer, size_t size, char * obuffer, size_t omaxsize, size_t & osize)
{
	for (int i = 0; i < (int)size; i++)
	{
		obuffer[i] = buffer[size - 1 - i] ^ ((char*)encrypt_key)[(size - 1 - i) % sizeof(encrypt_key)];
	}
	osize = size;
    return true;
}

enum compresstype
{
	ct_normal,
	ct_u64,
	ct_s64,
	ct_u32,
	ct_s32,
};

#define MAKE_COMPRESS_HEAD(type, len) ((((uint8_t)type) << 4) | ((uint8_t)len))
#define GET_COMPRESS_TYPE(head) (((uint8_t)head >> 4) & 0x0F)
#define GET_COMPRESS_LEN(head) ((uint8_t)head & 0x0F)

bool docompress(char * buffer, size_t size, int i, char * obuffer, size_t & oldsize, size_t & osize)
{
	do
	{
		if (i + sizeof(uint64_t) <= size)
		{
			uint64_t u64 = *(uint64_t*)&buffer[i];

			if (u64 <= UINT8_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_u64, sizeof(uint8_t));
				obuffer[osize] = head;
				*(uint8_t*)&obuffer[osize + 1] = u64;
				osize = 1 + sizeof(uint8_t);
				oldsize = sizeof(uint64_t);
				return true;
			}
			if (u64 <= UINT16_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_u64, sizeof(uint16_t));
				obuffer[osize] = head;
				*(uint16_t*)&obuffer[osize + 1] = u64;
				osize = 1 + sizeof(uint16_t);
				oldsize = sizeof(uint64_t);
				return true;
			}
			if (u64 <= UINT32_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_u64, sizeof(uint32_t));
				obuffer[osize] = head;
				*(uint32_t*)&obuffer[osize + 1] = u64;
				osize = 1 + sizeof(uint32_t);
				oldsize = sizeof(uint64_t);
				return true;
			}
		}
	}
	while(0);

	do
	{
		if (i + sizeof(uint64_t) <= size)
		{
			uint64_t s64 = *(uint64_t*)&buffer[i] * -1;

			if (s64 <= UINT8_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_s64, sizeof(uint8_t));
				obuffer[osize] = head;
				*(uint8_t*)&obuffer[osize + 1] = s64;
				osize = 1 + sizeof(uint8_t);
				oldsize = sizeof(uint64_t);
				return true;
			}
			if (s64 <= UINT16_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_s64, sizeof(uint16_t));
				obuffer[osize] = head;
				*(uint16_t*)&obuffer[osize + 1] = s64;
				osize = 1 + sizeof(uint16_t);
				oldsize = sizeof(uint64_t);
				return true;
			}
			if (s64 <= UINT32_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_s64, sizeof(uint32_t));
				obuffer[osize] = head;
				*(uint32_t*)&obuffer[osize + 1] = s64;
				osize = 1 + sizeof(uint32_t);
				oldsize = sizeof(uint64_t);
				return true;
			}
		}
	}
	while(0);

	do
	{
		if (i + sizeof(uint32_t) <= size)
		{
			uint32_t u32 = *(uint32_t*)&buffer[i];

			if (u32 <= UINT8_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_u32, sizeof(uint8_t));
				obuffer[osize] = head;
				*(uint8_t*)&obuffer[osize + 1] = u32;
				osize = 1 + sizeof(uint8_t);
				oldsize = sizeof(uint32_t);
				return true;
			}
			if (u32 <= UINT16_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_u32, sizeof(uint16_t));
				obuffer[osize] = head;
				*(uint16_t*)&obuffer[osize + 1] = u32;
				osize = 1 + sizeof(uint16_t);
				oldsize = sizeof(uint32_t);
				return true;
			}
		}
	}
	while(0);

	do
	{
		if (i + sizeof(uint32_t) <= size)
		{
			uint32_t s32 = *(uint32_t*)&buffer[i] * -1;

			if (s32 <= UINT8_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_s32, sizeof(uint8_t));
				obuffer[osize] = head;
				*(uint8_t*)&obuffer[osize + 1] = s32;
				osize = 1 + sizeof(uint8_t);
				oldsize = sizeof(uint32_t);
				return true;
			}
			if (s32 <= UINT16_MAX)
			{
				uint8_t head = MAKE_COMPRESS_HEAD(ct_s32, sizeof(uint16_t));
				obuffer[osize] = head;
				*(uint16_t*)&obuffer[osize + 1] = s32;
				osize = 1 + sizeof(uint16_t);
				oldsize = sizeof(uint32_t);
				return true;
			}
		}
	}
	while(0);

	return false;
}

bool dodecompress(char * buffer, size_t size, int i, char * obuffer, size_t & oldsize, size_t & csize)
{
	uint8_t head = buffer[i];
	int type = GET_COMPRESS_TYPE(head);
	int len = GET_COMPRESS_LEN(head);
	switch (type)
	{
	case ct_normal:
		{
        	if (len >= 16 || len <= 0)
        	{
        		return false;
        	}
			memcpy(obuffer, &buffer[i + 1], len);
			oldsize = len;
			csize = 1 + len;
		}
		break;
	case ct_u64:
		{
			uint64_t data;
			if (len == sizeof(uint32_t))
			{
				data = *(uint32_t*)&buffer[i + 1];
			}
			else if (len == sizeof(uint16_t))
			{
				data = *(uint16_t*)&buffer[i + 1];
			}
			else if (len == sizeof(uint8_t))
			{
				data = *(uint8_t*)&buffer[i + 1];
			}
			else
			{
				return false;
			}
			*(uint64_t*)obuffer = data;
			oldsize = sizeof(uint64_t);
			csize = 1 + len;
		}
		break;
	case ct_s64:
		{
			uint64_t data;
			if (len == sizeof(uint32_t))
			{
				data = *(uint32_t*)&buffer[i + 1] * -1;
			}
			else if (len == sizeof(uint16_t))
			{
				data = *(uint16_t*)&buffer[i + 1] * -1;
			}
			else if (len == sizeof(uint8_t))
			{
				data = *(uint8_t*)&buffer[i + 1] * -1;
			}
			else
			{
				return false;
			}
			*(uint64_t*)obuffer = data;
			oldsize = sizeof(uint64_t);
			csize = 1 + len;
		}
		break;
	case ct_u32:
		{
			uint32_t data;
			if (len == sizeof(uint16_t))
			{
				data = *(uint16_t*)&buffer[i + 1];
			}
			else if (len == sizeof(uint8_t))
			{
				data = *(uint8_t*)&buffer[i + 1];
			}
			else
			{
				return false;
			}
			*(uint32_t*)obuffer = data;
			oldsize = sizeof(uint32_t);
			csize = 1 + len;
		}
		break;
	case ct_s32:
		{
			uint32_t data;
			if (len == sizeof(uint16_t))
			{
				data = *(uint16_t*)&buffer[i + 1] * -1;
			}
			else if (len == sizeof(uint8_t))
			{
				data = *(uint8_t*)&buffer[i + 1] * -1;
			}
			else
			{
				return false;
			}
			*(uint32_t*)obuffer = data;
			oldsize = sizeof(uint32_t);
			csize = 1 + len;
		}
		break;
	default:
		return false;
	}
	return true;
}

bool compress_packet(char * buffer, size_t size, char * obuffer, size_t omaxsize, size_t & osize)
{
	char tmpbuff[sizeof(uint64_t)];
	osize = 0;
	int i = 0;
	while (i < (int)size)
	{
		size_t oldsize = 0;
		size_t csize = 0;
		int normallen = 0;
		for (int j = 0; j < 15 && i + normallen < (int)size; j++)
		{
			if (docompress(buffer, size, i + normallen, tmpbuff, oldsize, csize))
			{
				break;
			}
			normallen++;
		}
		if (normallen)
		{
			if (osize + 1 + normallen > omaxsize)
			{
				return false;
			}
			obuffer[osize] = MAKE_COMPRESS_HEAD(ct_normal, normallen);
			osize++;
			memcpy(obuffer + osize, buffer + i, normallen);
			osize += normallen;
			i += normallen;
		}
		if (csize)
		{
			if (osize + csize > omaxsize)
			{
				return false;
			}
			memcpy(obuffer + osize, tmpbuff, csize);
			osize += csize;
			i += oldsize;
		}
	}
	return true;
}

bool decompress_packet(char * buffer, size_t size, char * obuffer, size_t omaxsize, size_t & osize)
{
	char tmpbuff[16];
	osize = 0;
	int i = 0;
	while (i < (int)size)
	{
		size_t oldsize = 0;
		size_t csize = 0;
		if (!dodecompress(buffer, size, i, tmpbuff, oldsize, csize))
		{
			return false;
		}
		if (osize + oldsize > omaxsize)
		{
			return false;
		}
		memcpy(obuffer + osize, tmpbuff, oldsize);
		osize += oldsize;
		i += csize;
	}
	return true;
}

int pack_packet(lu * l, circle_buffer * cb, int connid, char * buffer, size_t size)
{
    if (size >= l->cfg.maxpacketlen)
    {
        LUERR("pack_packet %d too big size size(%d) maxsize(%d)", connid, (int)size, (int)l->cfg.maxpacketlen);
        return luet_msgtoobig;
    }

    LULOG("pack_packet %d buffer size(%d) size(%d)", connid, (int)cb->size(), (int)size);

    if (!cb->can_write(sizeof(msgheader) + size))
    {
        LULOG("pack_packet %d not enough size(%d) buffersize(%d)", connid, (int)size, (int)cb->size());
        return luet_sendbufffull;
    }
    
    char * srcbuffer = buffer;
    size_t srcdatasize = size;
    char * destbuffer = l->recvpacketbuffer;
    size_t destdatasize = 0;
    
    if (l->cfg.iscompress)
    {
        if (!compress_packet(srcbuffer, srcdatasize, destbuffer, l->cfg.maxpacketlen, destdatasize))
        {
            LUERR("pack_packet %d compress err size(%d)", connid, (int)srcdatasize);
            assert(0);
            return luet_compressfail;
        }
        LULOG("pack_packet %d compress new size(%d) oldsize(%d)", connid, (int)destdatasize, (int)srcdatasize);
        destbuffer = l->recvpacketbufferex;
        srcbuffer = l->recvpacketbuffer;
        luswap(srcdatasize, destdatasize);
    }

    if (l->cfg.isencrypt)
    {
        if (!encrypt_packet(srcbuffer, srcdatasize, destbuffer, l->cfg.maxpacketlen, destdatasize))
        {
            LUERR("pack_packet %d encrypt err size(%d)", connid, (int)srcdatasize);
            assert(0);
            return luet_encryptfail;
        }
        LULOG("pack_packet %d decrypt new size(%d) oldsize(%d)", connid, (int)destdatasize, (int)srcdatasize);
        luswap(srcbuffer, destbuffer);
        luswap(srcdatasize, destdatasize);
    }

    cb->store();

    // 写
    msgheader head;
    head.magic = HEAD_MAGIC;
    if (l->cfg.isencrypt)
    {
        head.flag = SET_ENCRYPT(head.flag);
    }
    if (l->cfg.iscompress)
    {
        head.flag = SET_COMPRESS(head.flag);
    }
    head.size = srcdatasize;
    
    // 写包头
    if (!cb->write((const char *)&head, sizeof(head)))
    {
        cb->restore();
        LULOG("pack_packet %d buffer full(%d) size(%d)", connid, (int)cb->size(), (int)size);
        return luet_sendbufffull;
    }

    // 写数据
    if (!cb->write(srcbuffer, srcdatasize))
    {
        cb->restore();
        LULOG("pack_packet %d buffer full(%d) size(%d)", connid, (int)cb->size(), (int)srcdatasize);
        return luet_sendbufffull;
    }

    return luet_ok;
}

int sendtcpserver(lutcpserver * lts, char * buffer, size_t size, int connid)
{
    lu * l = lts->l;
    
    if (connid <= 0 || connid >= (int)lts->ltlsnum)
    {
        return luet_conninvalid;
    }

    lutcplink & ltl = lts->ltls[connid];
    if (ltl.s < 0)
    {
        return luet_conninvalid;
    }
    
    return pack_packet(l, &ltl.sendbuff, ltl.index, buffer, size);
}

int sendtcpclient(lutcpclient * ltc, char * buffer, size_t size, int connid)
{
    lu * l = ltc->ltl.l;
    lutcplink & ltl = ltc->ltl;
    if (ltl.s < 0)
    {
        return luet_conninvalid;
    }
    
    return pack_packet(l, &ltl.sendbuff, ltl.index, buffer, size);
}

int on_tcpclient_err(lutcplink * ltl)
{   
    LULOG("on_tcpclient_err %d from %s:%u", ltl->s, ltl->peerip, ltl->peerport);

    return -1;
}

int on_tcpclient_in(lutcplink * ltl)
{
    LULOG("on_tcpclient_in %d from %s:%u", ltl->s, ltl->peerip, ltl->peerport);
    
    // 写入到输入缓冲区
	if (ltl->recvbuff.full())
	{
        LULOG("on_tcpclient_in recv buff is full(%u) %d from %s:%u", ltl->recvbuff.size(), ltl->s, ltl->peerip, ltl->peerport);
		return 0;
	}

	int len = ::recv(ltl->s, ltl->recvbuff.get_write_line_buffer(), ltl->recvbuff.get_write_line_size(), 0);

    LULOG("on_tcpclient_in recv size(%d) %d from %s:%u", len, ltl->s, ltl->peerip, ltl->peerport);
        
	if (len == 0 || len < 0)
	{
		if (GET_NET_ERROR != NET_BLOCK_ERROR && GET_NET_ERROR != NET_BLOCK_ERROR_EX)
		{
            LULOG("on_tcpclient_in socket error %d from %s:%u", ltl->s, ltl->peerip, ltl->peerport);
		    return -1;
		}
	}
	else
	{
		ltl->recvbuff.skip_write(len);
	}

    return 0;
}

int on_tcpclient_out(lutcplink * ltl)
{
    if (ltl->sendbuff.empty())
    {
    	return 0;
    }
    
    LULOG("on_tcpclient_out %d to %s:%u", ltl->s, ltl->peerip, ltl->peerport);

    int len = ::send(ltl->s, 
    	ltl->sendbuff.get_read_line_buffer(), 
    	ltl->sendbuff.get_read_line_size(), 0);

    LULOG("on_tcpclient_out send size(%d) %d to %s:%u", len, ltl->s, ltl->peerip, ltl->peerport);
    
    if (len == 0 || len < 0)
    {
    	if (GET_NET_ERROR != NET_BLOCK_ERROR && GET_NET_ERROR != NET_BLOCK_ERROR_EX)
    	{
            LULOG("on_tcpclient_out socket error %d to %s:%u", ltl->s, ltl->peerip, ltl->peerport);
    		return -1;
    	}
    }
    else
    {
    	ltl->sendbuff.skip_read(len);
    }

    return 0;
}

int on_tcpclient_close(lutcplink * ltl)
{
    LULOG("on_tcpclient_close %d from %s:%u", ltl->s, ltl->peerip, ltl->peerport);

    lu * l = ltl->l;
    lutcpclient * ltc = l->tc;

    // 上层的回调
    if (l->cfg.ccc)
    {
        l->cfg.ccc(l, ltl->index, ltl->userdata);
    }
    
    ltc->reconnect();
    
    return 0;
}



