#include "genpattern.h"
#include "area.h"
#include "basic_geometry.h"
#include "convex_hull.h"
#include "misc.h"
#include "polygon_distance.h"
#include "translate.h"

#include "test_image1._h"

#include <stdarg.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <windows.h>
static inline size_t nproc(void) {
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
}
#else
#include <unistd.h>
static inline size_t nproc(void) { return sysconf(_SC_NPROCESSORS_ONLN); }
#endif

#define WORK_SIZE 32

void *convex_polygon_thrd(void *_data) {
  CPThreadData *data = (CPThreadData *)(long)_data;
  CPParams *params = data->params;
  while (1) {
    printf("Running thread id: %lu\n", data->thread_id);
    image_convex_hull(&params->polygon_ptrs[data->thread_id],
                      params->alpha_ptrs[data->thread_id], params->t);
    // printf("%ld\n", params->polygon_ptrs[data->thread_id]->size);
    // polygon_translate(params->polygon_ptrs[data->thread_id], -56, 74);
    // printf("%f\n", polygon_area(params->polygon_ptrs[data->thread_id]));
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
    int retval = EXIT_SUCCESS;
    pthread_exit((void *)&retval);
  }
}

static void panic(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputs("\n", stderr);
  int retval = EXIT_FAILURE;
  pthread_exit((void *)&retval);
}

int main() {
  CPParams work;

  work.alpha_ptrs = malloc(WORK_SIZE * sizeof(ImgAlpha *));
  if (work.alpha_ptrs == NULL) {
    panic("Out of memory");
  }

  work.polygon_ptrs = malloc(WORK_SIZE * sizeof(Polygon *));
  if (work.polygon_ptrs == NULL) {
    panic("Out of memory");
  }

  for (size_t i = 0; i < WORK_SIZE; i++) {
    work.alpha_ptrs[i] = &test_image;
  }

  work.t = 32;

  size_t threads_size = nproc();
  uint64_t next_work = WORK_SIZE > threads_size ? threads_size : 0;
  work.next_work = &next_work;
  pthread_t *threads = malloc(threads_size * sizeof(pthread_t));
  if (threads == NULL) {
    panic("Out of memory");
  }

  if (pthread_mutex_init(&work.next_work_mtx, NULL) != 0) {
    panic("Failed to init mutex");
  }

  CPThreadData *threads_data = malloc(threads_size * sizeof(CPThreadData));
  if (threads_data == NULL) {
    panic("Out of memory");
  }

  for (size_t i = 0; i < threads_size; i++) {
    if (i == WORK_SIZE) {
      break;
    }
    threads_data[i].params = &work;
    threads_data[i].thread_id = i;
    int rc = pthread_create(&threads[i], NULL, convex_polygon_thrd,
                            (void *)(long)(threads_data + i));
    if (rc != 0) {
      panic("Thread creation error!");
    }
  }

  for (size_t i = 0; i < MIN(threads_size, WORK_SIZE); i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Failed to join thread %zu\n", i);
    }
  }
  /*
  for (size_t i = 0; i < work.polygon_ptrs[0]->size; i++) {
    printf("(%f, %f), ", work.polygon_ptrs[0]->x_ptr[i],
  work.polygon_ptrs[0]->y_ptr[i]);
  }
  puts("");
  */
  polygon_translate(work.polygon_ptrs[0], -5600, 2000);
  work.polygon_ptrs[0]->size--; // remove last point
  work.polygon_ptrs[1]->size--;
  printf("%f\n", polygon_distance(work.polygon_ptrs[0], work.polygon_ptrs[1]));

  for (size_t i = 0; i < WORK_SIZE; i++) {
    polygon_free(work.polygon_ptrs[i]);
  }
  free(work.polygon_ptrs);
  free(work.alpha_ptrs);
  free(threads_data);
  pthread_mutex_destroy(&work.next_work_mtx);
  free(threads);

  pthread_exit(EXIT_SUCCESS);
}
