#ifndef __UTIL_H_INCLUDE_
#define __UTIL_H_INCLUDE_

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

double stamp_sub(const struct timeval* from, const struct timeval* sub);
void show_speed(const struct timeval* begin, const struct timeval* end, long long n);
double compute_speed(long long n, double cost);

#ifdef __cplusplus
}
#endif

#endif
