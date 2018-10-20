/*
 * Revision Control Information
 *
 * $Source: /projects/mvsis/Repository/mvsis-1.3/src/sis/minimize/minimize.h,v $
 * $Author: wjiang $
 * $Revision: 1.1.1.1 $
 * $Date: 2003/02/24 22:24:10 $
 *
 */
#define NOCOMP		0
#define SNOCOMP		1
#define	DCSIMPLIFY	2

#ifndef MINIMIZE_H
#define MINIMIZE_H

EXTERN pcover minimize ARGS((pcover, pcover, int));

#endif

