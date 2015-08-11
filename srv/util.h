#ifndef SRV_UTIL_H_
#define SRV_UTIL_H_

#include <cstdio>
#include <cstdarg>
#include <mingw.thread.h>

inline void log(const char *format, ...) {
    va_list v;
    va_start(v, format);
    vprintf(format, v);
    fflush(stdout);
    va_end(v);
}

#endif /* SRV_UTIL_H_ */
