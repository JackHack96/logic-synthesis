/**CFile****************************************************************

  FileName    [resmInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: resmInt.h,v 1.2 2005/02/28 05:34:33 alanmi Exp $]

***********************************************************************/
 
#ifndef __RESM_INT_H__
#define __RESM_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mio.h"
#include "mapper.h"
#include "sat.h"
#include "resm.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

#define RESM_SIM_ROUNDS           64

// maximum/minimum operators
#define RESM_MIN(a,b)             (((a) < (b))? (a) : (b))
#define RESM_MAX(a,b)             (((a) > (b))? (a) : (b))

// the small and large numbers (min/max float are 1.17e-38/3.40e+38)
#define RESM_FLOAT_LARGE          ((float)1.0e+30)
#define RESM_FLOAT_SMALL          ((float)1.0e-03)

// generating random unsigned (#define RAND_MAX 0x7fff)
#define RESM_RANDOM_UNSIGNED   ((((unsigned)rand()) << 24) ^ (((unsigned)rand()) << 12) ^ ((unsigned)rand()))

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the mapping manager
struct Resm_ManStruct_t_
{
    // the nodes used for resynthesis
    Resm_Node_t **      pInputs;       // the array of inputs
    int                 nInputs;       // the number of inputs
    Resm_Node_t **      pOutputs;      // the array of outputs
    int                 nOutputs;      // the number of outputs
    Resm_Node_t *       pConst1;       // the constant 1 node
    Resm_Node_t *       pPseudo;       // the pseudo-node whose fanins are POs
    Resm_NodeVec_t *    vNodesAll;     // the array of all nodes
    int                 nNodes;        // the total number of nodes

    // the supergate library
    int                 nVarsMax;      // the max number of variables
    Map_SuperLib_t *    pSuperLib;     // the current supergate library
    unsigned            uTruths[6][2]; // the elementary truth tables
    float               AreaInv;       // area of the intertor
    Map_Time_t          tDelayInv;     // delay of the intertor

    // info about the original circuit
    char **             ppOutputNames; // the primary output names
    Map_Time_t *        pInputArrivals;// the PI arrival times

    // mapping data
    Resm_NodeVec_t *    vLevels;       // the levels of the AND-nodes
    Resm_NodeVec_t **   pvStack;       // the stack used for propagating changes
    int                 fVerbose;      // the verbosiness flag
    int                 fDoingArea;    // the mode switch
    int                 fGlobalCones;  // the flag to indicate global cones
    int                 fUseThree;     // the flag to try three-node groups
    int                 fAcceptMode;   // 0: accept if better; 1: accept if equal, 2: always
    int                 CritWindow;    // the timing window to define critical path (percentage)
    int                 ConeDepth;     // the depth of the logic cone considered (inv delays) 
    int                 ConeSize;      // the maximum number of nodes in the logic cone

    // the memory managers
    Extra_MmFixed_t *   mmSims;        // the memory manager for cuts
    Extra_MmFixed_t *   mmNodes;       // the memory manager for supports
    int                 nSimRounds;    // the number of words in the simulation info

    // interpolant computation
    int                 nVarsRes;      // the number of AND nodes when resynthesis begins
    Sat_IntVec_t *      vVars;         // these are the A+C variable (except B)
    Sat_IntVec_t *      vVarMap;       // mapping from var to its number (C) or -1 (A)
    Sat_IntVec_t *      vProj;         // temporary array
    Sat_IntVec_t *      vLits;         // temporary array
    Sat_Solver_t *      pSatImg;       // the SAT solver used for image computation

    // node window
    Resm_NodeVec_t *    vWinCands;     // the internal nodes of the window
    Resm_NodeVec_t *    vWinTotal;     // the union of all nodes
    Resm_NodeVec_t *    vWinNodes;     // the internal nodes used for SAT
    Resm_NodeVec_t *    vWinVisit;     // the TFO of the node
    Resm_NodeVec_t *    vWinTotalCone; // the union of all nodes
    Resm_NodeVec_t *    vWinNodesCone; // the internal nodes used for SAT

    // resynthesis parameters
    float               fRequiredGlo;  // the global required times
    float               fRequiredShift;// the shift of the required times
    float               fRequiredStart;// the starting global required times
    float               fRequiredGain; // the reduction in delay
    float               fAreaStatic;   // the static area
    float               fAreaGlo;      // the total area
    float               fAreaGain;     // the reduction in area
    float               fEpsilon;      // the epsilon used to compare floats
    float               fDelayWindow;  // the delay window for delay-oriented resynthesis
    float               DelayLimit;    // for resynthesis
    float               AreaLimit;     // for resynthesis
    float               TimeLimit;     // for resynthesis
    float               AreaBase;      // the area after delay-oriented mapping

    // image computation
    unsigned            uRes0All[4];   // the storage for truth tables
    unsigned            uRes1All[4];   // the storage for truth tables

    int                 nNodesTried;   // the number of nodes, which we tried to resub
    int                 nFaninsTried;  // the number of fanins, which we tried to resub
    int                 nCutsTried;    // the number of sets that were tried
    int                 nCutsTop;      // the number of sets filtered by topological considerations
    int                 nCutsSim;      // the number of sets filtered by simulation
    int                 nCutsUsed;     // the number of sets actually used
    int                 nSatSuccess;   // the number of times SAT proves resubstitution
    int                 nSatFailure;   // the number of times SAT found counter example
    int                 nFails;        // the number of times the solver returns false neg
    int                 nOkays;        // the number of times the solver worked well
    int                 nCutsGen[5];   // the number of cuts used depending on their genesis

    // runtime statistics
    int                 timeToMap;     // the time to transform from the network to resynthesis 
    int                 timeToNet;     // the time to transform from the resynthesis to network 
    int                 timeStart;     // time to start the process
    int                 timeStop;      // time to stop the process
    int                 timeLogic;     // time to select the nodes
    int                 timeSimul;     // time to simulate the nodes
    int                 timeMatch;     // time to select the nodes
    int                 timeFilTop;    // time to filter the node sets
    int                 timeFilSim;    // time to filter the node sets
    int                 timeFilSat;    // time to filter the node sets
    int                 timeResub;     // time to compute the image
    int                 timeUpdate;    // time to update the network and timing
    int                 timeTotal;     // the total time
    int                 time1;         // time to perform task 1
    int                 time2;         // time to perform task 2
    int                 time3;         // time to perform task 3
};

struct Resm_NodeStruct_t_
{
    Resm_Man_t *        p;             // the parent manager
    int                 Num;           // the number of this node
    unsigned            Num2    : 20;  // the number of this node
    unsigned            fUsed   :  1;  // the flad to mark a temporarily used node
    unsigned            fMark0  :  1;  // temporary mark 
    unsigned            fMark1  :  1;  // temporary mark  
    unsigned            nVolume :  2;  // temporary number
    unsigned            uPhase  :  6;  // phase for matching
    unsigned            fCompl  :  1;  // used to mark the cut implementing negative phase
    // connectivity information
    int                 nLeaves;       // the number of leaves
    Resm_Node_t **      ppLeaves;      // the leaves (can be complemented)
    Resm_Node_t *       ppArray[RESM_MAX_FANINS];    // the leaves (can be complemented)
    Resm_NodeVec_t *    vFanouts;      // the array of fanouts
    short               nRefs[2];      // the reference counters for both polarities
    // timing information
    Map_Time_t          tArrival;      // the arrival time
    Map_Time_t          tRequired[2];  // the required time
    Map_Time_t          tPrevious;     // the previous time
    float               aArea;         // effective area of the node
    // matching information
    unsigned            uTruthTemp[2]; // the onset truth table
    unsigned            uTruthZero[2]; // the offset truth table
    Map_Super_t *       pSuperBest;    // the best supergate
    // functionality and mapping
    Mio_Gate_t *        pGate;         // the gate assigned to this node
    unsigned *          pSims;         // the simulation information
    char *              pData[2];      // temporary data
    Resm_Node_t *       pNext;         // the next node
};

// the vector of nodes
struct Resm_NodeVecStruct_t_
{
    Resm_Node_t **      pArray;        // the array of nodes
    int                 nSize;         // the number of entries in the array
    int                 nCap;          // the number of allocated entries
};

#define Resm_NodeRef(p)            (Resm_Regular(p)->nRefs[!Resm_IsComplement(p)]++)
#define Resm_NodeDeref(p)          (Resm_Regular(p)->nRefs[!Resm_IsComplement(p)]--)
#define Resm_NodeReadRef(p)        (Resm_Regular(p)->nRefs[!Resm_IsComplement(p)])
#define Resm_NodeReadRefTotal(p)   (Resm_Regular(p)->nRefs[0]+Resm_Regular(p)->nRefs[1])

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== resFanin.c ===============================================================*/
extern Resm_Node_t *     Resm_NodeReplaceOneFanin( Resm_Node_t * pPivot, Resm_Node_t * pFanin, Resm_NodeVec_t * vNodes );
/*=== resFil.c ===============================================================*/
extern Resm_Node_t *     Resm_CheckCut( Resm_Node_t * pRoot, Resm_Node_t * ppNodes[], int nNodes, int fFilterTop );
extern Resm_Node_t *     Resm_CheckCutTop( Resm_Node_t * pRoot, Resm_Node_t * ppNodes[], int nNodes );
extern Resm_Node_t *     Resm_SelectBestCut( Resm_Node_t * pNode, Resm_Node_t * pCutList );
/*=== resFilSat.c ===============================================================*/
extern Resm_Node_t *     Resm_CheckSat( Resm_Man_t * p, Resm_Node_t * pPivot, Resm_Node_t * ppCands[], int nCands );
/*=== resFilSim.c ===============================================================*/
extern int               Resm_TestNodes( Resm_Man_t * p, Resm_Node_t * pRoot, Resm_Node_t * pNodes[], int nNodes );
extern void              Resm_SimulateWindow( Resm_Man_t * p );
/*=== resFilTop.c ===============================================================*/
extern int               Resm_TestTopological( Resm_Man_t * pMan, Resm_Node_t * pRoot, Resm_Node_t * pNodes[], int nNodes );
/*=== resImage.c ===============================================================*/
extern int               Resm_ResynthesisImage( Resm_Node_t * pPivot, Resm_Node_t * ppCands[], int nCands );
/*=== resLogic.c ===============================================================*/
extern void              Resm_CollectRelatedLogic( Resm_Node_t * pNode );
/*=== resMatch.c ===============================================================*/
extern void              Resm_MatchCuts( Resm_Node_t * pNode, Resm_Node_t * pCutList );
extern int               Resm_MatchCompare( Resm_Node_t * pNode, Resm_Node_t * pCut ); 
/*=== resNet.c ===============================================================*/
extern float             Resm_NodeInsertFaninFanouts( Resm_Node_t * pNode, int fRecursive );
extern float             Resm_NodeRemoveFaninFanouts( Resm_Node_t * pNode, int fRecursive );
extern void              Resm_NodeInsertFaninFanoutsOnly( Resm_Node_t * pNode );
extern void              Resm_NodeRemoveFaninFanoutsOnly( Resm_Node_t * pNode );
extern void              Resm_NodeTransferFanouts( Resm_Node_t * pNodeOld, Resm_Node_t * pNodeNew, int fCompl );
extern Resm_Node_t *     Rems_ConstructSuper( Resm_Cut_t * pCut );
/*=== resNode.c ===============================================================*/
extern int               Resm_NodeResynthesize( Resm_Node_t * pNode );
extern Resm_Node_t *     Resm_NodeResynthesizeFanin( Resm_Node_t * pNode, Resm_Node_t * pFanin );
extern void              Resm_NodeOrderFanins( Resm_Node_t * pNode, int * pPerm );
extern int               Resm_NodeIsCritical( Resm_Node_t * pNode ); 
/*=== resSupp.c ===============================================================*/
extern void              Resm_ManGetTrueSupports( Resm_Man_t * p );
/*=== resTime.c ===============================================================*/
extern int               Resm_TimesAreEqual( Resm_Time_t * pTime1, Resm_Time_t * pTime2, float tDelta );
extern int               Resm_WorstsAreEqual( Resm_Time_t * pTime1, Resm_Time_t * pTime2, float tDelta );
extern void              Resm_NodeComputeArrival_rec( Resm_Node_t * pNode );
extern void              Resm_ComputeArrivalAll( Resm_Man_t * p, int fVerify );
extern void              Resm_NodeComputeArrival( Resm_Node_t * pNode );
extern void              Resm_NodeComputeArrivalVerify( Resm_Node_t * pNode );
extern void              Resm_TimeComputeRequiredGlobal( Resm_Man_t * p );
extern void              Resm_TimeComputeRequired( Resm_Man_t * p, float fRequired );
extern void              Resm_TimePropagateRequired( Resm_Man_t * p, Resm_NodeVec_t * vNodes );
extern void              Resm_NodeComputeRequired( Resm_Node_t * pNode );
/*=== resUpdate.c ===============================================================*/
extern void              Resm_NodeUpdateArrivalTimeSlow( Resm_Node_t * pNode );
extern void              Resm_NodeUpdateRequiredTimeSlow( Resm_Node_t * pNode );
extern void              Resm_NodeUpdateArrivalTime( Resm_Node_t * pNode );
extern void              Resm_NodeUpdateRequiredTime( Resm_Node_t * pNodeOld, Resm_Node_t * pNodeNew );
/*=== resUtils.c ===============================================================*/
extern Resm_NodeVec_t *  Resm_CollectNodesDfs( Resm_Man_t * p );
extern int               Resm_VerifyRefs( Resm_Man_t * p );
extern void              Resm_MatchExpandTruth( unsigned uTruth[2], int nVars );
extern float             Resm_NodeGetAreaRefed( Resm_Node_t * pNode );
extern float             Resm_NodeGetAreaDerefed( Resm_Node_t * pNode );
extern float             Resm_NodeAreaRef( Resm_Node_t * pNode );
extern float             Resm_NodeAreaDeref( Resm_Node_t * pNode );
extern float             Resm_MappingArea( Resm_Man_t * p );
extern float             Resm_ComputeStaticArea( Resm_Man_t * p );
extern Resm_Node_t *     Resm_NodeListAppend( Resm_Node_t * pNodeAll, Resm_Node_t * pNodes );
extern void              Resm_NodeListRecycle( Resm_Man_t * p, Resm_Node_t * pNodeList, Resm_Node_t * pSave );
extern int               Resm_NodeListCount( Resm_Node_t * pNodes );
extern void              Resm_NodePrint( Resm_Node_t * pRoot );
extern int               Resm_CheckNoDuplicates( Resm_Node_t * pRoot, Resm_Node_t * ppNodes[], int nNodes );
/*=== mapperVec.c =============================================================*/
extern Resm_NodeVec_t *  Resm_NodeVecAlloc( int nCap );
extern Resm_NodeVec_t *  Resm_NodeVecDup( Resm_NodeVec_t * p );
extern void              Resm_NodeVecFree( Resm_NodeVec_t * p );
extern Resm_Node_t **    Resm_NodeVecReadArray( Resm_NodeVec_t * p );
extern int               Resm_NodeVecReadSize( Resm_NodeVec_t * p );
extern void              Resm_NodeVecGrow( Resm_NodeVec_t * p, int nCapMin );
extern void              Resm_NodeVecShrink( Resm_NodeVec_t * p, int nSizeNew );
extern void              Resm_NodeVecClear( Resm_NodeVec_t * p );
extern void              Resm_NodeVecPush( Resm_NodeVec_t * p, Resm_Node_t * Entry );
extern int               Resm_NodeVecPushUnique( Resm_NodeVec_t * p, Resm_Node_t * Entry );
extern Resm_Node_t *     Resm_NodeVecPop( Resm_NodeVec_t * p );
extern void              Resm_NodeVecWriteEntry( Resm_NodeVec_t * p, int i, Resm_Node_t * Entry );
extern Resm_Node_t *     Resm_NodeVecReadEntry( Resm_NodeVec_t * p, int i );
extern void              Resm_NodeVecSortByArrival( Resm_NodeVec_t * p, int fIncreasing );
extern void              Resm_NodeVecPushOrder( Resm_NodeVec_t * p, Resm_Node_t * pNode, int fIncreasing );
extern int               Resm_NodeVecPushUniqueOrder( Resm_NodeVec_t * p, Resm_Node_t * pNode, int fIncreasing );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
