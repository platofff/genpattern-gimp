#include <math.h>
#include <assert.h>

#include "convex_distance.h"
#include "convex_intersection.h"
#include "misc.h"
#include "polygon_translate.h"
#include "suitability.h"

// !!!: TODO base_offset

float gp_suitability(GPSParams p) {
  int32_t parts_len = 1;
  for (int i = 0; i < 8; i++) {
    bool intersected;
    p.polygon_buffers[parts_len].size = 0;
    p.polygon_buffers[parts_len].bounds.xmin = -INFINITY;
    p.polygon_buffers[parts_len].bounds.ymin = -INFINITY;
    p.polygon_buffers[parts_len].bounds.xmax = INFINITY;
    p.polygon_buffers[parts_len].bounds.ymax = INFINITY;
    gp_convex_intersection(&p.canvas_outside_areas[i], &p.polygon_buffers[0], &intersected, NULL,
                           &p.polygon_buffers[parts_len]);
    if (intersected) {
      p.polygon_buffers[parts_len].base_offset.x = p.polygon_buffers[0].base_offset.x;
      p.polygon_buffers[parts_len].base_offset.y = p.polygon_buffers[0].base_offset.y;
      gp_wrap_polygon_part(&p.polygon_buffers[parts_len], i, p.canvas->bounds.xmax, p.canvas->bounds.ymax);
      parts_len++;  // TODO array boundary check?
      assert(parts_len < 5);
    }
  }

  bool intersected = false;
  float res = 0.f;

  for (int32_t j = 0; j < parts_len; j++) {
    for (int32_t i = 0; i < p.polygons_len; i++) {
      bool _intersected;
      float intersection_area;
      gp_convex_intersection(&p.polygon_buffers[j], &p.polygons[i], &_intersected, &intersection_area, NULL);
      if (_intersected) {
        if (!intersected) {
          intersected = true;
          res = 0.f;
        }
        res += intersection_area;
      }
    }
  }

  if (intersected) {
    return res;
  }

  res = -p.target;

  for (int32_t j = 0; j < parts_len; j++) {
    for (int32_t i = 0; i < p.collection_len; i++) {
      float dist = -gp_convex_distance(&p.polygon_buffers[j], &p.collection[i]);
      res = MAX(res, dist);
    }
  }

  return res;
}