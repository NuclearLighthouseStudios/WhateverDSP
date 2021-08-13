#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DEBUG

#define debug(...)\
printf("%s:%d %s(): ", __FILE__, __LINE__, __func__);\
printf(__VA_ARGS__)

#define error(...)\
fprintf(stderr, "ERROR: %s:%d %s(): ", __FILE__, __LINE__, __func__);\
fprintf(stderr, __VA_ARGS__)

#define panic(...)\
fprintf(stderr, "PANIC: %s:%d %s(): ", __FILE__, __LINE__, __func__);\
fprintf(stderr, __VA_ARGS__);\
abort()

#else
#define debug(...) (void)(0)
#define error(...) (void)(0)
#define panic(...) abort()
#endif

#endif