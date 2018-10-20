/**CFile****************************************************************

  FileName    [msatSolverCore.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The SAT solver core procedures.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: msatSolverCore.c,v 1.1 2005/07/08 01:01:37 alanmi Exp $]

***********************************************************************/

#include "msatInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Adds one variable to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Msat_SolverAddVar( Msat_Solver_t * p )
{
    if ( p->nVars == p->nVarsAlloc )
        Msat_SolverResize( p, 2 * p->nVarsAlloc );
    p->nVars++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds one clause to the solver's clause database.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Msat_SolverAddClause( Msat_Solver_t * p, Msat_IntVec_t * vLits )
{
    Msat_Clause_t * pC; 
    bool Value;
    Value = Msat_ClauseCreate( p, vLits, 0, &pC );
    if ( pC != NULL )
        Msat_ClauseVecPush( p->vClauses, pC );
//    else if ( p->fProof )
//        Msat_ClauseCreateFake( p, vLits );
    return Value;
}

/**Function*************************************************************

  Synopsis    [Returns search-space coverage. Not extremely reliable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Msat_SolverProgressEstimate( Msat_Solver_t * p )
{
    double dProgress = 0.0;
    double dF = 1.0 / p->nVars;
    int i;
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pAssigns[i] != MSAT_VAR_UNASSIGNED )
            dProgress += pow( dF, p->pLevel[i] );
    return dProgress / p->nVars;
}

/**Function*************************************************************

  Synopsis    [Prints statistics about the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Msat_SolverPrintStats( Msat_Solver_t * p )
{
    printf("C solver (%d vars; %d clauses; %d learned):\n", 
        p->nVars, Msat_ClauseVecReadSize(p->vClauses), Msat_ClauseVecReadSize(p->vLearned) );
    printf("starts        : %lld\n", p->Stats.nStarts);
    printf("conflicts     : %lld\n", p->Stats.nConflicts);
    printf("decisions     : %lld\n", p->Stats.nDecisions);
    printf("propagations  : %lld\n", p->Stats.nPropagations);
    printf("inspects      : %lld\n", p->Stats.nInspects);
}

/**Function*************************************************************

  Synopsis    [Top-level solve.]

  Description [If using assumptions (non-empty 'assumps' vector), you must 
  call 'simplifyDB()' first to see that no top-level conflict is present 
  (which would put the solver in an undefined state. If the last argument
  is given (vProj), the solver enumerates through the satisfying solutions,
  which are projected on the variables listed in this array. Note that the
  variables in the array may be complemented, in which case the derived
  assignment for the variable is complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Msat_SolverSolve( Msat_Solver_t * p, Msat_IntVec_t * vAssumps, int nBackTrackLimit )
{
    Msat_SearchParams_t Params = { 0.95, 0.999 };
    double nConflictsLimit, nLearnedLimit;
    Msat_Type_t Status;
    int64 nConflictsOld = p->Stats.nConflicts;
    int64 nDecisionsOld = p->Stats.nDecisions;

    if ( vAssumps )
    {
        int * pAssumps, nAssumps, i;

        assert( Msat_IntVecReadSize(p->vTrailLim) == 0 );

        nAssumps = Msat_IntVecReadSize( vAssumps );
        pAssumps = Msat_IntVecReadArray( vAssumps );
        for ( i = 0; i < nAssumps; i++ )
        {
            if ( !Msat_SolverAssume(p, pAssumps[i]) || Msat_SolverPropagate(p) )
            {
                Msat_QueueClear( p->pQueue );
                Msat_SolverCancelUntil( p, 0 );
                return MSAT_FALSE;
            }
        }
    }
    p->nLevelRoot   = Msat_SolverReadDecisionLevel(p);
    p->nClausesInit = Msat_ClauseVecReadSize( p->vClauses );    
    nConflictsLimit = 100;
    nLearnedLimit   = Msat_ClauseVecReadSize(p->vClauses) / 3;
    Status = MSAT_UNKNOWN;
    p->nBackTracks = (int)p->Stats.nConflicts;
    while ( Status == MSAT_UNKNOWN )
    {
        if ( p->fVerbose )
            printf("Solving -- conflicts=%d   learnts=%d   progress=%.4f %%\n", 
                (int)nConflictsLimit, (int)nLearnedLimit, p->dProgress*100);
        Status = Msat_SolverSearch( p, (int)nConflictsLimit, (int)nLearnedLimit, nBackTrackLimit, &Params );
        nConflictsLimit *= 1.5;
        nLearnedLimit   *= 1.1;
        // if the limit on the number of backtracks is given, quit the restart loop
        if ( nBackTrackLimit > 0 )
            break;
    }
    Msat_SolverCancelUntil( p, 0 );
    p->nBackTracks = (int)p->Stats.nConflicts - p->nBackTracks;
    return Status;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


