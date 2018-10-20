/**CFile****************************************************************

  FileName    [sat.h]
 
  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [External definitions of the solver.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: sat.h,v 1.1 2005/07/08 01:01:26 alanmi Exp $]

***********************************************************************/

#ifndef __SAT_H__
#define __SAT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifdef bool
#undef bool
#endif

#ifndef __MVTYPES_H__
typedef int  bool;
#endif

typedef struct Sat_Solver_t_      Sat_Solver_t;

// the vector of intergers and of clauses
typedef struct Sat_IntVec_t_      Sat_IntVec_t;
typedef struct Sat_ClauseVec_t_   Sat_ClauseVec_t;
typedef struct Sat_VarHeap_t_     Sat_VarHeap_t;

// the return value of the solver
typedef enum { SAT_FALSE = -1, SAT_UNKNOWN = 0, SAT_TRUE = 1 } Sat_Type_t;

// representation of variables and literals
// the literal (l) is the variable (v) and the sign (s)
// s = 0 the variable is positive
// s = 1 the variable is negative
#define SAT_VAR2LIT(v,s) (2*(v)+(s)) 
#define SAT_LITNOT(l)    ((l)^1)
#define SAT_LITSIGN(l)   ((l)&1)     
#define SAT_LIT2VAR(l)   ((l)>>1)

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DECLARATIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== satSolver.c ===========================================================*/
// adding vars, clauses, simplifying the database, and solving
extern bool            Sat_SolverAddVar( Sat_Solver_t * p );
extern bool            Sat_SolverAddClause( Sat_Solver_t * p, Sat_IntVec_t * pLits );
extern bool            Sat_SolverSimplifyDB( Sat_Solver_t * p );
extern bool            Sat_SolverSolve( Sat_Solver_t * p, Sat_IntVec_t * pVecAssumps, Sat_IntVec_t * vProj, int nBackTrackLimit );
// printing stats, assignments, and clauses
extern void            Sat_SolverPrintStats( Sat_Solver_t * p );
extern void            Sat_SolverPrintAssignment( Sat_Solver_t * p );
extern void            Sat_SolverPrintClauses( Sat_Solver_t * p );
extern void            Sat_SolverWriteDimacs( Sat_Solver_t * p, char * pFileName );
// access to the solver internal data
extern int             Sat_SolverReadVarNum( Sat_Solver_t * p );
extern int             Sat_SolverReadVarAllocNum( Sat_Solver_t * p );
extern int *           Sat_SolverReadAssignsArray( Sat_Solver_t * p );
extern int *           Sat_SolverReadModelArray( Sat_Solver_t * p );
extern unsigned        Sat_SolverReadTruth( Sat_Solver_t * p );
extern int             Sat_SolverReadBackTracks( Sat_Solver_t * p );
extern void            Sat_SolverSetVerbosity( Sat_Solver_t * p, int fVerbose );
extern void            Sat_SolverSetProofWriting( Sat_Solver_t * p, int fProof );
extern void            Sat_SolverSetVarTypeA( Sat_Solver_t * p, int Var );
extern void            Sat_SolverSetVarMap( Sat_Solver_t * p, Sat_IntVec_t * vVarMap );
extern void            Sat_SolverMarkLastClauseTypeA( Sat_Solver_t * p );
extern void            Sat_SolverMarkClausesStart( Sat_Solver_t * p );
// returns the solution after incremental solving
extern int             Sat_SolverReadSolutions( Sat_Solver_t * p );
extern int *           Sat_SolverReadSolutionsArray( Sat_Solver_t * p );
/*=== satSolverSearch.c ===========================================================*/
extern void            Sat_SolverRemoveLearned( Sat_Solver_t * p );
extern void            Sat_SolverRemoveMarked( Sat_Solver_t * p );
/*=== satRead.c ===========================================================*/
extern bool            Sat_SolverParseDimacs( FILE * pFile, Sat_Solver_t ** p, int fVerbose, bool fProof );
/*=== satSolverApi.c ===========================================================*/
// allocation, cleaning, and freeing the solver
extern Sat_Solver_t *  Sat_SolverAlloc( int nVars, double dClaInc, double dClaDecay, double dVarInc, double dVarDecay, bool fVerbose );
extern void            Sat_SolverResize( Sat_Solver_t * pMan, int nVarsAlloc );
extern void            Sat_SolverClean( Sat_Solver_t * p, int nVars );
extern void            Sat_SolverPrepare( Sat_Solver_t * pSat, Sat_IntVec_t * vVars );
extern void            Sat_SolverFree( Sat_Solver_t * p );
/*=== satVec.c ===========================================================*/
extern Sat_IntVec_t *  Sat_IntVecAlloc( int nCap );
extern Sat_IntVec_t *  Sat_IntVecAllocArray( int * pArray, int nSize );
extern Sat_IntVec_t *  Sat_IntVecAllocArrayCopy( int * pArray, int nSize );
extern Sat_IntVec_t *  Sat_IntVecDup( Sat_IntVec_t * pVec );
extern Sat_IntVec_t *  Sat_IntVecDupArray( Sat_IntVec_t * pVec );
extern void            Sat_IntVecFree( Sat_IntVec_t * p );
extern void            Sat_IntVecFill( Sat_IntVec_t * p, int nSize, int Entry );
extern int *           Sat_IntVecReleaseArray( Sat_IntVec_t * p );
extern int *           Sat_IntVecReadArray( Sat_IntVec_t * p );
extern int             Sat_IntVecReadSize( Sat_IntVec_t * p );
extern int             Sat_IntVecReadEntry( Sat_IntVec_t * p, int i );
extern int             Sat_IntVecReadEntryLast( Sat_IntVec_t * p );
extern void            Sat_IntVecWriteEntry( Sat_IntVec_t * p, int i, int Entry );
extern void            Sat_IntVecGrow( Sat_IntVec_t * p, int nCapMin );
extern void            Sat_IntVecShrink( Sat_IntVec_t * p, int nSizeNew );
extern void            Sat_IntVecClear( Sat_IntVec_t * p );
extern void            Sat_IntVecPush( Sat_IntVec_t * p, int Entry );
extern int             Sat_IntVecPop( Sat_IntVec_t * p );
extern void            Sat_IntVecSort( Sat_IntVec_t * p, int fReverse );
/*=== satProof.c ===========================================================*/
extern Sat_ClauseVec_t *  Sat_SolverCollectProof( Sat_Solver_t * p );
extern unsigned        Sat_SolverCollectProofTruth( Sat_Solver_t * p );
extern unsigned        Sat_SolverReadTruth( Sat_Solver_t * pSat );
extern void            Sat_SolverPrintProof( Sat_Solver_t * p, Sat_ClauseVec_t * pProof, char * pFileName );
extern void            Sat_SolverVerifyProof( char * pFileName, char * pFileNameTrace );
/*=== satInter.c ===========================================================*/
extern char * Sat_SolverComputeInterpolant( Sat_Solver_t * p, 
    Sat_ClauseVec_t * vProof, 
    char * pMan,
    char * (* pProcConst0)(char * pMan), 
    char * (* pProcConst1)(char * pMan), 
    char * (* pProcVar)(char * pMan, int Var),
    char * (* pProcAnd)(char * pMan, char * pArg1, char * pArg2),
    char * (* pProcOr )(char * pMan, char * pArg1, char * pArg2),
    char * (* pProcNot)(char * pArg), 
    void   (* pProcRef)(char * pArg), 
    void   (* pProcRecDeref)(char * pMan, char * pArg) );

/*=== satHeap.c ===========================================================*/
extern Sat_VarHeap_t *  Sat_VarHeapAlloc();
extern void             Sat_VarHeapSetActivity( Sat_VarHeap_t * p, double * pActivity );
extern void             Sat_VarHeapStart( Sat_VarHeap_t * p, int * pVars, int nVars, int nVarsAlloc );
extern void             Sat_VarHeapGrow( Sat_VarHeap_t * p, int nSize );
extern void             Sat_VarHeapStop( Sat_VarHeap_t * p );
extern void             Sat_VarHeapPrint( FILE * pFile, Sat_VarHeap_t * p );
extern void             Sat_VarHeapCheck( Sat_VarHeap_t * p );
extern void             Sat_VarHeapCheckOne( Sat_VarHeap_t * p, int iVar );
extern int              Sat_VarHeapContainsVar( Sat_VarHeap_t * p, int iVar );
extern void             Sat_VarHeapInsert( Sat_VarHeap_t * p, int iVar );  
extern void             Sat_VarHeapUpdate( Sat_VarHeap_t * p, int iVar );  
extern void             Sat_VarHeapDelete( Sat_VarHeap_t * p, int iVar );  
extern double           Sat_VarHeapReadMaxWeight( Sat_VarHeap_t * p );
extern int              Sat_VarHeapCountNodes( Sat_VarHeap_t * p, double WeightLimit );  
extern int              Sat_VarHeapReadMax( Sat_VarHeap_t * p );  
extern int              Sat_VarHeapGetMax( Sat_VarHeap_t * p );  


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
