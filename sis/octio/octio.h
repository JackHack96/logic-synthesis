/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/octio/octio.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:47 $
 *
 */
#ifndef OCTIO_H
#define OCTIO_H

#define SIS_PKG_NAME "oct/sis"

EXTERN int external_read_oct  ARGS((network_t **,int,char**));
EXTERN int external_write_oct ARGS((network_t **,int,char**));

extern char *optProgName;
#endif
