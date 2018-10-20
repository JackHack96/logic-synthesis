/**CFile****************************************************************

  FileName    [mntkInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: mntkInt.h,v 1.1 2005/02/28 05:34:31 alanmi Exp $]

***********************************************************************/
 
#ifndef __MNTK_INT_H__
#define __MNTK_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mio.h"
#include "mapper.h"
#include "sat.h"
#include "mntk.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 
////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

#define MNTK_SIM_ROUNDS           64

// maximum/minimum operators
#define MNTK_MIN(a,b)             (((a) < (b))? (a) : (b))
#define MNTK_MAX(a,b)             (((a) > (b))? (a) : (b))

// the small and large numbers (min/max float are 1.17e-38/3.40e+38)
#define MNTK_FLOAT_LARGE          ((float)1.0e+30)
#define MNTK_FLOAT_SMALL          ((float)1.0e-03)

// generating random unsigned (#define RAND_MAX 0x7fff)
#define MNTK_RANDOM_UNSIGNED   ((((unsigned)rand()) << 24) ^ (((unsigned)rand()) << 12) ^ ((unsigned)rand()))

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the mapping manager
struct Mntk_ManStruct_t_
{
    // the nodes used for resynthesis
    Mntk_Node_t **      pInputs;       // the array of inputs
    int                 nInputs;       // the number of inputs
    Mntk_Node_t **      pOutputs;      // the array of outputs
    int                 nOutputs;      // the number of outputs
    Mntk_Node_t *       pConst1;       // the constant 1 node
    Mntk_Node_t *       pPseudo;       // the pseudo-node whose fanins are POs
    Mntk_NodeVec_t *    vNodesAll;     // the array of all nodes
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
//    Mntk_NodeVec_t *    vLevels;       // the levels of the AND-nodes
//    Mntk_NodeVec_t **   pvStack;       // the stack used for propagating changes
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
    Mntk_NodeVec_t *    vWinCands;     // the internal nodes of the window
    Mntk_NodeVec_t *    vWinTotal;     // the union of all nodes
    Mntk_NodeVec_t *    vWinNodes;     // the internal nodes used for SAT
    Mntk_NodeVec_t *    vWinVisit;     // the TFO of the node
    Mntk_NodeVec_t *    vWinTotalCone; // the union of all nodes
    Mntk_NodeVec_t *    vWinNodesCone; // the internal nodes used for SAT

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

struct Mntk_NodeStruct_t_
{
    Mntk_Man_t *        p;             // the parent manager
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
    Mntk_Node_t **      ppLeaves;      // the pointer to the array of leaves
    Mntk_Node_t *       ppArray[MNTK_MAX_FANINS];    // the leaves (can be complemented)
    Mntk_NodeVec_t *    vFanouts;      // the array of fanouts
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
    Mntk_Node_t *       pNext;         // the next node
};

// the vector of nodes
struct Mntk_NodeVecStruct_t_
{
    Mntk_Node_t **      pArray;        // the array of nodes
    int                 nSize;         // the number of entries in the array
    int                 nCap;          // the number of allocated entries
};

#define Mntk_NodeRef(p)            (Mntk_Regular(p)->nRefs[!Mntk_IsComplement(p)]++)
#define Mntk_NodeDeref(p)          (Mntk_Regular(p)->nRefs[!Mntk_IsComplement(p)]--)
#define Mntk_NodeReadRef(p)        (Mntk_Regular(p)->nRefs[!Mntk_IsComplement(p)])
#define Mntk_NodeReadRefTotal(p)   (Mntk_Regular(p)->nRefs[0]+Mntk_Regular(p)->nRefs[1])

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== resFanin.c ===============================================================*/
extern Mntk_Node_t *     Mntk_NodeReplaceOneFanin( Mntk_Node_t * pPivot, Mntk_Node_t * pFanin, Mntk_NodeVec_t * vNodes );
/*=== resFil.c ===============================================================*/
extern Mntk_Node_t *     Mntk_CheckCut( Mntk_Node_t * pRoot, Mntk_Node_t * ppNodes[], int nNodes, int fFilterTop );
extern Mntk_Node_t *     Mntk_CheckCutTop( Mntk_Node_t * pRoot, Mntk_Node_t * ppNodes[], int nNodes );
extern Mntk_Node_t *     Mntk_SelectBestCut( Mntk_Node_t * pNode, Mntk_Node_t * pCutList );
/*=== resFilSat.c ===============================================================*/
extern Mntk_Node_t *     Mntk_CheckSat( Mntk_Man_t * p, Mntk_Node_t * pPivot, Mntk_Node_t * ppCands[], int nCands );
/*=== resFilSim.c ===============================================================*/
extern int               Mntk_TestNodes( Mntk_Man_t * p, Mntk_Node_t * pRoot, Mntk_Node_t * pNodes[], int nNodes );
extern void              Mntk_SimulateWindow( Mntk_Man_t * p );
/*=== resFilTop.c ===============================================================*/
extern int               Mntk_TestTopological( Mntk_Man_t * pMan, Mntk_Node_t * pRoot, Mntk_Node_t * pNodes[], int nNodes );
/*=== resImage.c ===============================================================*/
extern int               Mntk_ResynthesisImage( Mntk_Node_t * pPivot, Mntk_Node_t * ppCands[], int nCands );
/*=== resLogic.c ===============================================================*/
extern void              Mntk_CollectRelatedLogic( Mntk_Node_t * pNode );
/*=== resMatch.c ===============================================================*/
extern void              Mntk_MatchCuts( Mntk_Node_t * pNode, Mntk_Node_t * pCutList );
extern int               Mntk_MatchCompare( Mntk_Node_t * pNode, Mntk_Node_t * pCut ); 
/*=== resNet.c ===============================================================*/
extern float             Mntk_NodeInsertFaninFanouts( Mntk_Node_t * pNode, int fRecursive );
extern float             Mntk_NodeRemoveFaninFanouts( Mntk_Node_t * pNode, int fRecursive );
extern void              Mntk_NodeInsertFaninFanoutsOnly( Mntk_Node_t * pNode );
extern void              Mntk_NodeRemoveFaninFanoutsOnly( Mntk_Node_t * pNode );
extern void              Mntk_NodeTransferFanouts( Mntk_Node_t * pNodeOld, Mntk_Node_t * pNodeNew, int fCompl );
extern Mntk_Node_t *     Rems_ConstructSuper( Mntk_Cut_t * pCut );
/*=== resNode.c ===============================================================*/
extern int               Mntk_NodeResynthesize( Mntk_Node_t * pNode );
extern Mntk_Node_t *     Mntk_NodeResynthesizeFanin( Mntk_Node_t * pNode, Mntk_Node_t * pFanin );
extern void              Mntk_NodeOrderFanins( Mntk_Node_t * pNode, int * pPerm );
extern int               Mntk_NodeIsCritical( Mntk_Node_t * pNode ); 
/*=== resSupp.c ===============================================================*/
extern void              Mntk_ManGetTrueSupports( Mntk_Man_t * p );
/*=== resTime.c ===============================================================*/
extern int               Mntk_TimesAreEqual( Mntk_Time_t * pTime1, Mntk_Time_t * pTime2, float tDelta );
extern int               Mntk_WorstsAreEqual( Mntk_Time_t * pTime1, Mntk_Time_t * pTime2, float tDelta );
extern void              Mntk_NodeComputeArrival_rec( Mntk_Node_t * pNode );
extern void              Mntk_ComputeArrivalAll( Mntk_Man_t * p, int fVerify );
extern void              Mntk_NodeComputeArrival( Mntk_Node_t * pNode );
extern void              Mntk_NodeComputeArrivalVerify( Mntk_Node_t * pNode );
extern void              Mntk_TimeComputeRequiredGlobal( Mntk_Man_t * p );
extern void              Mntk_TimeComputeRequired( Mntk_Man_t * p, float fRequired );
extern void              Mntk_TimePropagateRequired( Mntk_Man_t * p, Mntk_NodeVec_t * vNodes );
extern void              Mntk_NodeComputeRequired( Mntk_Node_t * pNode );
/*=== resUpdate.c ===============================================================*/
extern void              Mntk_NodeUpdateArrivalTimeSlow( Mntk_Node_t * pNode );
extern void              Mntk_NodeUpdateRequiredTimeSlow( Mntk_Node_t * pNode );
extern void              Mntk_NodeUpdateArrivalTime( Mntk_Node_t * pNode );
extern void              Mntk_NodeUpdateRequiredTime( Mntk_Node_t * pNodeOld, Mntk_Node_t * pNodeNew );
/*=== resUtils.c ===============================================================*/
extern Mntk_NodeVec_t *  Mntk_CollectNodesDfs( Mntk_Man_t * p );
extern int               Mntk_VerifyRefs( Mntk_Man_t * p );
extern void              Mntk_MatchExpandTruth( unsigned uTruth[2], int nVars );
extern float             Mntk_NodeGetAreaRefed( Mntk_Node_t * pNode );
extern float             Mntk_NodeGetAreaDerefed( Mntk_Node_t * pNode );
extern float             Mntk_NodeAreaRef( Mntk_Node_t * pNode );
extern float             Mntk_NodeAreaDeref( Mntk_Node_t * pNode );
extern float             Mntk_MappingArea( Mntk_Man_t * p );
extern float             Mntk_ComputeStaticArea( Mntk_Man_t * p );
extern Mntk_Node_t *     Mntk_NodeListAppend( Mntk_Node_t * pNodeAll, Mntk_Node_t * pNodes );
extern void              Mntk_NodeListRecycle( Mntk_Man_t * p, Mntk_Node_t * pNodeList, Mntk_Node_t * pSave );
extern int               Mntk_NodeListCount( Mntk_Node_t * pNodes );
extern void              Mntk_NodePrint( Mntk_Node_t * pRoot );
extern int               Mntk_CheckNoDuplicates( Mntk_Node_t * pRoot, Mntk_Node_t * ppNodes[], int nNodes );
/*=== mapperVec.c =============================================================*/
extern Mntk_NodeVec_t *  Mntk_NodeVecAlloc( int nCap );
extern Mntk_NodeVec_t *  Mntk_NodeVecDup( Mntk_NodeVec_t * p );
extern void              Mntk_NodeVecFree( Mntk_NodeVec_t * p );
extern Mntk_Node_t **    Mntk_NodeVecReadArray( Mntk_NodeVec_t * p );
extern int               Mntk_NodeVecReadSize( Mntk_NodeVec_t * p );
extern void              Mntk_NodeVecGrow( Mntk_NodeVec_t * p, int nCapMin );
extern void              Mntk_NodeVecShrink( Mntk_NodeVec_t * p, int nSizeNew );
extern void              Mntk_NodeVecClear( Mntk_NodeVec_t * p );
extern void              Mntk_NodeVecPush( Mntk_NodeVec_t * p, Mntk_Node_t * Entry );
extern int               Mntk_NodeVecPushUnique( Mntk_NodeVec_t * p, Mntk_Node_t * Entry );
extern Mntk_Node_t *     Mntk_NodeVecPop( Mntk_NodeVec_t * p );
extern void              Mntk_NodeVecWriteEntry( Mntk_NodeVec_t * p, int i, Mntk_Node_t * Entry );
extern Mntk_Node_t *     Mntk_NodeVecReadEntry( Mntk_NodeVec_t * p, int i );
extern void              Mntk_NodeVecSortByArrival( Mntk_NodeVec_t * p, int fIncreasing );
extern void              Mntk_NodeVecPushOrder( Mntk_NodeVec_t * p, Mntk_Node_t * pNode, int fIncreasing );
extern int               Mntk_NodeVecPushUniqueOrder( Mntk_NodeVec_t * p, Mntk_Node_t * pNode, int fIncreasing );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
