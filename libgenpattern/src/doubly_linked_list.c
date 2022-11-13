#include <stdio.h>

#include "doubly_linked_list.h"

DLElement *dllist_alloc(size_t size) {
  DLElement *res = malloc(sizeof(DLElement) * (size + 1));
  res[0].next = NULL;
  res[0].prev = NULL;
  return res;
}

void dllist_free(DLElement *el) {
  while (el->prev != NULL) {
    el = (DLElement *)el->prev;
  }
  free(el);
}

DLElement *dllist_push(DLElement *el, Point value) {
  while (el->next != NULL) {
    el = (DLElement *)el->next;
  }
  DLElement *next = el + 1;
  next->value.x = value.x;
  next->value.y = value.y;
  next->prev = el;
  next->next = NULL;
  el->next = next;
  return el;
}

DLElement *dllist_pop(DLElement *el) {
  DLElement *next = (DLElement *)el->next, *prev = (DLElement *)el->prev;
  next->prev = prev;
  prev->next = next;
  return next;
}

Polygon *dllist_dump(DLElement *el) {
  Polygon *res = malloc(sizeof(Polygon));
  res->size = 0;
  while (el->next != NULL) {
    el = (DLElement *)el->next;
  }
  while (el->prev != NULL) {
    res->size++;
    el = (DLElement *)el->prev;
  }

  res->x_ptr = aligned_alloc(32, sizeof(float) * res->size);
  res->y_ptr = aligned_alloc(32, sizeof(float) * res->size);
  for (size_t i = 0; i < res->size; i++) {
    el = el->next;
    res->x_ptr[i] = el->value.x;
    res->y_ptr[i] = el->value.y;
  }

  return res;
}

void to_start(DLElement **el) {
  while (((DLElement *)(*el)->prev)->prev != NULL) {
    *el = (*el)->prev;
  }
}
