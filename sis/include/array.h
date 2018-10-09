/*
An array_t is a dynamically allocated array.  As elements are inserted
the array is automatically grown to accomodate the new elements.

The first element of the array is always element 0, and the last
element is element n-1 (if the array contains n elements).

This array package is intended for generic objects (i.e., an array of
int, array of char, array of double, array of struct foo *, or even
array of struct foo).

Be careful when creating an array with holes (i.e., when there is no
object stored at a particular position).  An attempt to read such a
position will return a 'zero' object.  It is poor practice to assume
that this value will be properly interpreted as, for example,  (double)
0.0 or (char *) 0.
 */
#ifndef ARRAY_H
#define ARRAY_H

typedef struct array_t {
  char *space;
  int num;      // number of array elements
  int n_size;   // size of 'data' array (in objects)
  int obj_size; // size of each array object.
  int index;    // combined index and locking flag.
} array_t;

array_t *array_do_alloc(int size, int number);

/**
 * Create a duplicate copy of an array.
 */
array_t *array_dup(array_t *old);

/**
 * Returns a new array which consists of the elements from array1
 * followed by the elements of array2.
 */
array_t *array_join(array_t *array1, array_t *array2);

/**
 * Deallocate an array.  Freeing the individual elements of the
 * array is the responsibility of the user.
 */
void array_free(array_t *array);

/**
 * Appends the elements of array2 to the end of array1.
 */
void array_append(array_t *array1, array_t *array2);

/**
 * Sort the elements of an array.  `compare' is defined as:
 * 	int
 * 		compare(obj1, obj2)
 * 		char *obj1;
 * 		char *obj2;
 *
 * 	and should return -1 if obj1 < obj2, 0 if obj1 == obj2, or 1
 * 	if obj1 > obj2.
 */
void array_sort(array_t *array, int (*compare)());

/**
 * Compare adjacent elements of the array, and delete any duplicates.
 * Usually the array should be sorted (using array_sort) before calling
 * array_uniq.  `compare' is defined as:
 *
 * 	int
 * 		compare(obj1, obj2)
 * 		char *obj1;
 * 		char *obj2;
 *
 * 	and returns -1 if obj1 < obj2, 0 if obj1 == obj2, or 1 if obj1 > obj2.
 *
 * 	`free_func' (if non-null) is defined as:
 * 	 void
 * 	    free_func(obj1)
 * 	    char *obj1;
 *
 *
 * and frees the given array element.
 */
void array_uniq(array_t *array, int (*compare)(), void (*free_func)());

int array_abort(array_t *a, int i);

int array_resize(array_t *array, int new_size);

char *array_do_data(array_t *array);

extern unsigned int array_global_index;

/**
 * Allocate and initialize an array of objects of type `type'.
 * Polymorphic arrays are okay as long as the type of largest
 * object is used for initialization.  The array can initially
 * hold `number' objects.  Typical use sets `number' to 0, and
 * allows the array to grow dynamically.
 */
#define array_alloc(type, number) array_do_alloc(sizeof(type), number)

/**
 * Insert a new element into an array at the given position.  The
 * array is dynamically extended if necessary to accomodate the
 * new item.  It is a serious error if sizeof(type) is not the
 * same as the type used when the array was allocated.  It is also
 * a serious error for 'position' to be less than zero.
 */
#define array_insert(type, a, i, datum)                                        \
  (-(a)->index != sizeof(type) ? array_abort((a), 4) : 0, (a)->index = (i),    \
   (a)->index < 0 ? array_abort((a), 0) : 0,                                   \
   (a)->index >= (a)->n_size ? array_resize(a, (a)->index + 1) : 0,            \
   *((type *)((a)->space + (a)->index * (a)->obj_size)) = datum,               \
   (a)->index >= (a)->num ? (a)->num = (a)->index + 1 : 0,                     \
   (a)->index = -(int)sizeof(type))

/**
 * Insert a new element at the end of the array.  Equivalent to:
 * array_insert(type, array, array_n(array), object).
 */
#define array_insert_last(type, array, datum)                                  \
  array_insert(type, array, (array)->num, datum)

/**
 * Fetch an element from an array.  A runtime error occurs on an
 * attempt to reference outside the bounds of the array.  There is
 * no type-checking that the value at the given position is
 * actually of the type used when dereferencing the array.
 */
#define array_fetch(type, a, i)                                                \
  (array_global_index = (i),                                                   \
   (array_global_index >= (a)->num) ? array_abort(a, 1) : 0,                   \
   *((type *)((a)->space + array_global_index * (a)->obj_size)))

#define array_fetch_p(type, a, i)                                              \
  (array_global_index = (i),                                                   \
   (array_global_index >= (a)->num) ? array_abort((a), 1) : 0,                 \
   ((type *)((a)->space + array_global_index * (a)->obj_size)))

/**
 * Fetch the last element from an array.  A runtime error occurs
 * if there are no elements in the array.  There is no type-checking
 * that the value at the given position is actually of the type used
 * when dereferencing the array.  Equivalent to:
 * 	array_fetch(type, array, array_n(array))
 */
#define array_fetch_last(type, array)                                          \
  array_fetch(type, array, ((array)->num) - 1)

/**
 * Returns the number of elements stored in the array.  If this is
 * `n', then the last element of the array is at position `n' - 1.
 */
#define array_n(array) (array)->num

/**
 * Returns a normal `C' array from an array_t structure.  This is
 * sometimes necessary when dealing with functions which do not
 * understand the array_t data type.  A copy of the array is
 * returned, and it is the users responsibility to free it.  array_n()
 * can be used to get the number of elements in the array.
 */
#define array_data(type, array) (type *)array_do_data(array)

#endif
