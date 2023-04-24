#ifndef _GP_MISC_H
#define _GP_MISC_H 1

#include <malloc.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>

#include "basic_geometry.h"
#include "errors.h"

#ifdef _MSC_VER
#define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
#else
#define _aligned_free(ptr) free(ptr)
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define M_PIF 3.14159265358979323846f    // pi
#define M_PI_2F 1.57079632679489661923f  // pi/2
#define M_2PIF (M_PIF * 2.f)             // 2pi

void gp_array_shuffle(void* _arr, void* tmp, size_t sz, int32_t len);
int gp_grid_init(float x, float y, float resolution, GPVector** mesh, size_t* len);

#define GP_CHECK_ALLOC(ptr) \
  if (ptr == NULL) {        \
    return GP_ERR_ALLOC;    \
  }

#define EPSILON 0.001f

#endif
