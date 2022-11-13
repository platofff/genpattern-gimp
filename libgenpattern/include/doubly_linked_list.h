#include "basic_geometry.h"
#include <stdlib.h>

/*
  Restrictions:
    dllist_push() after dllist_pop() is not always safe
    "size" is the maximal number of dllist_push() calls
*/

typedef struct {
  void *prev;
  void *next;
  Point value;
} DLElement;

DLElement *dllist_alloc(size_t size);
void dllist_free(DLElement *el);
DLElement *dllist_push(DLElement *el, Point value);
DLElement *dllist_pop(DLElement *el); // returns next
Polygon *dllist_dump(DLElement *el);
void to_start(DLElement **el);
