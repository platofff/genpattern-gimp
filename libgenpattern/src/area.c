#include "area.h"

#ifdef __AVX__
#include <immintrin.h>
#endif

#include <math.h>
#include <stdalign.h>

float polygon_area(Polygon *polygon) {
  float p = 0.0;
  int64_t i = 0;
#ifdef __AVX__
  __m256 sum8 = _mm256_set1_ps(0.0);
  for (; i < polygon->size - 9; i += 8) {
    __m256 x_prev = _mm256_load_ps(polygon->x_ptr + i),
           x_next = _mm256_loadu_ps(polygon->x_ptr + i + 2),
           y = _mm256_loadu_ps(polygon->y_ptr + i + 1);
#ifdef __FMA__
    sum8 = _mm256_fmadd_ps(_mm256_sub_ps(x_prev, x_next), y, sum8);
#else
    sum8 = _mm256_add_ps(sum8, _mm256_mul_ps(_mm256_sub_ps(x_prev, x_next), y));
#endif
  }
#endif
  p += polygon->y_ptr[0] *
       (polygon->x_ptr[polygon->size - 1] - polygon->x_ptr[1]);
  p += polygon->y_ptr[polygon->size - 1] *
       (polygon->x_ptr[polygon->size - 2] - polygon->x_ptr[0]);
  for (; i < polygon->size - 2; i++) {
    p += polygon->y_ptr[i + 1] * (polygon->x_ptr[i] - polygon->x_ptr[i + 2]);
  }

#ifdef __AVX__

  __m128 sum4 =
      _mm_add_ps(_mm256_extractf128_ps(sum8, 1), _mm256_castps256_ps128(sum8));

  alignas(16) float ps[4];
  _mm_store_ps(ps, sum4);

  return fabsf(p + ps[0] + ps[1] + ps[2] + ps[3]) * 0.5;

#else
  return fabsf(p) * 0.5;
#endif
}