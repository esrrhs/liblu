#include "lu.h"
#include "lusocket.h"
#include <string.h>
#ifdef WIN32
#include <windows.h>
#endif

LU_API lu * newlu(luconfig * cfg)
{
	luconfig _cfg;
	if (cfg)
	{
		_cfg = *cfg;
	}
	lu * pl = (lu *)_cfg.lum(sizeof(lu));
	memset(pl, 0, sizeof(lu));
	pl->lum = _cfg.lum;
	pl->luf = _cfg.luf;
	pl->cfg = _cfg;
	pl->type = _cfg.type;
	pl->lud = _cfg.userdata;
	if (pl->type == lut_tcpserver)
	{
		LULOG("new tcp server");
		pl->ts = newtcpserver(pl);
        if (!pl->ts)
        {
            safelufree(pl, pl);
            return 0;
        }
	}
	else if (pl->type == lut_tcpclient)
	{
		LULOG("new tcp client");
		pl->tc = newtcpclient(pl);
        if (!pl->tc)
        {
            safelufree(pl, pl);
            return 0;
        }
	}
	else
	{
		LUERR("error type %d", pl->type);
        return 0;
	}

    return pl;
}

LU_API void dellu(lu * l)
{
	if (l->type == lut_tcpserver)
	{
		LULOG("del tcp server");
		deltcpserver(l->ts);
	}
	else if (l->type == lut_tcpclient)
	{
		LULOG("del tcp client");
		deltcpclient(l->tc);
	}
	else
	{
		LUERR("error type %d", l->type);
	}
	l->cfg.luf(l);
}

LU_API void inilu()
{
	// socket
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int32_t err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		LUERR("WSAStartup error\n");
		return;
	}
#endif
}

LU_API void ticklu(lu * l)
{
	if (l->type == lut_tcpserver)
	{
		ticktcpserver(l->ts);
	}
	else if (l->type == lut_tcpclient)
	{
		ticktcpclient(l->tc);
	}
	else
	{
		LUERR("error type %d", l->type);
	}
}

LU_API lutype gettypelu(lu * l)
{
    return l->type;
}

LU_API int sendlu(lu * l, const char * buffer, size_t size, int connid)
{
	if (l->type == lut_tcpserver)
	{
		return sendtcpserver(l->ts, buffer, size, connid);
	}
	else if (l->type == lut_tcpclient)
	{
		return sendtcpclient(l->tc, buffer, size, connid);
	}
	else
	{
		LUERR("error type %d", l->type);
		return luet_typeerr;
	}
}

LU_API luuserdata * getlu_userdata(lu * l)
{
	return &l->lud;
}
