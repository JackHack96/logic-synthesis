/**CFile****************************************************************

  FileName    [autiMinim.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [New state minimization algorithm.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autiMinim.c,v 1.2 2004/04/08 04:48:25 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define STMIN_STATE_HASH(s1,s2)   ((((unsigned)s1) << 16) | (((unsigned)s2) & 0xffff))
#define STMIN_STATE_UNHASH1(s)     (((unsigned)s) >> 16)
#define STMIN_STATE_UNHASH2(s)     (((unsigned)s) & 0xffff)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs state minimization of the automaton.]

  Description [The automaton should be deterministic. If the automaton
  is incomplete, this procedure completes it by adding a non-accepting 
  don't-care state. If the automaton can be reduced, returns the new 
  automaton, and leave the old automaton unchanged. If the automaton 
  cannot be reduced, returns the same automaton. The caller of this 
  function should compare the input and output pointers to determine 
  whether a new automaton was created.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Auti_AutoStateMinimize( Aut_Auto_t * pAut )
{
    ProgressBar * pProgress;
    Aut_Auto_t * pAutNew;
    Aut_State_t * pState, * pStateNew, * pStateOld, ** ppStates;
    Aut_State_t * pState1, * pState2, * pState1p, * pState2p;
    Aut_State_t * pListAcc, * pListNonAcc;
    Aut_Trans_t * pTrans1, * pTrans2, * pTransNew;
    Extra_Queue_t *  pQueue;
    Extra_BitMat_t * pMatr;
//    Extra_OverlapCache_t * pCache;
    int nStatesNew, nPairs, i;
    unsigned uHash;

    // check trivial cases
    if ( pAut->nStates == 0 )
    {
        printf( "Trying to state minimize an empty automaton.\n" );
        return pAut;
    }
    if ( pAut->nStates == 1 )
        return pAut;
    if ( pAut->nStates >= (1 << 16) )
    {
        printf( "Cannot state minimize automata with more than %d states.\n", (1 << 16) );
        return NULL;
    }

    // check if the automaton is deterministic
    if ( Auti_AutoCheckIsNd( stdout, pAut, pAut->nVars, 0 ) > 0 )
    {
        printf( "The automaton is non-deterministic.\n" );
        return NULL;
    }

    // check if the automaton is complete
    if ( Auti_AutoComplete( pAut, pAut->nVars, 0 ) )
        printf( "Warning: The automaton has been completed before state minimization.\n" );

//    Cudd_ReduceHeap( pAut->pMan->dd, CUDD_REORDER_SYMM_SIFT, 100 );

    // start the queue and the matrix
    pQueue = Extra_QueueStart( 50000 );
    pMatr  = Extra_BitMatrixStart( pAut->nStates );

    // set the state numbers
    Aut_AutoAssignNumbers( pAut );
    // save the states into one array
    ppStates = Aut_AutoCollectStates( pAut );

    // collect the accepting and non-accepting states into two lists
    pListAcc = pListNonAcc = NULL;
    Aut_AutoForEachState_int( pAut, pState )
        if ( pState->fAcc )
            pState->pOrder = pListAcc,    pListAcc = pState;
        else
            pState->pOrder = pListNonAcc, pListNonAcc = pState;

    // add the distinquished pairs
    nPairs = 0;
    for ( pState1 = pListAcc;    pState1; pState1 = pState1->pOrder )
    for ( pState2 = pListNonAcc; pState2; pState2 = pState2->pOrder )
    {
        // schedule this pair for visiting
        uHash = STMIN_STATE_HASH(pState1->uData, pState2->uData);
        Extra_QueueWrite( pQueue, (char *)uHash );
        // mark the pair as distinquishable
        Extra_BitMatrixInsert1( pMatr, pState1->uData, pState2->uData );
        nPairs++;
    }

//    pCache = Extra_OverlapCacheStart( pAut->pMan->dd, 250000 );

    // explore the pairs recursively
    pProgress = Extra_ProgressBarStart( stdout, pAut->nStates * pAut->nStates / 2 );
    while ( Extra_QueueRead( pQueue, (char **)&uHash ) )
    {
        pState1 = ppStates[STMIN_STATE_UNHASH1(uHash)];
        pState2 = ppStates[STMIN_STATE_UNHASH2(uHash)];
        // explore the pairs that can reach this pair 
        Aut_StateForEachTransitionTo_int( pState1, pTrans1 )
        Aut_StateForEachTransitionTo_int( pState2, pTrans2 )
        {
            // check if the conditions overlap
            if ( !Aut_AutoTransOverlap( pTrans1, pTrans2 ) )
//            if ( !Extra_bddOverlap( pCache, pTrans1->bCond, pTrans2->bCond ) )
                continue;
            pState1p = pTrans1->pFrom;
            pState2p = pTrans2->pFrom;
            // check if this pair was visited before
            if ( Extra_BitMatrixLookup1( pMatr, pState1p->uData, pState2p->uData ) )
                continue;
            // schedule this pair for visiting
            uHash = STMIN_STATE_HASH(pState1p->uData, pState2p->uData);
            Extra_QueueWrite( pQueue, (char *)uHash );
            // mark the pair as distinquishable
            Extra_BitMatrixInsert1( pMatr, pState1p->uData, pState2p->uData );
        }
        if ( nPairs++ % 10000 == 0 )
            Extra_ProgressBarUpdate( pProgress, nPairs, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    Extra_QueueStop( pQueue );
//printf( "Cache success = %d. Cache failure = %d.\n", 
//       Extra_OverlapCacheReadSuccess(pCache),
//       Extra_OverlapCacheReadFailure(pCache) );
//    Extra_OverlapCacheStop( pCache );

    // count the number of states after state minimization
    // create the mapping of states
    // pState->pData is the equivalent state of the new automaton for this state
    // pState->Mark indicates that this state is already considered
    nStatesNew = 0;
    Aut_AutoCleanMark( pAut );
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( pState->fMark )
            continue;
        // mark this state as visited
        pState->fMark = 1;
        // set is as equivalent to itself
        pState->pData = (char *)nStatesNew;
        // go though other states
        Aut_AutoForEachStateStart_int( pState->pNext, pState2 )
        {
            if ( pState2->fMark )
                continue;
            // check if they are distinquishable
            if ( Extra_BitMatrixLookup1( pMatr, pState->uData, pState2->uData ) )
                continue;  
            // these two states are equivalent
            // set pState2 as equivalent to pState
            pState2->pData = (char *)nStatesNew;
            // mark it as visited
            pState2->fMark = 1;
        }
        // count this state as new state
        ppStates[nStatesNew++] = pState;
    }

    // check if we achieved any reduction in the number of states
    if ( nStatesNew == pAut->nStates )
    {
        printf( "State minimization did not reduce the number of states.\n" );
        Extra_BitMatrixStop( pMatr );
        FREE( ppStates );
        return pAut;
    }

    // create the state minimum automaton

    // start the new automaton
    pAutNew = Aut_AutoClone( pAut, pAut->pMan );
    // set the automaton name
    pAutNew->pName = util_strsav(pAut->pName);

    // create and add the states of the new automaton
    for ( i = 0; i < nStatesNew; i++ )
    {
        pStateOld = ppStates[i];
        pStateNew = Aut_StateDup( pAutNew, pStateOld );
        Aut_AutoStateAddLast( pStateNew );
        // the old state remembers the new state
        pStateOld->pData2 = (char *)pStateNew;
    }

    // get the old state that is equivalent to the init state
    pStateOld = ppStates[(int)pAut->pInit[0]->pData];
    // set the new initial state
    pAutNew->pInit[0] = (Aut_State_t *)pStateOld->pData2;

    // add the transitions
    for ( i = 0; i < nStatesNew; i++ )
    {
        pStateOld = ppStates[i];
        // the starting new state
        pState1 = (Aut_State_t *)pStateOld->pData2;
        // go through the old state's transitions and add them
        Aut_StateForEachTransitionFrom_int( pStateOld, pTrans2 )
        {
            // the destination new states
            pState2 = (Aut_State_t *)ppStates[(int)pTrans2->pTo->pData]->pData2;
            // add the transition
            pTransNew = Aut_TransAlloc( pAutNew );
            pTransNew->pFrom = pState1;
            pTransNew->pTo   = pState2;
            pTransNew->bCond = pTrans2->bCond;  Cudd_Ref( pTransNew->bCond );
            // add the transition
            Aut_AutoAddTransition( pTransNew );
        }
    }

    printf( "State minimization:  (%d states, %d trans)  ->  (%d states, %d trans)\n", 
        pAut->nStates,    Aut_AutoCountTransitions( pAut ),
        pAutNew->nStates, Aut_AutoCountTransitions( pAutNew ) );

    Extra_BitMatrixStop( pMatr );
    FREE( ppStates );
    return pAutNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


