#include <math.h>

#include "doubly_linked_list.h"
#include "misc.h"
#include "polygon_translate.h"
#include "convex_area.h"

int gp_dllist_alloc(GPDList** res) {
  *res = calloc(1, sizeof(GPDList));
  GP_CHECK_ALLOC(*res);
  return 0;
}

void gp_dllist_free(GPDList* list) {
  GPDLElement* el = list->start;
  do {
    GPDLElement* next = el->next;
    free(el);
    el = next;
  } while (el != NULL);
  free(list);
}

 int gp_dllist_push(GPDList* list, GPPoint value) {
  GPDLElement* next = malloc(sizeof(GPDLElement));
  GP_CHECK_ALLOC(next);

  next->value.x = value.x;
  next->value.y = value.y;
  next->prev = list->end;
  next->next = NULL;
  if (list->end != NULL) {
    list->end->next = next;
  } else {
    list->start = next;
    list->end = next;
  }
  list->end = next;
  list->size++;
  return 0;
 }

GPDLElement *gp_dllist_pop(GPDList *list, GPDLElement *el) {
  GPDLElement *next = el->next, *prev = el->prev;
  next->prev = prev;
  prev->next = next;
  list->size--;
  if (list->start == el) {
    list->start = next;
  } else if (list->end == el) {
    list->end = prev;
  }
  free(el);
  return next;
}

int gp_dllist_to_polygon(GPDList* list, GPPolygon* res) {
  res->size = list->size - 1; // remove the last point as it's equal to the first one

  res->x_ptr = aligned_alloc(32, sizeof(float) * res->size);
  GP_CHECK_ALLOC(res->x_ptr);
  res->y_ptr = aligned_alloc(32, sizeof(float) * res->size);
  GP_CHECK_ALLOC(res->y_ptr);
  res->bounds.xmin = INFINITY;
  res->bounds.ymin = INFINITY;
  res->bounds.xmax = -INFINITY;
  res->bounds.ymax = -INFINITY;

  GPDLElement *el = list->end;
  for (size_t i = 0; i < res->size; i++) {
    res->bounds.xmin = MIN(res->bounds.xmin, el->value.x);
    res->bounds.ymin = MIN(res->bounds.ymin, el->value.y);
    res->bounds.xmax = MAX(res->bounds.xmax, el->value.x);
    res->bounds.ymax = MAX(res->bounds.ymax, el->value.y);
    res->x_ptr[i] = el->value.x;
    res->y_ptr[i] = el->value.y;
    el = el->prev;
  }

  res->base_offset.x = -res->bounds.xmin;
  res->base_offset.y = -res->bounds.ymin;
  gp_polygon_translate(res, res, res->base_offset.x, res->base_offset.y);

  res->area = gp_convex_area(res);

  return 0;
}

