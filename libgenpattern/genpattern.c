#include "genpattern.h"
#include "test_image1._h"

#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <sys/time.h>
#include <threads.h>
#include <unistd.h>

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

void extract_polygon(GEOSGeometry **lstring, ImgAlpha *alpha, int8_t t) {
  t--;
#ifdef __AVX2__
  __m256i cmp_vec = _mm256_set1_epi8(t);
#endif
  size_t buf_size = sizeof(double) * 2 * (alpha->width + alpha->height) +
                    1; // perimeter of the image
  double *x_list = malloc(buf_size), *y_list = malloc(buf_size);
  size_t seq_size = 0;

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
        x_list[seq_size] = i;
        y_list[seq_size++] = j;
        min = j;
        goto continue_a;
      }
    }
#else
    j = alpha->height - 1;
#endif
    for (; j > min; j--) {
      if (alpha->data[i * alpha->width + j] > t) {
        x_list[seq_size] = i;
        y_list[seq_size++] = j;
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
      int8_t *ptr = alpha->data + i * alpha->width + j;
      __m256i vec = LOAD_EPI8_COLUMN(ptr, alpha->width);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
        i += 31 - __builtin_clz(cmp);
        x_list[seq_size] = i;
        y_list[seq_size++] = j;
        min = i;
        goto continue_b;
      }
    }
#else
    i = alpha->width - 1;
#endif
    for (; i > min; i--) {
      if (alpha->data[i * alpha->width + j] > t) {
        x_list[seq_size] = i;
        y_list[seq_size++] = j;
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
        x_list[seq_size] = i;
        y_list[seq_size++] = j;
        max = j;
        goto continue_c;
      }
    }
#endif
    for (; j < max; j++) {
      if (alpha->data[i * alpha->width + j] > t) {
        x_list[seq_size] = i;
        y_list[seq_size++] = j;
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
      int8_t *ptr = alpha->data + i * alpha->width + j;
      __m256i vec = LOAD_EPI8_COLUMN(ptr, alpha->width);
      __m256i result = _mm256_cmpgt_epi8(vec, cmp_vec);
      int32_t cmp = _mm256_movemask_epi8(result);
      if (cmp != 0) {
        i += 33 - __builtin_ctz(cmp);
        x_list[seq_size] = i;
        y_list[seq_size++] = j;
        max = i;
        goto continue_d;
      }
    }
#endif
    for (; i < max; i++) {
      if (alpha->data[i * alpha->width + j] > t) {
        x_list[seq_size] = i;
        y_list[seq_size++] = j;
        max = i;
        break;
      }
    }
  continue_d:
    continue;
  }

  // x_list[seq_size] = x_list[0];
  // y_list[seq_size++] = y_list[0];

  GEOSCoordSequence *seq =
      GEOSCoordSeq_copyFromArrays(x_list, y_list, NULL, NULL, seq_size);
  free(x_list);
  free(y_list);
  *lstring = GEOSGeom_createLineString(seq);
}

#define WORK_SIZE 56

void convex_polygon(void *_params) {
  ConvexParams *params = (ConvexParams *)(long)_params;
  while (1) {
    printf("Running thread id: %lu\n", params->thread_id);
    GEOSGeometry *lstring;
    extract_polygon(&lstring, params->alpha_ptrs[params->thread_id], params->t);
    params->polygon = GEOSConvexHull(lstring);
    GEOSFree(lstring);

    mtx_lock(params->next_work_mtx);
    mtx_trylock(params->next_work_mtx);

    if (*params->next_work != 0) {
      params->thread_id = *params->next_work;
      *(params->next_work) += 1;
      if (*params->next_work == WORK_SIZE) {
        *params->next_work = 0;
      }
      mtx_unlock(params->next_work_mtx);
      continue;
    }

    mtx_unlock(params->next_work_mtx);
    thrd_exit(EXIT_SUCCESS);
  }
}

static void geos_msg_handler(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputs("\n", stderr);
}

static void panic(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputs("\n", stderr);
  thrd_exit(EXIT_FAILURE);
}

int main() {
  initGEOS(geos_msg_handler, geos_msg_handler);
  mtx_t next_work_mtx;

  ConvexParams *work = calloc(WORK_SIZE, sizeof(ConvexParams));
  if (work == NULL) {
    panic("Out of memory");
  }

  ImgAlpha **alpha_ptrs = malloc(WORK_SIZE * sizeof(ImgAlpha *));
  if (alpha_ptrs == NULL) {
    panic("Out of memory");
  }

  for (size_t i = 0; i < WORK_SIZE; i++) {
    alpha_ptrs[i] = &test_image;
  }

  size_t threads_size = sysconf(_SC_NPROCESSORS_ONLN);
  uint64_t next_work = WORK_SIZE - threads_size;

  thrd_t *threads = malloc(threads_size * sizeof(thrd_t));
  if (threads == NULL) {
    panic("Out of memory");
  }

  if (mtx_init(&next_work_mtx, mtx_plain) == thrd_error)
    panic("Failed to init mutex");
  for (size_t i = 0; i < threads_size; i++) {
    if (i == WORK_SIZE) {
      break;
    }
    work[i].alpha_ptrs = alpha_ptrs;
    work[i].t = -93;
    work[i].thread_id = i;
    work[i].next_work = &next_work;
    work[i].next_work_mtx = &next_work_mtx;
    int rc = thrd_create(&threads[i], (thrd_start_t)convex_polygon,
                         (void *)(long)(work + i));
    if (rc != thrd_success) {
      if (rc == thrd_error) {
        perror("Thread creation error");
      } else {
        perror("Out of memory");
      }
      thrd_exit(EXIT_FAILURE);
    }
  }

  int r;
  for (size_t i = 0; i < threads_size; i++) {
    if (thrd_join(threads[i], &r) != thrd_success) {
      fprintf(stderr, "Failed to join thread %zu\n", i);
    }
  }

  GEOSGeoJSONWriter *json_writer = GEOSGeoJSONWriter_create();
  char *json = GEOSGeoJSONWriter_writeGeometry(json_writer, work[0].polygon, 2);
  printf("%s\n", json);
  GEOSFree(json);
  GEOSGeoJSONWriter_destroy(json_writer);

  free(alpha_ptrs);
  for (uint64_t i = 0; i < WORK_SIZE; i++) {
    GEOSGeom_destroy(work[i].polygon);
  }
  free(work);
  free(threads);

  thrd_exit(EXIT_SUCCESS);
}
