#ifndef _GP_DOUBLY_LINKED_LIST_H
#define _GP_DOUBLY_LINKED_LIST_H 1

#include <stdlib.h>

#include "polygon.h"

typedef struct GPDLElement {
  struct GPDLElement* prev;
  struct GPDLElement* next;
  GPPoint value;
} GPDLElement;

typedef struct {
  GPDLElement* start;
  GPDLElement* end;
  int32_t size;
} GPDList;

int gp_dllist_alloc(GPDList** res);
void gp_dllist_free(GPDList* list);
int gp_dllist_push(GPDList* list, GPPoint value);
GPDLElement* gp_dllist_pop(GPDList* list, GPDLElement* el);  // returns next
int gp_dllist_to_polygon(GPDList* list, GPPolygon* res);

#endif
