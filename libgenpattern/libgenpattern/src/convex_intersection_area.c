/*
Algorithm:
  Joseph O'Rourke,
  Computational geometry in C (2nd ed.),
  1998,
  Cambridge University Press,
  Pages 252-262,
  ISBN: 978-0-521-64976-6

Implementation by Arkadii Chekha, 2023
Modified for an intersection area computation and handling polygon includes polygon cases.
*/

#include <math.h>
#include <stdbool.h>

#include "convex_area.h"
#include "convex_intersection_area.h"
#include "misc.h"

typedef enum { GP_IN_UNKNOWN, GP_IN_P, GP_IN_Q } GPInFlag;

static inline float _gp_area_sign(const GPPoint p1, const GPPoint p2, const GPPoint p3) {
  return (p3.x - p2.x) * (p2.y - p1.y) - (p2.x - p1.x) * (p3.y - p2.y);
}  // TODO common

// Formula from https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection#Given_two_points_on_each_line_segment
static inline bool _gp_line_segment_intersection(GPPoint p1, GPPoint p2, GPPoint p3, GPPoint p4, GPPoint* res) {
  float x1_x2 = p1.x - p2.x, y3_y4 = p3.y - p4.y, y1_y2 = p1.y - p2.y, x3_x4 = p3.x - p4.x, x1_x3 = p1.x - p3.x,
        y1_y3 = p1.y - p3.y;

  float denominator = x1_x2 * y3_y4 - y1_y2 * x3_x4;
  if (fabsf(denominator) < EPSILON) {
    return false;
  }

  float t = (x1_x3 * y3_y4 - y1_y3 * x3_x4) / denominator;
  float u = (x1_x3 * y1_y2 - y1_y3 * x1_x2) / denominator;

  if (0.0f <= t && t <= 1.0f && 0.0f <= u && u <= 1.0f) {
    res->x = p1.x + t * (p2.x - p1.x);
    res->y = p1.y + t * (p2.y - p1.y);
    return true;
  }

  return false;
}

static inline GPInFlag _gp_in_out(GPPoint p, GPInFlag inflag, float a_hb, float b_ha, GPPoint* last_point, float* res) {
  //printf("(%f,%f),", p.x, p.y);
  *res += (last_point->x * p.y) - (last_point->y * p.x);
  last_point->x = p.x;
  last_point->y = p.y;
  if (a_hb > 0) {
    return GP_IN_P;
  }
  if (b_ha > 0) {
    return GP_IN_Q;
  }
  return inflag;
}

static inline int32_t _gp_advance(int32_t a,
                                  int32_t* aa,
                                  GPPolygon* polygon,
                                  bool inside,
                                  GPPoint* last_point,
                                  float* res) {
  if (inside) {
    GPPoint v = GP_POLYGON_POINT(polygon, a);
    *res += (last_point->x * v.y) - (last_point->y * v.x);
    last_point->x = v.x;
    last_point->y = v.y;
    //printf("(%f,%f),", v.x, v.y);
  }
  (*aa)++;
  return GP_NEXT_IDX(polygon, a);
}

float gp_convex_intersection_area(GPPolygon* polygon1, GPPolygon* polygon2, bool* intersected) {
  float res = 0.0f;
  if (!gp_boxes_intersect(polygon1->bounds, polygon2->bounds)) {
    *intersected = false;
    return res;
  }
  int32_t points_size = 0;
  GPInFlag inflag = GP_IN_UNKNOWN;
  bool first_point = true;
  int32_t a = 0, b = 0, aa = 0, ba = 0;
  GPPoint last_point = {.x = 0.f, .y = 0.f}, p0 = {.x = 0.f, .y = 0.f}, p = {.x = 0.f, .y = 0.f};

  bool p_in_q = true;
  bool q_in_p = true;
  int32_t pq_last = 0;
  int32_t qp_last = 0;

  do {
    GPPoint a0 = GP_POLYGON_POINT(polygon1, a);
    GPPoint b0 = GP_POLYGON_POINT(polygon2, b);
    GPPoint a1 = GP_POLYGON_POINT(polygon1, GP_PREV_IDX(polygon1, a));
    GPPoint b1 = GP_POLYGON_POINT(polygon2, GP_PREV_IDX(polygon2, b));

    GPVector v_a = {.x = a1.x - a0.x, .y = a1.y - a0.y};
    GPVector v_b = {.x = b1.x - b0.x, .y = b1.y - b0.y};

    float cross = _gp_area_sign((GPPoint){.x = 0.f, .y = 0.f}, v_a, v_b);
    float b_ha = _gp_area_sign(b1, b0, a0);
    if (b_ha > 0.f) {
      p_in_q = false;
    }
    float a_hb = _gp_area_sign(a1, a0, b0);
    if (a_hb > 0.f) {
      q_in_p = false;
    }

    bool do_intersect = _gp_line_segment_intersection(a0, a1, b0, b1, &p);

    if (do_intersect) {
      if (inflag == GP_IN_UNKNOWN && first_point) {
        aa = 0;
        ba = 0;
        first_point = false;
        last_point.x = p.x;
        last_point.y = p.y;
        p0.x = p.x;
        p0.y = p.y;
        //printf("(%f,%f),", p0.x, p0.y);
      }
      inflag = _gp_in_out(p, inflag, a_hb, b_ha, &last_point, &res);
    }

    if (fabsf(cross) < EPSILON && fabsf(a_hb) < EPSILON && fabsf(b_ha) < EPSILON) {
      if (inflag == GP_IN_P) {
        b = _gp_advance(b, &ba, polygon2, inflag == GP_IN_Q, &last_point, &res);
      } else {
        a = _gp_advance(a, &aa, polygon1, inflag == GP_IN_P, &last_point, &res);
      }
    } else if (cross >= 0.f) {
      if (b_ha > 0.f) {
        a = _gp_advance(a, &aa, polygon1, inflag == GP_IN_P, &last_point, &res);
      } else {
        b = _gp_advance(b, &ba, polygon2, inflag == GP_IN_Q, &last_point, &res);
      }
    } else {
      if (a_hb > 0.f) {
        b = _gp_advance(b, &ba, polygon2, inflag == GP_IN_Q, &last_point, &res);
      } else {
        a = _gp_advance(a, &aa, polygon1, inflag == GP_IN_P, &last_point, &res);
      }
    }
  } while ((aa < polygon1->size || ba < polygon2->size) && aa < 2 * polygon1->size && ba < 2 * polygon2->size);

  res += (last_point.x * p0.y) - (last_point.y * p0.x);
  *intersected = !first_point;

  if (res < EPSILON) {
    if (p_in_q) {
      return polygon1->area;
    }
    if (q_in_p) {
      return polygon2->area;
    }
  }

  return res * .5f;
}
