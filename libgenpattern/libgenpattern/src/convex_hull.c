#include "convex_hull.h"
#include "doubly_linked_list.h"

#ifdef __AVX2__
#include <immintrin.h>

#define LOAD_EPI8_COLUMN(ptr, w)                                                                                      \
  _mm256_set_epi8(ptr[0], ptr[w], ptr[2 * w], ptr[3 * w], ptr[4 * w], ptr[5 * w], ptr[6 * w], ptr[7 * w], ptr[8 * w], \
                  ptr[9 * w], ptr[10 * w], ptr[11 * w], ptr[12 * w], ptr[13 * w], ptr[14 * w], ptr[15 * w],           \
                  ptr[16 * w], ptr[17 * w], ptr[18 * w], ptr[19 * w], ptr[20 * w], ptr[21 * w], ptr[22 * w],          \
                  ptr[23 * w], ptr[24 * w], ptr[25 * w], ptr[26 * w], ptr[27 * w], ptr[28 * w], ptr[29 * w],          \
                  ptr[30 * w], ptr[31 * w])
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif
#endif

static inline void _gp_convex_hull(GPDLElement** seq) {
  while (1) {
    GPDLElement *next = (*seq)->next, *nextnext = next->next;
    GPPoint *e = &(*seq)->value, *e1 = &next->value, *e2 = &nextnext->value;
    float p = (e1->y - e->y) * (e2->x - e1->x) - (e1->x - e->x) * (e2->y - e1->y);
    if (p > 0) {
      if (((GPDLElement*)nextnext->next)->next == NULL) {
        break;
      }
      *seq = next;
    } else {
      gp_dllist_pop(next);
      GPDLElement* prev = (*seq)->prev;
      if (prev->prev != NULL) {
        *seq = prev;
      } else {
        *seq = next;
      }
    }
  }
}

void gp_image_convex_hull(GPPolygon* polygon, GPImgAlpha* alpha, uint8_t _t) {
  _t--;
#ifdef __AVX2__
  int8_t t = *(int8_t*)(&_t);
  t ^= 0b10000000;
  __m256i cmp_vec = _mm256_set1_epi8(t);
  __m256i signed_convert_mask = _mm256_set1_epi8(0b10000000);
#endif
  size_t buf_size = 2 * (alpha->width + alpha->height) + 1;  // perimeter of the image
  GPDLElement* seq = gp_dllist_alloc(buf_size);
  GPDLElement* seq_start = seq;

  // a
  int32_t min = 0;
  for (int32_t i = 0; i < alpha->width; i++) {
    int32_t j;
#ifdef __AVX2__
    j = alpha->height - 32;
    for (; j > min; j -= 32) {
      __m256i vec = _mm256_load_si256((const __m256i*)&alpha->data[i * alpha->width + j]);
      vec = _mm256_xor_si256(vec, signed_convert_mask);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
#ifdef _MSC_VER
        unsigned long lz = 0;
        _BitScanReverse(&lz, *(unsigned long*)&cmp);
        j += lz;
#else
        j += __builtin_clz(*(unsigned int*)&cmp) ^ 31;
#endif
        seq = gp_dllist_push(seq, (GPPoint){i, j});
        min = j;
        goto continue_a;
      }
    }
    j += 31;
#else
    j = alpha->height - 1;
#endif
    for (; j > min; j--) {
      if (alpha->data[i * alpha->width + j] > _t) {
        seq = gp_dllist_push(seq, (GPPoint){i, j});
        min = j;
        break;
      }
    }
  continue_a:
    continue;
  }

  // b
  min = 0;
  for (int32_t j = alpha->height - 1; j >= 0; j--) {
    int32_t i;
#ifdef __AVX2__
    i = alpha->height - 33;
    for (; i > min; i -= 32) {
      uint8_t* ptr = alpha->data + i * alpha->width + j;
      __m256i vec = LOAD_EPI8_COLUMN(ptr, alpha->width);
      vec = _mm256_xor_si256(vec, signed_convert_mask);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
#ifdef _MSC_VER
        unsigned long tz = 0;
        _BitScanForward(&tz, *(unsigned long*)&cmp);
        j += tz ^ 31;
#else
        i += __builtin_ctz(*(unsigned int*)&cmp) ^ 31;
#endif
        seq = gp_dllist_push(seq, (GPPoint){i, j});
        min = i;
        goto continue_b;
      }
    }
    i += 31;
#else
    i = alpha->width - 1;
#endif
    for (; i > min; i--) {
      if (alpha->data[i * alpha->width + j] > _t) {
        seq = gp_dllist_push(seq, (GPPoint){i, j});
        min = i;
        break;
      }
    }
  continue_b:
    continue;
  }

  // c
  int32_t max = alpha->height - 1;
  for (int32_t i = alpha->width - 1; i >= 0; i--) {
    int32_t j = 0;
#ifdef __AVX2__
    int32_t sm = max - 32;
    for (; j < sm; j += 32) {
      __m256i vec = _mm256_load_si256((const __m256i*)&alpha->data[i * alpha->width + j]);
      vec = _mm256_xor_si256(vec, signed_convert_mask);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
#ifdef _MSC_VER
        unsigned long tz = 0;
        _BitScanForward(&tz, *(unsigned long*)&cmp);
        j += tz;
#else
        j += __builtin_ctz(*(unsigned int*)&cmp);
#endif
        seq = gp_dllist_push(seq, (GPPoint){i, j});
        max = j;
        goto continue_c;
      }
    }
#endif
    for (; j < max; j++) {
      if (alpha->data[i * alpha->width + j] > _t) {
        seq = gp_dllist_push(seq, (GPPoint){i, j});
        max = j;
        break;
      }
    }
  continue_c:
    continue;
  }

  // d
  max = alpha->width;
  for (int32_t j = 0; j < alpha->height; j++) {
    int32_t i = 0;
#ifdef __AVX2__
    int32_t sm = max - 32;
    for (; i < sm; i += 32) {
      uint8_t* ptr = alpha->data + i * alpha->width + j;
      __m256i vec = LOAD_EPI8_COLUMN(ptr, alpha->width);
      vec = _mm256_xor_si256(vec, signed_convert_mask);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
#ifdef _MSC_VER
        unsigned long lz = 0;
        _BitScanReverse(&lz, *(unsigned long*)&cmp);
        j += lz ^ 31;
#else
        i += __builtin_clz(*(unsigned int*)&cmp);
#endif
        seq = gp_dllist_push(seq, (GPPoint){i, j});
        max = i;
        goto continue_d;
      }
    }
#endif
    for (; i < max; i++) {
      if (alpha->data[i * alpha->width + j] > _t) {
        seq = gp_dllist_push(seq, (GPPoint){i, j});
        max = i;
        break;
      }
    }
  continue_d:
    continue;
  }

  GPDLElement* first_element = seq_start->next;
  gp_dllist_push(seq, first_element->value);
  gp_dllist_to_start(&seq);

  _gp_convex_hull(&seq);

  gp_dllist_dump(seq, polygon);
  gp_dllist_free(seq);
}