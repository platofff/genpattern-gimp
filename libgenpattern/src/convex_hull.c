#include "convex_hull.h"
#include "doubly_linked_list.h"

#ifdef __AVX2__
#include <immintrin.h>

#define LOAD_EPI8_COLUMN(ptr, w)                                               \
  _mm256_set_epi8(ptr[0], ptr[w], ptr[2 * w], ptr[3 * w], ptr[4 * w],          \
                  ptr[5 * w], ptr[6 * w], ptr[7 * w], ptr[8 * w], ptr[9 * w],  \
                  ptr[10 * w], ptr[11 * w], ptr[12 * w], ptr[13 * w],          \
                  ptr[14 * w], ptr[15 * w], ptr[16 * w], ptr[17 * w],          \
                  ptr[18 * w], ptr[19 * w], ptr[20 * w], ptr[21 * w],          \
                  ptr[22 * w], ptr[23 * w], ptr[24 * w], ptr[25 * w],          \
                  ptr[26 * w], ptr[27 * w], ptr[28 * w], ptr[29 * w],          \
                  ptr[30 * w], ptr[31 * w])
#endif

void convex_hull(DLElement **seq) {
  while (1) {
    DLElement *next = (*seq)->next, *nextnext = next->next;
    Point *e = &(*seq)->value, *e1 = &next->value, *e2 = &nextnext->value;
    float p =
        (e1->y - e->y) * (e2->x - e1->x) - (e1->x - e->x) * (e2->y - e1->y);
    if (p > 0) {
      if (((DLElement *)nextnext->next)->next == NULL) {
        break;
      }
      *seq = next;
    } else {
      dllist_pop(next);
      DLElement *prev = (*seq)->prev;
      if (prev->prev != NULL) {
        *seq = prev;
      } else {
        *seq = next;
      }
    }
  }
}

void image_convex_hull(Polygon **polygon, ImgAlpha *alpha, uint8_t t) {
  t--;
#ifdef __AVX2__
  __m256i cmp_vec = _mm256_set1_epi8(t);
#endif
  size_t buf_size =
      2 * (alpha->width + alpha->height) + 1; // perimeter of the image
  DLElement *seq = dllist_alloc(buf_size);
  DLElement *seq_start = seq;

  // a
  int64_t min = 0;
  for (int64_t i = 0; i < alpha->width; i++) {
    int64_t j;
#ifdef __AVX2__
    j = alpha->height - 32;
    for (; j > min + 32; j -= 32) {
      __m256i vec = _mm256_load_si256(
          (const __m256i *)&alpha->data[i * alpha->width + j]);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
        j += 31 - __builtin_clz(cmp);
        seq = dllist_push(seq, (Point){i, j});
        min = j;
#ifdef __AVX2__
        goto continue_a;
#endif
      }
    }
#else
    j = alpha->height - 1;
#endif
    for (; j > min; j--) {
      if (alpha->data[i * alpha->width + j] > t) {
        seq = dllist_push(seq, (Point){i, j});
        min = j;
        break;
      }
    }
  continue_a:
    continue;
  }

  // b
  min = 0;
  for (int64_t j = alpha->height - 1; j >= 0; j--) {
    int64_t i;
#ifdef __AVX2__
    i = alpha->height - 32;
    for (; i > min + 32; i -= 32) {
      uint8_t *ptr = alpha->data + i * alpha->width + j;
      __m256i vec = LOAD_EPI8_COLUMN(ptr, alpha->width);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
        i += 31 - __builtin_clz(cmp);
        seq = dllist_push(seq, (Point){i, j});
        min = i;
#ifdef __AVX2__
        goto continue_b;
#endif
      }
    }
#else
    i = alpha->width - 1;
#endif
    for (; i > min; i--) {
      if (alpha->data[i * alpha->width + j] > t) {
        seq = dllist_push(seq, (Point){i, j});
        min = i;
        break;
      }
    }
  continue_b:
    continue;
  }

  // c
  int64_t max = alpha->height - 1;
  for (int64_t i = alpha->width - 1; i >= 0; i--) {
    int64_t j = 0;
#ifdef __AVX2__
    for (; j < max - 32; j += 32) {
      __m256i vec = _mm256_load_si256(
          (const __m256i *)&alpha->data[i * alpha->width + j]);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
        j += 33 - __builtin_ctz(cmp);
        seq = dllist_push(seq, (Point){i, j});
        max = j;
#ifdef __AVX2__
        goto continue_c;
#endif
      }
    }
#endif
    for (; j < max; j++) {
      if (alpha->data[i * alpha->width + j] > t) {
        seq = dllist_push(seq, (Point){i, j});
        max = j;
        break;
      }
    }
  continue_c:
    continue;
  }

  // d
  max = alpha->width;
  for (int64_t j = 0; j < alpha->height; j++) {
    int64_t i = 0;
#ifdef __AVX2__
    for (; i < max - 32; i += 32) {
      uint8_t *ptr = alpha->data + i * alpha->width + j;
      __m256i vec = LOAD_EPI8_COLUMN(ptr, alpha->width);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
        i += 33 - __builtin_ctz(cmp);
        seq = dllist_push(seq, (Point){i, j});
        max = i;
#ifdef __AVX2__
        goto continue_d;
#endif
      }
    }
#endif
    for (; i < max; i++) {
      if (alpha->data[i * alpha->width + j] > t) {
        seq = dllist_push(seq, (Point){i, j});
        max = i;
        break;
      }
    }
  continue_d:
    continue;
  }

  DLElement *first_element = seq_start->next;
  dllist_push(seq, first_element->value);
  to_start(&seq);

  convex_hull(&seq);

  *polygon = dllist_dump(seq);

  dllist_free(seq);
}