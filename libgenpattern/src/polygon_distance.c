#include "polygon_distance.h"
#include "misc.h"

#include <math.h>
#include <stdbool.h>

#include <stdio.h>

#define POLYGON_POINT(polygon, idx)                                            \
  (Point) { polygon->x_ptr[idx], polygon->y_ptr[idx] }
#define NEXT_IDX(polygon, idx) idx == 0 ? polygon->size - 1 : idx - 1
#define PREV_IDX(polygon, idx) idx == polygon->size - 1 ? 0 : idx + 1

static inline bool point_halfplane(const Point p1, const Point l1,
                                   const Point l2) {
  return ((p1.x - l1.x) * (l2.y - l1.y) - (p1.y - l1.y) * (l2.x - l1.x)) >= 0;
}

static inline void neighbours_halfplanes(const Polygon *polygon, const Point p,
                                          const size_t idx, bool *prev_hp,
                                          bool *next_hp) {
  const size_t prev_idx = NEXT_IDX(polygon, idx);
  const size_t next_idx = PREV_IDX(polygon, idx);
  const Point cur_p = POLYGON_POINT(polygon, idx);
  *prev_hp = point_halfplane(POLYGON_POINT(polygon, prev_idx), cur_p, p);
  *next_hp = point_halfplane(POLYGON_POINT(polygon, next_idx), cur_p, p);
}

static inline void binary_search_tangent(const Polygon *polygon,
                                         const Point point, const bool target,
                                         size_t *res) {
  bool prev_hp, next_hp;
  size_t mid, low = 0, high = polygon->size - 1;

  while (low <= high) {
    mid = low + (high - low) / 2;
    neighbours_halfplanes(polygon, point, mid, &prev_hp, &next_hp);

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
                                          const Point point, size_t *t1_idx,
                                          size_t *t2_idx) {
  binary_search_tangent(polygon, point, 0, t1_idx);
  binary_search_tangent(polygon, point, 1, t2_idx);
}

static inline void initial_phase(const Polygon *polygon1,
                                 const Polygon *polygon2, size_t *p1,
                                 size_t *p2, size_t *q1, size_t *q2) {
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
  const float x1_x2 = l1.x - l2.x;
  const float y1_y2 = l1.y - l2.y;
  const float xpos = (p.x * x1_x2 * x1_x2 +
                      y1_y2 * (l1.x * (p.y - l2.y) + l2.x * (l1.y - p.y))) /
                     (x1_x2 * x1_x2 + y1_y2 * y1_y2);
  const float xmin = MIN(l1.x, l2.x);
  const float xmax = MAX(l1.x, l2.x);
  return xmin <= xpos && xmax >= xpos;
}

static inline void binary_elimination_case1(size_t *c1, size_t *c2, size_t mc,
                                            float a_, float a__) {
  if (a_ >= M_PI / 2) {
    *c1 = mc;
  }
  if (a__ >= M_PI / 2) {
    *c2 = mc;
  }
}

static inline void binary_elimination_case2(const Polygon *polygon1,
                                            const Polygon *polygon2, size_t *c1,
                                            size_t *c2, size_t mc, size_t *d1,
                                            size_t *d2, size_t md, float a_,
                                            float a__, float b_, float b__) {
  if (a_ > 0) {           // Case 2.1
    printf("2.1\n");
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

static inline void binary_elimination_case3(size_t *c1, size_t *c2, size_t mc,
                                            size_t *d1, size_t *d2, size_t md,
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
                                      const Polygon *polygon2, size_t p1,
                                      size_t p2, size_t q1, size_t q2) {
  while (MAX(p1, p2) - MIN(p1, p2) > 1 || MAX(q2, q1) - MIN(q1, q2) > 1) {
    printf("p1=%zu p2=%zu q1=%zu q2=%zu\n", p1, p2, q1, q2);

    size_t mp = (p1 + p2) / 2, mq = (q2 + q1) / 2;
    float alpha_, alpha__, beta_, beta__;
    if (p2 - p1 == 1) {
      printf("(%f %f)\n", polygon1->x_ptr[p2], polygon1->y_ptr[p2]);
      //mp = p2;
      alpha_ =
          point_angle(POLYGON_POINT(polygon1, mp), POLYGON_POINT(polygon1, p2),
                      POLYGON_POINT(polygon2, mq));
    } else if (p1 - p2 > 1) {
      alpha_ = point_angle(POLYGON_POINT(polygon1, mp),
                           POLYGON_POINT(polygon1, PREV_IDX(polygon1, mp)),
                           POLYGON_POINT(polygon2, mq));
      alpha__ = point_angle(POLYGON_POINT(polygon1, mp),
                            POLYGON_POINT(polygon1, NEXT_IDX(polygon1, mp)),
                            POLYGON_POINT(polygon2, mq));
    }

    if (q2 - q1 == 1) {
      //mq = q2;
      beta_ =
          point_angle(POLYGON_POINT(polygon2, mq), POLYGON_POINT(polygon2, q2),
                      POLYGON_POINT(polygon1, mp));
    } else if (q1 - q2 > 1) {
      beta_ = point_angle(POLYGON_POINT(polygon2, mq),
                          POLYGON_POINT(polygon2, NEXT_IDX(polygon2, mq)),
                          POLYGON_POINT(polygon1, mp));
      beta__ = point_angle(POLYGON_POINT(polygon2, mq),
                           POLYGON_POINT(polygon2, PREV_IDX(polygon2, mq)),
                           POLYGON_POINT(polygon1, mp));
    }

    printf("a'=%f a''=%f b'=%f b''=%f mp=%zu mq=%zu\n", alpha_, alpha__, beta_, beta__, mp, mq);

    //if (p1 == 2 && p2 == 2 && q1 == 0 && q2 == 2) break;

    // Case 1
    if (p1 == p2) {
      puts("case 1");
      binary_elimination_case1(&p1, &p2, mp, beta_, beta__);
      continue; // ???
    }
    if (q1 == q2) {
      puts("case 1");
      binary_elimination_case1(&p1, &p2, mp, beta_, beta__);
      // binary_elimination_case1(&q1, &q2, mq, alpha_, alpha__);
      continue; // ???
    }

    // Case 2
    if (p2 - p1 == 1) {
      puts("case 2");
      binary_elimination_case2(polygon1, polygon2, &p1, &p2, mp, &q1, &q2, mq,
                               alpha_, alpha__, beta_, beta__);
      continue; // ???
    }
    if (q2 - q1 == 1) {
      puts("case 2");
      binary_elimination_case2(polygon1, polygon2, &p1, &p2, mp, &q1, &q2, mq,
                               alpha_, alpha__, beta_, beta__);
      // binary_elimination_case2(polygon2, polygon1, &q1, &q2, mq, &p1, &p2,
      // mp,
      //                          beta_, beta__, alpha_, alpha__);
      continue; // ???
    }

    puts("case 3");
    // Case 3
    binary_elimination_case3(&p1, &p2, mp, &q1, &q2, mq, alpha_, alpha__, beta_,
                             beta__);

  }
}

int main() {
  /*
  float p1x[8] = {-4, -4.7, -2.6, 0, 1.95, 2.75, 0, -4},
        p1y[8] = {-4, 0, 2.6, 3, 1.4, -1.8, -3.75, -4};
  float p2x[8] = {16, 15.3, 17.4, 20, 21.95, 22.75, 20, 16},
        p2y[8] = {-4, 0, 2.6, 3, 1.4, -1.8, -3.75, -4};*/
  
  float p1x[5] = {-3, -5, -3, -2, -3},
        p1y[5] = {0,   2,  4,  2, 0};
  float p2x[5] = {3,  1, 3, 5, 3},
        p2y[5] = {-2, 1, 2, 1, -2};

  /*
  float p1x[7] = {0, 2.75, 1.95, 0, -2.6, -4.7, -4},
        p1y[7] = {-3.75, -1.8, 1.4, 3, 2.6, 0, -4};
  float p2x[7] = {20, 22.75, 21.95, 20, 17.4, 15.3, 16},
        p2y[7] = {-3.75, -1.8, 1.4, 3, 2.6, 0, -4};*/
  Polygon polygon1 = {&p1x[0], &p1y[0], 4};
  Polygon polygon2 = {&p2x[0], &p2y[0], 4};
  size_t p1, p2, q1, q2;

  initial_phase(&polygon1, &polygon2, &p1, &p2, &q1, &q2);
  printf("p1=%zu p2=%zu q1=%zu q2=%zu\n", p1, p2, q1, q2);
  binary_elimination(&polygon1, &polygon2, p1, p2 == 0 ? polygon1.size: 0, q1, q2 == 0 ? polygon2.size : q2); // !!!
  //printf("p1=%zu p2=%zu q1=%zu q2=%zu\n", p1, p2, q1, q2);
  //printf("%f\n", point_angle((Point){-2, 2}, (Point){-3, 4}, (Point){1, 1}));
}