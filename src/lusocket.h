#pragma once

#include "lutypes.h"
#include "lu.h"

struct lu;
struct lutcpserver
{
	lu * l;
	socket_t s;
};

struct lutcpclient
{
	lu * l;
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

bool set_socket_nonblocking(socket_t s, bool on);

void close_socket(socket_t s);
