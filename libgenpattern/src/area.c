#include "area.h"

#ifdef __AVX__
#include <immintrin.h>
#endif

#include <math.h>

float polygon_area(Polygon *polygon) {
  float p = 0.0;
  size_t i = 0;
#ifdef __AVX__
  __m256 sum8 = _mm256_set1_ps(0.0);
  for (i = 0; i < polygon->size - 8; i += 8) {
    __m256 x = _mm256_loadu_ps(polygon->x_ptr + i);
    __m256 y = _mm256_loadu_ps(polygon->y_ptr + i + 1);
    __m256 xy = _mm256_mul_ps(x, y);
    sum8 = _mm256_add_ps(sum8, xy);
  }

#endif
  for (; i < polygon->size - 1; i++) {
    p += polygon->x_ptr[i] * polygon->y_ptr[i + 1];
  }

  p += polygon->x_ptr[polygon->size - 1] * polygon->y_ptr[0];

  i = 0;
#ifdef __AVX__
  for (; i < polygon->size - 8; i += 8) {
    __m256 x = _mm256_loadu_ps(polygon->x_ptr + i + 1);
    __m256 y = _mm256_loadu_ps(polygon->y_ptr + i);
    __m256 xy = _mm256_mul_ps(x, y);
    sum8 = _mm256_sub_ps(sum8, xy);
  }

#endif
  for (; i < polygon->size - 1; i++) {
    p -= polygon->y_ptr[i] * polygon->x_ptr[i + 1];
  }

  p -= polygon->y_ptr[polygon->size - 1] * polygon->x_ptr[0];

#ifdef __AVX__

  __m128 sum4 =
      _mm_add_ps(_mm256_extractf128_ps(sum8, 1), _mm256_castps256_ps128(sum8));

  float ps[4];
  _mm_storeu_ps(ps, sum4);

  return fabsf(p + ps[0] + ps[1] + ps[2] + ps[3]) * 0.5;

#else
  return fabsf(p) * 0.5;
#endif
}