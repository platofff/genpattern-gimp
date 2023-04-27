#include "genpattern.h"
#include "basic_geometry.h"
#include "convex_area.h"
#include "convex_hull.h"
#include "debug.h"
#include "hooke.h"
#include "misc.h"
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

void* _gp_convex_polygon_thrd(void* _data) {
  GPThreadData* data = (GPThreadData*)_data;
  GPParams* params = data->params;
  int32_t img_id = data->thread_id;
  while (1) {
    // printf("Running thread id: %zu\n", data->thread_id);
    GPPolygon* polygon = &params->cp.polygons[img_id];
    GPImgAlpha* alpha = &params->cp.alphas[img_id];

    int res = gp_image_convex_hull(polygon, alpha, params->cp.t);
    if (res != 0) {
      pthread_exit(&res);
    }

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
  gp_polygon_copy(polygon_buffer, ref);
  int32_t node_id = data->thread_id;
  GPVector node = params->gp.grid[node_id];

  while (1) {
    gp_polygon_translate(polygon_buffer, ref, node.x, node.y);
    GPSParams sp = {polygon_buffer,        params->gp.target,         params->gp.polygons,        params->gp.current,
                    params->gp.collection, params->gp.collection_len, &params->gp.canvas_polygon, ref};

    GPPoint p;
    float res = gp_maximize_suitability(node, 50, -params->gp.target, &sp, &p);

    pthread_mutex_lock(&params->cp.next_work_mtx);
    if (*params->gp.next_work != 0) {
      node_id = *params->gp.next_work;
      node = params->gp.grid[node_id];
      *params->gp.next_work += 1;
      if (*params->gp.next_work == params->gp.work_size) {
        *params->gp.next_work = 0;
      }
      if (res <= -params->gp.target) {
        gp_polygon_translate(ref, ref, p.x, p.y);
        *params->gp.next_work = 0;
        pthread_mutex_unlock(&params->gp.next_work_mtx);
        return NULL;
      }
      pthread_mutex_unlock(&params->gp.next_work_mtx);
      continue;
    }

    pthread_mutex_unlock(&params->gp.next_work_mtx);
    return NULL;
  }
}

LIBGENPATTERN_API int gp_genpattern(GPImgAlpha* alphas,
                                    int32_t* collections_sizes,
                                    int32_t collections_len,
                                    int32_t canvas_width,
                                    int32_t canvas_height,
                                    uint8_t t,
                                    int32_t min_dist,
                                    int32_t grid_resolution,
                                    int32_t out_len,
                                    GPVector* out,
                                    size_t threads_num) {
  int exitcode = 0;

  GPParams work;
  work.cp.polygons = NULL;
  work.cp.work_size = out_len;

  work.cp.alphas = alphas;
  work.cp.t = t;
  pthread_t* threads = NULL;
  GPThreadData* threads_data = NULL;

  GPBox canvas_box = {.xmin = 0.f, .ymin = 0.f, .xmax = canvas_width, .ymax = canvas_height};
  exitcode = gp_box_to_polygon(&canvas_box, &work.cp.canvas_polygon);
  if (exitcode != 0) {
    return exitcode;
  }

  work.cp.polygons = malloc(out_len * sizeof(GPPolygon));
  GP_CHECK_ALLOC(work.cp.polygons);

  if (threads_num == 0) {
    threads_num = _gp_nproc() - 1;
    if (threads_num == 0) {
      threads_num = 1;
    }
  }

  size_t cp_threads_size = MIN(threads_num, out_len);
  size_t next_work = out_len > cp_threads_size ? cp_threads_size : 0;
  work.cp.next_work = &next_work;

  int32_t max_size = 0;
  work.cp.max_size = &max_size;

  threads = malloc(threads_num * sizeof(pthread_t));
  GP_CHECK_ALLOC(threads);

  work.cp.next_work_mtx = NULL;
  if (pthread_mutex_init(&work.cp.next_work_mtx, NULL) != 0) {
    return GP_ERR_THREADING;
  }

  threads_data = malloc(threads_num * sizeof(GPThreadData));
  GP_CHECK_ALLOC(threads_data);

  for (size_t i = 0; i < threads_num; i++) {
    threads_data[i].params = &work;
    threads_data[i].thread_id = i;
    if (i >= cp_threads_size) {
      continue;
    }
    int rc = pthread_create(&threads[i], NULL, _gp_convex_polygon_thrd, (void*)(threads_data + i));
    if (rc != 0) {
      return GP_ERR_THREADING;
    }
  }

  for (size_t i = 0; i < cp_threads_size; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Failed to join thread %zu\n", i);
    }
  }

  // Generate pattern

  work.gp.target = min_dist;
  work.gp.polygon_buffers = NULL;
  work.gp.polygon_buffers = malloc(threads_num * sizeof(GPPolygon));
  GP_CHECK_ALLOC(work.gp.polygon_buffers);

  for (int32_t i = 0; i < threads_num; i++) {
    work.gp.polygon_buffers[i].x_ptr = NULL;
    work.gp.polygon_buffers[i].y_ptr = NULL;
    work.gp.polygon_buffers[i].x_ptr = aligned_alloc(32, sizeof(float) * max_size);
    GP_CHECK_ALLOC(work.gp.polygon_buffers[i].x_ptr);
    work.gp.polygon_buffers[i].y_ptr = aligned_alloc(32, sizeof(float) * max_size);
    GP_CHECK_ALLOC(work.gp.polygon_buffers[i].y_ptr);
  }

  size_t grid_len = 1000;
  exitcode = gp_grid_init(canvas_width, canvas_height, grid_resolution, &work.gp.grid, &grid_len);
  if (exitcode != 0) {
    return exitcode;
  }
  GPVector* shuffle_buf = malloc(grid_len * sizeof(GPVector));
  gp_array_shuffle(work.gp.grid, shuffle_buf, sizeof(GPVector), grid_len);

  work.gp.collection = work.gp.polygons;
  int32_t collection_id = 0;
  work.gp.collection_len = 0;
  int32_t start_work = MIN(threads_num, grid_len);

  for (work.gp.current = 0; work.gp.current < out_len; work.gp.current++) {
    work.gp.work_size = grid_len;
    next_work = start_work;
    gp_array_shuffle(work.gp.grid, shuffle_buf, sizeof(GPVector), grid_len);

    for (size_t i = 0; i < start_work; i++) {
      int rc = pthread_create(&threads[i], NULL, _gp_generate_pattern_thrd, (void*)(threads_data + i));
      if (rc != 0) {
        return GP_ERR_THREADING;
      }
    }
    for (size_t i = 0; i < threads_num; i++) {
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

  for (int32_t i = 0; i < out_len; i++) {
    GPPolygon polygon = work.gp.polygons[i];
    out[i].x = polygon.base_offset.x;
    out[i].y = polygon.base_offset.y;
  }

  gp_debug_polygons(work.gp.polygons, out_len);

  free(shuffle_buf);
  for (int32_t i = 0; i < out_len; i++) {
    gp_polygon_free(&work.gp.polygons[i]);
  }
  for (int32_t i = 0; i < threads_num; i++) {
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

LIBGENPATTERN_API int gp_init(void) {
  srand(56);
  return 0;
}

/*
#define WORK_SIZE 8
#include "doubly_linked_list.h"
#include "convex_intersection_area.h"
#include "convex_area.h"

int main() {
  gp_init();

  GPVector* offsets = malloc(sizeof(GPVector) * WORK_SIZE);
  GP_CHECK_ALLOC(offsets);
  int32_t collections_len = 1;
  int32_t collections_sizes[1] = {WORK_SIZE};
  int32_t canvas_width = 3000;
  int32_t canvas_height = 3000;
  uint8_t t = 32;
  int32_t resolution = 50;
  int32_t out_len = WORK_SIZE;
  int32_t min_dist = 150;

  GPImgAlpha* alphas = malloc(sizeof(GPImgAlpha) * WORK_SIZE);
  GP_CHECK_ALLOC(alphas);
  for (int32_t i = 0; i < WORK_SIZE; i++) {
    memcpy(&alphas[i], &test_image, sizeof(test_image));
  }

  gp_genpattern(alphas, &collections_sizes[0], collections_len, canvas_width, canvas_height, t, min_dist, resolution,
                out_len, offsets, 0);

  free(alphas);
  free(offsets);
    


  GPDList *list1, *list2;
  gp_dllist_alloc(&list1);
  gp_dllist_alloc(&list2);

  GPPoint points[] = {
      {60.000000, 1047.000000},  {60.000000, 1019.000000},  {61.000000, 1016.000000},  {63.000000, 1012.000000},
      {68.000000, 1007.000000},  {71.000000, 1005.000000},  {79.000000, 1001.000000},  {130.000000, 977.000000},
      {207.000000, 941.000000},  {237.000000, 927.000000},  {250.000000, 921.000000},  {253.000000, 920.000000},
      {263.000000, 917.000000},  {271.000000, 917.000000},  {275.000000, 918.000000},  {277.000000, 919.000000},
      {283.000000, 925.000000},  {285.000000, 928.000000},  {293.000000, 943.000000},  {294.000000, 945.000000},
      {296.000000, 951.000000},  {296.000000, 962.000000},  {282.000000, 1032.000000}, {263.000000, 1126.000000},
      {262.000000, 1129.000000}, {261.000000, 1131.000000}, {258.000000, 1135.000000}, {256.000000, 1137.000000},
      {253.000000, 1139.000000}, {251.000000, 1140.000000}, {248.000000, 1141.000000}, {235.000000, 1141.000000},
      {227.000000, 1139.000000}, {216.000000, 1134.000000}, {204.000000, 1128.000000}, {73.000000, 1062.000000},
      {68.000000, 1059.000000},  {63.000000, 1054.000000},  {61.000000, 1050.000000}
  };

  GPPoint points2[] = {
      {0.000000, 1000.000000}, {0.000000, 0.000000}, {1000.000000, 0.000000}, {1000.000000, 1000.000000}};

  for (int32_t i = sizeof(points) / sizeof(GPPoint) - 1; i >= 0; i--) {
    gp_dllist_push(list1, points[i]);
    //gp_dllist_push(list2, (GPPoint){points[i].x + x_off, points[i].y + y_off});
  }
  for (int32_t i = sizeof(points2) / sizeof(GPPoint) - 1; i >= 0; i--) {
    gp_dllist_push(list2, points2[i]);
    // gp_dllist_push(list2, (GPPoint){points[i].x + x_off, points[i].y + y_off});
  }

  GPPolygon polygon1, polygon2, canvas;
  gp_dllist_to_polygon(list1, &polygon1);
  gp_polygon_translate(&polygon1, &polygon1, 340, 307);

  //gp_debug_polygons(&polygon1, 1);
  gp_dllist_to_polygon(list1, &polygon2);
  gp_polygon_translate(&polygon2, &polygon2, 200, 450);

  //gp_debug_polygons(&polygon2, 1);
  gp_dllist_to_polygon(list2, &canvas);

  gp_dllist_free(list1);
  gp_dllist_free(list2);

  bool intersected = false;

  float res = gp_convex_intersection_area(&polygon1, &polygon2, &intersected);
  printf("%f\n", res);

  float s = gp_suitability((GPSParams){&polygon2, 10, &polygon1, 1, &polygon1, 1, &canvas, NULL});

  printf("%f\n", res);
}*/
