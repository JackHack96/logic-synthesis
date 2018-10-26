#ifndef LIB_INT_H
#define LIB_INT_H
/* file @(#)lib_int.h	1.4 */
/* last modified on 7/2/91 at 19:44:31 */
#include "library.h"
#include "array.h"

/*
 * $Log: lib_int.h,v $
 * Revision 1.1.1.1  2004/02/07 10:14:25  pchong
 * imported
 *
 * Revision 1.3  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  21:58:28  sis
 * *** empty log message ***
 *
 * Revision 1.1  92/01/08  17:40:43  sis
 * Initial revision
 *
 */

library_t *lib_current_library;

int lib_pattern_matches();

int lib_read_blif();

void lib_free();

void lib_dump();

void lib_dump_class();

double lib_gate_area();

lib_gate_t *lib_get_default_inverter();

array_t *lib_get_prims_from_class(/* lib_class_t *class */);

void lib_dump_gate();

#endif