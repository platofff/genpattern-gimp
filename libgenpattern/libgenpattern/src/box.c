#include "box.h"

bool gp_boxes_intersect(GPBox box1, GPBox box2) {
  return box1.xmin <= box2.xmax && box1.xmax >= box2.xmin && box1.ymin <= box2.ymax && box1.ymax >= box2.ymin;
}

float gp_boxes_inner_distance(GPBox box1, GPBox box2) {
  float dist[4];
  dist[0] = box2.xmin - box1.xmin;
  dist[1] = box2.ymin - box1.ymin;
  dist[2] = box1.xmax - box2.xmax;
  dist[3] = box1.ymax - box2.ymax;
  if (dist[1] < dist[0])
    dist[0] = dist[1];
  if (dist[2] < dist[0])
    dist[0] = dist[2];
  if (dist[3] < dist[0])
    dist[0] = dist[3];
  return dist[0];
}