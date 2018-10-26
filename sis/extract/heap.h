
/*  Author : Huey-Yih Wang */
/*  Date   : Nov 1. 1990   */
/*  Dynamic 2-heap data structure */

/*  Entry in the heap */
typedef struct heap_entry_struct heap_entry_t;
struct heap_entry_struct {
    int key;      /* Comparison key */
    char *item;   /* Item pointer */
};

/*  Heap structure */
typedef struct heap_struct heap_t;
struct heap_struct {
    int heapnum;           /* Currently the number of objects in the heap. */ 
    int heapsize;          /* Currently allowed size of the heap */
    heap_entry_t **tree;   /* Heap tree structure */
};

/* heap.c */
extern heap_entry_t *heap_entry_alloc();
extern void heap_entry_free();
extern heap_t *heap_alloc();
extern void heap_free();
extern heap_entry_t *findmax_heap();
extern void swap_entry();
extern void resize_heap();
extern void insert_heap();
extern heap_entry_t *deletemax_heap();
