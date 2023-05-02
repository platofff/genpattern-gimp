#include <math.h>

#include "doubly_linked_list.h"
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
  int i_res = gp_polygon_init_empty(res, list->size - 1); // remove the last point as it's equal to the first one
  if (i_res != 0) {
    return i_res;
  }

  GPDLElement *el = list->end->prev;
  while (el != NULL) {
    gp_polygon_add_point(res, el->value);
    el = el->prev;
  }

  res->base_offset.x = -res->bounds.xmin;
  res->base_offset.y = -res->bounds.ymin;
  gp_polygon_translate(res, res, res->base_offset.x, res->base_offset.y);

  res->area = gp_convex_area(res);

  return 0;
}

