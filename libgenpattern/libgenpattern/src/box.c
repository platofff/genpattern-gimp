#include "box.h"

extern inline bool gp_boxes_intersect(GPBox box1, GPBox box2) {
  return box1.xmin <= box2.xmax && box1.xmax >= box2.xmin && box1.ymin <= box2.ymax && box1.ymax >= box2.ymin;
}