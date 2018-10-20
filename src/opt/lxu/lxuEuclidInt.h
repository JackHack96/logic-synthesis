/**CFile****************************************************************

  FileName    [lxuEuclidInt.h]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of Euclid module which adds physical reasoning capabilities to lxu]

  Author      [Satrajit Chatterjee]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuEuclidInt.h,v 1.2 2004/01/30 00:18:47 satrajit Exp $]

***********************************************************************/
 
#ifndef __LXU_EUCLIDINT_H__
#define __LXU_EUCLIDINT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "lxuInt.h"
#include "util.h"
#include <string.h>
#include <time.h>
#include <limits.h>

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// uncomment this macro to switch to standard memory management
//#define USE_SYSTEM_MEMORY_MANAGEMENT 

#define LXU_EUCLID_MAX_NETS 100

int verbose=1;

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct lxuEuclidState Lxu_EuclidState;
typedef struct lxuEdge Lxu_Edge;
typedef struct lxuEuclidTuple Lxu_EuclidTuple;

struct lxuEuclidState {

	int			interval;
	weightType	alpha, beta;
	int 		numNodes;
	Lxu_Rect 	arena;			// The area where we place stuff
};

Lxu_EuclidState es;	// Singleton 

struct lxuEdge {

	coord		x;				// Coordinate of the edge
	int			l;				// left edge or right edge
};

struct lxuEuclidTuple {

	coord		x;
	weightType	cost;
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*===== lxuEuclid.c ====================================================*/




////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////
#endif

