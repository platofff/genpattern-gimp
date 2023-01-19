#include "genpattern.h"
#include "convex_area.h"
#include "basic_geometry.h"
#include "convex_distance.h"
#include "convex_hull.h"
#include "misc.h"
#include "polygon_translate.h"
#include "convex_intersection_area.h"

#include "test_image1._h"

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

#define WORK_SIZE 32

void* _gp_convex_polygon_thrd(void* _data) {
  GPCPThreadData* data = (GPCPThreadData*)_data;
  GPCPParams* params = data->params;
  while (1) {
    printf("Running thread id: %zu\n", data->thread_id);
    if (data->thread_id != 4) {
      return NULL;
    }
    gp_image_convex_hull(&params->polygons[data->thread_id], params->alpha_ptrs[data->thread_id], params->t);
    gp_polygon_translate(&params->polygons[data->thread_id], 21, 20);
    float s = 1000000.f;
    for (int32_t i = 0; i < 4; i++) {
      bool intersected;
      float r = gp_convex_distance(&params->polygons[data->thread_id], &params->polygons[i]);
      s = MIN(s, r);
      //printf("%f %d\n", r, intersected);
    }
    printf("Distance: %f\n", s);
    pthread_mutex_lock(&params->next_work_mtx);
    if (*params->next_work != 0) {
      data->thread_id = *params->next_work;
      *params->next_work += 1;
      if (*params->next_work == WORK_SIZE) {
        *params->next_work = 0;
      }
      pthread_mutex_unlock(&params->next_work_mtx);
      continue;
    }

    pthread_mutex_unlock(&params->next_work_mtx);
    return NULL;
  }
}

#define PANIC(msg)                \
  {                               \
    fprintf(stderr, "%s\n", msg); \
    exitcode = EXIT_FAILURE;      \
    goto cleanup;                 \
  }

int main() {
  int32_t xres = 3000;
  int32_t yres = 2500;
  int exitcode = EXIT_SUCCESS;
  GPCPParams work = {.alpha_ptrs = NULL, .polygons = NULL };
  pthread_t* threads = NULL;
  GPCPThreadData* threads_data = NULL;

  work.alpha_ptrs = malloc((4 + WORK_SIZE) * sizeof(GPImgAlpha*));
  GP_CHECK_ALLOC(work.alpha_ptrs);

  work.polygons = malloc(WORK_SIZE * sizeof(GPPolygon));
  GP_CHECK_ALLOC(work.polygons);

  gp_canvas_outside_area(xres, yres, work.polygons);

  for (size_t i = 0; i < WORK_SIZE; i++) {
    work.alpha_ptrs[i] = &test_image;
  }

  work.t = 32;

  size_t threads_size = _gp_nproc();
  uint64_t next_work = WORK_SIZE > threads_size ? threads_size : 0;
  work.next_work = &next_work;

  threads = malloc(threads_size * sizeof(pthread_t));
  GP_CHECK_ALLOC(threads);

  if (pthread_mutex_init(&work.next_work_mtx, NULL) != 0) {
    PANIC("Failed to init mutex");
  }

  threads_data = malloc(threads_size * sizeof(GPCPThreadData));
  GP_CHECK_ALLOC(threads_data);

  for (size_t i = 0; i < threads_size; i++) {
    if (i == WORK_SIZE) {
      break;
    }
    threads_data[i].params = &work;
    threads_data[i].thread_id = i + 4;
    int rc = pthread_create(&threads[i], NULL, _gp_convex_polygon_thrd, (void*)(threads_data + i));
    if (rc != 0) {
      PANIC("Thread creation error!");
    }
  }

  for (size_t i = 0; i < MIN(threads_size, WORK_SIZE); i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Failed to join thread %zu\n", i);
    }
  }

cleanup:
  free(work.polygons);
  free(work.alpha_ptrs);
  free(threads_data);
  pthread_mutex_destroy(&work.next_work_mtx);
  free(threads);

  return exitcode;
}
