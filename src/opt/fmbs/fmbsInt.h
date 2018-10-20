/**CFile****************************************************************

  FileName    [fmbsInt.h]

  PackageName [Binary flexibility manager.]

  Synopsis    [Internal declarations of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbsInt.h,v 1.2 2004/10/18 01:39:22 alanmi Exp $]

***********************************************************************/

#ifndef __FMBS_INT_H__
#define __FMBS_INT_H__

#include "mv.h"
#include "sh.h"
#include "wn.h"
#include "sat.h"
#include "fmbs.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct Fmbs_Manager_t_ 
{
    // managers
    DdManager *     dd;         // the local bdd manager
    Sh_Manager_t *  pShMan;     // the strashing manager
    Sat_Solver_t *  pSat;       // the SAT solver

	// data related to the network
	Ntk_Network_t * pNet;       // the network processed by the manager
    Wn_Window_t *   pWnd;       // the current window
    Sh_Network_t *  pShNet;     // the strashed window
    int             nFanins;    // the number of fanins in the node, whose CF is computed
    int             nNodes;     // the number of internal nodes in the strashed window

    // temporary storage for the care set
    char *          pCareSet;
    int             nCareSetAlloc;

	// statistical data
	int             timeStart;  // starting time
	int             timeWnd;    // time to compute a window
	int             timeStr;    // time to strash the window
	int             timeSim;    // time to simulate
	int             timeSat;    // time to run SAT
	int             timeSopMin; // time to minimize SOP
	int             timeResub;  // time spent by the user
	int             timeOther;  // other time
	int             timeTotal;  // total time
	int             timeTemp;   // temporary clock
	int             timeCdc;    // total time
	int             timeAig;    // total time

    // various parameters
    Fmbs_Params_t *  pPars;
}; 


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
 
/*=== fmbsSimul.c =========================================================*/
extern int Fmbs_Simulation( Fmbs_Manager_t * p );

#endif
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
