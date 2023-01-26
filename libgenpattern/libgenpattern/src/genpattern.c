#include "genpattern.h"
#include "basic_geometry.h"
#include "convex_area.h"
#include "convex_hull.h"
#include "misc.h"
#include "polygon_translate.h"
#include "suitability.h"

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
  GPThreadData* data = (GPThreadData*)_data;
  GPParams* params = data->params;
  int32_t img_id = data->thread_id;
  while (1) {
    printf("Running thread id: %zu\n", data->thread_id);
    GPPolygon* polygon = &params->cp.polygons[img_id];
    GPImgAlpha* alpha = &params->cp.alphas[img_id];

    gp_image_convex_hull(polygon, alpha, params->cp.t);

    pthread_mutex_lock(&params->cp.next_work_mtx);
    if (*params->cp.max_size < polygon->size) {
      *params->cp.max_size = polygon->size;
    }
    if (*params->cp.next_work != 0) {
      img_id = *params->cp.next_work;
      *params->cp.next_work += 1;
      if (*params->cp.next_work == params->cp.work_size) {
        *params->cp.next_work = 0;
      }
      pthread_mutex_unlock(&params->cp.next_work_mtx);
      continue;
    }

    pthread_mutex_unlock(&params->cp.next_work_mtx);
    return NULL;
  }
}

void* _gp_generate_pattern_thrd(void* _data) {
  GPThreadData* data = (GPThreadData*)_data;
  GPParams* params = data->params;

  GPPolygon* polygon_buffer = &params->gp.polygon_buffers[data->thread_id];
  GPPolygon* ref = &params->gp.polygons[params->gp.current];
  //gp_polygon_copy(polygon_buffer, ref);
  int32_t node_id = data->thread_id;
  GPVector node = params->gp.grid[node_id];

  while (1) {
    printf("Thread %d, point (%f, %f)\n", data->thread_id, node.x, node.y);

    pthread_mutex_lock(&params->cp.next_work_mtx);
    if (*params->gp.next_work != 0) {
      node_id = *params->gp.next_work;
      node = params->gp.grid[node_id];
      *params->gp.next_work += 1;
      if (*params->gp.next_work == params->gp.work_size) {
        *params->gp.next_work = 0;
      }
      pthread_mutex_unlock(&params->gp.next_work_mtx);
      continue;
    }

    pthread_mutex_unlock(&params->gp.next_work_mtx);
    return NULL;
  }
}

int gp_genpattern(GPImgAlpha* alphas,
                  int32_t* collections_sizes,
                  int32_t collections_len,
                  int32_t canvas_width,
                  int32_t canvas_height,
                  uint8_t t,
                  int32_t grid_resolution,
                  int32_t out_len,
                  GPVector* out) {
  int exitcode = EXIT_SUCCESS;
  GPParams work;
  work.cp.polygons = NULL;
  work.cp.work_size = out_len;

  work.cp.alphas = alphas;
  work.cp.t = t;
  pthread_t* threads = NULL;
  GPThreadData* threads_data = NULL;

  GPBox canvas_box = {.xmin = 0.f, .ymin = 0.f, .xmax = canvas_width, .ymax = canvas_height};
  gp_box_to_polygon(&canvas_box, &work.cp.canvas_polygon);

  work.cp.polygons = malloc(out_len * sizeof(GPPolygon));
  GP_CHECK_ALLOC(work.cp.polygons);

  size_t threads_size = _gp_nproc() - 1;
  if (threads_size == 0) {
    threads_size = 1;
  }
  size_t cp_threads_size = MIN(threads_size, out_len);
  size_t next_work = out_len > cp_threads_size ? cp_threads_size : 0;
  work.cp.next_work = &next_work;

  int32_t max_size = 0;
  work.cp.max_size = &max_size;

  threads = malloc(threads_size * sizeof(pthread_t));
  GP_CHECK_ALLOC(threads);

  work.cp.next_work_mtx = NULL;
  if (pthread_mutex_init(&work.cp.next_work_mtx, NULL) != 0) {
    PANIC("Failed to init mutex");
  }

  threads_data = malloc(threads_size * sizeof(GPThreadData));
  GP_CHECK_ALLOC(threads_data);

  for (size_t i = 0; i < threads_size; i++) {
    threads_data[i].params = &work;
    threads_data[i].thread_id = i;
    if (i >= cp_threads_size) {
      continue;
    }
    int rc = pthread_create(&threads[i], NULL, _gp_convex_polygon_thrd, (void*)(threads_data + i));
    if (rc != 0) {
      PANIC("Thread creation error!");
    }
  }

  for (size_t i = 0; i < cp_threads_size; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Failed to join thread %zu\n", i);
    }
  }

  // Generate pattern
  
  work.gp.polygon_buffers = NULL;
  work.gp.polygon_buffers = malloc(threads_size *sizeof(GPPolygon));
  GP_CHECK_ALLOC(work.gp.polygon_buffers);
  
  
  for (int32_t i = 0; i < threads_size; i++) {
    work.gp.polygon_buffers[i].x_ptr = NULL;
    work.gp.polygon_buffers[i].y_ptr = NULL;
    work.gp.polygon_buffers[i].x_ptr = aligned_alloc(32, sizeof(float) * max_size);
    GP_CHECK_ALLOC(work.gp.polygon_buffers[i].x_ptr);
    work.gp.polygon_buffers[i].y_ptr = aligned_alloc(32, sizeof(float) * max_size);
    GP_CHECK_ALLOC(work.gp.polygon_buffers[i].y_ptr);
  }
  
  size_t grid_len = 1000;
  gp_grid_init(canvas_width, canvas_height, grid_resolution, &work.gp.grid, &grid_len);
  gp_array_shuffle(work.gp.grid, sizeof(GPVector), grid_len);
  
  work.gp.collection = work.gp.polygons;
  int32_t collection_id = 0;
  work.gp.collection_len = 0;
  int32_t start_work = MIN(threads_size, grid_len);

  for (work.gp.current = 0; work.gp.current < out_len; work.gp.current++) {
    work.gp.work_size = grid_len;
    next_work = start_work;
    // gp_array_shuffle(&work.gp.grid, sizeof(GPVector), grid_len);
    
    for (size_t i = 0; i < start_work; i++) {
      int rc = pthread_create(&threads[i], NULL, _gp_generate_pattern_thrd, (void*)(threads_data + i));
      if (rc != 0) {
        PANIC("Thread creation error!");
      }
    }
    for (size_t i = 0; i < threads_size; i++) {
      if (pthread_join(threads[i], NULL) != 0) {
        fprintf(stderr, "Failed to join thread %zu\n", i);
      }
    }

    work.gp.collection_len++;
    if (work.gp.collection_len == collections_sizes[collection_id]) {
      work.gp.collection += work.gp.collection_len;
      collection_id++;
      work.gp.collection_len = 0;
    }
  }
  
cleanup:
  
  for (int32_t i = 0; i < out_len; i++) {
    gp_polygon_free(&work.gp.polygons[i]);
  }
  for (int32_t i = 0; i < threads_size; i++) {
    gp_polygon_free(&work.gp.polygon_buffers[i]);
  }
  free(work.gp.grid);
  free(work.gp.polygons);
  free(work.gp.polygon_buffers);
  free(threads_data);
  pthread_mutex_destroy(&work.gp.next_work_mtx);
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
  int32_t resolution = 50;
  int32_t out_len = WORK_SIZE;

  GPImgAlpha* alphas = malloc(sizeof(GPImgAlpha) * WORK_SIZE);
  GP_CHECK_ALLOC(alphas);
  for (int32_t i = 0; i < WORK_SIZE; i++) {
    memcpy(&alphas[i], &test_image, sizeof(test_image));
  }

  gp_genpattern(alphas, &collections_sizes[0], collections_len, canvas_width, canvas_height, t, resolution, out_len,
                offsets);

  free(alphas);
  free(offsets);
}
