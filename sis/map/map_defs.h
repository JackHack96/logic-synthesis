
/* file @(#)map_defs.h	1.6 */
/* last modified on 7/25/91 at 11:34:53 */
/*
 * $Log: map_defs.h,v $
 * Revision 1.1.1.1  2004/02/07 10:14:25  pchong
 * imported
 *
 * Revision 1.3  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  21:58:28  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:40:46  sis
 * Initial revision
 *
 * Revision 1.1  91/07/02  11:13:59  touati
 * Initial revision
 *
 */

#ifndef MAP_DEFS_H
#define MAP_DEFS_H

#define FP_EQUAL(a, b) (ABS((a) - (b)) < .0001)
#define LARGE 0x7fffffff

typedef int (*IntFunc)();

#endif /* MAP_DEFS_H */
