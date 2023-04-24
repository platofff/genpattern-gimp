/*
Algorithm:
  H Edelsbrunner,
  Computing the extreme distances between two convex polygons,
  Journal of Algorithms,
  Volume 6, Issue 2,
  1985,
  Pages 213-224,
  ISSN 0196-6774,
  https://doi.org/10.1016/0196-6774(85)90039-2.
  (https://www.sciencedirect.com/science/article/pii/0196677485900392)

Implementation by Arkadii Chekha, 2022
*/

#include "convex_distance.h"
#include "fibonacci.h"
#include "misc.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>


static inline float _gp_area_sign(const GPPoint p1, const GPPoint p2, const GPPoint p3) {
  float res = (p2.x - p1.x) * (p3.y - p2.y) - (p3.x - p2.x) * (p2.y - p1.y);
  return res;
}

static inline void _gp_neighbors_halfplanes(const GPPolygon* polygon,
                                            const GPPoint p,
                                            const int32_t idx,
                                            bool* prev_hp,
                                            bool* next_hp) {
  const int32_t prev_idx = GP_PREV_IDX(polygon, idx);
  const int32_t next_idx = GP_NEXT_IDX(polygon, idx);
  const GPPoint cur_p = GP_POLYGON_POINT(polygon, idx);
  float prev_sign = _gp_area_sign(p, cur_p, GP_POLYGON_POINT(polygon, prev_idx));
  float next_sign = _gp_area_sign(p, cur_p, GP_POLYGON_POINT(polygon, next_idx));

  if (fpclassify(prev_sign) == FP_ZERO) {
    prev_sign = next_sign;
  }
  if (fpclassify(next_sign) == FP_ZERO) {
    next_sign = prev_sign;
  }

  *prev_hp = prev_sign >= 0;
  *next_hp = next_sign >= 0;
}

static inline float _gp_vector_angle(const GPVector a, const GPVector b) {
  const float angle = atan2f(a.x * b.y - a.y * b.x, a.x * b.x + a.y * b.y);
  return fmodf(fmodf(angle, M_2PIF) + M_2PIF, M_2PIF);
}

static inline double _gp_vector_angle_basic(double ax, double ay, double bx, double by) {
  return atan2(ax * by - ay * bx, ax * bx + ay * by);
}

// An unimodal function optimization using Fibonacci search
// https://web.archive.org/web/20191201003031/http://mathfaculty.fullerton.edu/mathews/n2003/FibonacciSearchMod.html
static inline void _gp_search_tangent(const GPPolygon* polygon, const GPPoint point, const bool target, int32_t* res) {
  /*
  float prev_hp, next_hp;
  GPPoint prev_p, next_p, p;

  while (1) {
    for (int32_t i = 0; i < polygon->size; i++) {
      //_gp_neighbors_halfplanes(polygon, point, i, &prev_hp, &next_hp);
      prev_p = GP_POLYGON_POINT(polygon, GP_PREV_IDX(polygon, i)),
      next_p = GP_POLYGON_POINT(polygon, GP_NEXT_IDX(polygon, i)), p = GP_POLYGON_POINT(polygon, i);
      prev_hp = _gp_area_sign(prev_p, p, point);
      next_hp = _gp_area_sign(next_p, p, point);

      if (fpclassify(prev_hp) == FP_ZERO) {
        prev_hp = next_hp;
      }
      if (fpclassify(next_hp) == FP_ZERO) {
        next_hp = prev_hp;
      }

      if ((prev_hp >= 0 && next_hp >= 0 && target) || (next_hp < 0 && prev_hp < 0 && !target)) {
        *res = i;
        return;
      }
    }
    assert(false);
    puts("");
  }*/
  
  float a = 0.f;
  float b = polygon->size - 1;
  bool prev, next;

  _gp_neighbors_halfplanes(polygon, point, a, &prev, &next);
  if (prev == target && next == target) {
    *res = a;
    return;
  }
  _gp_neighbors_halfplanes(polygon, point, b, &prev, &next);
  if (prev == target && next == target) {
    *res = b;
    return;
  }

  float len = (b - a) * 2.f;
  int32_t n = -1, c, d;

  for (int32_t i = 0; i < (int32_t)(sizeof(gp_fibonacci_numbers) / sizeof(float)); i++) {
    if (gp_fibonacci_numbers[i] >= len) {
      n = i;
      break;
    }
  }
  assert(n != -1);

  int32_t max = n - 3;
  GPPoint p0 = GP_POLYGON_POINT(polygon, 0), pc, pd;
  double vec0x = p0.x - point.x;
  double vec0y = p0.y - point.y;
  double angle_c, angle_d, r, vec_cx, vec_cy, vec_dx, vec_dy;

  for (int32_t k = 1; k != max; k++) {
    r = gp_fibonacci_numbers[n - k - 1] / gp_fibonacci_numbers[n - k];
    c = round(a + (b - a) * (1.f - r));
    d = round(a + (b - a) * r);

    pc = GP_POLYGON_POINT(polygon, c);
    pd = GP_POLYGON_POINT(polygon, d);
    vec_cx = pc.x - point.x;
    vec_cy = pc.y - point.y;
    vec_dx = pd.x - point.x;
    vec_dy = pd.y - point.y;

    angle_c = _gp_vector_angle_basic(vec0x, vec0y, vec_cx, vec_cy);
    angle_d = _gp_vector_angle_basic(vec0x, vec0y, vec_dx, vec_dy);

    if (target) {
      if (angle_c <= angle_d) {
        b = d;
      } else {
        a = c;
      }
    } else {
      if (angle_c >= angle_d) {
        b = d;
      } else {
        a = c;
      }
    }
  }

  *res = a;
  /*
  int32_t astart = GP_PREV_IDX(polygon, (int32_t)a);
  int32_t bend = GP_NEXT_IDX(polygon, (int32_t)b);

  while (1) {
    for (int32_t i = astart; i != bend; i = GP_NEXT_IDX(polygon, i)) {
      _gp_neighbors_halfplanes(polygon, point, i, &prev, &next);
      //if (prev == target && next == target) {
      if (prev == next) {
        *res = i;
        return;
      }
    }
    puts("");
  }
  assert(false);
  */
}

static inline void _gp_polygon_tangent_points(const GPPolygon* polygon,
                                              const GPPoint point,
                                              int32_t* t1_idx,
                                              int32_t* t2_idx) {
  _gp_search_tangent(polygon, point, false, t1_idx);
  _gp_search_tangent(polygon, point, true, t2_idx);
}

static inline void _gp_initial_phase(const GPPolygon* polygon1,
                                     const GPPolygon* polygon2,
                                     int32_t* p1,
                                     int32_t* p2,
                                     int32_t* q1,
                                     int32_t* q2) {
  _gp_polygon_tangent_points(polygon1, GP_POLYGON_POINT(polygon2, 0), p1, p2);
  _gp_polygon_tangent_points(polygon2, GP_POLYGON_POINT(polygon1, 0), q2, q1);
}

static inline float _gp_point_to_line_segment_param(const GPPoint l1,
                                                    const GPPoint l2,
                                                    const GPPoint p,
                                                    float* c,
                                                    float* d) {
  const float a = p.x - l1.x, b = p.y - l1.y;
  *c = l2.x - l1.x, *d = l2.y - l1.y;
  const float dot = a * *c + b * *d;
  const float len_sq = *c * *c + *d * *d;
  return dot / len_sq;
}

/*
    Vector from l1 to p: a
    Vector from l2 to l2: b
    Orthogonal projection exists if projection of vector a on vector b is
    codirectional with b and its length is not bigger than length of b.
    Orthogonal projection of vector a on vector b ("*" - dot product):
    a * b
    ----- * b
    b * b

    So, we can convert this condition to:
          a * b
    0 <=  ----- <= 1
          b * b

    b * b is always >= 0, so to check if fraction value >= 0 we can simply check if a * b >= 0
    To check if fraction value <= 1 we should check if a * b <= b * b
*/
static inline bool _gp_orthogonal_projection_exists(const GPPoint l1, const GPPoint l2, const GPPoint p) {
  const float ax = p.x - l1.x, ay = p.y - l1.y, bx = l2.x - l1.x, by = l2.y - l1.y;
  const float dot = ax * bx + ay * by;
  const float len_sq = bx * bx + by * by;
  return dot <= len_sq && dot >= 0;
}

static inline float _gp_point_to_line_segment_distance(const GPPoint l1, const GPPoint l2, const GPPoint p) {
  float c, d;
  const float param = _gp_point_to_line_segment_param(l1, l2, p, &c, &d);

  float dx, dy;

  if (param < 0) {
    dx = p.x - l1.x;
    dy = p.y - l1.y;
  } else if (param > 1) {
    dx = p.x - l2.x;
    dy = p.y - l2.y;
  } else {
    dx = p.x - l1.x - param * c;
    dy = p.y - l1.y - param * d;
  }

  return sqrtf(dx * dx + dy * dy);
}

static inline int32_t _gp_median_idx(const int32_t size, const int32_t* i1, const int32_t* i2) {
  if (*i1 > *i2) {
    const int32_t in1 = *i1 - size;
    const int32_t resn = (*i2 + in1) / 2;
    if (resn < 0) {
      return size + resn;
    }
    return resn;
  }
  return (*i1 + *i2) / 2;
}

static inline int32_t _gp_vertex_count(const int32_t size, const int32_t* i1, const int32_t* i2) {
  if (*i1 > *i2) {
    return size - *i1 + *i2;
  }
  return *i2 - *i1;
}

static inline void _gp_binary_elimination_case1(int32_t* c1, int32_t* c2, int32_t mc, float b_, float b__) {
  if ((M_PI_2F - b_) < EPSILON) {
    *c1 = mc;
  }
  if ((M_PI_2F - b__) < EPSILON) {
    *c2 = mc;
  }
}

static inline void _gp_binary_elimination_case2(const GPPolygon* polygon1,
                                                const GPPolygon* polygon2,
                                                int32_t* c1,
                                                int32_t* c2,
                                                int32_t mc,
                                                int32_t* d1,
                                                int32_t* d2,
                                                int32_t md,
                                                float a_,
                                                float b_,
                                                float b__) {
  if (a_ > 0) {             // Case 2.1
    if (a_ + b_ > M_PIF) {  // (1)
      if ((M_PI_2F - a_) < EPSILON) {
        *c1 = *c2;
      }
      if ((M_PI_2F - b_) < EPSILON) {
        *d1 = md;
      }
    }
    if ((M_PI_2F - b__) < EPSILON) {  // (2)
      *d2 = md;
    }
    if ((a_ - b__) < EPSILON && b__ < M_PI_2F) {  // (3)
      if (_gp_orthogonal_projection_exists(GP_POLYGON_POINT(polygon1, *c1), GP_POLYGON_POINT(polygon1, *c2),
                                           GP_POLYGON_POINT(polygon2, md))) {
        *d2 = md;
      } else {
        *c2 = *c1;
      }
    }
  } else {  // Case 2.2
    *c2 = *c1;
    if ((M_PIF - b_) < EPSILON) {
      *d1 = md;
    }
    if ((M_PIF - b__) < EPSILON) {
      *d2 = md;
    }
  }
}

static inline void _gp_binary_elimination_case3(int32_t* p1,
                                                int32_t* p2,
                                                int32_t mp,
                                                int32_t* q1,
                                                int32_t* q2,
                                                int32_t mq,
                                                float a_,
                                                float a__,
                                                float b_,
                                                float b__) {
  if (a_ > 0 && a__ > 0 && b_ > 0 && b__ > 0) {  // Case 3.1
    if (a_ + b_ > M_PIF) {                       // (1)
      if ((M_PI_2F - a_) < EPSILON) {
        *p1 = mp;
      }
      if ((M_PI_2F - b_) < EPSILON) {
        *q1 = mq;
      }
    }
    if (a__ + b__ > M_PIF) {  // (2)
      if ((M_PI_2F - a__) < EPSILON) {
        *p2 = mp;
      }
      if ((M_PI_2F - b__) < EPSILON) {
        *q2 = mq;
      }
    }
  } else {  // Case 3.2
    if (a_ <= 0) {
      *p2 = mp;
      /*
      if ((M_PI_2F - b_) < EPSILON) {
        *q1 = mq;
      }
      if ((M_PI_2F - b__) < EPSILON) {
        *q2 = mq;
      }
      */
    }
    if (a__ <= 0) {
      *p1 = mp;
    }
    if (b_ <= 0) {
      *q2 = mq;
    }
    if (b__ <= 0) {
      *q1 = mq;
    }
  }
}

static inline void _gp_binary_elimination(const GPPolygon* polygon1,
                                          const GPPolygon* polygon2,
                                          int32_t* p1,
                                          int32_t* p2,
                                          int32_t* q1,
                                          int32_t* q2) {
  int32_t vc_p = _gp_vertex_count(polygon1->size, p1, p2);
  int32_t vc_q = _gp_vertex_count(polygon2->size, q2, q1);
  float alpha_, alpha__, beta_, beta__;
  while (vc_p > 1 || vc_q > 1) {
    vc_p = _gp_vertex_count(polygon1->size, p1, p2);
    vc_q = _gp_vertex_count(polygon2->size, q2, q1);

    int32_t mp = _gp_median_idx(polygon1->size, p1, p2), mq = _gp_median_idx(polygon2->size, q2, q1);

    if (vc_p == 1) {
      mp = *p2;
    }
    if (vc_q == 1) {
      mq = *q1;
    }

    GPPoint mp_point = GP_POLYGON_POINT(polygon1, mp);
    GPPoint mq_point = GP_POLYGON_POINT(polygon2, mq);
    GPPoint mp_prev_point = GP_POLYGON_POINT(polygon1, GP_PREV_IDX(polygon1, mp));
    GPPoint mq_prev_point = GP_POLYGON_POINT(polygon2, GP_PREV_IDX(polygon2, mq));
    GPPoint mp_next_point = GP_POLYGON_POINT(polygon1, GP_NEXT_IDX(polygon1, mp));
    GPPoint mq_next_point = GP_POLYGON_POINT(polygon2, GP_NEXT_IDX(polygon2, mq));
    GPVector m = {.x = mq_point.x - mp_point.x, .y = mq_point.y - mp_point.y};
    GPVector m_reversed = {.x = -m.x, .y = -m.y};

    GPVector e_prev = {.x = mp_prev_point.x - mp_point.x, mp_prev_point.y - mp_point.y};
    GPVector e_next = {.x = mp_next_point.x - mp_point.x, mp_next_point.y - mp_point.y};
    GPVector f_prev = {.x = mq_prev_point.x - mq_point.x, mq_prev_point.y - mq_point.y};
    GPVector f_next = {.x = mq_next_point.x - mq_point.x, mq_next_point.y - mq_point.y};

    alpha_ = _gp_vector_angle(e_prev, m);
    alpha__ = _gp_vector_angle(m, e_next);
    beta_ = _gp_vector_angle(m_reversed, f_next);
    beta__ = _gp_vector_angle(f_prev, m_reversed);

    // Case 1
    if (vc_p == 0) {
      _gp_binary_elimination_case1(q1, q2, mq, beta_, beta__);
      continue;
    }
    if (vc_q == 0) {
      _gp_binary_elimination_case1(p2, p1, mp, alpha__, alpha_);
      continue;
    }

    // Case 2
    if (vc_p == 1) {
      _gp_binary_elimination_case2(polygon1, polygon2, p1, p2, mp, q1, q2, mq, alpha_, beta_, beta__);
      continue;
    }
    if (vc_q == 1) {
      _gp_binary_elimination_case2(polygon2, polygon1, q2, q1, mq, p2, p1, mp, beta__, alpha__, alpha_);
      continue;
    }

    // Case 3
    _gp_binary_elimination_case3(p1, p2, mp, q1, q2, mq, alpha_, alpha__, beta_, beta__);
  }
}

static inline float _gp_final_phase(GPPoint p1, GPPoint p2, GPPoint q1, GPPoint q2) {
  if (p1.x == p2.x && p1.y == p2.y && q1.x == q2.x && q1.y == q2.y) {  // Case 1
    const float dx = p1.x - q1.x;
    const float dy = p1.y - q1.y;
    return sqrtf(dx * dx + dy * dy);
  }
  if (p1.x == p2.x && p1.y == p2.y) {  // Case 2
    return _gp_point_to_line_segment_distance(q1, q2, p1);
  }
  if (q1.x == q2.x && q1.y == q2.y) {  // Case 2
    return _gp_point_to_line_segment_distance(p1, p2, q1);
  }
  // Case 3
  float f[4];
  f[0] = _gp_point_to_line_segment_distance(q1, q2, p1);
  f[1] = _gp_point_to_line_segment_distance(q1, q2, p2);
  f[2] = _gp_point_to_line_segment_distance(p1, p2, q1);
  f[3] = _gp_point_to_line_segment_distance(p1, p2, q2);
  if (f[1] < f[0])
    f[0] = f[1];
  if (f[2] < f[0])
    f[0] = f[2];
  if (f[3] < f[0])
    f[0] = f[3];
  return f[0];
}

float gp_convex_distance(GPPolygon* polygon1, GPPolygon* polygon2) {
  int32_t p1, p2, q1, q2;

  _gp_initial_phase(polygon1, polygon2, &p1, &p2, &q1, &q2);

  _gp_binary_elimination(polygon1, polygon2, &p1, &p2, &q1, &q2);

  return _gp_final_phase(GP_POLYGON_POINT(polygon1, p1), GP_POLYGON_POINT(polygon1, p2), GP_POLYGON_POINT(polygon2, q1),
                         GP_POLYGON_POINT(polygon2, q2));
}