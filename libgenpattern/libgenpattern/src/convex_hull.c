#include "convex_hull.h"
#include "doubly_linked_list.h"
#include "misc.h"

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

static inline void _gp_convex_hull(GPDList** seq) {
  GPDLElement* el = (*seq)->start;
  while (1) {
    GPDLElement *next = el->next, *nextnext = next->next;
    GPPoint *e = &el->value, *e1 = &next->value, *e2 = &nextnext->value;
    gp_float p = (e1->y - e->y) * (e2->x - e1->x) - (e1->x - e->x) * (e2->y - e1->y);
    if (p > 0) {
      if ((nextnext->next)->next == NULL) {
        break;
      }
      el = next;
    } else {
      gp_dllist_pop(*seq, next);
      GPDLElement* prev = el->prev;
      if (prev != NULL) {
        el = prev;
      } else {
        el = next;
      }
    }
  }
}

int gp_image_convex_hull(GPPolygon* polygon, GPImgAlpha* alpha, uint8_t _t) {
  _t--;
#ifdef __AVX2__
  int8_t t = *(int8_t*)(&_t);
  t ^= 0b10000000;
  __m256i cmp_vec = _mm256_set1_epi8(t);
  __m256i signed_convert_mask = _mm256_set1_epi8(0b10000000);
#endif
  size_t buf_size = 2 * (alpha->width + alpha->height) + 1;  // perimeter of the image
  GPDList* seq;
  int res = gp_dllist_alloc(&seq);
  if (res != 0) {
    return -1;
  }

  // a
  int32_t min = 0;
  for (int32_t i = 0; i < alpha->width; i++) {
    int32_t j;
#ifdef __AVX2__
    j = alpha->height - 32;
    for (; j > min; j -= 32) {
      // load 32 uchars from the row into an AVX regsiter. Each uchar represents alpha value from 0 to 255.
      __m256i vec = _mm256_load_si256((const __m256i*)&alpha->data[i * alpha->width + j]);
      // flip most significant bit to compare these uchars as signed chars (no instruction for comparing uchars
      // directly)
      vec = _mm256_xor_si256(vec, signed_convert_mask);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      // load bitmask containing comparison results (one bit for each alpha value compared with target-1)
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {  // if any alpha value exceeds target-1
#ifdef _MSC_VER
        unsigned long lz = 0;
        // get the index of the first positive comparison result (BitScanReverse yields first positive bit in comparison
        // bitmask read in little endian)
        _BitScanReverse(&lz, *(unsigned long*)&cmp);
        j += lz;  // store the index of the first alpha value exceeding target-1 in j
#else
        j += __builtin_clz(*(unsigned int*)&cmp) ^ 31;  // it compiles to bsr too
#endif
        res = gp_dllist_push(seq, (GPPoint){i, j});
        if (res != 0) {
          return 0;
        }
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
        res = gp_dllist_push(seq, (GPPoint){i, j});
        if (res != 0) {
          return 0;
        }
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
        res = gp_dllist_push(seq, (GPPoint){i, j});
        if (res != 0) {
          return 0;
        }
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
        res = gp_dllist_push(seq, (GPPoint){i, j});
        if (res != 0) {
          return 0;
        }
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
        res = gp_dllist_push(seq, (GPPoint){i, j});
        if (res != 0) {
          return 0;
        }
        max = j;
        goto continue_c;
      }
    }
#endif
    for (; j < max; j++) {
      if (alpha->data[i * alpha->width + j] > _t) {
        res = gp_dllist_push(seq, (GPPoint){i, j});
        if (res != 0) {
          return 0;
        }
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
        res = gp_dllist_push(seq, (GPPoint){i, j});
        if (res != 0) {
          return 0;
        }
        max = i;
        goto continue_d;
      }
    }
#endif
    for (; i < max; i++) {
      if (alpha->data[i * alpha->width + j] > _t) {
        res = gp_dllist_push(seq, (GPPoint){i, j});
        if (res != 0) {
          return 0;
        }
        max = i;
        break;
      }
    }
  continue_d:
    continue;
  }

  res = gp_dllist_push(seq, seq->start->value);
  if (res != 0) {
    return res;
  }

  _gp_convex_hull(&seq);

  gp_dllist_to_polygon(seq, polygon);
  gp_dllist_free(seq);
  return 0;
}