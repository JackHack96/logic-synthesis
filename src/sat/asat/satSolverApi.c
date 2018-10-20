/**CFile****************************************************************

  FileName    [satSolverApi.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [APIs of the SAT solver.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satSolverApi.c,v 1.1 2005/07/08 01:01:28 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Sat_SolverSetupTruthTables( unsigned uTruths[][2] );

extern unsigned      Sat_ResolReadTruth( Sat_Resol_t * pR );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Simple SAT solver APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int                Sat_SolverReadVarNum( Sat_Solver_t * p )                  { return p->nVars;     }
int                Sat_SolverReadVarAllocNum( Sat_Solver_t * p )             { return p->nVarsAlloc;}
int                Sat_SolverReadDecisionLevel( Sat_Solver_t * p )           { return Sat_IntVecReadSize(p->vTrailLim); }
int *              Sat_SolverReadDecisionLevelArray( Sat_Solver_t * p )      { return p->pLevel;    }
Sat_Clause_t **    Sat_SolverReadReasonArray( Sat_Solver_t * p )             { return p->pReasons;  }
Sat_Lit_t          Sat_SolverReadVarValue( Sat_Solver_t * p, Sat_Var_t Var ) { return p->pAssigns[Var]; }
Sat_ClauseVec_t *  Sat_SolverReadLearned( Sat_Solver_t * p )                 { return p->vLearned;  }
Sat_ClauseVec_t ** Sat_SolverReadWatchedArray( Sat_Solver_t * p )            { return p->pvWatched; }
int *              Sat_SolverReadAssignsArray( Sat_Solver_t * p )            { return p->pAssigns;  }
int *              Sat_SolverReadModelArray( Sat_Solver_t * p )              { return p->pModel;  }
int                Sat_SolverReadBackTracks( Sat_Solver_t * p )              { return p->nBackTracks; }
Sat_MmStep_t *     Sat_SolverReadMem( Sat_Solver_t * p )                     { return p->pMem;      }
int                Sat_SolverReadSolutions( Sat_Solver_t * p )               { return Sat_IntVecReadSize(p->vSols);  }
int *              Sat_SolverReadSolutionsArray( Sat_Solver_t * p )          { return Sat_IntVecReadArray(p->vSols); }
int *              Sat_SolverReadSeenArray( Sat_Solver_t * p )               { return p->pSeen;     }
int                Sat_SolverIncrementSeenId( Sat_Solver_t * p )             { return ++p->nSeenId; }
void               Sat_SolverSetVerbosity( Sat_Solver_t * p, int fVerbose )  { p->fVerbose = fVerbose; }
void               Sat_SolverSetProofWriting( Sat_Solver_t * p, int fProof ) { p->fProof   = fProof;   }
void               Sat_SolverClausesIncrement( Sat_Solver_t * p )            { p->nClausesAlloc++;  }
void               Sat_SolverClausesDecrement( Sat_Solver_t * p )            { p->nClausesAlloc--;  }
void               Sat_SolverClausesIncrementL( Sat_Solver_t * p )           { p->nClausesAllocL++; }
void               Sat_SolverClausesDecrementL( Sat_Solver_t * p )           { p->nClausesAllocL--; }
void               Sat_SolverMarkLastClauseTypeA( Sat_Solver_t * p )         { Sat_ClauseSetTypeA( Sat_ClauseVecReadEntry( p->vClauses, Sat_ClauseVecReadSize(p->vClauses)-1 ), 1 ); }
void               Sat_SolverMarkClausesStart( Sat_Solver_t * p )            { p->nClausesStart = Sat_ClauseVecReadSize(p->vClauses); }

/**Function*************************************************************

  Synopsis    [Reads resolution of the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Resol_t * Sat_SolverReadResol( Sat_Solver_t * p, Sat_Clause_t * pC )
{
    return (Sat_Resol_t *)Sat_ClauseVecReadEntry( p->vResols, Sat_ClauseReadNum(pC) );
}

/**Function*************************************************************

  Synopsis    [Sets the resolution of the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverSetResol( Sat_Solver_t * p, Sat_Clause_t * pC, Sat_Resol_t * pResol )
{
    Sat_ClauseVecWriteEntry( p->vResols, Sat_ClauseReadNum(pC), (Sat_Clause_t *)pResol );
}

/**Function*************************************************************

  Synopsis    [Reads resolution of the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Resol_t * Sat_SolverReadResolTemp( Sat_Solver_t * p, Sat_Clause_t * pC )
{
    return (Sat_Resol_t *)Sat_ClauseVecReadEntry( p->vResolsTemp, Sat_ClauseReadNum(pC) );
}

/**Function*************************************************************

  Synopsis    [Sets the resolution of the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverSetResolTemp( Sat_Solver_t * p, Sat_Clause_t * pC, Sat_Resol_t * pResol )
{
    Sat_ClauseVecWriteEntry( p->vResolsTemp, Sat_ClauseReadNum(pC), (Sat_Clause_t *)pResol );
}

/**Function*************************************************************

  Synopsis    [Sets the resolution of the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverSetVarTypeA( Sat_Solver_t * p, int Var )
{
    p->pVarTypeA[Var] = 1;
}

/**Function*************************************************************

  Synopsis    [Sets the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverSetVarMap( Sat_Solver_t * p, Sat_IntVec_t * vVarMap )
{
    p->vVarMap = vVarMap;
}

/**Function*************************************************************

  Synopsis    [Reads the clause with the given number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Clause_t * Sat_SolverReadClause( Sat_Solver_t * p, int Num )
{
    int nClausesP;
    assert( Num < p->nClauses );
    nClausesP = Sat_ClauseVecReadSize( p->vClauses );
    if ( Num < nClausesP )
        return Sat_ClauseVecReadEntry( p->vClauses, Num );
    return Sat_ClauseVecReadEntry( p->vLearned, Num - nClausesP );
}

/**Function*************************************************************

  Synopsis    [Allocates the solver.]

  Description [After the solver is allocated, the procedure
  Sat_SolverClean() should be called to set the number of variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Solver_t * Sat_SolverAlloc( int nVarsAlloc,
    double dClaInc, double dClaDecay, 
    double dVarInc, double dVarDecay, 
    bool fVerbose )
{
    Sat_Solver_t * p;
    int i;

    assert(sizeof(Sat_Lit_t) == sizeof(unsigned));
    assert(sizeof(float)     == sizeof(unsigned));

    p = ALLOC( Sat_Solver_t, 1 );
    memset( p, 0, sizeof(Sat_Solver_t) );
    p->fProof    = 0;

    p->nVarsAlloc = nVarsAlloc;
    p->nVars     = 0;

    p->nClauses  = 0;
    p->vClauses  = Sat_ClauseVecAlloc( 512 );
    p->vLearned  = Sat_ClauseVecAlloc( 512 );

    p->dClaInc   = dClaInc;
    p->dClaDecay = dClaDecay;
    p->dVarInc   = dVarInc;
    p->dVarDecay = dVarDecay;

    p->pdActivity = ALLOC( double, p->nVarsAlloc );
    for ( i = 0; i < p->nVarsAlloc; i++ )
        p->pdActivity[i] = 0;

    p->pAssigns  = ALLOC( int, p->nVarsAlloc ); 
    p->pModel    = ALLOC( int, p->nVarsAlloc ); 
    for ( i = 0; i < p->nVarsAlloc; i++ )
        p->pAssigns[i] = SAT_VAR_UNASSIGNED;
    p->pOrder    = Sat_OrderAlloc( p->pAssigns, p->pdActivity, p->nVarsAlloc );

    p->pvWatched = ALLOC( Sat_ClauseVec_t *, 2 * p->nVarsAlloc );
    for ( i = 0; i < 2 * p->nVarsAlloc; i++ )
        p->pvWatched[i] = Sat_ClauseVecAlloc( 16 );
    p->pQueue    = Sat_QueueAlloc( p->nVarsAlloc );

    p->vTrail    = Sat_IntVecAlloc( p->nVarsAlloc );
    p->vTrailLim = Sat_IntVecAlloc( p->nVarsAlloc );
    p->pReasons  = ALLOC( Sat_Clause_t *, p->nVarsAlloc );
    memset( p->pReasons, 0, sizeof(Sat_Clause_t *) * p->nVarsAlloc );
    p->pLevel = ALLOC( int, p->nVarsAlloc );
    for ( i = 0; i < p->nVarsAlloc; i++ )
        p->pLevel[i] = -1;
    p->dRandSeed = 91648253;
    p->fVerbose  = fVerbose;
    p->dProgress = 0.0;
//    p->pModel = Sat_IntVecAlloc( p->nVarsAlloc );
    p->pMem = Sat_MmStepStart( 10 );

    p->pResols  = ALLOC( Sat_Clause_t *, p->nVarsAlloc );
    memset( p->pResols, 0, sizeof(Sat_Clause_t *) * p->nVarsAlloc );
    p->vResols     = Sat_ClauseVecAlloc( 512 );
    p->vResolsTemp = Sat_ClauseVecAlloc( 512 );

    p->pSeen     = ALLOC( int, p->nVarsAlloc );
    memset( p->pSeen, 0, sizeof(int) * p->nVarsAlloc );
    p->nSeenId   = 1;
    p->vReason   = Sat_IntVecAlloc( p->nVarsAlloc );
    p->vTemp     = Sat_IntVecAlloc( p->nVarsAlloc );
    p->vTemp0    = Sat_ClauseVecAlloc( p->nVarsAlloc );
    p->vTemp1    = Sat_ClauseVecAlloc( p->nVarsAlloc );

    p->pVarTypeA = ALLOC( int, p->nVarsAlloc );
    memset( p->pVarTypeA, 0, sizeof(int) * p->nVarsAlloc );

    p->vSols     = Sat_IntVecAlloc( 1000 );
    Sat_SolverSetupTruthTables( p->uTruths );
    return p;
}

/**Function*************************************************************

  Synopsis    [Resizes the solver.]

  Description [Assumes that the solver contains some clauses, and that 
  it is currently between the calls. Resizes the solver to accomodate 
  more variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverResize( Sat_Solver_t * p, int nVarsAlloc )
{
    int nVarsAllocOld, i;

    nVarsAllocOld = p->nVarsAlloc;
    p->nVarsAlloc = nVarsAlloc;

    p->pdActivity = REALLOC( double, p->pdActivity, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
        p->pdActivity[i] = 0;

    p->pAssigns  = REALLOC( int, p->pAssigns, p->nVarsAlloc );
    p->pModel    = REALLOC( int, p->pModel, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
        p->pAssigns[i] = SAT_VAR_UNASSIGNED;

    Sat_OrderRealloc( p->pOrder, p->pAssigns, p->pdActivity, p->nVarsAlloc );

    p->pvWatched = REALLOC( Sat_ClauseVec_t *, p->pvWatched, 2 * p->nVarsAlloc );
    for ( i = 2 * nVarsAllocOld; i < 2 * p->nVarsAlloc; i++ )
        p->pvWatched[i] = Sat_ClauseVecAlloc( 16 );

    Sat_QueueFree( p->pQueue );
    p->pQueue    = Sat_QueueAlloc( p->nVarsAlloc );

    p->pReasons  = REALLOC( Sat_Clause_t *, p->pReasons, p->nVarsAlloc );
    p->pLevel    = REALLOC( int, p->pLevel, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
    {
        p->pReasons[i] = NULL;
        p->pLevel[i] = -1;
    }

    p->pResols  = REALLOC( Sat_Clause_t *, p->pResols, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
        p->pResols[i] = NULL;

    p->pSeen     = REALLOC( int, p->pSeen, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
        p->pSeen[i] = 0;

    p->pVarTypeA = REALLOC( int, p->pVarTypeA, p->nVarsAlloc );
    for ( i = nVarsAllocOld; i < p->nVarsAlloc; i++ )
        p->pVarTypeA[i] = 0;

    Sat_IntVecGrow( p->vTrail, p->nVarsAlloc );
    Sat_IntVecGrow( p->vTrailLim, p->nVarsAlloc );
}

/**Function*************************************************************

  Synopsis    [Prepares the solver.]

  Description [Cleans the solver assuming that the problem will involve 
  the given number of variables (nVars). This procedure is useful 
  for many small (incremental) SAT problems, to prevent the solver
  from being reallocated each time.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverClean( Sat_Solver_t * p, int nVars )
{
    int i;
    // free the clauses
    int nClauses;
    Sat_Clause_t ** pClauses;

    assert( p->nVarsAlloc >= nVars );
    p->nVars    = nVars;
    p->nClauses = 0;

    nClauses = Sat_ClauseVecReadSize( p->vClauses );
    pClauses = Sat_ClauseVecReadArray( p->vClauses );
    for ( i = 0; i < nClauses; i++ )
        Sat_ClauseFree( p, pClauses[i], 0 );
//    Sat_ClauseVecFree( p->vClauses );
    Sat_ClauseVecClear( p->vClauses );

    nClauses = Sat_ClauseVecReadSize( p->vLearned );
    pClauses = Sat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nClauses; i++ )
        Sat_ClauseFree( p, pClauses[i], 0 );
//    Sat_ClauseVecFree( p->vLearned );
    Sat_ClauseVecClear( p->vLearned );

//    FREE( p->pdActivity );
    for ( i = 0; i < p->nVars; i++ )
        p->pdActivity[i] = 0;

//    Sat_OrderFree( p->pOrder );
    Sat_OrderClean( p->pOrder, p->nVars, NULL );

    for ( i = 0; i < 2 * p->nVars; i++ )
//        Sat_ClauseVecFree( p->pvWatched[i] );
        Sat_ClauseVecClear( p->pvWatched[i] );
//    FREE( p->pvWatched );
//    Sat_QueueFree( p->pQueue );
    Sat_QueueClear( p->pQueue );

//    FREE( p->pAssigns );
    for ( i = 0; i < p->nVars; i++ )
        p->pAssigns[i] = SAT_VAR_UNASSIGNED;
//    Sat_IntVecFree( p->vTrail );
    Sat_IntVecClear( p->vTrail );
//    Sat_IntVecFree( p->vTrailLim );
    Sat_IntVecClear( p->vTrailLim );
//    FREE( p->pReasons );
    memset( p->pReasons, 0, sizeof(Sat_Clause_t *) * p->nVars );
//    FREE( p->pLevel );
    for ( i = 0; i < p->nVars; i++ )
        p->pLevel[i] = -1;
//    Sat_IntVecFree( p->pModel );
//    Sat_MmStepStop( p->pMem, 0 );
    p->dRandSeed = 91648253;
    p->dProgress = 0.0;

//    memset( p->pResols, 0, sizeof(Sat_Resol_t *) * p->nVars );
    for ( i = 0; i < p->nVars; i++ )
    {
        if ( p->pResols[i] == NULL )
            continue;
        Sat_ClauseFree( p, p->pResols[i], 0 );
        p->pResols[i] = NULL;
    }
    // free the resolution DAGs
    nClauses = Sat_ClauseVecReadSize( p->vResols );
    pClauses = Sat_ClauseVecReadArray( p->vResols );
    for ( i = 0; i < nClauses; i++ )
        if ( pClauses[i] )
        {
            Sat_SolverFreeResolTree_rec( p, (Sat_Resol_t *)pClauses[i] );
        }
    Sat_ClauseVecClear( p->vResols );
    Sat_ClauseVecClear( p->vResolsTemp );

//    FREE( p->pSeen );
    memset( p->pSeen, 0, sizeof(int) * p->nVars );
    p->nSeenId = 1;
//    Sat_IntVecFree( p->vReason );
    Sat_IntVecClear( p->vReason );
//    Sat_IntVecFree( p->vTemp );
    Sat_IntVecClear( p->vTemp );
    Sat_ClauseVecClear( p->vTemp0 );
    Sat_ClauseVecClear( p->vTemp1 );
    Sat_IntVecClear( p->vSols );

    memset( p->pVarTypeA, 0, sizeof(int) * p->nVars );

//    printf(" The number of clauses remaining = %d (%d).\n", p->nClausesAlloc, p->nClausesAllocL );
//    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Frees the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverFree( Sat_Solver_t * p )
{
    int i;

    // free the clauses
    int nClauses;
    Sat_Clause_t ** pClauses;
//printf( "clauses = %d. learned = %d.\n", Sat_ClauseVecReadSize( p->vClauses ), 
//                                         Sat_ClauseVecReadSize( p->vLearned ) );

    nClauses = Sat_ClauseVecReadSize( p->vClauses );
    pClauses = Sat_ClauseVecReadArray( p->vClauses );
    for ( i = 0; i < nClauses; i++ )
        Sat_ClauseFree( p, pClauses[i], 0 );
    Sat_ClauseVecFree( p->vClauses );

    nClauses = Sat_ClauseVecReadSize( p->vLearned );
    pClauses = Sat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nClauses; i++ )
        Sat_ClauseFree( p, pClauses[i], 0 );
    Sat_ClauseVecFree( p->vLearned );

    FREE( p->pdActivity );
    Sat_OrderFree( p->pOrder );

    for ( i = 0; i < 2 * p->nVarsAlloc; i++ )
        Sat_ClauseVecFree( p->pvWatched[i] );
    FREE( p->pvWatched );
    Sat_QueueFree( p->pQueue );

    FREE( p->pAssigns );
    FREE( p->pModel );
    Sat_IntVecFree( p->vTrail );
    Sat_IntVecFree( p->vTrailLim );
    FREE( p->pReasons );
    FREE( p->pLevel );

    if ( p->pMemProof )
        Sat_MmFixedStop( p->pMemProof, 0 );

    for ( i = 0; i < p->nVarsAlloc; i++ )
    {
        if ( p->pResols[i] == NULL )
            continue;
        Sat_ClauseFree( p, p->pResols[i], 0 );
        p->pResols[i] = NULL;
    }
    FREE( p->pResols );

    // free the resolution DAGs
    nClauses = Sat_ClauseVecReadSize( p->vResols );
    pClauses = Sat_ClauseVecReadArray( p->vResols );
    for ( i = 0; i < nClauses; i++ )
        if ( pClauses[i] )
        {
            Sat_SolverFreeResolTree_rec( p, (Sat_Resol_t *)pClauses[i] );
        }
    Sat_ClauseVecFree( p->vResols );
    Sat_ClauseVecFree( p->vResolsTemp );

    Sat_MmStepStop( p->pMem, 0 );

    FREE( p->pSeen );
    Sat_IntVecFree( p->vReason );
    Sat_IntVecFree( p->vTemp );
    Sat_ClauseVecFree( p->vTemp0 );
    Sat_ClauseVecFree( p->vTemp1 );
    Sat_IntVecFree( p->vSols );
    FREE( p->pVarTypeA );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prepares the solver to run on a subset of variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverPrepare( Sat_Solver_t * p, Sat_IntVec_t * vVars )
{

    int i;
    // undo the previous data
    for ( i = 0; i < p->nVarsAlloc; i++ )
    {
        p->pAssigns[i]   = SAT_VAR_UNASSIGNED;
        p->pReasons[i]   = NULL;
        p->pLevel[i]     = -1;
        p->pdActivity[i] = 0.0;
    }

    // set the new variable order
    Sat_OrderClean( p->pOrder, Sat_IntVecReadSize(vVars), Sat_IntVecReadArray(vVars) );
    Sat_QueueClear( p->pQueue );
    Sat_IntVecClear( p->vTrail );
    Sat_IntVecClear( p->vTrailLim );
    Sat_IntVecClear( p->vSols );
    p->dProgress = 0.0;
}

/**Function*************************************************************

  Synopsis    [Sets up the truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverSetupTruthTables( unsigned uTruths[][2] )
{
    int m, v;
    // set up the truth tables
    for ( m = 0; m < 32; m++ )
        for ( v = 0; v < 5; v++ )
            if ( m & (1 << v) )
                uTruths[v][0] |= (1 << m);
    // make adjustments for the case of 6 variables
    for ( v = 0; v < 5; v++ )
        uTruths[v][1] = uTruths[v][0];
    uTruths[5][0] = 0;
    uTruths[5][1] = ~((unsigned)0);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


