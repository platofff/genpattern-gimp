#ifndef _GP_DOUBLY_LINKED_LIST_H
#define _GP_DOUBLY_LINKED_LIST_H 1

#include <stdlib.h>

#include "polygon.h"

/*
  Restrictions:
    gp_dllist_push() after gp_dllist_pop() is not always safe
    "size" is the maximal number of gp_dllist_push() calls
    order is reversed
*/

typedef struct {
  void* prev;
  void* next;
  GPPoint value;
} GPDLElement;

GPDLElement* gp_dllist_alloc(int32_t size);
void gp_dllist_free(GPDLElement* el);
GPDLElement* gp_dllist_push(GPDLElement* el, GPPoint value);
GPDLElement* gp_dllist_pop(GPDLElement* el);  // returns next
void gp_dllist_dump(GPDLElement* el, GPPolygon* res);
void gp_dllist_to_start(GPDLElement** el);

#endif
