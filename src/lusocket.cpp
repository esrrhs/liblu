#include "lusocket.h"

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <sys/time.h>
#endif

lutcpserver * newtcpserver(lu * l)
{
    lutcpserver * ts = (lutcpserver *)safelumalloc(l, sizeof(lutcpserver));
    memset(ts, 0, sizeof(lutcpserver));
    ts->l = l;
    ts->s = LU_INVALID_SOCKET;

    LULOG("start tcp server %s %d", l->cfg.ip, l->cfg.port);

    ts->s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (!LU_SOCKET_VALID(ts->s))
    {
        LUERR("create socket error %s:%u", l->cfg.ip, l->cfg.port);
        safelufree(l, ts);
        return 0;
    }

    int reuse = 1;
    ::setsockopt(ts->s, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

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
        _sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (::bind(ts->s, (const sockaddr *)&_sockaddr, sizeof(_sockaddr)) != 0)
    {
        LUERR("bind socket error %s:%u", l->cfg.ip, l->cfg.port);
        close_socket(ts->s);
        safelufree(l, ts);
        return 0;
    }

    if (::listen(ts->s, l->cfg.backlog) != 0)
    {
        LUERR("listen socket error %s:%u", l->cfg.ip, l->cfg.port);
        close_socket(ts->s);
        safelufree(l, ts);
        return 0;
    }

    if (!set_socket_nonblocking(ts->s, l->cfg.isnonblocking))
    {
        LUERR("set nonblocking socket error %s:%u", l->cfg.ip, l->cfg.port);
        close_socket(ts->s);
        safelufree(l, ts);
        return 0;
    }

    ts->ltlsnum = l->cfg.maxconnnum + 1;
    ts->ltls = (lutcplink *)safelumalloc(l, sizeof(lutcplink) * ts->ltlsnum);
    memset(ts->ltls, 0, sizeof(lutcplink) * ts->ltlsnum);
    for (int i = 0; i < (int)ts->ltlsnum; i++)
    {
        ts->ltls[i].ini(l, i);
    }

    ts->ls.ini(l, ts->ltlsnum, l->cfg.waittimeout,
               on_tcpserver_err, on_tcpserver_in, on_tcpserver_out, on_tcpserver_close);

    ts->ltls[0].s = ts->s;
    ts->ls.add(&ts->ltls[0]);

    ts->ltlsfreeindex.ini(l, ts->ltlsnum);
    for (int i = (int)ts->ltlsnum - 1; i >= 1; i--)
    {
        ts->ltlsfreeindex.push(i);
    }

    l->recvpacketbuffer = (char *)safelumalloc(l, l->cfg.maxpacketlen);

    LULOG("start tcp server ok %s %d", l->cfg.ip, l->cfg.port);
    return ts;
}

void deltcpserver(lutcpserver * lts)
{
    lu * l = lts->l;
    safelufree(l, l->recvpacketbuffer);
    l->recvpacketbuffer = 0;
    lts->ltlsfreeindex.fini();
    lts->ls.fini();
    for (int i = 0; i < (int)lts->ltlsnum; i++)
    {
        lts->ltls[i].fini();
    }
    safelufree(l, lts->ltls);
    close_socket(lts->s);
    lts->s = LU_INVALID_SOCKET;
    safelufree(l, lts);
}

bool lutcpclient::reconnect()
{
    lu * l = ltl.l;

    LULOG("reconnect tcp client %s %d", l->cfg.ip, l->cfg.port);

    if (LU_SOCKET_VALID(ltl.s))
    {
        ls.del(&ltl);
        close_socket(ltl.s);
        ltl.s = LU_INVALID_SOCKET;
    }

    strcpy(ltl.peerip, l->cfg.ip);
    ltl.peerport = l->cfg.port;

    socket_t s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (!LU_SOCKET_VALID(s))
    {
        LUERR("create socket error %s:%u", l->cfg.ip, l->cfg.port);
        return false;
    }
    ltl.s = s;

    sockaddr_in _sockaddr;
    memset(&_sockaddr, 0, sizeof(_sockaddr));
    _sockaddr.sin_family = AF_INET;
    _sockaddr.sin_port = htons(l->cfg.port);
    _sockaddr.sin_addr.s_addr = inet_addr(l->cfg.ip);
    if (::connect(s, (const sockaddr *)&_sockaddr, sizeof(_sockaddr)) != 0)
    {
        LUERR("connect error %s:%u", l->cfg.ip, l->cfg.port);
        close_socket(ltl.s);
        ltl.s = LU_INVALID_SOCKET;
        return false;
    }

    ::setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char *)&l->cfg.socket_recvbuff, sizeof(int));
    ::setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char *)&l->cfg.socket_sendbuff, sizeof(int));
    set_socket_nonblocking(s, l->cfg.isnonblocking);
    set_socket_nodelay(s, l->cfg.isnodelay);
    set_socket_keepalive(s, l->cfg.iskeepalive, l->cfg.keepidle, l->cfg.keepinterval, l->cfg.keepcount);
    set_socket_linger(s, l->cfg.linger);

    sockaddr_in _local_sockaddr;
    memset(&_local_sockaddr, 0, sizeof(_local_sockaddr));
    socklen_t size = sizeof(_local_sockaddr);
    if (::getsockname(s, (sockaddr *)&_local_sockaddr, &size) == 0)
    {
        strcpy(ltl.ip, inet_ntoa(_local_sockaddr.sin_addr));
        ltl.port = ntohs(_local_sockaddr.sin_port);
    }

    LULOG("reconnect tcp client ok %s %d", l->cfg.ip, l->cfg.port);

    ls.add(&ltl);

    if (l->cfg.cco)
    {
        l->cfg.cco(l, ltl.index, ltl.userdata);
    }

    return true;
}

lutcpclient * newtcpclient(lu * l)
{
    LULOG("start tcp client %s %d", l->cfg.ip, l->cfg.port);

    lutcpclient * tc = (lutcpclient *)l->lum(sizeof(lutcpclient));
    memset(tc, 0, sizeof(lutcpclient));
    tc->ltl.ini(l, 0);

    tc->ls.ini(l, 1, l->cfg.waittimeout,
               on_tcpclient_err, on_tcpclient_in, on_tcpclient_out, on_tcpclient_close);

    l->recvpacketbuffer = (char *)safelumalloc(l, l->cfg.maxpacketlen);

    LULOG("start tcp client ok %s %d", l->cfg.ip, l->cfg.port);
    return tc;
}

void deltcpclient(lutcpclient * ltc)
{
    lu * l = ltc->ltl.l;
    safelufree(l, l->recvpacketbuffer);
    l->recvpacketbuffer = 0;
    ltc->ltl.fini();
    ltc->ls.fini();
    safelufree(l, ltc);
}

bool set_socket_nonblocking(socket_t s, bool on)
{
#if defined(_WIN32)
    u_long mode = on ? 1 : 0;
    return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    int opts = fcntl(s, F_GETFL, 0);
    if (opts < 0)
    {
        return false;
    }
    if (on)
    {
        opts |= O_NONBLOCK;
    }
    else
    {
        opts &= ~O_NONBLOCK;
    }
    return fcntl(s, F_SETFL, opts) >= 0;
#endif
}

bool set_socket_linger(socket_t s, uint32_t lingertime)
{
    linger so_linger;
    so_linger.l_onoff = 1;
#if defined(_WIN32)
    so_linger.l_linger = static_cast<u_short>(lingertime);
#else
    so_linger.l_linger = static_cast<int>(lingertime);
#endif
    return ::setsockopt(s, SOL_SOCKET, SO_LINGER, (const char *)&so_linger, sizeof(so_linger)) == 0;
}

bool set_socket_nodelay(socket_t s, bool isnodelay)
{
    int boptval = isnodelay ? 1 : 0;
    return ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&boptval, sizeof(boptval)) == 0;
}

bool set_socket_keepalive(socket_t s, bool keepalive, int keepidle, int keepinterval, int keepcount)
{
    int nkeepalive = keepalive ? 1 : 0;
    if (::setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (const char *)&nkeepalive, sizeof(nkeepalive)) != 0)
    {
        return false;
    }

#if defined(__linux__)
    if (::setsockopt(s, IPPROTO_TCP, TCP_KEEPIDLE, (const char *)&keepidle, sizeof(keepidle)) != 0)
    {
        return false;
    }
    if (::setsockopt(s, IPPROTO_TCP, TCP_KEEPINTVL, (const char *)&keepinterval, sizeof(keepinterval)) != 0)
    {
        return false;
    }
    if (::setsockopt(s, IPPROTO_TCP, TCP_KEEPCNT, (const char *)&keepcount, sizeof(keepcount)) != 0)
    {
        return false;
    }
#elif defined(__APPLE__)
    (void)keepinterval;
    (void)keepcount;
    if (::setsockopt(s, IPPROTO_TCP, TCP_KEEPALIVE, (const char *)&keepidle, sizeof(keepidle)) != 0)
    {
        return false;
    }
#else
    (void)keepidle;
    (void)keepinterval;
    (void)keepcount;
#endif

    return true;
}

void close_socket(socket_t s)
{
    if (LU_SOCKET_VALID(s))
    {
#if defined(_WIN32)
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
    int maxvalidnum = (int)lts->ltlsnum - (int)lts->ltlsfreeindex.size() - 1;
    int validnum = 0;
    for (int i = 1; i < (int)lts->ltlsnum && validnum < maxvalidnum; i++)
    {
        lutcplink & ltl = lts->ltls[i];
        if (LU_SOCKET_VALID(ltl.s))
        {
            for (int j = 0; j < (int)l->cfg.maxrecvpacket_perframe; j++)
            {
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

    msgheader head;
    head.magic = 0;
    head.size = 0;
    while (1)
    {
        cb->store();
        if (!cb->read((char *)&head, sizeof(head)))
        {
            return -1;
        }

        if (head.magic == HEAD_MAGIC)
        {
            break;
        }

        cb->skip_read(1);
        LUERR("unpack_packet from %d wrong head %u size(%d)", connid, head.magic, (int)cb->size());
    }

    if (head.size >= l->cfg.maxpacketlen)
    {
        LUERR("unpack_packet from %d max size %d size(%d)", connid, (int)head.size, (int)l->cfg.maxpacketlen);
        cb->restore();
        cb->skip_read(1);
        return -1;
    }

    if (!cb->read(l->recvpacketbuffer, head.size))
    {
        cb->restore();
        return -1;
    }

    if (l->cfg.ccrp)
    {
        l->cfg.ccrp(l, connid, l->recvpacketbuffer, head.size, userdata);
    }

    return 0;
}

void ticktcpclient(lutcpclient * ltc)
{
    if (!LU_SOCKET_VALID(ltc->ltl.s))
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
    if (LU_SOCKET_VALID(ltl.s))
    {
        for (int j = 0; j < (int)l->cfg.maxrecvpacket_perframe; j++)
        {
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
    s = LU_INVALID_SOCKET;
    sendbuff.ini(_l, _l->cfg.sendbuff);
    recvbuff.ini(_l, _l->cfg.recvbuff);
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

void lutcplink::fini()
{
    close_socket(s);
    s = LU_INVALID_SOCKET;
    sendbuff.fini();
    recvbuff.fini();
}

void lutcplink::clear()
{
    close_socket(s);
    s = LU_INVALID_SOCKET;
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
#if defined(LU_USE_SELECT)
    ltlmap = (lutcplinkmap *)safelumalloc(l, sizeof(lutcplinkmap));
    new (ltlmap) lutcplinkmap();
#else
    epollfd = ::epoll_create(static_cast<int>(len));
    events = safelumalloc(l, sizeof(epoll_event) * len);
#endif
}

void luselector::fini()
{
#if defined(LU_USE_SELECT)
    ltlmap->~lutcplinkmap();
    safelufree(l, ltlmap);
    ltlmap = 0;
#else
    ::close(epollfd);
    safelufree(l, events);
    epollfd = -1;
    events = 0;
#endif
}

bool luselector::add(lutcplink * ltl)
{
#if defined(LU_USE_SELECT)
    (*ltlmap)[ltl->s] = ltl;
    return true;
#else
    epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN | EPOLLOUT | EPOLLERR;
    ev.data.ptr = ltl;
    return ::epoll_ctl(epollfd, EPOLL_CTL_ADD, ltl->s, &ev) == 0;
#endif
}

bool luselector::del(lutcplink * ltl)
{
#if defined(LU_USE_SELECT)
    (*ltlmap).erase(ltl->s);
    return true;
#else
    return ::epoll_ctl(epollfd, EPOLL_CTL_DEL, ltl->s, 0) != -1;
#endif
}

bool luselector::select()
{
#if defined(LU_USE_SELECT)
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    socket_t maxfd = 0;
    for (lutcplinkmap::iterator it = (*ltlmap).begin(); it != (*ltlmap).end(); ++it)
    {
        socket_t s = it->first;
        FD_SET(s, &readfds);
        FD_SET(s, &writefds);
        FD_SET(s, &exceptfds);
        if (s > maxfd)
        {
            maxfd = s;
        }
    }

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = waittime > 0 ? waittime * 1000 : 0;

    int ret = ::select(static_cast<int>(maxfd + 1), &readfds, &writefds, &exceptfds, &timeout);
    if (ret < 0)
    {
        int err = GET_NET_ERROR;
        if (err == NET_INTR_ERROR)
        {
            return true;
        }
        return false;
    }

    for (lutcplinkmap::iterator it = (*ltlmap).begin(); it != (*ltlmap).end();)
    {
        lutcplinkmap::iterator itbak = it;
        ++it;
        socket_t s = itbak->first;
        lutcplink * ltl = itbak->second;
        if (FD_ISSET(s, &exceptfds))
        {
            cbe(ltl);
            del(ltl);
            cbc(ltl, -1);
            continue;
        }
        if (FD_ISSET(s, &readfds))
        {
            int reason = 0;
            if (cbi(ltl, reason) != 0)
            {
                del(ltl);
                cbc(ltl, reason);
                continue;
            }
        }
        if (FD_ISSET(s, &writefds))
        {
            int reason = 0;
            if (cbo(ltl, reason) != 0)
            {
                del(ltl);
                cbc(ltl, reason);
                continue;
            }
        }
    }
    return true;
#else
    epoll_event * evs = (epoll_event *)events;
    int ret = ::epoll_wait(epollfd, evs, static_cast<int>(len), waittime);
    if (ret < 0)
    {
        if (errno == EINTR)
        {
            return true;
        }
        return false;
    }

    for (int i = 0; i < ret; i++)
    {
        epoll_event & ev = evs[i];
        lutcplink * ltl = (lutcplink *)ev.data.ptr;
        uint32_t e = ev.events;
#ifdef EPOLLRDHUP
        if (EPOLLRDHUP & e)
        {
            cbe(ltl);
            del(ltl);
            cbc(ltl, -1);
            continue;
        }
#endif
        if ((EPOLLERR | EPOLLHUP) & e)
        {
            e |= EPOLLIN | EPOLLOUT;
        }
        if (EPOLLIN & e)
        {
            int reason = 0;
            if (cbi(ltl, reason) != 0)
            {
                del(ltl);
                cbc(ltl, reason);
                continue;
            }
        }
        if (EPOLLOUT & e)
        {
            int reason = 0;
            if (cbo(ltl, reason) != 0)
            {
                del(ltl);
                cbc(ltl, reason);
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

int on_tcpserver_in(lutcplink * ltl, int & reason)
{
    lu * l = ltl->l;
    lutcpserver * lts = l->ts;

    if (lts->s == ltl->s)
    {
        on_tcpserver_accept(ltl);
        return 0;
    }

    if (ltl->recvbuff.full())
    {
        return 0;
    }

    int len = ::recv(ltl->s, ltl->recvbuff.get_write_line_buffer(),
                     (int)ltl->recvbuff.get_write_line_size(), 0);
    if (len == 0)
    {
        return -1;
    }
    if (len < 0)
    {
        reason = GET_NET_ERROR;
        if (reason != NET_BLOCK_ERROR &&
            reason != NET_BLOCK_ERROR_EX &&
            reason != NET_INTR_ERROR)
        {
            return -1;
        }
        return 0;
    }

    ltl->recvbuff.skip_write(len);
    return 0;
}

int on_tcpserver_accept(lutcplink * ltl)
{
    sockaddr_in _sockaddr;
    memset(&_sockaddr, 0, sizeof(_sockaddr));
    socklen_t size = sizeof(_sockaddr);
    socket_t s = ::accept(ltl->s, (sockaddr *)&_sockaddr, &size);
    if (!LU_SOCKET_VALID(s))
    {
        return -1;
    }

    lu * l = ltl->l;
    lutcpserver * lts = l->ts;
    lutcplink * newltl = lts->alloc_tcplink();
    if (!newltl)
    {
        close_socket(s);
        return -1;
    }

    newltl->s = s;
    strcpy(newltl->peerip, inet_ntoa(_sockaddr.sin_addr));
    newltl->peerport = ntohs(_sockaddr.sin_port);

    ::setsockopt(s, SOL_SOCKET, SO_RCVBUF, (const char *)&l->cfg.socket_recvbuff, sizeof(int));
    ::setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char *)&l->cfg.socket_sendbuff, sizeof(int));
    set_socket_nonblocking(s, l->cfg.isnonblocking);
    set_socket_nodelay(s, l->cfg.isnodelay);
    set_socket_keepalive(s, l->cfg.iskeepalive, l->cfg.keepidle, l->cfg.keepinterval, l->cfg.keepcount);
    set_socket_linger(s, l->cfg.linger);

    sockaddr_in _local_sockaddr;
    memset(&_local_sockaddr, 0, sizeof(_local_sockaddr));
    size = sizeof(_local_sockaddr);
    if (::getsockname(s, (sockaddr *)&_local_sockaddr, &size) == 0)
    {
        strcpy(newltl->ip, inet_ntoa(_local_sockaddr.sin_addr));
        newltl->port = ntohs(_local_sockaddr.sin_port);
    }

    lts->ls.add(newltl);

    if (l->cfg.cco)
    {
        l->cfg.cco(l, newltl->index, newltl->userdata);
    }

    return 0;
}

int on_tcpserver_out(lutcplink * ltl, int & reason)
{
    if (ltl->sendbuff.empty())
    {
        return 0;
    }

    int len = ::send(ltl->s,
                     ltl->sendbuff.get_read_line_buffer(),
                     (int)ltl->sendbuff.get_read_line_size(), 0);
    if (len == 0)
    {
        return -1;
    }
    if (len < 0)
    {
        reason = GET_NET_ERROR;
        if (reason != NET_BLOCK_ERROR &&
            reason != NET_BLOCK_ERROR_EX &&
            reason != NET_INTR_ERROR)
        {
            return -1;
        }
        return 0;
    }

    ltl->sendbuff.skip_read(len);
    return 0;
}

int on_tcpserver_close(lutcplink * ltl, int reason)
{
    lu * l = ltl->l;
    lutcpserver * lts = l->ts;

    if (l->cfg.ccc)
    {
        l->cfg.ccc(l, ltl->index, ltl->userdata, reason);
    }

    lts->dealloc_tcplink(ltl);
    return 0;
}

int pack_packet(lu * l, circle_buffer * cb, int connid, const char * buffer, size_t size)
{
    (void)connid;
    if (size >= l->cfg.maxpacketlen)
    {
        return luet_msgtoobig;
    }

    if (!cb->can_write(sizeof(msgheader) + size))
    {
        return luet_sendbufffull;
    }

    cb->store();

    msgheader head;
    head.magic = HEAD_MAGIC;
    head.size = (uint32_t)size;

    if (!cb->write((const char *)&head, sizeof(head)))
    {
        cb->restore();
        return luet_sendbufffull;
    }

    if (!cb->write(buffer, size))
    {
        cb->restore();
        return luet_sendbufffull;
    }

    return luet_ok;
}

int sendtcpserver(lutcpserver * lts, const char * buffer, size_t size, int connid)
{
    if (connid <= 0 || connid >= (int)lts->ltlsnum)
    {
        return luet_conninvalid;
    }

    lutcplink & ltl = lts->ltls[connid];
    if (!LU_SOCKET_VALID(ltl.s))
    {
        return luet_conninvalid;
    }

    return pack_packet(lts->l, &ltl.sendbuff, ltl.index, buffer, size);
}

int sendtcpclient(lutcpclient * ltc, const char * buffer, size_t size, int connid)
{
    (void)connid;
    lutcplink & ltl = ltc->ltl;
    if (!LU_SOCKET_VALID(ltl.s))
    {
        return luet_conninvalid;
    }

    return pack_packet(ltc->ltl.l, &ltl.sendbuff, ltl.index, buffer, size);
}

int on_tcpclient_err(lutcplink * ltl)
{
    LULOG("on_tcpclient_err %d from %s:%u", ltl->s, ltl->peerip, ltl->peerport);
    return -1;
}

int on_tcpclient_in(lutcplink * ltl, int & reason)
{
    if (ltl->recvbuff.full())
    {
        return 0;
    }

    int len = ::recv(ltl->s, ltl->recvbuff.get_write_line_buffer(),
                     (int)ltl->recvbuff.get_write_line_size(), 0);
    if (len == 0)
    {
        return -1;
    }
    if (len < 0)
    {
        reason = GET_NET_ERROR;
        if (reason != NET_BLOCK_ERROR &&
            reason != NET_BLOCK_ERROR_EX &&
            reason != NET_INTR_ERROR)
        {
            return -1;
        }
        return 0;
    }

    ltl->recvbuff.skip_write(len);
    return 0;
}

int on_tcpclient_out(lutcplink * ltl, int & reason)
{
    if (ltl->sendbuff.empty())
    {
        return 0;
    }

    int len = ::send(ltl->s,
                     ltl->sendbuff.get_read_line_buffer(),
                     (int)ltl->sendbuff.get_read_line_size(), 0);
    if (len == 0)
    {
        return -1;
    }
    if (len < 0)
    {
        reason = GET_NET_ERROR;
        if (reason != NET_BLOCK_ERROR &&
            reason != NET_BLOCK_ERROR_EX &&
            reason != NET_INTR_ERROR)
        {
            return -1;
        }
        return 0;
    }

    ltl->sendbuff.skip_read(len);
    return 0;
}

int on_tcpclient_close(lutcplink * ltl, int reason)
{
    lu * l = ltl->l;
    lutcpclient * ltc = l->tc;

    if (l->cfg.ccc)
    {
        l->cfg.ccc(l, ltl->index, ltl->userdata, reason);
    }

    ltc->reconnect();
    return 0;
}
