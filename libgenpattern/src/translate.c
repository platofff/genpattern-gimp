#include "translate.h"

#ifdef __AVX__
#include <immintrin.h>
#endif

void polygon_translate(Polygon *polygon, float x_off_val, float y_off_val) {
  size_t i = 0;
#ifdef __AVX__
  __m256 x_off = _mm256_set1_ps(x_off_val), y_off = _mm256_set1_ps(y_off_val);
  for (; i < polygon->size - 8; i += 8) {
    __m256 x = _mm256_load_ps(polygon->x_ptr + i),
           y = _mm256_load_ps(polygon->y_ptr + i);
    x = _mm256_add_ps(x, x_off);
    _mm256_store_ps(polygon->x_ptr + i, x);
    y = _mm256_add_ps(y, y_off);
    _mm256_store_ps(polygon->y_ptr + i, y);
  }
#endif
  for (; i < polygon->size; i++) {
    polygon->x_ptr[i] += x_off_val;
    polygon->y_ptr[i] += y_off_val;
  }
}
