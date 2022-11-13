#include "polygon_distance.h"

#include <math.h>
#include <stdbool.h>

#include <stdio.h>

static inline bool point_halfplane(const Point p1, const Point l1,
                                   const Point l2) {
  //printf("(%f %f) - (%f %f). (%f, %f)\n", l1.x, l1.y, l2.x, l2.y, p1.x, p1.y);
  return ((p1.x - l1.x) * (l2.y - l1.y) - (p1.y - l1.y) * (l2.x - l1.x)) > 0;
}

static inline void neightbours_halfplanes(Polygon *polygon, const Point p,
                                          const size_t idx, bool *prev_hp,
                                          bool *next_hp) {
  const size_t prev_idx = idx == 0 ? polygon->size - 1 : idx - 1;
  const size_t next_idx = idx == polygon->size - 1 ? 0 : idx + 1;
  //printf("%zu %zu %zu\n", prev_idx, idx, next_idx);
  *prev_hp = point_halfplane(
      (Point){polygon->x_ptr[prev_idx], polygon->y_ptr[prev_idx]},
      (Point){polygon->x_ptr[idx], polygon->y_ptr[idx]}, p);
  *next_hp = point_halfplane(
      (Point){polygon->x_ptr[next_idx], polygon->y_ptr[next_idx]},
      (Point){polygon->x_ptr[idx], polygon->y_ptr[idx]}, p);
}

static inline void
polygon_tangent_points(Polygon *polygon, const Point point, size_t *t1_idx,
                       size_t *t2_idx) { // for clockwise polygon
  size_t low = 0, high = polygon->size - 1, mid;
  bool prev_hp, next_hp, target;

  while (low <= high) {
    mid = low + (high - low) / 2;
    neightbours_halfplanes(polygon, point, mid, &prev_hp, &next_hp);

    if (prev_hp == 1) {
      if (mid == 0) {
        high = polygon->size - 1;
        low = polygon->size - 1;
      } else {
        high = mid - 1;
      }
    } else if (next_hp == 1) {
      if (mid == polygon->size - 1) {
        low = 0;
        high = 0;
      } else {
        low = mid + 1;
      }
    } else {
      *t1_idx = mid;
      break;
    }
  }

c:
  low = 0;
  high = polygon->size - 1;

  while (low <= high) {
    mid = low + (high - low) / 2;
    neightbours_halfplanes(polygon, point, mid, &prev_hp, &next_hp);
    if (prev_hp == 0) {
      if (mid == 0) {
        high = polygon->size - 1;
        low = polygon->size - 1;
      } else {
        high = mid - 1;
      }
    } else if (next_hp == 0) {
      if (mid == polygon->size - 1) {
        low = 0;
        high = 0;
      } else {
        low = mid + 1;
      }
    } else {
      *t2_idx = mid;
      break;
    }
  }
}

static inline void initial_phase(Polygon *polygon1, Polygon *polygon2,
                                 size_t *p1, size_t *p2, size_t *q1,
                                 size_t *q2) {
  polygon_tangent_points(
      polygon1, (Point){polygon2->x_ptr[0], polygon2->y_ptr[0]}, p1, p2);
  polygon_tangent_points(
      polygon2, (Point){polygon1->x_ptr[0], polygon1->y_ptr[0]}, q1, q2);
}

int main() {

  float p1x[7] = {-4, -4.7, -2.6, 0, 1.95, 2.75, 0},
        p1y[7] = {-4, 0, 2.6, 3, 1.4, -1.8, -3.75};
  float p2x[7] = {16, 15.3, 17.4, 20, 21.95, 22.75, 20},
        p2y[7] = {-4, 0, 2.6, 3, 1.4, -1.8, -3.75};

  /*
  float p1x[7] = {0, 2.75, 1.95, 0, -2.6, -4.7, -4},
        p1y[7] = {-3.75, -1.8, 1.4, 3, 2.6, 0, -4};
  float p2x[7] = {20, 22.75, 21.95, 20, 17.4, 15.3, 16},
        p2y[7] = {-3.75, -1.8, 1.4, 3, 2.6, 0, -4};*/
  Polygon polygon1 = {&p1x[0], &p1y[0], 7};
  Polygon polygon2 = {&p2x[0], &p2y[0], 7};
  size_t p1, p2, q1, q2;
  initial_phase(&polygon1, &polygon2, &p1, &p2, &q1, &q2);
  printf("p1=%zu p2=%zu q1=%zu q2=%zu\n", p1, p2, q1, q2);
}