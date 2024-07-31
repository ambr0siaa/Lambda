#include "types.h"

#include <stdio.h>
#include <stdarg.h>

void report(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "REPORT. ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}
