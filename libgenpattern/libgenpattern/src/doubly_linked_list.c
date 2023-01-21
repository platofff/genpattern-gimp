#include <math.h>

#include "doubly_linked_list.h"
#include "misc.h"
#include "polygon_translate.h"
#include "convex_area.h"

GPDLElement *gp_dllist_alloc(int32_t size) {
  GPDLElement *res = malloc(sizeof(GPDLElement) * (size + 1));
  GP_CHECK_ALLOC(res);
  res[0].next = NULL;
  res[0].prev = NULL;
  return res;
}

void gp_dllist_free(GPDLElement *el) {
  while (el->prev != NULL) {
    el = (GPDLElement *)el->prev;
  }
  free(el);
}

GPDLElement *gp_dllist_push(GPDLElement *el, GPPoint value) {
  while (el->next != NULL) {
    el = (GPDLElement *)el->next;
  }
  GPDLElement *next = el + 1;
  next->value.x = value.x;
  next->value.y = value.y;
  next->prev = el;
  next->next = NULL;
  el->next = next;
  return el;
}

GPDLElement *gp_dllist_pop(GPDLElement *el) {
  GPDLElement *next = (GPDLElement *)el->next, *prev = (GPDLElement *)el->prev;
  next->prev = prev;
  prev->next = next;
  return next;
}

void gp_dllist_dump(GPDLElement* el, GPPolygon* res) {
  res->size = 0;
  /*
  while (el->next != NULL) {
    el = (GPDLElement *)el->next;
  }
  while (el->prev != NULL) {
    res->size++;
    el = (GPDLElement *)el->prev;
  }*/
  while (el->prev != NULL) {
    el = (GPDLElement *)el->prev;
  }
  while (el->next != NULL) {
    res->size++;
    el = (GPDLElement *)el->next;
  }

  res->size--;

  res->x_ptr = aligned_alloc(32, sizeof(float) * res->size);
  GP_CHECK_ALLOC(res->x_ptr);
  res->y_ptr = aligned_alloc(32, sizeof(float) * res->size);
  GP_CHECK_ALLOC(res->y_ptr);
  res->bounds.xmin = INFINITY;
  res->bounds.ymin = INFINITY;
  res->bounds.xmax = -INFINITY;
  res->bounds.ymax = -INFINITY;
  for (size_t i = 0; i < res->size; i++) {
    el = el->prev;
    res->bounds.xmin = MIN(res->bounds.xmin, el->value.x);
    res->bounds.ymin = MIN(res->bounds.ymin, el->value.y);
    res->bounds.xmax = MAX(res->bounds.xmax, el->value.x);
    res->bounds.ymax = MAX(res->bounds.ymax, el->value.y);
    res->x_ptr[i] = el->value.x;
    res->y_ptr[i] = el->value.y;
  }

  res->base_offset.x = -res->bounds.xmin;
  res->base_offset.y = -res->bounds.ymin;
  gp_polygon_translate(res, res->base_offset.x, res->base_offset.y);

  res->area = gp_convex_area(res);
}

void gp_dllist_to_start(GPDLElement **el) {
  while (((GPDLElement *)(*el)->prev)->prev != NULL) {
    *el = (*el)->prev;
  }
}
