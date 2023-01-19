#ifndef _GP_MISC_H
#define _GP_MISC_H 1

#include <stdint.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <malloc.h>
#define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define M_PIF 3.14159265358979323846f   /* pi */
#define M_PI_2F 1.57079632679489661923f /* pi/2 */
#define M_2PIF (M_PIF * 2.f)

void gp_array_shuffle(void* _arr, int32_t sz, int32_t len);

#define GP_CHECK_ALLOC(ptr)                        \
  if (ptr == NULL) {                            \
    fputs("Memory allocation error\n", stderr); \
    exit(-1);                                   \
  }

#endif
