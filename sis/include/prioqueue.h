
/* file %M% release %I% */
/* last modified: %G% at %U% */
typedef struct {
  char *VPtr;
  int QPosn; /* there for historical reasons. Updated but not used anywhere */
} queue_entry_t;

typedef struct {
  int MaxQSize;
  int NumInQueue;
  IntFn QueueCmp;
  VoidFn print_entry;
  queue_entry_t **Queue;
  st_table *table; /* maps the external VPtr's to the corresponding
                      queue_entry_t * in Queue */
} queue_t;

extern queue_t *
    init_queue(/* int max_size; int (*cmp)(); void (*print_fn)() */);

extern void free_queue(/* queue_t *queue */);

extern void put_queue(/* queue_t *queue; char *ptr */);

extern char *get_queue(/* queue_t *queue */);

extern char *top_queue(/* queue_t *queue */);

extern void adj_queue(/* queue_t *queue; char *ptr */);

extern void adj_up_queue(/* queue_t *queue; char *ptr */);

extern void adj_down_queue(/* queue_t *queue; char *ptr */);

extern int queue_size(/* queue_t *queue */);

extern void print_queue(/* queue_t *queue */);

extern void check_queue(/* queue_t *queue */);
