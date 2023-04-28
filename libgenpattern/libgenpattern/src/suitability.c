#include <assert.h>
#include <math.h>

#include "convex_distance.h"
#include "convex_intersection.h"
#include "misc.h"
#include "polygon_translate.h"
#include "suitability.h"


float gp_suitability(GPSParams p) {
  int32_t parts_len = 1;

  GPPolygon* pb = p.polygon_buffers;

  for (int i = 0; i < 8; i++) {
    bool intersected;
    pb[parts_len].size = 0;
    pb[parts_len].bounds.xmin = INFINITY;
    pb[parts_len].bounds.ymin = INFINITY;
    pb[parts_len].bounds.xmax = -INFINITY;
    pb[parts_len].bounds.ymax = -INFINITY;
    gp_convex_intersection(&p.canvas_outside_areas[i], &pb[0], &intersected, NULL, &pb[parts_len]);
    if (intersected) {
      pb[parts_len].base_offset.x = pb[0].base_offset.x;
      pb[parts_len].base_offset.y = pb[0].base_offset.y;
      gp_wrap_polygon_part(&pb[parts_len], i, p.canvas->bounds.xmax, p.canvas->bounds.ymax);
      parts_len++;
      assert(parts_len <= 4);
    }
  }
  if (parts_len > 1) {
    gp_convex_intersection(p.canvas, &pb[0], NULL, NULL, &pb[parts_len]);
    pb++;
  }


  bool intersected = false;
  float res = 0.f;

  for (int32_t j = 0; j < parts_len; j++) {
    for (int32_t i = 0; i < p.polygons_len; i++) {
      bool _intersected;
      float intersection_area;
      gp_convex_intersection(&pb[j], &p.polygons[i], &_intersected, &intersection_area, NULL);
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
      float dist = -gp_convex_distance(&pb[j], &p.collection[i]);
      res = MAX(res, dist);
    }
  }

  return res;
}