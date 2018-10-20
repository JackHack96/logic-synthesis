/**CFile****************************************************************

  FileName    [satInt.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Internal definitions of the solver.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satInt.h,v 1.1 2005/07/08 01:01:27 alanmi Exp $]

***********************************************************************/

#ifndef __SAT_INT_H__
#define __SAT_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

//#include "leaks.h"       
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "sat.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
typedef __int64     int64;
#else
typedef long long   int64;
#endif

// outputs the runtime in seconds
#define PRT(a,t) \
    printf( "%s = ", (a) ); printf( "%6.2f sec\n", (float)(t)/(float)(CLOCKS_PER_SEC) )

// memory management macros
#define ALLOC(type, num)	\
    ((type *) malloc(sizeof(type) * (num)))
#define REALLOC(type, obj, num)	\
    (obj) ? ((type *) realloc((char *) obj, sizeof(type) * (num))) : \
	    ((type *) malloc(sizeof(type) * (num)))
#define FREE(obj)		\
    ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)

// By default, custom memory management is used
// which guarantees constant time allocation/deallocation 
// for SAT clauses and other frequently modified objects.
// For debugging, it is possible use system memory management
// directly. In which case, uncomment the macro below.
//#define USE_SYSTEM_MEMORY_MANAGEMENT 

// internal data structures
typedef struct Sat_Clause_t_      Sat_Clause_t;
typedef struct Sat_Queue_t_       Sat_Queue_t;
typedef struct Sat_Order_t_       Sat_Order_t;
typedef struct Sat_Resol_t_       Sat_Resol_t;
// memory managers (duplicated from Extra for stand-aloneness)
typedef struct Sat_MmFixed_t_     Sat_MmFixed_t;    
typedef struct Sat_MmFlex_t_      Sat_MmFlex_t;     
typedef struct Sat_MmStep_t_      Sat_MmStep_t;     
// variables and literals
typedef int                       Sat_Lit_t;  
typedef int                       Sat_Var_t;  
// the type of return value
#define SAT_VAR_UNASSIGNED (-1)
#define SAT_LIT_UNASSIGNED (-2)
#define SAT_ORDER_UNKNOWN  (-3)

// printing the search tree
#define L_IND      "%-*d"
#define L_ind      Sat_SolverReadDecisionLevel(p)*3+3,Sat_SolverReadDecisionLevel(p)
#define L_LIT      "%s%d"
#define L_lit(Lit) SAT_LITSIGN(Lit)?"-":"", SAT_LIT2VAR(Lit)+1

typedef struct Sat_SolverStats_t_ Sat_SolverStats_t;
struct Sat_SolverStats_t_
{
    int64   nStarts;        // the number of restarts
    int64   nDecisions;     // the number of decisions
    int64   nPropagations;  // the number of implications
    int64   nInspects;      // the number of times clauses are vising while watching them
    int64   nConflicts;     // the number of conflicts
    int64   nSuccesses;     // the number of sat assignments found
};

typedef struct Sat_SearchParams_t_ Sat_SearchParams_t;
struct Sat_SearchParams_t_
{
    double  dVarDecay;
    double  dClaDecay;
};

// sat solver data structure visible through all the internal files
struct Sat_Solver_t_
{
    int                 nClauses;    // the total number of clauses
    int                 nClausesStart; // the number of clauses before adding
    Sat_ClauseVec_t *   vClauses;    // problem clauses
    Sat_ClauseVec_t *   vLearned;    // learned clauses
    double              dClaInc;     // Amount to bump next clause with.
    double              dClaDecay;   // INVERSE decay factor for clause activity: stores 1/decay.

    double *            pdActivity;  // A heuristic measurement of the activity of a variable.
    double              dVarInc;     // Amount to bump next variable with.
    double              dVarDecay;   // INVERSE decay factor for variable activity: stores 1/decay. Use negative value for static variable order.
    Sat_Order_t *       pOrder;      // Keeps track of the decision variable order.

    Sat_ClauseVec_t **  pvWatched;   // 'pvWatched[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    Sat_Queue_t *       pQueue;      // Propagation queue.

    int                 nVars;       // the current number of variables
    int                 nVarsAlloc;  // the maximum allowed number of variables
    int *               pAssigns;    // The current assignments (literals or SAT_VAR_UNKOWN)
    int *               pModel;      // The satisfying assignment
    Sat_IntVec_t *      vTrail;      // List of assignments made. 
    Sat_IntVec_t *      vTrailLim;   // Separator indices for different decision levels in 'trail'.
    Sat_Clause_t **     pReasons;    // 'reason[var]' is the clause that implied the variables current value, or 'NULL' if none.
    int *               pLevel;      // 'level[var]' is the decision level at which assignment was made. 
    int                 nLevelRoot;  // Level of first proper decision.

    double              dRandSeed;   // For the internal random number generator (makes solver deterministic over different platforms). 

    bool                fVerbose;    // the verbosity flag
    double              dProgress;   // Set by 'search()'.

    // the variable cone and variable connectivity
    Sat_IntVec_t *      vConeVars;
    Sat_ClauseVec_t *   vAdjacents;

    // internal data used during conflict analysis
    int *               pSeen;       // time when a lit was seen for the last time
    int                 nSeenId;     // the id of current seeing
    Sat_IntVec_t *      vReason;     // the temporary array of literals
    Sat_IntVec_t *      vTemp;       // the temporary array of literals

    // proof recording
    bool                fProof;      // enables proof recording
    Sat_Resol_t *       pRoot;       // the root of the resolution DAG representing the proof
    Sat_ClauseVec_t *   vResols;     // the resolution DAGs for each learned clause
    Sat_ClauseVec_t *   vResolsTemp; // the storage for temporary pointers to resolution DAGs
    Sat_Clause_t **     pResols;     // fake 1-lit learned clauses for 0-level implications
    Sat_ClauseVec_t *   vTemp0;      // temporary storage for marked clauses
    Sat_ClauseVec_t *   vTemp1;      // temporary storage for marked 0-level clauses
    // interpolant computation
    bool *              pVarTypeA;   // 1 for a var indicates a var belonging to Part A

    // the memory manager
    Sat_MmStep_t *      pMem;
    Sat_MmFixed_t *     pMemProof;

    // computation of the interpolant
    unsigned            uTruths[6][2]; // the elementary truth tables
//    Sat_IntVec_t *      vVarsAC;     // these are the A+C variable (except B)
    Sat_IntVec_t *      vVarMap;     // mapping from var to its number (C) or -1 (A)

    // used to enumerate the satisfying solutions
    Sat_IntVec_t *      vProj;       // the projection variables
    Sat_IntVec_t *      vSols;       // the solutions

    // statistics
    Sat_SolverStats_t   Stats;
    int                 nTwoLits;
    int                 nTwoLitsL;
    int                 nClausesInit;
    int                 nClausesAlloc;
    int                 nClausesAllocL;
    int                 nBackTracks;
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== satActivity.c ===========================================================*/
extern void               Sat_SolverVarDecayActivity( Sat_Solver_t * p );
extern void               Sat_SolverVarRescaleActivity( Sat_Solver_t * p );
extern void               Sat_SolverClaDecayActivity( Sat_Solver_t * p );
extern void               Sat_SolverClaRescaleActivity( Sat_Solver_t * p );
/*=== satSolverApi.c ===========================================================*/
extern Sat_Resol_t *      Sat_SolverReadResol( Sat_Solver_t * p, Sat_Clause_t * pC );
extern void               Sat_SolverSetResol( Sat_Solver_t * p, Sat_Clause_t * pC, Sat_Resol_t * pResol );
extern Sat_Resol_t *      Sat_SolverReadResolTemp( Sat_Solver_t * p, Sat_Clause_t * pC );
extern void               Sat_SolverSetResolTemp( Sat_Solver_t * p, Sat_Clause_t * pC, Sat_Resol_t * pResol );
extern Sat_Clause_t *     Sat_SolverReadClause( Sat_Solver_t * p, int Num );
/*=== satSolver.c ===========================================================*/
extern int                Sat_SolverReadDecisionLevel( Sat_Solver_t * p );
extern int *              Sat_SolverReadDecisionLevelArray( Sat_Solver_t * p );
extern Sat_Clause_t **    Sat_SolverReadReasonArray( Sat_Solver_t * p );
extern Sat_Type_t         Sat_SolverReadVarValue( Sat_Solver_t * p, Sat_Var_t Var );
extern Sat_ClauseVec_t *  Sat_SolverReadLearned( Sat_Solver_t * p );
extern Sat_ClauseVec_t ** Sat_SolverReadWatchedArray( Sat_Solver_t * p );
extern int *              Sat_SolverReadSeenArray( Sat_Solver_t * p );
extern int                Sat_SolverIncrementSeenId( Sat_Solver_t * p );
extern Sat_MmStep_t *     Sat_SolverReadMem( Sat_Solver_t * p );
extern void               Sat_SolverClausesIncrement( Sat_Solver_t * p );
extern void               Sat_SolverClausesDecrement( Sat_Solver_t * p );
extern void               Sat_SolverClausesIncrementL( Sat_Solver_t * p );
extern void               Sat_SolverClausesDecrementL( Sat_Solver_t * p );
extern void               Sat_SolverVarBumpActivity( Sat_Solver_t * p, Sat_Lit_t Lit );
extern void               Sat_SolverClaBumpActivity( Sat_Solver_t * p, Sat_Clause_t * pC );
extern bool               Sat_SolverEnqueue( Sat_Solver_t * p, Sat_Lit_t Lit, Sat_Clause_t * pC );
extern void               Sat_SolverFreeResolTree( Sat_Solver_t * p, Sat_Resol_t * pResol );
extern double             Sat_SolverProgressEstimate( Sat_Solver_t * p );
/*=== satSolverSearch.c ===========================================================*/
extern bool               Sat_SolverAssume( Sat_Solver_t * p, Sat_Lit_t Lit );
extern Sat_Clause_t *     Sat_SolverPropagate( Sat_Solver_t * p );
extern void               Sat_SolverCancelUntil( Sat_Solver_t * p, int Level );
extern Sat_Type_t         Sat_SolverSearch( Sat_Solver_t * p, int nConfLimit, int nLearnedLimit, int nBackTrackLimit, Sat_SearchParams_t * pPars );
/*=== satQueue.c ===========================================================*/
extern Sat_Queue_t *      Sat_QueueAlloc( int nVars );
extern void               Sat_QueueFree( Sat_Queue_t * p );
extern int                Sat_QueueReadSize( Sat_Queue_t * p );
extern void               Sat_QueueInsert( Sat_Queue_t * p, int Lit );
extern int                Sat_QueueExtract( Sat_Queue_t * p );
extern void               Sat_QueueClear( Sat_Queue_t * p );
/*=== satOrder.c ===========================================================*/
extern Sat_Order_t *      Sat_OrderAlloc( int * pAssigns, double * pdActivity, int nVars );
extern void               Sat_OrderRealloc( Sat_Order_t * p, int * pAssigns, double * pdActivity, int nVarsAlloc );
extern void               Sat_OrderClean( Sat_Order_t * p, int nVars, int * pVars );
extern void               Sat_OrderFree( Sat_Order_t * p );
extern int                Sat_OrderSelect( Sat_Order_t * p );
extern void               Sat_OrderUpdate( Sat_Order_t * p, int Var );
extern void               Sat_OrderUndo( Sat_Order_t * p, int Var );
/*=== satClause.c ===========================================================*/
extern bool               Sat_ClauseCreate( Sat_Solver_t * p, Sat_IntVec_t * vLits, bool fLearnt, Sat_Clause_t ** pClause_out );
extern Sat_Clause_t *     Sat_ClauseCreateFake( Sat_Solver_t * p, Sat_IntVec_t * vLits );
extern Sat_Clause_t *     Sat_ClauseCreateFakeLit( Sat_Solver_t * p, Sat_Lit_t Lit );
extern bool               Sat_ClauseReadLearned( Sat_Clause_t * pC );
extern int                Sat_ClauseReadSize( Sat_Clause_t * pC );
extern int *              Sat_ClauseReadLits( Sat_Clause_t * pC );
extern bool               Sat_ClauseReadMark( Sat_Clause_t * pC );
extern void               Sat_ClauseSetMark( Sat_Clause_t * pC, bool fMark );
extern int                Sat_ClauseReadNum( Sat_Clause_t * pC );
extern void               Sat_ClauseSetNum( Sat_Clause_t * pC, int Num );
extern bool               Sat_ClauseReadTypeA( Sat_Clause_t * pC );
extern void               Sat_ClauseSetTypeA( Sat_Clause_t * pC, bool fTypeA );
extern bool               Sat_ClauseIsLocked( Sat_Solver_t * p, Sat_Clause_t * pC );
extern float              Sat_ClauseReadActivity( Sat_Clause_t * pC );
extern void               Sat_ClauseWriteActivity( Sat_Clause_t * pC, float Num );
extern void               Sat_ClauseFree( Sat_Solver_t * p, Sat_Clause_t * pC, bool fRemoveWatched );
extern bool               Sat_ClausePropagate( Sat_Clause_t * pC, Sat_Lit_t Lit, int * pAssigns, Sat_Lit_t * pLit_out );
extern bool               Sat_ClauseSimplify( Sat_Clause_t * pC, int * pAssigns );
extern void               Sat_ClauseCalcReason( Sat_Solver_t * p, Sat_Clause_t * pC, Sat_Lit_t Lit, Sat_IntVec_t * vLits_out );
extern void               Sat_ClauseRemoveWatch( Sat_ClauseVec_t * vClauses, Sat_Clause_t * pC );
extern void               Sat_ClausePrint( Sat_Clause_t * pC );
extern void               Sat_ClausePrintSymbols( Sat_Clause_t * pC );
extern void               Sat_ClauseWriteDimacs( FILE * pFile, Sat_Clause_t * pC, bool fIncrement );
extern unsigned           Sat_ClauseComputeTruth( Sat_Solver_t * p, Sat_Clause_t * pC );
/*=== satSort.c ===========================================================*/
extern void               Sat_SolverSortDB( Sat_Solver_t * p );
/*=== satProof.c ===========================================================*/
extern Sat_Resol_t *      Sat_SolverDeriveClauseProof( Sat_Solver_t * p, Sat_Clause_t * pC, int LitUIP );
extern void               Sat_SolverFreeResolTree_rec( Sat_Solver_t * p, Sat_Resol_t * pR );
extern void               Sat_SolverSetResolClause( Sat_Resol_t * pR, Sat_Clause_t * pC );
/*=== satClauseVec.c ===========================================================*/
extern Sat_ClauseVec_t *  Sat_ClauseVecAlloc( int nCap );
extern void               Sat_ClauseVecFree( Sat_ClauseVec_t * p );
extern Sat_Clause_t **    Sat_ClauseVecReadArray( Sat_ClauseVec_t * p );
extern int                Sat_ClauseVecReadSize( Sat_ClauseVec_t * p );
extern void               Sat_ClauseVecGrow( Sat_ClauseVec_t * p, int nCapMin );
extern void               Sat_ClauseVecShrink( Sat_ClauseVec_t * p, int nSizeNew );
extern void               Sat_ClauseVecClear( Sat_ClauseVec_t * p );
extern void               Sat_ClauseVecPush( Sat_ClauseVec_t * p, Sat_Clause_t * Entry );
extern Sat_Clause_t *     Sat_ClauseVecPop( Sat_ClauseVec_t * p );
extern void               Sat_ClauseVecWriteEntry( Sat_ClauseVec_t * p, int i, Sat_Clause_t * Entry );
extern Sat_Clause_t *     Sat_ClauseVecReadEntry( Sat_ClauseVec_t * p, int i );

/*=== satMem.c ===========================================================*/
// fixed-size-block memory manager
extern Sat_MmFixed_t *    Sat_MmFixedStart( int nEntrySize, int nChunkSize, int nChunksAlloc );
extern void               Sat_MmFixedStop( Sat_MmFixed_t * p, int fVerbose );
extern char *             Sat_MmFixedEntryFetch( Sat_MmFixed_t * p );
extern void               Sat_MmFixedEntryRecycle( Sat_MmFixed_t * p, char * pEntry );
extern void               Sat_MmFixedRestart( Sat_MmFixed_t * p );
extern int                Sat_MmFixedReadMemUsage( Sat_MmFixed_t * p );
// flexible-size-block memory manager
extern Sat_MmFlex_t *     Sat_MmFlexStart( int nChunkSize, int nChunksAlloc );
extern void               Sat_MmFlexStop( Sat_MmFlex_t * p, int fVerbose );
extern char *             Sat_MmFlexEntryFetch( Sat_MmFlex_t * p, int nBytes );
extern int                Sat_MmFlexReadMemUsage( Sat_MmFlex_t * p );
// hierarchical memory manager
extern Sat_MmStep_t *     Sat_MmStepStart( int nSteps );
extern void               Sat_MmStepStop( Sat_MmStep_t * p, int fVerbose );
extern char *             Sat_MmStepEntryFetch( Sat_MmStep_t * p, int nBytes );
extern void               Sat_MmStepEntryRecycle( Sat_MmStep_t * p, char * pEntry, int nBytes );
extern int                Sat_MmStepReadMemUsage( Sat_MmStep_t * p );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
