#pragma once

#include <string>
#include <list>
#include <vector>
#include <map>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <typeinfo>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <assert.h>

#ifdef _DEBUG
#define LULOG(...) lulog("[DEBUG] ", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define LUERR(...) lulog("[ERROR] ", __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#else
#define LULOG(...)
#define LUERR(...)
#endif

void lulog(const char * header, const char * file, const char * func, int pos, const char *fmt, ...);

typedef	int socket_t;
#if defined(WIN32)
typedef int socklen_t;
#else
#endif

#define LUMIN(a, b) ((a) < (b) ? (a) : (b))

struct lu;
void * safelumalloc(lu * l, size_t len);
void safelufree(lu * l, void * p);

