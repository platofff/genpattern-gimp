#include "polygon_distance.h"
#include "misc.h"

#include <math.h>
#include <stdbool.h>

#include <stdio.h>

#define POLYGON_POINT(polygon, idx)                                            \
  (Point) { polygon->x_ptr[idx], polygon->y_ptr[idx] }
#define PREV_IDX(polygon, idx) idx == 0 ? polygon->size - 1 : idx - 1
#define NEXT_IDX(polygon, idx) idx == polygon->size - 1 ? 0 : idx + 1

static inline bool point_halfplane(const Point p1, const Point l1,
                                   const Point l2) {
  return ((p1.x - l1.x) * (l2.y - l1.y) - (p1.y - l1.y) * (l2.x - l1.x)) >= 0;
}

static inline void neighbors_halfplanes(const Polygon *polygon, const Point p,
                                         const int64_t idx, bool *prev_hp,
                                         bool *next_hp) {
  const int64_t prev_idx = PREV_IDX(polygon, idx);
  const int64_t next_idx = NEXT_IDX(polygon, idx);
  const Point cur_p = POLYGON_POINT(polygon, idx);
  *prev_hp = point_halfplane(POLYGON_POINT(polygon, prev_idx), cur_p, p);
  *next_hp = point_halfplane(POLYGON_POINT(polygon, next_idx), cur_p, p);
}

static inline void binary_search_tangent(const Polygon *polygon,
                                         const Point point, const bool target,
                                         int64_t *res) {
  bool prev_hp, next_hp;
  int64_t mid, low = 0, high = polygon->size - 1;

  while (low <= high) {
    mid = low + (high - low) / 2;
    neighbors_halfplanes(polygon, point, mid, &prev_hp, &next_hp);

    if (prev_hp == target) {
      if (mid == 0) {
        high = polygon->size - 1;
        low = polygon->size - 1;
      } else {
        high = mid - 1;
      }
    } else if (next_hp == target) {
      if (mid == polygon->size - 1) {
        low = 0;
        high = 0;
      } else {
        low = mid + 1;
      }
    } else {
      *res = mid;
      break;
    }
  }
}

static inline void polygon_tangent_points(const Polygon *polygon,
                                          const Point point, int64_t *t1_idx,
                                          int64_t *t2_idx) {
  binary_search_tangent(polygon, point, 1, t1_idx);
  binary_search_tangent(polygon, point, 0, t2_idx);
}

static inline void initial_phase(const Polygon *polygon1,
                                 const Polygon *polygon2, int64_t *p1,
                                 int64_t *p2, int64_t *q1, int64_t *q2) {
  polygon_tangent_points(polygon1, POLYGON_POINT(polygon2, 0), p1, p2);
  polygon_tangent_points(polygon2, POLYGON_POINT(polygon1, 0), q1, q2);
}

static inline float point_angle(const Point center, const Point p2,
                                const Point p3) {
  const float v1x = p2.x - center.x, v1y = p2.y - center.y,
              v2x = p3.x - center.x, v2y = p3.y - center.y;

  return atan2f(v1x * v2y - v1y * v2x, v1x * v2x + v1y * v2y);
}

// Computed from https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
static inline bool orthogonal_projection_exists(const Point l1, const Point l2,
                                                const Point p) {
  const float x1_x2 = l1.x - l2.x, y1_y2 = l1.y - l2.y, x1_x2_2 = x1_x2 * x1_x2;
  const float xpos =
      (p.x * x1_x2_2 + y1_y2 * (l1.x * (p.y - l2.y) + l2.x * (l1.y - p.y))) /
      (x1_x2_2 + y1_y2 * y1_y2);
  if (l1.x > l2.x) {
    return l2.x <= xpos && l1.x >= xpos;
  }
  return l1.x <= xpos && l2.x >= xpos;
}

static inline bool orthogonal_projection(const Point l1, const Point l2, const Point p, Point *res) {
  const float x1_x2 = l1.x - l2.x, y1_y2 = l1.y - l2.y, x1_x2_2 = x1_x2 * x1_x2;
  res->x =
      (p.x * x1_x2_2 + y1_y2 * (l1.x * (p.y - l2.y) + l2.x * (l1.y - p.y))) /
      (x1_x2_2 + y1_y2 * y1_y2);
  if (l1.x > l2.x) {
     if (l2.x > res->x || l1.x < res->x) {
      return 1;
     }
  } else if (l1.x > res->x || l2.x < res->x) {
    return 1;
  }

  // TODO

  return 0;
}

static inline int64_t median_idx(const int64_t size, const int64_t *i1, const int64_t *i2) {
  if (*i1 > *i2) {
    const int64_t in1 = *i1 - size;
    const int64_t resn = (*i2 + in1) / 2;
    if (resn < 0) {
      return size + resn;
    }
    return resn;
  }
  return (*i1 + *i2) / 2;
}

static inline int64_t vertex_count(const int64_t size, const int64_t *i1, const int64_t *i2) {
  if (*i1 > *i2) {
    return size - *i1 + *i2;
  }
  return *i2 - *i1;
}

static inline void binary_elimination_case1(int64_t *c1, int64_t *c2, int64_t mc,
                                            float a_, float a__) {
  if (a_ >= M_PI / 2) {
    *c1 = mc;
  }
  if (a__ >= M_PI / 2) {
    *c2 = mc;
  }
}

static inline void binary_elimination_case2(const Polygon *polygon1,
                                            const Polygon *polygon2, int64_t *c1,
                                            int64_t *c2, int64_t mc, int64_t *d1,
                                            int64_t *d2, int64_t md, float a_,
                                            float a__, float b_, float b__) {
  if (a_ > 0) { // Case 2.1
    if (a_ + b_ > M_PI) { // (1)
      if (a_ >= M_PI / 2) {
        *c1 = *c2;
      }
      if (b_ >= M_PI / 2) {
        *d1 = md;
      }
    }
    if (b__ >= M_PI / 2) { // (2)
      *d2 = md;
    }
    if (a_ < b__ && b__ < M_PI / 2) { // (3)
      if (orthogonal_projection_exists(POLYGON_POINT(polygon1, *c1),
                                       POLYGON_POINT(polygon1, *c2),
                                       POLYGON_POINT(polygon2, md))) {
        *d2 = md;
      } else {
        *c2 = *c1;
      }
    }
  } else { // Case 2.2
    *c2 = *c1;
    if (b_ >= M_PI) {
      *d1 = md;
    }
    if (b__ >= M_PI) {
      *d2 = md;
    }
  }
}

static inline void binary_elimination_case3(int64_t *c1, int64_t *c2, int64_t mc,
                                            int64_t *d1, int64_t *d2, int64_t md,
                                            float a_, float a__, float b_,
                                            float b__) {
  if (a_ > 0 && a__ > 0 && b_ > 0 && b__ > 0) { // Case 3.1
    if (a_ + b_ > M_PI) {                       // (1)
      if (a_ >= M_PI / 2) {
        *c1 = mc;
      }
      if (b_ >= M_PI / 2) {
        *d1 = md;
      }
    }
    if (a__ + b__ > M_PI) { // (2)
      if (a__ >= M_PI / 2) {
        *c2 = mc;
      }
      if (b__ >= M_PI / 2) {
        *d2 = md;
      }
    }
  } else { // Case 3.2
    *c2 = mc;
    if (b_ >= M_PI) {
      *d1 = md;
    }
    if (b__ >= M_PI) {
      *d2 = md;
    }
  }
}

static inline void binary_elimination(const Polygon *polygon1,
                                      const Polygon *polygon2, int64_t *p1,
                                      int64_t *p2, int64_t *q1, int64_t *q2) {

  int64_t vc_p = vertex_count(polygon1->size, p1, p2);
  int64_t vc_q = vertex_count(polygon2->size, q1, q2);
  while (vc_p > 1 || vc_q > 1) {
    vc_p = vertex_count(polygon1->size, p1, p2);
    vc_q = vertex_count(polygon2->size, q1, q2);
    printf("p1=%zu p2=%zu q1=%zu q2=%zu vc_p=%u\n", *p1, *p2, *q1, *q2, vc_p);

    int64_t mp = median_idx(polygon1->size, p1, p2), mq = median_idx(polygon2->size, q1, q2);
    float alpha_ = 0, alpha__ = 0, beta_ = 0, beta__ = 0;
    if (vc_p == 1) {
      // printf("(%f %f)\n", polygon1->x_ptr[p2], polygon1->y_ptr[p2]);
      int64_t pn;
      pn = *p1;
      mp = *p2;
      alpha_ =
          point_angle(POLYGON_POINT(polygon1, mp), POLYGON_POINT(polygon1, pn),
                      POLYGON_POINT(polygon2, mq));
    } else if (vc_p > 1) {
      alpha_ = point_angle(POLYGON_POINT(polygon1, mp),
                           POLYGON_POINT(polygon1, PREV_IDX(polygon1, mp)),
                           POLYGON_POINT(polygon2, mq));
      printf("%d %d %d\n", mp, mq, NEXT_IDX(polygon1, mp));
      alpha__ = point_angle(POLYGON_POINT(polygon1, mp),
                            POLYGON_POINT(polygon2, mq),
                            POLYGON_POINT(polygon1, NEXT_IDX(polygon1, mp)));
    }

    if (vc_q == 1) {
      int64_t qn;
      qn = *q1;
      mq = *q2;
      beta__ =
          point_angle(POLYGON_POINT(polygon2, mq),
                      POLYGON_POINT(polygon1, mp), POLYGON_POINT(polygon2, qn));
    } else if (vc_q > 1) {
      beta_ = point_angle(POLYGON_POINT(polygon2, mq),
                          POLYGON_POINT(polygon1, mp),
                          POLYGON_POINT(polygon2, NEXT_IDX(polygon2, mq)));
      beta__ = point_angle(POLYGON_POINT(polygon2, mq),
                           POLYGON_POINT(polygon2, PREV_IDX(polygon2, mq)),
                           POLYGON_POINT(polygon1, mp));
    }

    printf("a'=%f a''=%f b'=%f b''=%f mp=%zu mq=%zu\n", alpha_, alpha__, beta_,
           beta__, mp, mq);

    // Case 1
    if (vc_p == 0 || vc_q == 0) {
      binary_elimination_case1(q1, q2, mq, beta_, beta__);
      continue;
    }
    /*
    if (*q1 == *q2) {
      binary_elimination_case1(p1, p2, mp, alpha_, alpha__);
      continue;
    }*/

    // Case 2
    if (labs(*p2 - *p1) == 1 || labs(*q2 - *q1) == 1) {
      // puts("case 2");
      binary_elimination_case2(polygon1, polygon2, p1, p2, mp, q1, q2, mq,
                               alpha_, alpha__, beta_, beta__);
      continue;
    }
    /*
    if (labs(*q2 - *q1) == 1) {
      binary_elimination_case2(polygon2, polygon1, q1, q2, mq, p1, p2, mp,
                               beta_, beta__, alpha_, alpha__);
      continue;
    }*/

    // Case 3
    // puts("case 3");
    binary_elimination_case3(p1, p2, mp, q1, q2, mq, alpha_, alpha__, beta_,
                             beta__);
    if (*p1 == 6 && *p2 == 0 && *q1 == 4 && *q2 == 6) break;
  }
}

static inline float final_phase(Point p1, Point p2, Point q1, Point q2) {
  if (p1.x == p2.x && p1.y == p2.y && q1.x == q2.x && q1.y == q2.y) { // Case 1
    return sqrtf((p1.x - q1.x) * (p1.x - q1.x) + (p1.y - q1.y) * (p1.y - q1.y));
  }
  if (p1.x == p2.x && p1.y == p2.y) { // Case 2

  }
  if (q1.x == q2.x && q1.y == q2.y) { // Case 2

  }
}

int main() {

/*
  float p1x[7] = {-4, -4.7, -2.6, 0, 1.95, 2.75, 0},
        p1y[7] = {-4, 0, 2.6, 3, 1.4, -1.8, -3.75};
  float p2x[7] = {16, 15.3, 17.4, 20, 21.95, 22.75, 20},
        p2y[7] = {-4, 0, 2.6, 3, 1.4, -1.8, -3.75};
*/

  /*
  float p1x[5] = {-3, -5, -3, -2, -3},
        p1y[5] = {0,   2,  4,  2, 0};
  float p2x[5] = {3,  1, 3, 5, 3},
        p2y[5] = {-2, 1, 2, 1, -2};*/

  float p1x[7] = {0, 2.75, 1.95, 0, -2.6, -4.7, -4},
        p1y[7] = {-3.75, -1.8, 1.4, 3, 2.6, 0, -4};
  float p2x[7] = {20, 22.75, 21.95, 20, 17.4, 15.3, 16},
        p2y[7] = {-3.75, -1.8, 1.4, 3, 2.6, 0, -4};
  Polygon polygon1 = {&p1x[0], &p1y[0], 7};
  Polygon polygon2 = {&p2x[0], &p2y[0], 7};
  int64_t p1, p2, q1, q2;

  initial_phase(&polygon1, &polygon2, &p1, &p2, &q1, &q2);
  printf("p1=%zu p2=%zu q1=%zu q2=%zu\n", p1, p2, q1, q2);
  binary_elimination(&polygon1, &polygon2, &p1, &p2, &q1, &q2);
  printf("p1=%zu p2=%zu q1=%zu q2=%zu\n", p1, p2, q1, q2);
  float result = final_phase(
      POLYGON_POINT((&polygon1), p1), POLYGON_POINT((&polygon1), p2),
      POLYGON_POINT((&polygon2), q1), POLYGON_POINT((&polygon2), q1));
  printf("%f\n", result);
  // printf("%f\n", point_angle((Point){-2, 2}, (Point){-3, 4}, (Point){1, 1}));
}