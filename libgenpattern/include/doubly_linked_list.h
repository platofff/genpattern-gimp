#include <stdlib.h>
#include "basic_geometry.h"

/*
    -INFINITY - start element
     INFINITY - no data
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
