#include "basic_geometry.h"
#include <stdlib.h>

#ifdef _MSC_VER
#include <malloc.h>
#define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
#endif

/*
  Restrictions:
    dllist_push() after dllist_pop() is not always safe
    "size" is the maximal number of dllist_push() calls
    order is reversed
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
