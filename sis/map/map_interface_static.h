/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/map_interface_static.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:26 $
 *
 */
/* file @(#)map_interface_static.h	1.3 */
/* last modified on 7/31/91 at 13:01:23 */
static network_t *build_network_from_annotations();
static void map_compute_annotated_arrival_times();
static void map_compute_loads();
static void map_remove_unnecessary_annotations();
static void map_report_data();
static void map_report_data_annotated();
static void unmap_network();
static void visit_mapped_nodes();
static int map_check_library();
