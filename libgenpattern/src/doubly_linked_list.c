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

  res->x_ptr = malloc(sizeof(float) * res->size);
  res->y_ptr = malloc(sizeof(float) * res->size);
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

/*
int main() {
    DLElement *el = dllist_alloc(1024);
    el = dllist_push(el, (Point){17.0, 56.0});
    el = dllist_push(el, (Point){56.0, 43.0});
    DLElement *p = el;
    el = dllist_push(el, (Point){43.0, 74.0});
    dllist_pop(p);

    Polygon *dump = dllist_dump(el);
    printf("size %zu\n", dump->size);
    for (size_t i = 0; i < dump->size; i++) {
        printf("el %zu = (%f, %f)\n", i, dump->x_ptr[i], dump->y_ptr[i]);
    }

    dllist_free(el);

}
*/