#include "polygon_translate.h"

void gp_polygon_translate(GPPolygon* dst_polygon, GPPolygon* src_polygon, gp_float x_off_val, gp_float y_off_val) {
  for (int32_t i = 0; i < src_polygon->size; i++) {
    dst_polygon->x_ptr[i] = src_polygon->x_ptr[i] + x_off_val;
    dst_polygon->y_ptr[i] = src_polygon->y_ptr[i] + y_off_val;
  }
  dst_polygon->bounds.xmax = src_polygon->bounds.xmax + x_off_val;
  dst_polygon->bounds.xmin = src_polygon->bounds.xmin + x_off_val;
  dst_polygon->bounds.ymax = src_polygon->bounds.ymax + y_off_val;
  dst_polygon->bounds.ymin = src_polygon->bounds.ymin + y_off_val;
  dst_polygon->base_offset.x = src_polygon->base_offset.x + x_off_val;
  dst_polygon->base_offset.y = src_polygon->base_offset.y + y_off_val;
}

void gp_wrap_polygon_part(GPPolygon* polygon, int area_type, gp_float xres, gp_float yres) {
  gp_float x_off_val = 0;
  gp_float y_off_val = 0;

  switch (area_type) {
    case GP_AREA_LEFT:
      x_off_val = xres;
      break;
    case GP_AREA_TOP:
      y_off_val = -yres;
      break;
    case GP_AREA_BOTTOM:
      y_off_val = yres;
      break;
    case GP_AREA_RIGHT:
      x_off_val = -xres;
      break;
    case GP_AREA_TOP_LEFT:
      x_off_val = xres;
      y_off_val = -yres;
      break;
    case GP_AREA_TOP_RIGHT:
      x_off_val = -xres;
      y_off_val = -yres;
      break;
    case GP_AREA_BOTTOM_RIGHT:
      x_off_val = -xres;
      y_off_val = yres;
      break;
    case GP_AREA_BOTTOM_LEFT:
      x_off_val = xres;
      y_off_val = yres;
      break;
  }

  gp_polygon_translate(polygon, polygon, x_off_val, y_off_val);
}
