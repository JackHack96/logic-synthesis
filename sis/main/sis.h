/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/main/sis.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:30 $
 *
 */
#ifndef SIS_H
#define SIS_H

/* espresso brings in sparse.h, mincov.h, util.h */
#include "espresso.h"

#include "avl.h"
#include "enc.h"
#include "st.h"
#include "array.h"
#include "list.h"
#include "sat.h"
#include "spMatrix.h"
#include "var_set.h"
#ifdef SIS
#include "graph.h"
#include "graph_static.h"
#endif /* SIS */

#include "error.h"
#include "node.h"
#include "nodeindex.h"

#ifdef SIS
#include "stg.h"
#include "astg.h"
#endif /* SIS */
#include "network.h"
#include "command.h"
#include "io.h"

#include "factor.h"
#include "decomp.h"
#include "resub.h"	
#include "phase.h"	
#include "simplify.h"	
#include "minimize.h"
#include "graphics.h"

#include "extract.h"
#ifdef SIS
#include "clock.h"
#include "latch.h"
#include "retime.h"
#endif /* SIS */

#include "delay.h"
#include "library.h"
#include "map.h" 
#include "pld.h" 

#include "bdd.h"
#include "order.h"
#include "ntbdd.h"

#ifdef SIS
#include "seqbdd.h"
#endif /* SIS */

#include "gcd.h"
#include "maxflow.h"
#include "speed.h"

extern FILE *sisout;
extern FILE *siserr;
extern FILE *sishist;
extern array_t *command_hist;
extern char *program_name;
extern char *sis_version();
extern char *sis_library();

#define misout sisout
#define miserr siserr

#define INFINITY	(1 << 30)

#endif
