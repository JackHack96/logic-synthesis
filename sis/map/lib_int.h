
/* file @(#)lib_int.h	1.4 */
/* last modified on 7/2/91 at 19:44:31 */
/*
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

extern library_t *lib_current_library;

extern int lib_pattern_matches();
extern int lib_read_blif();
extern void lib_free();
extern void lib_dump();
extern void lib_dump_class();
extern double lib_gate_area();
extern lib_gate_t *lib_get_default_inverter();
extern array_t *lib_get_prims_from_class(/* lib_class_t *class */);
extern void lib_dump_gate();
