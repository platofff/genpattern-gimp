#include <math.h>
#include <stdio.h>

#include "doubly_linked_list.h"

DLElement *dllist_alloc(size_t size) {
  DLElement *res = malloc(sizeof(DLElement) * (size + 1));
  res[0].value.x = -INFINITY;
  res[0].next = (void *)(long)&res[1];
  res[0].prev = (void *)(long)&res[size - 1];
  for (size_t i = 1; i <= size; i++) {
    res[i].value.x = INFINITY;
    res[i].prev = (void *)(long)&res[i - 1];
    res[i].next = (void *)(long)&res[i + 1];
  }
  return res;
}

void dllist_free(DLElement *el) {
  while (el->value.x != -INFINITY) {
    el = (DLElement *)el->prev;
  }
  free(el);
}

DLElement *dllist_push(DLElement *el, Point value) {
  while (el->value.x != INFINITY) {
    el = (DLElement *)el->next;
  }
  el->value.x = value.x;
  el->value.y = value.y;
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
  while ((el->value).x != INFINITY) {
    el = (DLElement *)el->next;
  }
  while ((el->value).x != -INFINITY) {
    res->size++;
    el = (DLElement *)el->prev;
  }
  res->size--;

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
  while (((DLElement *)(*el)->prev)->value.x != -INFINITY) {
    *el = (*el)->prev;
  }
}

/*
int main() {
    DLElement *el = dllist_alloc(1024);
    el = dllist_push(el, (Point){17.0, 56.0});
    el = dllist_push(el, (Point){56.0, 43.0});
    el = dllist_push(el, (Point){43.0, 74.0});

    Polygon dump = dllist_dump(el);
    printf("size %zu\n", dump.size);
    for (size_t i = 0; i < dump.size; i++) {
        printf("el %zu = (%f, %f)\n", i, dump.x_ptr[i], dump.y_ptr[i]);
    }

    dllist_free(el);
}
*/