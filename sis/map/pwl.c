/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/pwl.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:26 $
 *
 */
/* file @(#)pwl.c	1.2 */
/* last modified on 5/1/91 at 15:51:47 */
#include "pwl.h"
#include "pwl_static.h"

 /* NOTE: PWL == Piece-Wise Linear function */

 /* implements a package for piece-wise linear functions */
 /* makes the "optimal" mapper for delay a lot simpler */
 /* no need for discretization!!! */
 /* can throw away bin_util.c!!! */

 /* This package could have been made a bit more generic */
 /* the essentials are there, the rest would just be cosmetics */


 /* computes the max of two pwl functions */

pwl_t *pwl_min(f1, f2)
pwl_t *f1;
pwl_t *f2;
{
  return pwl_compute_min(f1, f2);
}

pwl_t *pwl_max(f1, f2)
pwl_t *f1;
pwl_t *f2;
{
  pwl_t *f;
  change_sign(f1);
  change_sign(f2);
  f = pwl_compute_min(f1, f2);
  change_sign(f1);
  change_sign(f2);
  change_sign(f);
  return f;
}

pwl_t *pwl_sum(f1, f2)
pwl_t *f1;
pwl_t *f2;
{
  return pwl_compute_sum(f1, f2);
}

 /* only positive shifts are allowed */

pwl_t *pwl_shift(f, shift)
pwl_t *f;
double shift;
{
  if (f->n_points == 0 || shift == 0.0) return pwl_dup(f);
  assert(shift > 0.0);
  return compute_pwl_shift(f, shift);
}


 /* given a bunch of lines, return their linear max: */
 /* that is a sequence of points starting from x=0 */
 /* that represents the max of these lines */
 /* the termination condition is activated when we */
 /* have reached the line with highest slope */

pwl_t *pwl_create_linear_max(n_points, points)
int n_points;
pwl_point_t *points;
{
  return create_linear_max(n_points, points);
}


 /* creates an empty pwl */

pwl_t *pwl_create_empty()
{
  pwl_t *result = ALLOC(pwl_t, 1);
  result->n_points = 0;
  result->points = NIL(pwl_point_t);
  return result;
}


 /* make a copy of a pwl_t */

pwl_t *pwl_dup(f)
pwl_t *f;
{
  int i;
  pwl_t *new_f = ALLOC(pwl_t, 1);

  *new_f = *f;
  if (f->n_points == 0) {
    new_f->points = NIL(pwl_point_t);
  } else {
    new_f->points = ALLOC(pwl_point_t, new_f->n_points);
    for (i = 0; i < new_f->n_points; i++) {
      new_f->points[i] = f->points[i];
    }
  }
  return new_f;
}

 /* sort them if necessary */

pwl_t *pwl_create(points)
array_t *points;
{
  int i;
  pwl_point_t *point;
  pwl_t *result;

  if (points == NIL(array_t)) return pwl_create_empty();
  result = ALLOC(pwl_t, 1);
  if (array_n(points) == 0) {
    result->n_points = 0;
    result->points = NIL(pwl_point_t);
  } else {
    result->n_points = array_n(points);
    result->points = ALLOC(pwl_point_t, array_n(points));
    array_sort(points, pwl_point_cmp);
    for (i = 0; i < result->n_points; i++) {
      point = array_fetch_p(pwl_point_t, points, i);
      result->points[i] = *point;
    }
  }
  return result;
}

 /* think they are sorted and unique */
 /* very trusting. Should really be exported??? */

pwl_t *pwl_extract(points)
array_t *points;
{
  int i;
  pwl_point_t *point;
  pwl_t *result;

  if (points == NIL(array_t)) return pwl_create_empty();
  result = ALLOC(pwl_t, 1);
  if (array_n(points) == 0) {
    result->n_points = 0;
    result->points = NIL(pwl_point_t);
  } else {
    result->n_points = array_n(points);
    result->points = ALLOC(pwl_point_t, array_n(points));
    for (i = 0; i < result->n_points; i++) {
      point = array_fetch_p(pwl_point_t, points, i);
      result->points[i] = *point;
    }
  }
  return result;
}

void pwl_free(f)
pwl_t *f;
{
  assert (f != NIL(pwl_t));
  if (f->n_points > 0) {
    FREE(f->points);
  }
  FREE(f);
}

void pwl_print(fp, f, print_data)
FILE *fp;
pwl_t *f;
VoidFn print_data;
{
  int i;

  for (i = 0; i < f->n_points; i++) {
    pwl_point_print(fp, &(f->points[i]), print_data);
  }
}

 /* assert the same data to all these elements */

void pwl_set_data(pwl, data)
pwl_t *pwl;
char *data;
{
  int i;

  for (i = 0; i < pwl->n_points; i++) {
    pwl->points[i].data = data;
  }
}



 /* for a given "abscisse", returns the data */
 /* for the best solution there */

char *pwl_select(f, x)
pwl_t *f;
double x;
{
  pwl_point_t *point = pwl_binary_search(f, x);
  return point->data;
}


 /* lookup the interval including a given x */

pwl_point_t *pwl_lookup(f, x)
pwl_t *f;
double x;
{
  return pwl_binary_search(f, x);
}

double pwl_point_eval(point, x)
pwl_point_t *point;
double x;
{
  return point->y + point->slope * (x - point->x);
}


char *pwl_point_data(point)
pwl_point_t *point;
{
  return point->data;
}


 /* INTERNAL INTERFACE */

static pwl_point_t *pwl_binary_search(f, value)
pwl_t *f;
double value;
{
  int min, max, middle;
  double min_value;
  double max_value;
  double middle_value;

 /* initialization and special cases */
  if (f->n_points == 0) return NIL(pwl_point_t);
  if (f->n_points == 1) return &(f->points[0]);
  min = 0;
  min_value = f->points[min].x;
  if (value <= min_value) return &(f->points[min]);
  max = f->n_points - 1;
  max_value = f->points[max].x;
  if (max_value <= value) return &(f->points[max]);

 /* the binary search itself */
  for (;;) {
    middle = min + (max - min) / 2;
    if (middle == min) {
      assert(value < max_value);
      assert(value >= min_value);
      break;
    }
    middle_value = f->points[middle].x;
    if (value < middle_value) {
      max = middle;
      max_value = middle_value;
    } else {
      min = middle;
      min_value = middle_value;
    }
  }
  return &(f->points[min]);
}

 /* avoid copying an entry if prolongating the previous one would do as well */

static pwl_t *pwl_compute_min(f1, f2)
pwl_t *f1;
pwl_t *f2;
{
  int intersect_flag;
  pwl_t *result;
  int indices[2];
  pwl_t *fns[2];
  int select;
  double x;
  pwl_point_t point;
  pwl_point_t *best_point;
  array_t *points;

  if (f1->n_points == 0) return pwl_dup(f2);
  if (f2->n_points == 0) return pwl_dup(f1);
  fns[0] = f1;
  fns[1] = f2;
  points  = array_alloc(pwl_point_t, 0);
  point.slope = - INFINITY;
  point.data  = NIL(char);
  first_point(fns, indices, &select, &x, &intersect_flag);
  do {
    best_point = &(fns[select]->points[indices[select]]);
    if (point.data == best_point->data && point.slope == best_point->slope) continue;
    point.x = x;
    point.y = best_point->y + best_point->slope * (x - best_point->x);
    point.slope = best_point->slope;
    point.data = best_point->data;
    array_insert_last(pwl_point_t, points, point);
  } while (next_point(fns, indices, &select, &x, &intersect_flag));

 /* copy the array points in the result */
 /* if two consecutive entries with same slope and same data */
 /* we do not need to keep the second one */
  result = pwl_extract(points);
  array_free(points);
  return result;
}

static void first_point(fns, indices, select_p, x_p, intersect_flag_p)
pwl_t **fns;
int *indices;
int *select_p;
double *x_p;
int *intersect_flag_p;
{
  double diff;

  *x_p = 0.0;
  indices[0] = indices[1] = 0;
  diff = fns[0]->points[0].y - fns[1]->points[0].y;
  if (diff == 0) {
    diff = fns[0]->points[0].slope - fns[1]->points[0].slope;
  }
  *select_p = (diff <= 0) ? 0 : 1;
  *intersect_flag_p = 0;
}


 /* returns 1 iff there is something left, 0 otherwise */

static int next_point(fns, indices, select_p, x_p, intersect_flag_p)
pwl_t **fns;
int *indices;
int *select_p;
double *x_p;
int *intersect_flag_p;
{
  int i;
  double diff;
  double min;
  double mins[2];
  double intersect;
  pwl_point_t *seg[2];

  /* first compute the next interesting x:  */
  /* either the end of current interval or the intersection of the two current segments */
  /* if previous one was intersection, can't be intersection again: avoid recomputing */
  min = INFINITY;
  for (i = 0; i < 2; i++) {
    mins[i] = (fns[i]->n_points - 1 == indices[i]) ? INFINITY : fns[i]->points[indices[i] + 1].x;
    min = MIN(min, mins[i]);
  }
  if (*intersect_flag_p) {
    intersect = - INFINITY;
  } else {
    intersect = compute_intersect(&(fns[0]->points[indices[0]]), &(fns[1]->points[indices[1]]));
  }
  if (intersect > *x_p && intersect < min) {
    *intersect_flag_p = 1;
    *x_p = intersect;
  } else if (mins[0] == INFINITY && mins[1] == INFINITY) { 
    return 0;
  } else {
    *intersect_flag_p = 0;
    *x_p = min;
  }
  
  /* now we need to determine which is the better fn */

  if (*intersect_flag_p) {
    /* after intersection point, the better one is the one with lower slope */
    diff = fns[0]->points[indices[0]].slope - fns[1]->points[indices[1]].slope;
    assert(diff != 0);
    *select_p = (diff < 0) ? 0 : 1;
    return 1;
  }

  /* need to go to next interval if it was reached */
  if (min >= mins[0]) indices[0]++;
  if (min >= mins[1]) indices[1]++;

  /* the better one is the one that evaluates for less now */
  seg[0] = &(fns[0]->points[indices[0]]);
  seg[1] = &(fns[1]->points[indices[1]]);
  diff = (seg[0]->y + seg[0]->slope * (*x_p - seg[0]->x)) - (seg[1]->y + seg[1]->slope * (*x_p - seg[1]->x));
  if (diff == 0) {
 /* if intersect at this point, the one with smaller slope wins */
    diff = seg[0]->slope - seg[1]->slope;
  }
  *select_p = (diff <= 0) ? 0 : 1;
  return 1;
}

static pwl_t *pwl_compute_sum(f1, f2)
pwl_t *f1;
pwl_t *f2;
{
  int i;
  int select;
  double diff;
  int indices[2];
  pwl_t *fns[2];
  pwl_point_t *points[2];
  pwl_point_t point;
  array_t *array_of_points;
  pwl_t *result;

  if (f1->n_points == 0) return pwl_dup(f2);
  if (f2->n_points == 0) return pwl_dup(f1);

 /* this code works only if starting points of the two functions are the same */
  assert(f1->points[0].x == f2->points[0].x);

  fns[0] = f1;
  fns[1] = f2;

 /* put points in this array; build the result from that at the end */
  array_of_points = array_alloc(pwl_point_t, 0);

 /* pointers to the points in each fi */
  indices[0] = indices[1] = 0;

 /* if a point has been visited, the pointer is incremented */
 /* at the end. Indices outside the range */
 /* need special treatment to take care of remaining points */
  for (;;) {
    if (indices[0] == fns[0]->n_points && indices[1] == fns[1]->n_points) break;
    for (i = 0; i < 2; i++) {
      if (indices[i] < fns[i]->n_points) {
	points[i] = &(fns[i]->points[indices[i]]);
      } else {
	points[i] = &(fns[i]->points[indices[i]]);
      }
    }
    diff = points[0]->x - points[1]->x;
    select = (diff <= 0) ? 0 : 1;
    if (indices[select] == fns[select]->n_points) select = 1 - select;
    assert(indices[select] < fns[select]->n_points);
    point.x = points[select]->x;
    point.y = points[0]->y + (point.x - points[0]->x) * points[0]->slope;
    point.y += points[1]->y + (point.x - points[1]->x) * points[1]->slope;
    point.slope = points[0]->slope + points[1]->slope;
    point.data = points[select]->data;
    array_insert_last(pwl_point_t, array_of_points, point);
    indices[select]++;
    if (diff == 0) {
      assert(indices[1 - select] < fns[1 - select]->n_points);
      indices[1 - select]++;
    }
  }

 /* copy the array array_of_points in the result */
  result = pwl_extract(array_of_points);
  array_free(array_of_points);
  return result;
}


static void pwl_point_print(fp, point, print_data)
FILE *fp;
pwl_point_t *point;
VoidFn print_data;
{
  (void) fprintf(fp, "%2.2f %2.2f %2.2f ", point->x, point->y, point->slope);
  (*print_data)(fp, point->data);
  (void) fprintf(fp, "\n");
}

static int pwl_point_cmp(obj1, obj2)
char *obj1;
char *obj2;
{
  double diff;
  pwl_point_t *p1 = (pwl_point_t *) obj1;
  pwl_point_t *p2 = (pwl_point_t *) obj2;

  diff = p1->x - p2->x;
  if (diff < 0) return -1;
  if (diff > 0) return 1;
  return 0;
}

 /* return the point where the two lines intersect */
 /* or - INFINITY if they are parallel */
static double compute_intersect(p1, p2)
pwl_point_t *p1;
pwl_point_t *p2;
{
  if (p1->slope == p2->slope) return (double) - INFINITY;
  return (((p2->y - p2->slope * p2->x) - (p1->y - p1->slope * p1->x)) / (p1->slope - p2->slope));
}

 /* exact in the interval 0 <= x <= INFINITY */

static pwl_t *create_linear_max(n_lines, origins)
int n_lines;
pwl_point_t *origins;
{
  double x, next_x;
  double max_slope;
  double intersect;
  int i;
  int selected;
  int *valid;
  array_t *points;
  pwl_point_t point;
  pwl_t *result;

  valid = ALLOC(int, n_lines);
  for (i = 0; i < n_lines; i++) {
    valid[i] = 1;
  }
  x = 0.0;
  points = array_alloc(pwl_point_t, 0);
  max_slope = compute_max_slope(n_lines, origins);

  for (;;) {
 /* take the largest line at this time that is still valid */
 /* if floating point were exact, there would be no need to check for validity */
 /* but the intersection point may be interestimated by the FP hardware */
 /* in that case we need to make sure that we do not select a line already selected */
    selected = compute_argmax_y(x, n_lines, origins, valid);
    point.x = x;
    point.y = origins[selected].y + origins[selected].slope * x;
    point.slope = origins[selected].slope;
    point.data = origins[selected].data;
    array_insert_last(pwl_point_t, points, point);
    valid[selected] = 0;

 /* if reached the largest slope, nothing will ever intersect it in the future */
    if (point.slope == max_slope) break;

 /* compute the next interesting point */
    next_x = INFINITY;
    for (i = 0; i < n_lines; i++) {
      if (! valid[i]) continue;
      if (point.slope >= origins[i].slope) {
	valid[i] = 0;
	continue;
      }
      intersect = compute_intersect(&point, &origins[i]);
 /* need to make some progress */
      if (intersect <= x) continue;
      next_x = MIN(next_x, intersect);
    }
 /* this means intersection only after INFINITY: ignore the rest */
    if (next_x >= INFINITY) break;
    x = next_x;
  }
  FREE(valid);
  result = pwl_extract(points);
  array_free(points);
  return result;
}

static double compute_max_slope(n_lines, points)
int n_lines;
pwl_point_t *points;
{
  int i;
  double slope = - INFINITY;
  
  for (i = 0; i < n_lines; i++) {
    slope = MAX(slope, points[i].slope);
  }
  return slope;
}

static int compute_argmax_y(x, n_lines, points, valid)
double x;
int n_lines;
pwl_point_t *points;
int *valid;
{
  int i;
  double y, tmp_y;
  int argmax_y;
  double slope;

  y = - INFINITY;
  slope = - INFINITY;
  argmax_y = -1;
  for (i = 0; i < n_lines; i++) {
    if (! valid[i]) continue;
    tmp_y = points[i].y + points[i].slope * (x - points[i].x);
    if (y < tmp_y || (y == tmp_y && slope < points[i].slope)) {
      argmax_y = i;
      y = tmp_y;
      slope = points[i].slope;
    }
  }
  assert(argmax_y != -1);
  return argmax_y;
}

static void change_sign(f)
pwl_t *f;
{
  register pwl_point_t *to;
  register pwl_point_t *from;

  if (f->n_points == 0) return;
  to = f->points + f->n_points;
  for (from = f->points; from < to; from++) {
    from->y = - from->y;
    from->slope = - from->slope;
  }
}

static pwl_t *compute_pwl_shift(f, shift)
pwl_t *f;
double shift;
{
  pwl_t *result;
  pwl_point_t point;
  pwl_point_t *ptr = pwl_binary_search(f, shift);
  pwl_point_t *to = f->points + f->n_points;
  array_t *array_of_points = array_alloc(pwl_point_t, 0);
  
  point = *ptr;
  point.y += point.slope * (shift - point.x);
  point.x = 0.0;
  array_insert_last(pwl_point_t, array_of_points, point);
  for (ptr++; ptr < to; ptr++) {
    point = *ptr;
    point.x -= shift;
    array_insert_last(pwl_point_t, array_of_points, point);
  }
  result = pwl_extract(array_of_points);
  array_free(array_of_points);
  return result;
}
