/**CFile****************************************************************

  FileName    [resInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: resInt.h,v 1.1 2005/01/23 06:59:48 alanmi Exp $]

***********************************************************************/
 
#ifndef __RES_INT_H__
#define __RES_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fraig.h"
#include "fpga.h"
#include "fpgaInt.h"
#include "sat.h"
#include "res.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

// macros to get hold of the bits in the support info
#define Fpga_NodeSetSuppVar(p,i)  (p[(i)>>5] |= (1<<((i) & 31)))
#define Fpga_NodeHasSuppVar(p,i) ((p[(i)>>5]  & (1<<((i) & 31))) > 0)

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the mapping manager
struct Res_ManStruct_t_
{
    // mapping data
    Fpga_Man_t *        pMan;          // the mapping manager
    Fpga_NodeVec_t *    vLevels;       // the levels of the AND-nodes
    Fpga_NodeVec_t **   pvStack;       // the stack used for propagating changes
    int                 fVerbose;      // the verbosiness flag
    int                 fDoingArea;    // the mode switch
    int                 fGlobalCones;  // the logic collection switch

    // the memory managers
    Extra_MmFixed_t *   mmSims;        // the memory manager for cuts
    Extra_MmFixed_t *   mmSupps;       // the memory manager for supports
    int                 nSuppWords;    // the number of unsigned ints in the support
    int                 nSimmWords;    // the number of unsigned ints in the simmulation info
    unsigned **         pSuppF;        // pointers to the subsets of true supports of the nodes
    unsigned **         pSuppS;        // pointers to the subsets of true supports of the nodes
    unsigned **         pSimms;        // pointers to the simulation information

    // interpolant computation
    int                 nVarsRes;      // the number of AND nodes when resynthesis begins
    Sat_IntVec_t *      vVars;         // these are the A+C variable (except B)
    Sat_IntVec_t *      vVarMap;       // mapping from var to its number (C) or -1 (A)
    Sat_IntVec_t *      vProj;         // temporary array
    Sat_IntVec_t *      vLits;         // temporary array
    Sat_Solver_t *      pSatImg;       // the SAT solver used for image computation

    // node window
    Fpga_NodeVec_t *    vWinCands;     // the internal nodes of the window
    Fpga_NodeVec_t *    vWinTotal;     // the union of all nodes
    Fpga_NodeVec_t *    vWinNodes;     // the AND nodes used for SAT
    Fpga_NodeVec_t *    vWinVisit;     // the TFO of the node

    // image computation
    unsigned            uRes0All[4];   // the storage for truth tables
    unsigned            uRes1All[4];   // the storage for truth tables

    int                 nNodesTried;   // the number of nodes, which we tried to resub
    int                 nCutsTried;    // the number of sets that were tried
    int                 nCutsTop;      // the number of sets filtered by topological considerations
    int                 nCutsSim;      // the number of sets filtered by simulation
    int                 nCutsUsed;     // the number of sets actually used
    int                 nSatSuccess;   // the number of times SAT proves resubstitution
    int                 nSatFailure;   // the number of times SAT found counter example
    int                 nFails;        // the number of times the solver returns false neg
    int                 nOkays;        // the number of times the solver worked well
    int                 nCutsGen[5];  // the number of cuts used depending on their genesis

    // runtime statistics
    int                 timeStart;     // time to start the process
    int                 timeStop;      // time to stop the process
    int                 timeSelect;    // time to select the nodes
    int                 timeFilTop;    // time to filter the node sets
    int                 timeFilSim;    // time to filter the node sets
    int                 timeFilSat;    // time to filter the node sets
    int                 timeResub;     // time to compute the image
    int                 timeUpdate;    // time to update the network and timing
    int                 timeTotal;     // the total time
    int                 time1;         // time to perform task 1
    int                 time2;         // time to perform task 2
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== resCreate.c ===============================================================*/
extern Res_Man_t *       Res_ManStart( Fpga_Man_t * pMan );
extern void              Res_ManStop( Res_Man_t * pMan );
extern void              Res_ManPrintTimeStats( Res_Man_t * p );
/*=== resFanin.c ===============================================================*/
extern Fpga_Cut_t *      Res_NodeReplaceOneFanin( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pFanin, Fpga_NodeVec_t * vNodes, int fTryTwo );
/*=== resFil.c ===============================================================*/
extern Fpga_Cut_t *      Res_CheckCut( Res_Man_t * p, Fpga_Node_t * pRoot, Fpga_Node_t * ppNodes[], int nNodes, int fFilterTop );
extern Fpga_Cut_t *      Res_CheckCutTop( Res_Man_t * p, Fpga_Node_t * pRoot, Fpga_Node_t * ppNodes[], int nNodes );
extern Fpga_Cut_t *      Res_SelectBestCut( Res_Man_t * p, Fpga_Node_t * pNode, Fpga_Cut_t * pCutList );
/*=== resFilSat.c ===============================================================*/
extern Fpga_Cut_t *      Res_CheckSat( Res_Man_t * p, Fpga_Node_t * pPivot, Fpga_Node_t * ppCands[], int nCands );
/*=== resFilSim.c ===============================================================*/
extern int               Res_TestNodes( Res_Man_t * p, Fpga_Node_t * pRoot, Fpga_Node_t * pNodes[], int nNodes );
/*=== resFilTop.c ===============================================================*/
extern int               Res_TestTopological( Res_Man_t * pMan, Fpga_Node_t * pRoot, Fpga_Node_t * pNodes[], int nNodes );
/*=== resImage.c ===============================================================*/
extern int               Res_ResynthesisImage( Res_Man_t * p, Fpga_Node_t * pPivot, Fpga_Node_t * ppCands[], int nCands );
extern int               Res_ResynthesisImage2( Res_Man_t * p, Fpga_Node_t * pPivot, Fpga_Node_t * ppCands[], int nCands );
extern int               Res_NodeAddClauses( Res_Man_t * p, Sat_Solver_t * pSat, Fpga_Node_t * pNode );
/*=== resLogic.c ===============================================================*/
extern Fpga_NodeVec_t *  Res_CollectRelatedLogic( Res_Man_t * p, Fpga_Node_t * pNode );
extern void              Res_CollectRelatedLogic2( Res_Man_t * p, Fpga_Node_t * pNode );
/*=== resNode.c ===============================================================*/
extern int               Res_NodeResynthesize( Res_Man_t * p, Fpga_Node_t * pNode );
extern void              Res_NodeOrderFanins( Res_Man_t * p, Fpga_Cut_t * pCut, int * pPerm );
extern int               Res_NodeIsCritical( Res_Man_t * p, Fpga_Node_t * pNode );
/*=== resSupp.c ===============================================================*/
extern void              Res_ManGetTrueSupports( Res_Man_t * p );
/*=== resUpdate.c ===============================================================*/
extern void              Res_NodeUpdateArrivalTimeSlow( Res_Man_t * p, Fpga_Node_t * pNode );
extern void              Res_NodeUpdateRequiredTimeSlow( Res_Man_t * p, Fpga_Node_t * pNode );
extern void              Res_NodeUpdateArrivalTime( Res_Man_t * p, Fpga_Node_t * pNode );
extern void              Res_NodeUpdateRequiredTime( Res_Man_t * p, Fpga_Node_t * pNode );
/*=== resUtils.c ===============================================================*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
