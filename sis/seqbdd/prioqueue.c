/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/seqbdd/prioqueue.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:54 $
 *
 */
 /* file %M% release %I% */
 /* last modified: %G% at %U% */
/*
 *	Code File	: prio.c
 *	Original Author	: Antony P-C Ng
 *	Created		: 11 Apr, 1990
 *
 *	Modified by	: Herve' Touati
 *			: June, 1990
 *
 ************************************************
 *		>> XPSim <<			*
 *						*
 *		Antony P-C Ng			*
 *		Robert K. Brayton		*
 *						*
 *		Computer Science Dept,		*
 *		573 Evans Hall,			*
 *		U.C. Berkeley,			*
 *		Berkeley, CA 94720.		*
 *						*
 ************************************************
 *
 *	Priority queue operations.
 *
 * A priority queue of all active vertex-instances is maintained.
 * The ordering is user-specified, thru a callback boolean function, QueueCmp.
 *
 * Creates a queue and returns a pointer to it.
 * Should give the max_size and a comparison function 
 * standard UNIX comparision function (e.g. as in qsort).
 * It returns an integer that is negative if V1 < V2,
 * positive if V1 > V2, and 0 iff V1 == V2 in the ordering.
 * The print_fn is used to print entries. NIL is OK there (nothing will be printed)
 *
 *
 *	queue_t	*init_queue(max_size, cmp, print_fn)
 *	int	max_size;
 *	int	(*cmp)();
 *      void    (*print_fn)();
 *
 * free the storage associated with a queue.
 *
 *      void free_queue(queue)
 *       queue_t *queue;
 *
 * inserts an pointer to an object into the priority queue.
 * (char *) is understood as generic pointer. The actual type
 * of the object should be the one expected by the cmp function.
 *
 *	void	put_queue(queue, ptr)
 *	queue_t *queue;
 *	char *ptr;
 *
 * gets a pointer to the object at the top of the queue
 * and pops it from the queue.
 *
 *	char *get_queue(queue)
 *	queue_t *queue;
 *
 * gets a pointer to the object at the top of the queue
 * but does not pop it from the queue.
 *
 *	char *top_queue(queue)
 *	queue_t *queue;
 *
 * adjusts the ordering of an object already in the queue.
 * makes no assumption about the direction of the adjustment.
 *
 *	void	adj_queue(queue, ptr)
 *	queue_t *queue;
 *	char *ptr;
 *
 * adjusts the ordering of an object already in the queue.
 * expects the priority of the object not to have decreased.
 * 
 *	void	adj_up_queue(queue, ptr)
 *	queue_t *queue;
 *	char *ptr;
 *
 * adjusts the ordering of an object already in the queue.
 * expects the priority of the object not to have increased.
 * 
 *	void	adj_up_queue(queue, ptr)
 *	queue_t *queue;
 *	char *ptr;
 *
 * returns the number of elements in the queue.
 *
 *	int	queue_size(queue)
 *	queue_t *queue;
 *
 */
#ifdef SIS
#include "sis.h"
#include "prioqueue.h"

static void SiftUp();
static void SiftDown();

/*
 * Allocate the heap-array. Note that the queue-pointer is decremented. This is
 * because Heap accesses start at 1.
 */

queue_t	*init_queue(max_size, cmp_fn, print_fn)
int max_size;
IntFn cmp_fn;
VoidFn print_fn;
{
  int i;
  queue_t *result = ALLOC(queue_t, 1);

  result->Queue = ALLOC(queue_entry_t *, max_size);
  for (i = 0; i < max_size; i++) {
    result->Queue[i] = NIL(queue_entry_t);
  }
  result->Queue--;
  result->MaxQSize = max_size;
  result->QueueCmp = cmp_fn;
  result->print_entry = print_fn;
  result->table = st_init_table(st_ptrcmp, st_ptrhash);
  result->NumInQueue = 0;
  return result;
}

 /* have to restore the Queue pointer to the ALLOC'ed address */

void free_queue(queue)
queue_t *queue;
{
  st_free_table(queue->table);
  queue->Queue++;
  FREE(queue->Queue);
  FREE(queue);
}

/*
 * Standard heap sifting routines.
 */

static void SiftUp(queue, SiftIndx )
queue_t *queue;
int SiftIndx;
{
  queue_entry_t *entry;
  int NextIndx;

  entry = queue->Queue[SiftIndx];
  while( SiftIndx > 1 ) {
    NextIndx = SiftIndx / 2;
    if((*(queue->QueueCmp))(entry->VPtr , queue->Queue[NextIndx]->VPtr) >= 0 ) break;
    queue->Queue[SiftIndx] = queue->Queue[NextIndx];
    queue->Queue[SiftIndx]->QPosn = SiftIndx;
    SiftIndx = NextIndx;
  }
  queue->Queue[SiftIndx] = entry;
  queue->Queue[SiftIndx]->QPosn = SiftIndx;
}

static void SiftDown(queue, SiftIndx)
queue_t *queue;
int SiftIndx;
{
  queue_entry_t *entry;
  int NextIndx;

  entry = queue->Queue[SiftIndx];
  while(( NextIndx = SiftIndx * 2 ) <= queue->NumInQueue ) {
    if(NextIndx < queue->NumInQueue && 
       (*(queue->QueueCmp))(queue->Queue[NextIndx+1]->VPtr, queue->Queue[NextIndx]->VPtr) < 0) {
      NextIndx++;
    }
    if((*(queue->QueueCmp))(entry->VPtr, queue->Queue[NextIndx]->VPtr) <= 0) break;
    queue->Queue[SiftIndx] = queue->Queue[NextIndx];
    queue->Queue[SiftIndx]->QPosn = SiftIndx;
    SiftIndx = NextIndx;
  }
  queue->Queue[SiftIndx] = entry;
  queue->Queue[SiftIndx]->QPosn = SiftIndx;
}


void put_queue(queue, VPtr)
queue_t *queue;
char *VPtr;
{
  queue_entry_t *new_entry;

  assert(queue->NumInQueue < queue->MaxQSize);
  queue->NumInQueue++;
  new_entry = ALLOC(queue_entry_t, 1);
  queue->Queue[queue->NumInQueue] = new_entry;
  new_entry->VPtr = VPtr;
  new_entry->QPosn = queue->NumInQueue;
  st_insert(queue->table, VPtr, (char *) new_entry);
  SiftUp(queue, queue->NumInQueue);
}

char *get_queue(queue)
queue_t *queue;
{
  char *VPtr;
  queue_entry_t *entry;

  if (queue->NumInQueue == 0) return NIL(char);
  VPtr = queue->Queue[1]->VPtr;
  (void) st_delete(queue->table, (char **) &VPtr, (char **) &entry);
  assert(entry == queue->Queue[1]);
  FREE(entry);
  queue->Queue[1] = queue->Queue[queue->NumInQueue];
  queue->NumInQueue--;
  if (queue->NumInQueue > 0) {
    queue->Queue[1]->QPosn = 1;
    SiftDown(queue, 1);
  }
  return VPtr;
}

char *top_queue(queue)
queue_t *queue;
{
  if (queue->NumInQueue == 0) return NIL(char);
  return queue->Queue[1]->VPtr;
}

void adj_queue(queue, VPtr)
queue_t *queue;
char *VPtr;
{
  queue_entry_t *entry;

  assert(st_lookup(queue->table, VPtr, (char **) &entry));
  SiftUp(queue, entry->QPosn);
  SiftDown(queue, entry->QPosn);
}

void adj_up_queue(queue, VPtr)
queue_t *queue;
char *VPtr;
{
  queue_entry_t *entry;

  assert(st_lookup(queue->table, VPtr, (char **) &entry));
  SiftUp(queue, entry->QPosn);
}

void adj_down_queue(queue, VPtr)
queue_t *queue;
char *VPtr;
{
  queue_entry_t *entry;

  assert(st_lookup(queue->table, VPtr, (char **) &entry));
  SiftDown(queue, entry->QPosn);
}

int queue_size(queue)
queue_t *queue;
{
  return queue->NumInQueue;
}

/* ------------------------------------------------------------------------- */

/*
 * Consistency Check on queues.
 */

/*
 * Display the queue.
 */

void print_queue(queue)
queue_t *queue;
{
  int	Indx , Stop;

  fprintf(misout, "priority queue\n\t");
  Stop = 2;
  for( Indx = 1 ; Indx <= queue->NumInQueue ; ++ Indx ) {
    if( Indx == Stop ) {
      fprintf(misout, "\n\t");
      Stop *= 2;
    }
    (*(queue->print_entry))(queue->Queue[Indx]->VPtr);
    fprintf(misout, " ");
  }
  fprintf(misout, "\n");
}

/*
 * Check queue for consistency. Returns 0 if everything is OK.
 */

int CheckQueue(queue)
queue_t *queue;
{
  int	Indx;
  int CheckFailed;
  st_generator *gen;
  char *VPtr;
  queue_entry_t *entry;

  CheckFailed = 0;

  /*
   * Check that Queue positions are correct
   */

  for( Indx = 1 ; Indx <= queue->NumInQueue ; ++ Indx ) {
    if( Indx != queue->Queue[Indx]->QPosn ) {
      fprintf(miserr, "\nQueue Position, Computed=%d, Stored=%d.", Indx , queue->Queue[Indx]->QPosn);
      CheckFailed = 1;
    }
  }

  /*
   * Check that the ordering relations are correct
   */

  for( Indx = 2 ; Indx <= queue->NumInQueue ; ++ Indx ) {
    if((*(queue->QueueCmp))(queue->Queue[Indx/2]->VPtr, queue->Queue[Indx]->VPtr) > 0) {
      (void) fprintf(misout, "\nQueue Ordering Mixup:" );
      (void) fprintf(misout, "\n\tPosn %3d, Entry ", Indx/2);
      (*(queue->print_entry))(queue->Queue[Indx/2]->VPtr);
      (void) fprintf(misout, "\n\tPosn %3d, Entry ", Indx);
      (*(queue->print_entry))(queue->Queue[Indx]->VPtr);
      (void) fprintf(misout, "\n");
      CheckFailed = 1;
    }
  }

 /* check that the entries have all the right index */
  if (st_count(queue->table) != queue->NumInQueue) {
    (void) fprintf(misout, "\nQueue Accounting Mixup: queue->%d, table->%d\n", queue->NumInQueue, st_count(queue->table));
    CheckFailed = 1;
  }
  st_foreach_item(queue->table, gen, &VPtr, (char **) &entry) {
    if (entry->VPtr != VPtr) {
      (void) fprintf(misout, "\nEntry Mixup: entry->0x%x, ptr->0x%x\n", entry->VPtr, VPtr);
      CheckFailed = 1;
    }
    if (entry->QPosn < 1 || entry->QPosn > queue->NumInQueue) {
      (void) fprintf(misout, "\nEntry Mixup: out of range: 1 <= %d <= %d\n", entry->QPosn, queue->NumInQueue);
      CheckFailed = 1;
    }
    if (queue->Queue[entry->QPosn] != entry) {
      (void) fprintf(misout, "\nEntry Mixup: entry->0x%x, actual->0x%x\n", entry, queue->Queue[entry->QPosn]);
      CheckFailed = 1;
    }
  }

  return(! CheckFailed);
}
#endif /* SIS */

