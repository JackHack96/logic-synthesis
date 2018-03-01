/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/order/order.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:52 $
 *
 */
#ifndef ORDER_H /* { */
#define ORDER_H

/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/order/order.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:52 $
 * $Log: order.h,v $
 * Revision 1.1.1.1  2004/02/07 10:14:52  pchong
 * imported
 *
 * Revision 1.4  1993/05/28  23:19:35  sis
 * Aesthetic changes to prototypes.
 *
 * Revision 1.3  1992/05/06  18:58:25  sis
 * SIS release 1.1
 *
 * Revision 1.3  1992/05/06  18:58:25  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  22:06:10  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:43:57  sis
 * Initial revision
 * 
 * Revision 1.2  91/05/01  17:37:26  shiple
 * convert to new declaration format using EXTERN and ARGS
 * 
 * Revision 1.1  91/04/01  00:53:14  shiple
 * Initial revision
 * 
 *
 */

/* 
 * Variable ordering methods
 */
EXTERN array_t *order_dfs ARGS((array_t *, st_table *, int));
EXTERN array_t *order_random ARGS((array_t *, st_table *, int));
typedef enum {DFS_ORDER, RANDOM_ORDER} order_method_t;

#endif /* } */
