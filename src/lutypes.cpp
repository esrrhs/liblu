#include "lutypes.h"

void lulog(const char * header, const char * file, const char * func, int pos, const char *fmt, ...)
{
	FILE *pLog = NULL;
	time_t clock1;
	struct tm * tptr;
	va_list ap;

	pLog = fopen("liblu.log", "a+");
	if (pLog == NULL)
	{
		return;
	}

	clock1 = time(0);
	tptr = localtime(&clock1);

	fprintf(pLog, "===========================[%d.%d.%d, %d.%d.%d]%s:%d,%s:===========================\n%s",
		tptr->tm_year + 1990, tptr->tm_mon + 1,
		tptr->tm_mday, tptr->tm_hour, tptr->tm_min,
		tptr->tm_sec, file, pos, func, header);

	va_start(ap, fmt);
	vfprintf(pLog, fmt, ap);
	fprintf(pLog, "\n\n");
	va_end(ap);

	fclose(pLog);
}
