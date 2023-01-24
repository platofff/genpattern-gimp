#include "genpattern.h"
#include "basic_geometry.h"
#include "convex_area.h"
#include "convex_hull.h"
#include "misc.h"
#include "suitability.h"
#include "polygon_translate.h"

#include <stdarg.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <windows.h>
static inline size_t _gp_nproc(void) {
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
}
#else
#include <unistd.h>
static inline size_t _gp_nproc(void) {
  return sysconf(_SC_NPROCESSORS_ONLN);
}
#endif

#define PANIC(msg)                \
  {                               \
    fprintf(stderr, "%s\n", msg); \
    exitcode = EXIT_FAILURE;      \
    goto cleanup;                 \
  }

void* _gp_convex_polygon_thrd(void* _data) {
  GPCPThreadData* data = (GPCPThreadData*)_data;
  GPCPParams* params = data->params;
  while (1) {
    printf("Running thread id: %zu\n", data->thread_id);
    GPPolygon* polygon = &params->polygons[data->thread_id];
    GPImgAlpha* alpha = &params->alphas[data->thread_id];
    int32_t collection_id = params->collection_ids[data->thread_id];

    gp_image_convex_hull(polygon, alpha, params->t, collection_id);

    gp_polygon_translate(polygon, polygon, 56, -17);
    float s = gp_suitability(polygon, params->polygons, 0, &params->canvas_polygon);
    printf("Distance: %f\n", s);

    pthread_mutex_lock(&params->next_work_mtx);
    if (*params->next_work != 0) {
      data->thread_id = *params->next_work;
      *params->next_work += 1;
      if (*params->next_work == params->work_size) {
        *params->next_work = 0;
      }
      pthread_mutex_unlock(&params->next_work_mtx);
      continue;
    }

    pthread_mutex_unlock(&params->next_work_mtx);
    return NULL;
  }
}

int gp_genpattern(GPImgAlpha* alphas,
                  int32_t* collections_sizes,
                  int32_t collections_len,
                  int32_t canvas_width,
                  int32_t canvas_height,
                  uint8_t t,
                  int32_t out_len,
                  GPVector* out) {
  int exitcode = EXIT_SUCCESS;
  GPCPParams work = {.alphas = alphas, .polygons = NULL, .collection_ids = NULL, .t = t, .work_size = out_len};
  pthread_t* threads = NULL;
  GPCPThreadData* threads_data = NULL;

  GPBox canvas_box = {.xmin = 0.f, .ymin = 0.f, .xmax = canvas_width, .ymax = canvas_height};
  gp_box_to_polygon(&canvas_box, &work.canvas_polygon);

  work.polygons = malloc(out_len * sizeof(GPPolygon));
  GP_CHECK_ALLOC(work.polygons);

  // gp_canvas_outside_area(canvas_width, canvas_height, work.polygons);

  work.collection_ids = malloc(out_len * sizeof(int32_t));
  GP_CHECK_ALLOC(work.collection_ids);
  for (int32_t i = 0, cid = 0, j = 1; i < out_len; i++, j++) {
    work.collection_ids[i] = cid;
    if (j == collections_sizes[cid]) {
      cid++;
      j = 1;
    }
  }

  size_t threads_size = _gp_nproc() - 1;
  if (threads_size == 0) {
    threads_size = 1;
  }
  threads_size = MIN(threads_size, out_len);
  size_t next_work = out_len > threads_size ? threads_size : 0;
  work.next_work = &next_work;

  threads = malloc(threads_size * sizeof(pthread_t));
  GP_CHECK_ALLOC(threads);

  if (pthread_mutex_init(&work.next_work_mtx, NULL) != 0) {
    PANIC("Failed to init mutex");
  }

  threads_data = malloc(threads_size * sizeof(GPCPThreadData));
  GP_CHECK_ALLOC(threads_data);

  for (size_t i = 0; i < threads_size; i++) {
    threads_data[i].params = &work;
    threads_data[i].thread_id = i;
    int rc = pthread_create(&threads[i], NULL, _gp_convex_polygon_thrd, (void*)(threads_data + i));
    if (rc != 0) {
      PANIC("Thread creation error!");
    }
  }

  for (size_t i = 0; i < threads_size; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Failed to join thread %zu\n", i);
    }
  }

cleanup:
  for (int32_t i = 0; i < out_len; i++) {
    gp_polygon_free(&work.polygons[i]);
  }
  free(work.polygons);
  free(work.collection_ids);
  free(threads_data);
  pthread_mutex_destroy(&work.next_work_mtx);
  free(threads);

  return exitcode;
}

#include "test_image1._h"
#define WORK_SIZE 1

int main() {
  GPVector* offsets = malloc(sizeof(GPVector) * WORK_SIZE);
  GP_CHECK_ALLOC(offsets);
  int32_t collections_len = 1;
  int32_t collections_sizes[1] = {WORK_SIZE};
  int32_t canvas_width = 2500;
  int32_t canvas_height = 3000;
  uint8_t t = 32;
  int32_t out_len = WORK_SIZE;

  GPImgAlpha* alphas = malloc(sizeof(GPImgAlpha) * WORK_SIZE);
  GP_CHECK_ALLOC(alphas);
  for (int32_t i = 0; i < WORK_SIZE; i++) {
    memcpy(&alphas[i], &test_image, sizeof(test_image));
  }

  gp_genpattern(alphas, &collections_sizes[0], collections_len, canvas_width, canvas_height, t, out_len, offsets);

  free(alphas);
  free(offsets);
}
