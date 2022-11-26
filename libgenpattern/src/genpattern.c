#include "genpattern.h"
#include "basic_geometry.h"
#include "convex_hull.h"
#include "area.h"
#include "misc.h"
#include "translate.h"
#include "polygon_distance.h"

#include "test_image1._h"

#include <stdarg.h>
#include <stdio.h>
#include <threads.h>
#include <unistd.h>


#define WORK_SIZE 2

void convex_polygon_thrd(void *_data) {
  CPThreadData *data = (CPThreadData *)(long)_data;
  CPParams *params = data->params;
  while (1) {
    printf("Running thread id: %lu\n", data->thread_id);
    image_convex_hull(&params->polygon_ptrs[data->thread_id],
                      params->alpha_ptrs[data->thread_id],
                      params->t);
    //printf("%ld\n", params->polygon_ptrs[data->thread_id]->size);
    //polygon_translate(params->polygon_ptrs[data->thread_id], -56, 74);
    //printf("%f\n", polygon_area(params->polygon_ptrs[data->thread_id]));
    mtx_lock(&params->next_work_mtx);
    if (*params->next_work != 0) {
      data->thread_id = *params->next_work;
      *params->next_work += 1;
      if (*params->next_work == WORK_SIZE) {
        *params->next_work = 0;
      }
      mtx_unlock(&params->next_work_mtx);
      continue;
    }

    mtx_unlock(&params->next_work_mtx);
    thrd_exit(EXIT_SUCCESS);
  }
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

  size_t threads_size = sysconf(_SC_NPROCESSORS_ONLN);
  uint64_t next_work = WORK_SIZE > threads_size ? threads_size : 0;
  work.next_work = &next_work;
  thrd_t *threads = malloc(threads_size * sizeof(thrd_t));
  if (threads == NULL) {
    panic("Out of memory");
  }

  if (mtx_init(&work.next_work_mtx, mtx_plain) == thrd_error) {
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
    int rc = thrd_create(&threads[i], (thrd_start_t)convex_polygon_thrd,
                         (void *)(long)(threads_data + i));
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
  for (size_t i = 0; i < MIN(threads_size, WORK_SIZE); i++) {
    if (thrd_join(threads[i], &r) != thrd_success) {
      fprintf(stderr, "Failed to join thread %zu\n", i);
    }
  }
  /*
  for (size_t i = 0; i < work.polygon_ptrs[0]->size; i++) {
    printf("(%f, %f), ", work.polygon_ptrs[0]->x_ptr[i], work.polygon_ptrs[0]->y_ptr[i]);
  }
  puts("");
  */
  polygon_translate(work.polygon_ptrs[1], -560, 2000);
  work.polygon_ptrs[0]->size--; // remove last point
  work.polygon_ptrs[1]->size--;
  printf("%f\n", polygon_distance(work.polygon_ptrs[0], work.polygon_ptrs[1]));


  for (size_t i = 0; i < WORK_SIZE; i++) {
    polygon_free(work.polygon_ptrs[i]);
  }
  free(work.polygon_ptrs);
  free(work.alpha_ptrs);
  free(threads_data);
  free(threads);

  thrd_exit(EXIT_SUCCESS);
}
