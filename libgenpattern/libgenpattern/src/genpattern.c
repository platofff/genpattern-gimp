#include "genpattern.h"
#include "basic_geometry.h"
#include "convex_area.h"
#include "convex_hull.h"
#include "debug.h"
#include "hooke.h"
#include "misc.h"
#include "polygon_translate.h"

#include <assert.h>
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

// This is the entry point for each thread that computes the minimal convex hulls of images.
void* _gp_convex_polygon_thrd(void* _data) {
  // Cast the input data to the appropriate structure (GPThreadData).
  GPThreadData* data = (GPThreadData*)_data;
  GPParams* params = data->params;
  int32_t img_id = data->thread_id;

  // Main loop for processing images.
  while (1) {
    // Get the polygon and the alpha channel for the current image ID.
    GPPolygon* polygon = &params->cp.polygons[img_id];
    GPImgAlpha* alpha = &params->cp.alphas[img_id];

    // Compute the convex hull of the image using its alpha channel and the given threshold.
    int res = gp_image_convex_hull(polygon, alpha, params->cp.t);

    if (res != 0) {
      pthread_exit(NULL);
    }

    // Lock the mutex to safely update the shared data structures.
    pthread_mutex_lock(&params->cp.next_work_mtx);

    // Update the maximum polygon size if necessary.
    if (*params->cp.max_size < polygon->size) {
      *params->cp.max_size = polygon->size;
    }

    // If there's more work to do, update the image ID and continue processing.
    if (*params->cp.next_work != 0) {
      img_id = *params->cp.next_work;
      *params->cp.next_work += 1;
      if (*params->cp.next_work == params->cp.work_size) {
        *params->cp.next_work = 0;
      }
      // Unlock the mutex before continuing with the next image.
      pthread_mutex_unlock(&params->cp.next_work_mtx);
      continue;
    }

    // If there's no more work, unlock the mutex and exit the thread.
    pthread_mutex_unlock(&params->cp.next_work_mtx);
    return NULL;
  }
}

static inline int _gp_polygons_buffer_alloc(const size_t buf_size,
                                            const size_t max_points,
                                            GPPolygon** res) {
  *res = gp_aligned_alloc(64, sizeof(GPPolygon) * buf_size);
  GP_CHECK_ALLOC(res);
  for (int32_t i = 0; i < buf_size; i++) {
    gp_polygon_init_empty(&(*res)[i], max_points);
  }
  return 0;
}

void* _gp_generate_pattern_thrd(void* _data) {
  GPThreadData* data = (GPThreadData*)_data;
  GPParams* params = data->params;

  GPPolygon* ref = &params->gp.polygons[params->gp.current];
  GPPolygon* polygons_buffer = params->gp.polygon_buffers[data->thread_id];

  for (int32_t i = 0; i < GP_HOOKE_CACHE_SIZE * 5; i++) {
    gp_polygon_copy(&polygons_buffer[i], ref);
  }

  int32_t node_id = data->thread_id;
  GPVector node = params->gp.grid[node_id];

  while (1) {
    for (int32_t i = 0; i < GP_HOOKE_CACHE_SIZE; i++) {
      gp_polygon_translate(&polygons_buffer[i * 5], ref, node.x, node.y);
    }
    GPSParams sp = {polygons_buffer,
                    params->gp.target,
                    params->gp.out_polygons,
                    *params->gp.out_polygons_len,
                    params->gp.collection,
                    params->gp.collection_len,
                    &params->gp.canvas_polygon,
                    params->gp.canvas_outside_areas,
                    ref};

    GPPoint out_points[4] = {0};
    GPPolygon* out_polygons = NULL;
    int32_t out_len = 0;
    gp_float res = gp_maximize_suitability(node, params->gp.initial_step, -params->gp.target, &sp,
                                        &out_points[0], &out_len, &out_polygons);

    assert(out_len <= 4);

    pthread_mutex_lock(&params->cp.next_work_mtx);
    if (*params->gp.next_work != 0) {
      node_id = *params->gp.next_work;
      node = params->gp.grid[node_id];
      *params->gp.next_work += 1;
      if (*params->gp.next_work == params->gp.work_size) {
        *params->gp.next_work = 0;
      }
      if (res <= -params->gp.target) {
        for (int32_t i = 0; i < out_len; i++) {
          size_t idx = *params->gp.out_polygons_len;
          gp_polygon_copy_all(&params->gp.out_polygons[idx], &out_polygons[i]);
          *(params->gp.out_polygons_len) += 1;
        }
        params->gp.collection_len += out_len;

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
                                    uint32_t initial_step,
                                    size_t threads_num,
                                    GPResult** out,
                                    size_t* out_len) {
  int exitcode = 0;
  GPParams work = {0};
  *out_len = 0;
  int32_t pics_len = 0;

  pthread_t* threads = NULL;
  GPThreadData* threads_data = NULL;
  *out = NULL;

  for (int32_t i = 0; i < collections_len; i++) {
    pics_len += collections_sizes[i];
  }

  *out = malloc(pics_len * 4 * sizeof(GPResult));
  GP_CHECK_ALLOC(*out);

  work.cp.work_size = pics_len;
  work.cp.alphas = alphas;
  work.cp.t = t;

  GPBox canvas_box = {.xmin = 0, .ymin = 0, .xmax = canvas_width, .ymax = canvas_height};
  exitcode = gp_box_to_polygon(&canvas_box, &work.cp.canvas_polygon);
  if (exitcode != 0) {
    return exitcode;
  }

  work.cp.polygons = malloc(pics_len * sizeof(GPPolygon));
  GP_CHECK_ALLOC(work.cp.polygons);

  if (threads_num == 0) {
    threads_num = _gp_nproc() - 1;
    if (threads_num == 0) {
      threads_num = 1;
    }
  }

  size_t cp_threads_size = GP_MIN(threads_num, pics_len);
  size_t next_work = pics_len > cp_threads_size ? cp_threads_size : 0;
  work.cp.next_work = &next_work;

  int32_t max_size = 0;
  work.cp.max_size = &max_size;

  threads = malloc(threads_num * sizeof(pthread_t));
  GP_CHECK_ALLOC(threads);

  if (pthread_mutex_init(&work.cp.next_work_mtx, NULL) != 0) {
    return GP_ERR_THREADING;
  }

  threads_data = malloc(threads_num * sizeof(GPThreadData));
  GP_CHECK_ALLOC(threads_data);

  // Each thread processes a single image at a time, and when done, it picks up the next image to
  // process.
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
  work.gp.out_polygons_len = out_len;
  work.gp.initial_step = initial_step;

  /*
  In order to use these polygon buffers, we need to take into account the polygons
  that cross the canvas boundaries and reappear on the other side. Since the intersection
  of two convex polygons can have at most m + n vertices in the worst-case scenario, it
  is reasonable to add 3 vertices to the maximum length of each buffer, rather than 4.
  This is because the space outside the canvas extends to infinity, and it is possible to
  intersect a maximum of 3 of its edges.
  */
  max_size += 3;
  /*
  Allocating 5 polygon buffers per thread, the first one serves as a read-only source
  polygon. Out of the remaining four polygon buffers, one is utilized for the primary
  section of the polygon, while the other three cater to its segments that may cross
  over to the opposite sides of the canvas. This guarantees that the algorithm can
  appropriately manage polygons that wrap around the canvas edges.
  */
  work.gp.polygon_buffers = malloc(sizeof(GPPolygon*) * threads_num);
  GP_CHECK_ALLOC(work.gp.polygon_buffers);

  for (int32_t i = 0; i < threads_num; i++) {
    exitcode = _gp_polygons_buffer_alloc((size_t)5 * GP_HOOKE_CACHE_SIZE, *work.gp.max_size,
                                         &work.gp.polygon_buffers[i]);
    if (exitcode != 0) {
      return exitcode;
    }
  }

  // out_polygons buffer to store already placed parts
  exitcode =
      _gp_polygons_buffer_alloc((size_t)4 * pics_len, *work.gp.max_size, &work.gp.out_polygons);

  if (exitcode != 0) {
    return exitcode;
  }

  size_t grid_len;

  exitcode = gp_grid_init(canvas_width, canvas_height, grid_resolution, &work.gp.grid, &grid_len);
  if (exitcode != 0) {
    return exitcode;
  }

  GPVector shuffle_buf;
  gp_array_shuffle(work.gp.grid, &shuffle_buf, sizeof(GPVector), grid_len);

  work.gp.collection = work.gp.out_polygons;
  int32_t collection_id = 0,
          polygon_id = 0;  // last processed polygon
  work.gp.collection_len = 0;
  int32_t start_work = GP_MIN(threads_num, grid_len);

  GPPolygon canvas_outside_areas[8] = {0};
  exitcode = gp_canvas_outside_areas(canvas_width, canvas_height, &canvas_outside_areas[0]);
  if (exitcode != 0) {
    return exitcode;
  }
  work.gp.canvas_outside_areas = &canvas_outside_areas[0];

  int32_t current_collection_len = 0;  // collection len in polygons, not parts

  for (work.gp.current = 0; work.gp.current < pics_len; work.gp.current++) {
    work.gp.work_size = grid_len;
    next_work = start_work;
    gp_array_shuffle(work.gp.grid, &shuffle_buf, sizeof(GPVector), grid_len);

    for (size_t i = 0; i < start_work; i++) {
      int rc =
          pthread_create(&threads[i], NULL, _gp_generate_pattern_thrd, (void*)(threads_data + i));
      if (rc != 0) {
        return GP_ERR_THREADING;
      }
    }
    for (size_t i = 0; i < threads_num; i++) {
      if (pthread_join(threads[i], NULL) != 0) {
        fprintf(stderr, "Failed to join thread %zu\n", i);
      }
    }

    for (; polygon_id < *out_len; polygon_id++) {
      (*out)[polygon_id].collection_idx = collection_id;
      (*out)[polygon_id].idx = current_collection_len;
      (*out)[polygon_id].offset.x = work.gp.out_polygons[polygon_id].base_offset.x;
      (*out)[polygon_id].offset.y = work.gp.out_polygons[polygon_id].base_offset.y;
    }

    current_collection_len++;
    if (current_collection_len == collections_sizes[collection_id]) {
      work.gp.collection += work.gp.collection_len;
      collection_id++;
      work.gp.collection_len = 0;
      current_collection_len = 0;
    }
  }

  for (int32_t i = 0; i < pics_len; i++) {
    gp_polygon_free(&work.gp.polygons[i]);
  }
  for (size_t i = 0; i < threads_num; i++) {
    for (int32_t j = 0; j < 5 * GP_HOOKE_CACHE_SIZE; j++) {
      gp_polygon_free(&work.gp.polygon_buffers[i][j]);
    }
    gp_aligned_free(work.gp.polygon_buffers[i]);
  }
  for (int32_t i = 0; i < 8; i++) {
    gp_polygon_free(&canvas_outside_areas[i]);
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

#define COL1 30
#define COL2 20

const uint8_t image1[] = {
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
};

const uint8_t image2[] = {
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF,
};

int main() {
  gp_init();

  GPResult* result = NULL;
  size_t result_len = 0;

  int32_t collections_len = 2;
  int32_t collections_sizes[2] = {COL1, COL2};
  int32_t canvas_width = 100;
  int32_t canvas_height = 100;
  uint8_t t = 32;
  int32_t resolution = 10;
  int32_t min_dist = 10;
  uint32_t initial_step = 50;

  GPImgAlpha* alphas = malloc(sizeof(GPImgAlpha) * (COL1 + COL2));
  GP_CHECK_ALLOC(alphas);
  for (int32_t i = 0; i < COL1; i++) {
    alphas[i].data = image1;
    alphas[i].height = 16;
    alphas[i].width = 8;
  }
  for (int32_t i = COL1; i < COL1 + COL2; i++) {
    alphas[i].data = image2;
    alphas[i].height = 16;
    alphas[i].width = 8;
  }

  gp_genpattern(alphas, collections_sizes, collections_len, canvas_width, canvas_height, t,
                min_dist, resolution, initial_step, 1, &result, &result_len);

  free(alphas);

  for (int32_t i = 0; i < result_len; i++) {
    printf("Collection: %u\nIndex: %u\nOffset: (%f, %f)\n\n", result[i].collection_idx,
           result[i].idx, result[i].offset.x, result[i].offset.y);
  }

  /*

  GPDList *list1, *list2;
  gp_dllist_alloc(&list1);
  gp_dllist_alloc(&list2);

  GPPoint points[] = {
      {60.000000, 1047.000000},  {60.000000, 1019.000000},  {61.000000, 1016.000000},  {63.000000,
1012.000000}, {68.000000, 1007.000000},  {71.000000, 1005.000000},  {79.000000, 1001.000000},
{130.000000, 977.000000}, {207.000000, 941.000000},  {237.000000, 927.000000},  {250.000000,
921.000000},  {253.000000, 920.000000}, {263.000000, 917.000000},  {271.000000, 917.000000},
{275.000000, 918.000000},  {277.000000, 919.000000}, {283.000000, 925.000000},  {285.000000,
928.000000},  {293.000000, 943.000000},  {294.000000, 945.000000}, {296.000000, 951.000000},
{296.000000, 962.000000},  {282.000000, 1032.000000}, {263.000000, 1126.000000}, {262.000000,
1129.000000}, {261.000000, 1131.000000}, {258.000000, 1135.000000}, {256.000000, 1137.000000},
      {253.000000, 1139.000000}, {251.000000, 1140.000000}, {248.000000, 1141.000000}, {235.000000,
1141.000000}, {227.000000, 1139.000000}, {216.000000, 1134.000000}, {204.000000, 1128.000000},
{73.000000, 1062.000000}, {68.000000, 1059.000000},  {63.000000, 1054.000000},  {61.000000,
1050.000000}
  };

  GPPoint points2[] = {
      {0.000000, 1000.000000}, {0.000000, 0.000000}, {1000.000000, 0.000000}, {1000.000000,
1000.000000}};

  for (int32_t i = sizeof(points) / sizeof(GPPoint) - 1; i >= 0; i--) {
    gp_dllist_push(list1, points[i]);
    //gp_dllist_push(list2, (GPPoint){points[i].x + x_off, points[i].y + y_off});
  }
  for (int32_t i = sizeof(points2) / sizeof(GPPoint) - 1; i >= 0; i--) {
    gp_dllist_push(list2, points2[i]);
    // gp_dllist_push(list2, (GPPoint){points[i].x + x_off, points[i].y + y_off});
  }

  GPPolygon polygon1, polygon2, canvas;
  GPPolygon corners[8];
  gp_canvas_outside_areas(1000, 1000, corners);

  gp_dllist_to_polygon(list1, &polygon1);
  gp_polygon_translate(&polygon1, &polygon1, -120, 0);

  //gp_debug_polygons(&polygon1, 1);
  gp_dllist_to_polygon(list1, &polygon2);
  gp_polygon_translate(&polygon2, &polygon2, 200, 450);

  //gp_debug_polygons(&polygon2, 1);
  gp_dllist_to_polygon(list2, &canvas);

  gp_dllist_free(list1);
  gp_dllist_free(list2);

  bool intersected = false;

  gp_float res;
  GPPolygon pol;
  gp_polygon_init_empty(&pol, 175);
  gp_convex_intersection_area(&polygon1, &corners[0], &intersected, NULL, &pol);
  printf("%f\n", res); */
}
