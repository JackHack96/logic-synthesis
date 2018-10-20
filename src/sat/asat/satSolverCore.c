/**CFile****************************************************************

  FileName    [satSolverCore.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The SAT solver core procedures.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satSolverCore.c,v 1.1 2005/07/08 01:01:28 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

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
bool Sat_SolverAddVar( Sat_Solver_t * p )
{
    if ( p->nVars == p->nVarsAlloc )
        Sat_SolverResize( p, 2 * p->nVarsAlloc );
    p->nVars++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds one clause to the solver's clause database.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_SolverAddClause( Sat_Solver_t * p, Sat_IntVec_t * vLits )
{
    Sat_Clause_t * pC; 
    bool Value;
    Value = Sat_ClauseCreate( p, vLits, 0, &pC );
    if ( pC != NULL )
        Sat_ClauseVecPush( p->vClauses, pC );
//    else if ( p->fProof )
//        Sat_ClauseCreateFake( p, vLits );
    return Value;
}

/**Function*************************************************************

  Synopsis    [Returns search-space coverage. Not extremely reliable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Sat_SolverProgressEstimate( Sat_Solver_t * p )
{
    double dProgress = 0.0;
    double dF = 1.0 / p->nVars;
    int i;
    for ( i = 0; i < p->nVars; i++ )
        if ( p->pAssigns[i] != SAT_VAR_UNASSIGNED )
            dProgress += pow( dF, p->pLevel[i] );
    return dProgress / p->nVars;
}

/**Function*************************************************************

  Synopsis    [Prints statistics about the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverPrintStats( Sat_Solver_t * p )
{
    printf("C solver (%d vars; %d clauses; %d learned):\n", 
        p->nVars, Sat_ClauseVecReadSize(p->vClauses), Sat_ClauseVecReadSize(p->vLearned) );
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
bool Sat_SolverSolve( Sat_Solver_t * p, Sat_IntVec_t * vAssumps, Sat_IntVec_t * vProj, int nBackTrackLimit )
{
    Sat_SearchParams_t Params = { 0.95, 0.999 };
    double nConflictsLimit, nLearnedLimit;
    Sat_Type_t Status;
    int64 nConflictsOld = p->Stats.nConflicts;
    int64 nDecisionsOld = p->Stats.nDecisions;
//    Sat_ClauseVec_t * vProof;
//    int clk;

//Sat_SolverWriteDimacs( p, "problem.cnf" );
//Sat_SolverPrintAssignment( p );
//Sat_SolverWriteGml( p, "problem.gml" );

    p->vProj = vProj;
    if ( vAssumps )
    {
        int * pAssumps, nAssumps, i;

        assert( Sat_IntVecReadSize(p->vTrailLim) == 0 );

        nAssumps = Sat_IntVecReadSize( vAssumps );
        pAssumps = Sat_IntVecReadArray( vAssumps );
        for ( i = 0; i < nAssumps; i++ )
        {
//            if ( p->fProof )
//                Sat_ClauseCreateFakeLit( p, pAssumps[i] );
            if ( !Sat_SolverAssume(p, pAssumps[i]) || Sat_SolverPropagate(p) )
            {
                Sat_QueueClear( p->pQueue );
                Sat_SolverCancelUntil( p, 0 );
                return SAT_FALSE;
            }
        }
    }
    p->nLevelRoot   = Sat_SolverReadDecisionLevel(p);
    p->nClausesInit = Sat_ClauseVecReadSize( p->vClauses );    
    nConflictsLimit = 100;
    nLearnedLimit   = Sat_ClauseVecReadSize(p->vClauses) / 3;
    Status = SAT_UNKNOWN;
    p->nBackTracks = (int)p->Stats.nConflicts;
    while ( Status == SAT_UNKNOWN )
    {
        if ( p->fVerbose )
            printf("Solving -- conflicts=%d   learnts=%d   progress=%.4f %%\n", 
                (int)nConflictsLimit, (int)nLearnedLimit, p->dProgress*100);
        Status = Sat_SolverSearch( p, (int)nConflictsLimit, (int)nLearnedLimit, nBackTrackLimit, &Params );
        nConflictsLimit *= 1.5;
        nLearnedLimit   *= 1.1;
        // if the limit on the number of backtracks is given, quit the restart loop
        if ( nBackTrackLimit > 0 )
            break;
    }
    Sat_SolverCancelUntil( p, 0 );
    p->nBackTracks = (int)p->Stats.nConflicts - p->nBackTracks;

//    Sat_QueueClear( p->pQueue );

/*
    if ( vAssumps && p->fProof )
    {
        int * pAssumps, nAssumps, i;
        nAssumps = Sat_IntVecReadSize( vAssumps );
        pAssumps = Sat_IntVecReadArray( vAssumps );
        for ( i = 0; i < nAssumps; i++ )
        {
            Sat_ClauseFree( p, p->pResols[SAT_LIT2VAR(pAssumps[i])], 0 );
            p->pResols[SAT_LIT2VAR(pAssumps[i])] = NULL;
        }
    }
*/

//printf( "%d(%d) ", (int)(p->Stats.nConflicts - nConflictsOld), 
//       (int)(p->Stats.nDecisions - nDecisionsOld) );

//    Sat_SolverPrintStats( p );

    if ( p->fProof && Status == SAT_FALSE )
    {
        int clk;
        Sat_ClauseVec_t * vProof;
        int i = 0;
/*
        {
            Sat_IntVec_t * vVarMap;
            vVarMap = Sat_IntVecAlloc( 10 );
            Sat_IntVecPush( vVarMap, 1 );
            Sat_IntVecPush( vVarMap, 2 );
            Sat_IntVecPush( vVarMap, 0 );
            p->vVarMap = vVarMap;
        }
*/

/*
clk = clock();
    vProof = Sat_SolverCollectProof( p );
PRT( "Proof generation", clock() - clk );
    Sat_SolverPrintProof( p, vProof, "proof.txt" );
//clk = clock();
//    Sat_SolverVerifyProof( "proof.txt", "proof_trace.txt" );
//PRT( "Proof verification", clock() - clk );
    Sat_ClauseVecFree( vProof );
*/

    }
    else
    {
//        printf( "The instance is satisfiable. The proof is not recorded.\n" );
    }

    return Status;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


