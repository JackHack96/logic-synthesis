
/* file cluster.h release 1.2 */
/* last modified: 7/2/91 at 19:44:29 */
/*
 * $Log: cluster.h,v $
 * Revision 1.1.1.1  2004/02/07 10:14:24  pchong
 * imported
 *
 * Revision 1.5  1993/07/14  18:10:07  sis
 * -f option added to reduce_depth (Herve Touati)
 *
 * Revision 1.4  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.4  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.3  1992/04/17  21:48:51  sis
 * *** empty log message ***
 *
 * Revision 1.3  1992/04/17  21:48:51  sis
 * *** empty log message ***
 *
 * Revision 1.2  1992/03/25  22:03:32  sis
 * __args is replaced by ARGS
 *
 * Revision 1.2  1992/03/25  22:03:32  sis
 * __args is replaced by ARGS
 *
 * Revision 1.2  1992/03/25  21:48:08  cmoon
 * __args is replaced by ARGS
 *
 * Revision 1.1  92/01/08  17:40:33  sis
 * Initial revision
 *
 * Revision 1.1  91/07/02  10:49:47  touati
 * Initial revision
 *
 */

#ifndef CLUSTER_H
#define CLUSTER_H

typedef enum {
  DEPTH_CONSTRAINT,
  SIZE_CONSTRAINT,
  SIZE_AS_DEPTH_CONSTRAINT,
  CLUSTER_STATISTICS,
  BEST_RATIO_CONSTRAINT,
  FANIN_CONSTRAINT
} clust_type_t;

typedef struct {
  clust_type_t type;
  int cluster_size;
  int depth;
  int relabel;
  int verbose;
  double dup_ratio;
} clust_options_t;

/* redundant - use ARGS defined in ansi.h
#ifdef __STDC__
#define __args(x)	(x)
#else
#define __args(x)	()
#endif
*/

extern network_t *cluster_under_constraint(network_t *, clust_options_t *);

#endif /* CLUSTER_H */
