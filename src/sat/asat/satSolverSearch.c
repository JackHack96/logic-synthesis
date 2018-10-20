/**CFile****************************************************************

  FileName    [satSolverSearch.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [The search part of the solver.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satSolverSearch.c,v 1.1 2005/07/08 01:01:29 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void            Sat_SolverUndoOne( Sat_Solver_t * p );
static void            Sat_SolverCancel( Sat_Solver_t * p );
static Sat_Clause_t *  Sat_SolverRecord( Sat_Solver_t * p, Sat_IntVec_t * vLits );
static void            Sat_SolverAnalyze( Sat_Solver_t * p, Sat_Clause_t * pC, Sat_IntVec_t * vLits_out, int * pLevel_out );
static bool            Sat_SolverAnalyzeSuccess( Sat_Solver_t * p, Sat_IntVec_t * vLits_out, int * pLevel_out );
static void            Sat_SolverReduceDB( Sat_Solver_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Makes the next assumption (Lit).]

  Description [Returns FALSE if immediate conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_SolverAssume( Sat_Solver_t * p, Sat_Lit_t Lit )
{
    assert( Sat_QueueReadSize(p->pQueue) == 0 );
    if ( p->fVerbose )
        printf(L_IND"assume("L_LIT")\n", L_ind, L_lit(Lit));
    Sat_IntVecPush( p->vTrailLim, Sat_IntVecReadSize(p->vTrail) );
//    assert( Sat_IntVecReadSize(p->vTrailLim) <= Sat_IntVecReadSize(p->vTrail) + 1 );
//    assert( Sat_IntVecReadSize( p->vTrailLim ) < p->nVars );
    return Sat_SolverEnqueue( p, Lit, NULL );
}

/**Function*************************************************************

  Synopsis    [Reverts one variable binding on the trail.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverUndoOne( Sat_Solver_t * p )
{
    Sat_Lit_t Lit;
    Sat_Var_t Var;
    Lit = Sat_IntVecPop( p->vTrail ); 
    Var = SAT_LIT2VAR(Lit);
    p->pAssigns[Var] = SAT_VAR_UNASSIGNED;
    p->pReasons[Var] = NULL;
    p->pLevel[Var]   = -1;
    Sat_OrderUndo( p->pOrder, Var );
    if ( p->fVerbose )
        printf(L_IND"unbind("L_LIT")\n", L_ind, L_lit(Lit)); 
}

/**Function*************************************************************

  Synopsis    [Reverts to the state before last Sat_SolverAssume().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverCancel( Sat_Solver_t * p )
{
    int c;
    assert( Sat_QueueReadSize(p->pQueue) == 0 );
    if ( p->fVerbose )
    {
        if ( Sat_IntVecReadSize(p->vTrail) != Sat_IntVecReadEntryLast(p->vTrailLim) )
        {
            Sat_Lit_t Lit;
            Lit = Sat_IntVecReadEntry( p->vTrail, Sat_IntVecReadEntryLast(p->vTrailLim) ); 
            printf(L_IND"cancel("L_LIT")\n", L_ind, L_lit(Lit));
        }
    }
    for ( c = Sat_IntVecReadSize(p->vTrail) - Sat_IntVecPop( p->vTrailLim ); c != 0; c-- )
        Sat_SolverUndoOne( p );
}

/**Function*************************************************************

  Synopsis    [Reverts to the state at given level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverCancelUntil( Sat_Solver_t * p, int Level )
{
    while ( Sat_IntVecReadSize(p->vTrailLim) > Level )
        Sat_SolverCancel(p);
}


/**Function*************************************************************

  Synopsis    [Record a clause and drive backtracking.]

  Description [vLits[0] must contain the asserting literal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Clause_t * Sat_SolverRecord( Sat_Solver_t * p, Sat_IntVec_t * vLits )
{
    Sat_Clause_t * pC;
    int Value;
    assert( Sat_IntVecReadSize(vLits) != 0 );
    Value = Sat_ClauseCreate( p, vLits, 1, &pC );
    assert( Value );
    Value = Sat_SolverEnqueue( p, Sat_IntVecReadEntry(vLits,0), pC );
    assert( Value );
    if ( pC )
        Sat_ClauseVecPush( p->vLearned, pC );
    return pC;
}

/**Function*************************************************************

  Synopsis    [Enqueues one variable assignment.]

  Description [Puts a new fact on the propagation queue and immediately 
  updates the variable value. Should a conflict arise, FALSE is returned.
  Otherwise returns TRUE.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_SolverEnqueue( Sat_Solver_t * p, Sat_Lit_t Lit, Sat_Clause_t * pC )
{
    Sat_Var_t Var = SAT_LIT2VAR(Lit);
//    assert( Sat_QueueReadSize(p->pQueue) == Sat_IntVecReadSize(p->vTrail) );
    // if the literal is assigned
    // return 1 if the assignment is consistent
    // return 0 if the assignment is inconsistent (conflict)
    if ( p->pAssigns[Var] != SAT_VAR_UNASSIGNED )
        return p->pAssigns[Var] == Lit;
    // new fact - store it
    if ( p->fVerbose )
    {
//        printf(L_IND"bind("L_LIT")\n", L_ind, L_lit(Lit));
        printf(L_IND"bind("L_LIT")  ", L_ind, L_lit(Lit));
        Sat_ClausePrintSymbols( pC );
    }
    p->pAssigns[Var] = Lit;
    p->pLevel[Var]   = Sat_IntVecReadSize(p->vTrailLim);
//    p->pReasons[Var] = p->pLevel[Var]? pC: NULL;
    p->pReasons[Var] = pC;
    Sat_IntVecPush( p->vTrail, Lit );
    Sat_QueueInsert( p->pQueue, Lit );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Propagates the assignments in the queue.]

  Description [Propagates all enqueued facts. If a conflict arises, 
  the conflicting clause is returned, otherwise NULL.]
               
  SideEffects [The propagation queue is empty, even if there was a conflict.]

  SeeAlso     []

***********************************************************************/
Sat_Clause_t * Sat_SolverPropagate( Sat_Solver_t * p )
{
    Sat_ClauseVec_t ** pvWatched = p->pvWatched;
    Sat_Clause_t ** pClauses;
    Sat_Clause_t * pConflict;
    Sat_Lit_t Lit, Lit_out;
    int i, j, nClauses;

    // propagate all the literals in the queue
    while ( (Lit = Sat_QueueExtract( p->pQueue )) >= 0 )
    {
        p->Stats.nPropagations++;
        // get the clauses watched by this literal
        nClauses = Sat_ClauseVecReadSize( pvWatched[Lit] );
        pClauses = Sat_ClauseVecReadArray( pvWatched[Lit] );
        // go through the watched clauses and decide what to do with them
        for ( i = j = 0; i < nClauses; i++ )
        {
            p->Stats.nInspects++;
            // clear the returned literal
            Lit_out = -1;
            // propagate the clause
            if ( !Sat_ClausePropagate( pClauses[i], Lit, p->pAssigns, &Lit_out ) )
            {   // the clause is unit
                // "Lit_out" contains the new assignment to be enqueued
                if ( Sat_SolverEnqueue( p, Lit_out, pClauses[i] ) ) 
                { // consistent assignment 
                    // no changes to the implication queue; the watch is the same too
                    pClauses[j++] = pClauses[i];
                    continue;
                }
                // remember the reason of conflict (will be returned)
                pConflict = pClauses[i];
                // leave the remaning clauses in the same watched list
                for ( ; i < nClauses; i++ )
                    pClauses[j++] = pClauses[i];
                Sat_ClauseVecShrink( pvWatched[Lit], j );
                // clear the propagation queue
                Sat_QueueClear( p->pQueue );
                return pConflict;
            }
            // the clause is not unit
            // in this case "Lit_out" contains the new watch if it has changed
            if ( Lit_out >= 0 )
                Sat_ClauseVecPush( pvWatched[Lit_out], pClauses[i] );
            else // the watch did not change
                pClauses[j++] = pClauses[i];
        }
        Sat_ClauseVecShrink( pvWatched[Lit], j );
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Simplifies the data base.]

  Description [Simplify all constraints according to the current top-level 
  assigment (redundant constraints may be removed altogether).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_SolverSimplifyDB( Sat_Solver_t * p )
{
    Sat_ClauseVec_t * vClauses;
    Sat_Clause_t ** pClauses;
    int nClauses, Type, i, j;
    int * pAssigns;
    int Counter;

    assert( Sat_SolverReadDecisionLevel(p) == 0 );
    if ( Sat_SolverPropagate(p) != NULL )
        return 0;
//Sat_SolverPrintClauses( p );
//Sat_SolverPrintAssignment( p );
//printf( "Simplification\n" );

    // simplify and reassign clause numbers
    Counter = 0;
    pAssigns = Sat_SolverReadAssignsArray( p );
    for ( Type = 0; Type < 2; Type++ )
    {
        vClauses = Type? p->vLearned : p->vClauses;
        nClauses = Sat_ClauseVecReadSize( vClauses );
        pClauses = Sat_ClauseVecReadArray( vClauses );
        for ( i = j = 0; i < nClauses; i++ )
            if ( Sat_ClauseSimplify( pClauses[i], pAssigns ) )
                Sat_ClauseFree( p, pClauses[i], 1 );
            else
            {
                pClauses[j++] = pClauses[i];
                Sat_ClauseSetNum( pClauses[i], Counter++ );
            }
        Sat_ClauseVecShrink( vClauses, j );
    }
    p->nClauses = Counter;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Cleans the clause databased from the useless learnt clauses.]

  Description [Removes half of the learnt clauses, minus the clauses locked 
  by the current assignment. Locked clauses are clauses that are reason 
  to a some assignment.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverReduceDB( Sat_Solver_t * p )
{
    Sat_Clause_t ** pLearned;
    int nLearned, i, j;
    double dExtraLim = p->dClaInc / Sat_ClauseVecReadSize(p->vLearned); 
    // Remove any clause below this activity

    // sort the learned clauses in the increasing order of activity
    Sat_SolverSortDB( p );

    // discard the first half the clauses (the less active ones)
    nLearned = Sat_ClauseVecReadSize( p->vLearned );
    pLearned = Sat_ClauseVecReadArray( p->vLearned );
    for ( i = j = 0; i < nLearned / 2; i++ )
        if ( !Sat_ClauseIsLocked( p, pLearned[i]) )
            Sat_ClauseFree( p, pLearned[i], 1 );
        else
            pLearned[j++] = pLearned[i];
    // filter the more active clauses and leave those above the limit
    for (  ; i < nLearned; i++ )
        if ( !Sat_ClauseIsLocked( p, pLearned[i] ) && 
            Sat_ClauseReadActivity(pLearned[i]) < dExtraLim )
            Sat_ClauseFree( p, pLearned[i], 1 );
        else
            pLearned[j++] = pLearned[i];
    Sat_ClauseVecShrink( p->vLearned, j );
}

/**Function*************************************************************

  Synopsis    [Removes the learned clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverRemoveLearned( Sat_Solver_t * p )
{
    Sat_Clause_t ** pLearned;
    int nLearned, i, iNum;

    // discard the learned clauses
    nLearned = Sat_ClauseVecReadSize( p->vLearned );
    pLearned = Sat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nLearned; i++ )
    {
        assert( !Sat_ClauseIsLocked( p, pLearned[i]) );

        iNum = Sat_ClauseReadNum(pLearned[i]);
        Sat_ClauseVecWriteEntry( p->vResols, iNum, NULL );
        Sat_ClauseVecWriteEntry( p->vResolsTemp, iNum, NULL );
        
        Sat_ClauseFree( p, pLearned[i], 1 );
    }
    Sat_ClauseVecShrink( p->vLearned, 0 );
    p->nClauses = Sat_ClauseVecReadSize(p->vClauses);

    // the proof recording was set, recycle the proof node memory
//    if ( !p->fProof )
//        return;

    for ( i = 0; i < p->nVarsAlloc; i++ )
    {
        if ( p->pResols[i] )
        {
            iNum = Sat_ClauseReadNum(p->pResols[i]);
            Sat_ClauseVecWriteEntry( p->vResols, iNum, NULL );
            Sat_ClauseVecWriteEntry( p->vResolsTemp, iNum, NULL );
        
            Sat_ClauseFree( p, p->pResols[i], 0 );
            p->pResols[i] = NULL;
        }
        p->pReasons[i] = NULL;
    }
    p->pRoot = NULL;

    // realloc the memory
    if ( p->pMemProof )
    Sat_MmFixedRestart( p->pMemProof );
}

/**Function*************************************************************

  Synopsis    [Removes the recently added clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_SolverRemoveMarked( Sat_Solver_t * p )
{
    Sat_Clause_t ** pLearned, ** pClauses;
    int nLearned, nClauses, i;

    // discard the learned clauses
    nClauses = Sat_ClauseVecReadSize( p->vClauses );
    pClauses = Sat_ClauseVecReadArray( p->vClauses );
    for ( i = p->nClausesStart; i < nClauses; i++ )
    {
//        assert( !Sat_ClauseIsLocked( p, pClauses[i]) );
        Sat_ClauseFree( p, pClauses[i], 1 );
    }
    Sat_ClauseVecShrink( p->vClauses, p->nClausesStart );

    // discard the learned clauses
    nLearned = Sat_ClauseVecReadSize( p->vLearned );
    pLearned = Sat_ClauseVecReadArray( p->vLearned );
    for ( i = 0; i < nLearned; i++ )
    {
//        assert( !Sat_ClauseIsLocked( p, pLearned[i]) );
        Sat_ClauseFree( p, pLearned[i], 1 );
    }
    Sat_ClauseVecShrink( p->vLearned, 0 );
    p->nClauses = Sat_ClauseVecReadSize(p->vClauses);
/*
    // undo the previous data
    for ( i = 0; i < p->nVarsAlloc; i++ )
    {
        p->pAssigns[i]   = SAT_VAR_UNASSIGNED;
        p->pReasons[i]   = NULL;
        p->pLevel[i]     = -1;
        p->pdActivity[i] = 0.0;
    }
    Sat_OrderClean( p->pOrder, p->nVars, NULL );
    Sat_QueueClear( p->pQueue );
*/
}




/**Function*************************************************************

  Synopsis    [Analyze success and produce a breaking clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sat_SolverAnalyzeSuccess( Sat_Solver_t * p, Sat_IntVec_t * vLits_out, int * pLevel_out )
{
    int * pVecArray, * pProjArray;
    int nVecSize, nProjSize, iLitTemp, i;
    int LevelMax1, iLitMax1;
    int LevelMax2, iLitMax2;
    unsigned Mint;

    // make sure the projection is specified
    assert( p->vProj );

    // empty the vector array
    Sat_IntVecClear( vLits_out );

    // get the variables
    nProjSize  = Sat_IntVecReadSize( p->vProj );
    pProjArray = Sat_IntVecReadArray( p->vProj );
    // make sure the projection is not empty
    assert( nProjSize > 0 );
    // create the solution
    Mint = 0;
    for ( i = 0; i < nProjSize; i++ )
    {
        assert( p->pAssigns[pProjArray[i]] != SAT_VAR_UNASSIGNED );
        if ( !SAT_LITSIGN( p->pAssigns[pProjArray[i]] ) )
            Mint |= (1<<i);
        if ( p->pLevel[ pProjArray[i] ] > 0 )
        {
            assert( p->pAssigns[pProjArray[i]] < 2 * p->nVars );
            Sat_IntVecPush( vLits_out, SAT_LITNOT( p->pAssigns[pProjArray[i]] ) );
        }
    }
    Sat_IntVecPush( p->vSols, Mint );

    nVecSize  = Sat_IntVecReadSize( vLits_out );
    pVecArray = Sat_IntVecReadArray( vLits_out );

    if ( p->fVerbose )
    {
        printf( L_IND"Success {", L_ind );
        for ( i = 0; i < nVecSize; i++ ) 
            printf(" "L_LIT, L_lit(pVecArray[i]));
        printf(" } at level %d\n", *pLevel_out); 
    }

    // check the size of clause to be added
    // if the size is 0, it is time to stop
//    assert( nVecSize != 0 ); // temporary!
    // if the size is 1, a new constant will be added
    if ( nVecSize < 2 )
    {
        *pLevel_out = 0;
        return nVecSize;
    }

    // find the latest decision level
    LevelMax1 = -1;
    for ( i = 0; i < nVecSize; i++ )
        if ( LevelMax1 < p->pLevel[ SAT_LIT2VAR(pVecArray[i]) ] )
            LevelMax1 = p->pLevel[ SAT_LIT2VAR(pVecArray[i]) ], iLitMax1 = i;
    assert( LevelMax1 >= 0 );

    // put this literal in front
    iLitTemp            = pVecArray[0];
    pVecArray[0]        = pVecArray[iLitMax1];
    pVecArray[iLitMax1] = iLitTemp;

    // find the second latest decision level
    LevelMax2 = -1;
    for ( i = 1; i < nVecSize; i++ )
        if ( LevelMax2 < p->pLevel[ SAT_LIT2VAR(pVecArray[i]) ] )
            LevelMax2 = p->pLevel[ SAT_LIT2VAR(pVecArray[i]) ], iLitMax2 = i;
    assert( LevelMax2 >= 0 );

    // put this literal on the second place
    iLitTemp            = pVecArray[1];
    pVecArray[1]        = pVecArray[iLitMax2];
    pVecArray[iLitMax2] = iLitTemp;

    // set the overall ealier decision level
    if ( LevelMax1 < LevelMax2 )
        *pLevel_out = LevelMax1 - 1;
    else
        *pLevel_out = LevelMax2 - 1;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Analyze conflict and produce a reason clause.]

  Description [Current decision level must be greater than root level.]
               
  SideEffects [vLits_out[0] is the asserting literal at level pLevel_out.]

  SeeAlso     []

***********************************************************************/
void Sat_SolverAnalyze( Sat_Solver_t * p, Sat_Clause_t * pC, Sat_IntVec_t * vLits_out, int * pLevel_out )
{
    Sat_Lit_t LitQ, Lit = SAT_LIT_UNASSIGNED;
    Sat_Var_t VarQ, Var;
    int * pReasonArray, nReasonSize;
    int j, pathC = 0, nLevelCur = Sat_IntVecReadSize(p->vTrailLim);
    int iStep = Sat_IntVecReadSize(p->vTrail) - 1;

    // increment the seen counter
    p->nSeenId++;
    // empty the vector array
    Sat_IntVecClear( vLits_out );
    Sat_IntVecPush( vLits_out, -1 ); // (leave room for the asserting literal)
    *pLevel_out = 0;
    do {
        assert( pC != NULL );  // (otherwise should be UIP)
        // get the reason of conflict
        Sat_ClauseCalcReason( p, pC, Lit, p->vReason );
        nReasonSize  = Sat_IntVecReadSize( p->vReason );
        pReasonArray = Sat_IntVecReadArray( p->vReason );
        for ( j = 0; j < nReasonSize; j++ ) {
            LitQ = pReasonArray[j];
            VarQ = SAT_LIT2VAR(LitQ);
            if ( p->pSeen[VarQ] != p->nSeenId ) {
                p->pSeen[VarQ] = p->nSeenId;
//                Sat_SolverVarBumpActivity( p, LitQ );
                // skip all the literals on this decision level
                if ( p->pLevel[VarQ] == nLevelCur )
                    pathC++;
                else if ( p->pLevel[VarQ] > 0 ) { 
                    // add the literals on other decision levels but
                    // exclude variables from decision level 0
                    Sat_IntVecPush( vLits_out, SAT_LITNOT(LitQ) );
                    if ( *pLevel_out < p->pLevel[VarQ] )
                        *pLevel_out = p->pLevel[VarQ];
                }
            }
        }
        // Select next clause to look at:
        do {
//            Lit = Sat_IntVecReadEntryLast(p->vTrail);
            Lit = Sat_IntVecReadEntry( p->vTrail, iStep-- );
            Var = SAT_LIT2VAR(Lit);
            pC = p->pReasons[Var];
//            Sat_SolverUndoOne( p );
        } while ( p->pSeen[Var] != p->nSeenId );
        pathC--;
    } while ( pathC > 0 );
    // we do not unbind the variables above
    // this will be done after conflict analysis

    Sat_IntVecWriteEntry( vLits_out, 0, SAT_LITNOT(Lit) );
    if ( p->fVerbose )
    {
        printf( L_IND"Learnt {", L_ind );
        nReasonSize  = Sat_IntVecReadSize( vLits_out );
        pReasonArray = Sat_IntVecReadArray( vLits_out );
        for ( j = 0; j < nReasonSize; j++ ) 
            printf(" "L_LIT, L_lit(pReasonArray[j]));
        printf(" } at level %d\n", *pLevel_out); 
    }
}

/**Function*************************************************************

  Synopsis    [The search procedure called between the restarts.]

  Description [Search for a satisfying solution as long as the number of 
  conflicts does not exceed the limit (nConfLimit) while keeping the number 
  of learnt clauses below the provided limit (nLearnedLimit). NOTE! Use 
  negative value for nConfLimit or nLearnedLimit to indicate infinity.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_Type_t Sat_SolverSearch( Sat_Solver_t * p, int nConfLimit, int nLearnedLimit, int nBackTrackLimit, Sat_SearchParams_t * pPars )
{
    Sat_Resol_t * pResol;
    Sat_Clause_t * pConf;
    Sat_Clause_t * pLearnt;
    Sat_Var_t Var;
    int nLevelBack, nConfs, nAssigns, Value;

    assert( Sat_SolverReadDecisionLevel(p) == p->nLevelRoot );
    p->Stats.nStarts++;
    p->dVarDecay = 1 / pPars->dVarDecay;
    p->dClaDecay = 1 / pPars->dClaDecay;

    nConfs = 0;
    while ( 1 )
    {
        pConf = Sat_SolverPropagate( p );
        if ( pConf != NULL ){
            // CONFLICT
            if ( p->fVerbose )
            {
//                printf(L_IND"**CONFLICT**\n", L_ind);
                printf(L_IND"**CONFLICT**  ", L_ind);
                Sat_ClausePrintSymbols( pConf );
            }
            // count conflicts
            p->Stats.nConflicts++;
            nConfs++;

            // if top level, return UNSAT
            if ( Sat_SolverReadDecisionLevel(p) == p->nLevelRoot )
            {
                if ( p->fProof ) 
                    p->pRoot = Sat_SolverDeriveClauseProof( p, pConf, -1 );
                return SAT_FALSE;
            }

            // perform conflict analysis
            Sat_SolverAnalyze( p, pConf, p->vTemp, &nLevelBack );
            if ( p->fProof ) 
            {
                pResol = Sat_SolverDeriveClauseProof( p, pConf, Sat_IntVecReadEntry(p->vTemp,0) );
                Sat_SolverCancelUntil( p, (p->nLevelRoot > nLevelBack)? p->nLevelRoot : nLevelBack );
                pLearnt = Sat_SolverRecord( p, p->vTemp );
                if ( pLearnt == NULL )
                    pLearnt = Sat_ClauseCreateFake( p, p->vTemp );
                Sat_SolverSetResol( p, pLearnt, pResol );
                Sat_SolverSetResolClause( pResol, pLearnt );
            }
            else
            {
                Sat_SolverCancelUntil( p, (p->nLevelRoot > nLevelBack)? p->nLevelRoot : nLevelBack );
                Sat_SolverRecord( p, p->vTemp );
            }

            // it is important that recording is done after cancelling
            // because canceling cleans the queue while recording adds to it
            Sat_SolverVarDecayActivity( p );
            Sat_SolverClaDecayActivity( p );

        }
        else{
            // NO CONFLICT
            if ( Sat_IntVecReadSize(p->vTrailLim) == 0 ) {
                // Simplify the set of problem clauses:
//                Value = Sat_SolverSimplifyDB(p);
//                assert( Value );
            }
            nAssigns = Sat_IntVecReadSize( p->vTrail );
            if ( nLearnedLimit >= 0 && Sat_ClauseVecReadSize(p->vLearned) >= nLearnedLimit + nAssigns ) {
                // Reduce the set of learnt clauses:
                if ( !p->fProof )
                    Sat_SolverReduceDB(p);
            }

            {
                int clk = clock();
                extern int timeSelect;
            Var = Sat_OrderSelect(p->pOrder);
                timeSelect += clock() - clk;
            }
//            if ( nAssigns == p->nVars ) {
            if ( Var == SAT_ORDER_UNKNOWN ) {
                // Model found and stored in p->pAssigns
                if ( p->vProj == NULL )
                {
                    memcpy( p->pModel, p->pAssigns, sizeof(int) * p->nVars );
                    Sat_QueueClear( p->pQueue );
                    Sat_SolverCancelUntil( p, p->nLevelRoot );
                    return SAT_TRUE;
                }
                ///////////////////////////////////////////
                if ( p->fVerbose )
                    printf(L_IND"**SUCCESS**\n", L_ind);
                // count conflicts
                p->Stats.nSuccesses++;
//                nConfs++;
                // perform conflict analysis
                if ( !Sat_SolverAnalyzeSuccess( p, p->vTemp, &nLevelBack ) )
                {
                    Sat_SolverCancelUntil( p, p->nLevelRoot );
                    return SAT_FALSE;
                }

//                Sat_SolverRecord( p, p->vTemp );
                {
                    Sat_Clause_t * pC;
                    int Value;
                    if ( Sat_IntVecReadSize(p->vTemp) == 1 )
                    {
                        Sat_SolverCancelUntil( p, (p->nLevelRoot > nLevelBack)? p->nLevelRoot : nLevelBack );
                        Sat_ClauseCreate( p, p->vTemp, 1, &pC );
                        assert( pC == NULL );
                    }
                    else
                    {
                        Value = Sat_ClauseCreate( p, p->vTemp, 1, &pC );
                        assert( Value );
                        Sat_SolverCancelUntil( p, (p->nLevelRoot > nLevelBack)? p->nLevelRoot : nLevelBack );
                        Sat_ClauseVecPush( p->vClauses, pC );
                    }
                }

                // it is important that recording is done before cancelling
                // because recording in this case does not add anything to the queue
                // but cancelling uses info about levels to assign the second watch
                // (this info would not be available if we cancelled before recording)
//                Sat_SolverVarDecayActivity( p );
//                Sat_SolverClaDecayActivity( p );
                Sat_QueueClear( p->pQueue );
                ///////////////////////////////////////////
                nAssigns = Sat_IntVecReadSize( p->vTrail );
                if ( nAssigns == p->nVars ) 
                    continue;

                // repeat variable selection if this the minterm enumeration process
                // and the corresponding clause has been added
                Var = Sat_OrderSelect(p->pOrder);
            }
            if ( nConfLimit > 0 && nConfs > nConfLimit ) {
                // Reached bound on number of conflicts:
                p->dProgress = Sat_SolverProgressEstimate( p );
                Sat_QueueClear( p->pQueue );
                Sat_SolverCancelUntil( p, p->nLevelRoot );
                return SAT_UNKNOWN; 
            }
            else if ( nBackTrackLimit > 0 && nConfs > nBackTrackLimit ) {
                // Reached bound on number of conflicts:
                Sat_QueueClear( p->pQueue );
                Sat_SolverCancelUntil( p, p->nLevelRoot );
                return SAT_UNKNOWN; 
            }
            else{
                // New variable decision:
                p->Stats.nDecisions++;
                // select the next var and assume its positive polarity to be true
//                check(assume(Lit(order.select()))); 
//Sat_SolverWriteDimacs( p, "problem.cnf" );
//Sat_SolverPrintAssignment( p );
//Sat_SolverWriteGml( p, "problem.gml" );
//                Var = Sat_OrderSelect(p->pOrder);

                assert( Var != SAT_ORDER_UNKNOWN );
                assert( Var >= 0 );
                assert( Var < p->nVars );
                Value = Sat_SolverAssume(p, SAT_VAR2LIT(Var,0) );
                assert( Value );

//                assert( Sat_IntVecReadSize(p->vTrailLim) <= Sat_IntVecReadSize(p->vTrail) + 1 );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


