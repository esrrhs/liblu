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
	lutcpserver * ts = (lutcpserver *)l->lum(sizeof(lutcpserver));
	memset(ts, 0, sizeof(lutcpserver));
	ts->l = l;
	ts->s = -1;

	LULOG("start tcp server %s %d", l->cfg.ip, l->cfg.port);

	// create
	ts->s = ::socket(AF_INET, SOCK_STREAM, 0);
	if (ts->s == -1)
	{
		LUERR("create socket error");
		return ts;
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
		return ts;
	}

	// listen
	ret = ::listen(ts->s, l->cfg.backlog);
	if (ret != 0)
	{
		LUERR("listen socket error");
		return ts;
	}

	// ·Ç×èÈû
	if (!set_socket_nonblocking(ts->s, l->cfg.isnonblocking))
	{
		LUERR("set nonblocking socket error");
		return ts;
	}

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

