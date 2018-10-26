
#include <stdio.h>	/* for BDD_DEBUG_LIFESPAN */

#include "util.h"
#include "array.h"
#include "st.h"

#include "bdd.h"
#include "bdd_int.h"



static int get_var_id();
static int int_compare();

 /* bdd read-only so no need for protection */
 /* provides two functions */
 /* one, given an array of bdd_t *, representing variables */
 /* extract the var_id and put them back in an array */

 /* if var_array is non nil, extract the var-id from it */
 /* if var_array is nil, return an array of ints from 0 to total_n_vars */

array_t *bdd_get_sorted_varids(var_array)
array_t *var_array;
{
  array_t *result = bdd_get_varids(var_array);
  (void) array_sort(result, int_compare);
  return result;
}

array_t *bdd_get_varids(var_array)
array_t *var_array;
{
  int i;
  bdd_t *var;
  array_t *result = array_alloc(int, 0);
  
  for (i = 0; i < array_n(var_array); i++) {
    var = array_fetch(bdd_t *, var_array, i);
    (void) array_insert_last(int, result, get_var_id(var));
  }
  return result;
}

int bdd_varid_cmp(obj1, obj2)
char *obj1;
{
  bdd_t *f1 = (bdd_t *) obj1;
  bdd_t *f2 = (bdd_t *) obj2;
  
  return get_var_id(f1) - get_var_id(f2);
}

 /* checks that the BDD is a variable node (positive literal) */
 /* and if it is the case, returns its ID */

static int get_var_id(var)
bdd_t *var;
{
  bdd_manager *bdd = var->bdd;
  bdd_node *f = var->node;
  bdd_node *f0, *f1;

  assert(f == BDD_REGULAR(f));
  (void) bdd_get_branches(f, &f1, &f0);
  assert(f1 == BDD_ONE(bdd));
  assert(f0 == BDD_ZERO(bdd));
  return f->id;
}


 /* needed for sorting arrays of var_id's */

static int int_compare(obj1, obj2)
char *obj1;
char *obj2;
{
  return *((int *) obj1) - *((int *) obj2);
}

