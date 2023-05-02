#ifndef _GP_MISC_H
#define _GP_MISC_H 1

#include <malloc.h>
#include <math.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>

#include "basic_geometry.h"
#include "errors.h"
#include "exports.h"

#ifdef _MSC_VER
#define gp_aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
#define gp_aligned_free(ptr) _aligned_free(ptr)
#else
#define gp_aligned_alloc(alignment, size) aligned_alloc(alignment, size)
#define gp_aligned_free(ptr) free(ptr)
#endif
// clang-format off
#define GP_MIN(a, b)               \
  _Generic((a) + (b),              \
          float : fminf((a), (b)), \
         double : fmin((a), (b)),  \
    long double : fminl((a), (b)), \
        default : ((a) < (b)) ? (a) : (b))


#define GP_MAX(a, b)               \
  _Generic((a) + (b),              \
          float : fmaxf((a), (b)), \
         double : fmax((a), (b)),  \
    long double : fmaxl((a), (b)), \
        default : ((a) > (b)) ? (a) : (b))
// clang-format on

#define GP_PI (gp_float)3.141592653589793238462643383279502884L    // pi
#define GP_PI_2 (gp_float)1.570796326794896619231321691639751442L  // pi/2
#define GP_2PI (GP_PI * 2)                                         // 2pi

void gp_array_shuffle(void* _arr, void* tmp, size_t sz, int32_t len);
int gp_grid_init(gp_float x, gp_float y, gp_float resolution, GPVector** mesh, size_t* len);

#define GP_CHECK_ALLOC(ptr) \
  if (ptr == NULL) {        \
    return GP_ERR_ALLOC;    \
  }

#define GP_EPSILON (gp_float)0.001

#define gp_atan2(Y, X) \
  _Generic((Y) + (X), float : atan2f, double : atan2, long double : atan2l)((Y), (X))

#define gp_fabs(X) _Generic((X), float : fabsf, double : fabs, long double : fabsl)((X))

#define gp_round(X) _Generic((X), float : roundf, double : round, long double : roundl)((X))

#define gp_fmod(X, Y) \
  _Generic((X) + (Y), float : fmodf, double : fmod, long double : fmodl)((X), (Y))

#endif
