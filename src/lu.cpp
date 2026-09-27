#include "lu.h"
#include "lusocket.h"

#include <cstring>

#if defined(_WIN32)
#include <winsock2.h>
#endif

LU_API lu *newlu(luconfig *cfg) {
    luconfig _cfg = cfg ? *cfg : luconfig();
    lu *pl = (lu *) _cfg.lum(sizeof(lu));
    memset(pl, 0, sizeof(*pl));
    pl->lum = _cfg.lum;
    pl->luf = _cfg.luf;
    pl->cfg = _cfg;
    pl->type = _cfg.type;
    pl->lud = _cfg.userdata;

    if (pl->type == lut_tcpserver) {
        LULOG("new tcp server");
        pl->ts = newtcpserver(pl);
        if (!pl->ts) {
            safelufree(pl, pl);
            return 0;
        }
    } else if (pl->type == lut_tcpclient) {
        LULOG("new tcp client");
        pl->tc = newtcpclient(pl);
        if (!pl->tc) {
            safelufree(pl, pl);
            return 0;
        }
    } else {
        LUERR("error type %d", pl->type);
        safelufree(pl, pl);
        return 0;
    }

    return pl;
}

LU_API void dellu(lu *l) {
    if (!l) {
        return;
    }

    if (l->type == lut_tcpserver) {
        LULOG("del tcp server");
        deltcpserver(l->ts);
    } else if (l->type == lut_tcpclient) {
        LULOG("del tcp client");
        deltcpclient(l->tc);
    } else {
        LUERR("error type %d", l->type);
    }
    l->cfg.luf(l);
}

LU_API void inilu() {
#if defined(_WIN32)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LUERR("WSAStartup error\n");
    }
#endif
}

LU_API void ticklu(lu *l) {
    if (l->type == lut_tcpserver) {
        ticktcpserver(l->ts);
    } else if (l->type == lut_tcpclient) {
        ticktcpclient(l->tc);
    } else {
        LUERR("error type %d", l->type);
    }
}

LU_API lutype gettypelu(lu *l) {
    return l->type;
}

LU_API int sendlu(lu *l, const char *buffer, size_t size, int connid) {
    if (l->type == lut_tcpserver) {
        return sendtcpserver(l->ts, buffer, size, connid);
    }
    if (l->type == lut_tcpclient) {
        return sendtcpclient(l->tc, buffer, size, connid);
    }
    LUERR("error type %d", l->type);
    return luet_typeerr;
}

LU_API luuserdata *getlu_userdata(lu *l) {
    return &l->lud;
}

LU_API luuserdata *getlu_conn_userdata(lu *l, int connid) {
    if (l->type == lut_tcpserver) {
        if (connid <= 0 || connid >= (int) l->ts->ltlsnum) {
            return 0;
        }
        return &l->ts->ltls[connid].userdata;
    }
    if (l->type == lut_tcpclient) {
        return &l->tc->ltl.userdata;
    }
    LUERR("error type %d", l->type);
    return 0;
}
