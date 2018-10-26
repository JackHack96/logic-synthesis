
#include "heap.h"
#include "sis.h"

/*  Author : Huey-Yih Wang */
/*  Date   : Nov 1. 1990   */
/*  Dynamic 2-heap data structure */

/* Allocate and free the entry of heap */
heap_entry_t *heap_entry_alloc() {
    register heap_entry_t *entry;

    entry = ALLOC(heap_entry_t, 1);
    entry->key  = -1;
    entry->item = NIL(char);
    return entry;
}

void heap_entry_free(heap_entry_t *entry) { FREE(entry); }

/* Allocate and free heap */
heap_t *heap_alloc() {
    int             i;
    int             size = 10;
    register heap_t *heap;

    heap = ALLOC(heap_t, 1);
    heap->heapnum  = 0;
    heap->heapsize = size;
    heap->tree     = ALLOC(heap_entry_t *, size);
    heap->tree[0] = heap_entry_alloc();
    heap->tree[0]->key = INFINITY;
    for (i = 1; i < size; i++) {
        heap->tree[i] = NIL(heap_entry_t);
    }
    return heap;
}

void heap_free(register heap_t *heap, void (*delFunc)()) {
    int i;

    for (i = 0; i <= heap->heapnum; i++) {
        if (heap->tree[i]->item)
            (*delFunc)(heap->tree[i]->item);
        (void) heap_entry_free(heap->tree[i]);
    }
    FREE(heap->tree);
    FREE(heap);
}

heap_entry_t *findmax_heap(register heap_t *heap) {
    if (heap->heapnum == 0) {
        return NIL(heap_entry_t);
    } else {
        return heap->tree[1];
    }
}

/* Swap entry s1 and s2 */
void swap_entry(heap_entry_t *s1, heap_entry_t *s2) {
    heap_entry_t temp;

    temp = *s1;
    *s1 = *s2;
    *s2 = temp;
}

/* Dynamically increase the size of heap */
void resize_heap(register heap_t *heap) {
    heap->heapsize *= 2;
    heap->tree = REALLOC(heap_entry_t *, heap->tree, heap->heapsize);
}

/* Insert a new entry in the heap */
void insert_heap(register heap_t *heap, register heap_entry_t *entry) {
    int current, parent;

    if ((++heap->heapnum) >= heap->heapsize)
        (void) resize_heap(heap);
    current = (heap->heapnum);
    heap->tree[current] = entry;
    parent = current / 2;
    while ((heap->tree[parent])->key < (heap->tree[current])->key) {
        swap_entry(heap->tree[parent], heap->tree[current]);
        current = parent;
        parent  = current / 2;
    }
}

/* Delete the topest in the heap and return this entry, if empty(heap) then
 * return NILL.
 */
heap_entry_t *deletemax_heap(register heap_t *heap) {
    int          current = 1;
    int          child   = 2;
    heap_entry_t *entry;

    if (heap->heapnum == 0)
        return NIL(heap_entry_t);
    entry = heap->tree[1];
    heap->tree[1] = heap->tree[(heap->heapnum)--];

    while (child <= heap->heapnum) {
        if ((child < heap->heapnum) &&
            (heap->tree[child + 1]->key > heap->tree[child]->key))
            child++;
        if (heap->tree[current]->key < heap->tree[child]->key) {
            swap_entry(heap->tree[current], heap->tree[child]);
            current = child;
            child   = 2 * current;
        } else {
            break;
        }
    }
    return entry;
}
