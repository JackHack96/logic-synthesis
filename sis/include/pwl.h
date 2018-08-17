
/* file @(#)pwl.h	1.7 */
/* last modified on 7/25/91 at 11:45:54 */
/* 
 * $Log: pwl.h,v $
 * Revision 1.1.1.1  2004/02/07 10:14:26  pchong
 * imported
 *
 * Revision 1.3  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  22:00:59  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:40:53  sis
 * Initial revision
 * 
 * Revision 1.1  91/07/02  11:18:35  touati
 * Initial revision
 * 
 */
#ifndef PWL_H
#define PWL_H

#include "sis.h"

/* implements piece-wise linear scalar valued functions */

typedef struct {
    double x;
    double y;
    double slope;
    char   *data;
} pwl_point_t;

typedef struct {
    int         n_points;
    pwl_point_t *points;
} pwl_t;

#ifndef INFINITY
#define INFINITY 0x7fffffff
#endif

extern pwl_t *pwl_min(/* pwl_t *f1, pwl_t *f2 */);

extern pwl_t *pwl_max(/* pwl_t *f1, pwl_t *f2 */);

extern pwl_t *pwl_sum(/* pwl_t *f1, pwl_t *f2 */);

extern pwl_t *pwl_shift(/* pwl_t *f, double shift */);

extern pwl_t *pwl_create_linear_max(/* int n_points; pwl_point_t *points; */);

extern pwl_t *pwl_dup(/*  pwl_t *f; */);

/* sort the points if necessary; should be made same interface as create_linear_max */
extern pwl_t *pwl_create(/*  array_t *points; */);

/* does not sort */
extern pwl_t *pwl_extract(/*  array_t *points; */);

extern void pwl_free(/* pwl_t *f; */);

extern void pwl_print(/* FILE *fp; pwl_t *f; VoidFn print_data; */);

extern void pwl_set_data(/* pwl_t *pwl; char *data; */);

extern pwl_point_t *pwl_lookup(/* pwl_t *f; double x; */);

extern double pwl_point_eval(/* pwl_point_t *point, double x */);

extern char *pwl_point_data(/* pwl_point_t *point */);

extern pwl_t *pwl_create_empty(/* */);

typedef struct {
    pwl_t *rise;
    pwl_t *fall;
} delay_pwl_t;

#endif /* PWL_H */
