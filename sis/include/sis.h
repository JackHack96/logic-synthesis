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
#include "graph.h"
#include "graph_static.h"

#include "error.h"
#include "node.h"
#include "nodeindex.h"
#include "stg.h"
#include "astg.h"
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
#include "clock.h"
#include "latch.h"
#include "retime.h"

#include "delay.h"
#include "library.h"
#include "map.h"
#include "pld.h"

#include "bdd.h"
#include "order.h"
#include "ntbdd.h"

#include "seqbdd.h"

#include "gcd.h"
#include "maxflow.h"
#include "speed.h"

extern FILE    *sisout;
extern FILE    *siserr;
extern FILE    *sishist;
extern array_t *command_hist;
extern char    *program_name;

extern char *sis_version();

extern char *sis_library();

#define misout sisout
#define miserr siserr

#ifndef INFINITY
#define INFINITY    (1 << 30)
#endif

#endif // SIS_H