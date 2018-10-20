/**CFile****************************************************************

  FileName    [autiDcMin.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with automata and FSMs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: autiDcMin.c,v 1.1 2004/02/19 03:06:50 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"
 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Extra_BitGraphColorTest( int nVerts );

static void               Aut_AutoComputeCareSets( Aut_Auto_t * pAut, Aut_State_t * pStateDcA );
static DdNode *           Aut_StateComputeCareSet( Aut_State_t * pState, Aut_State_t * pStateDcA );
static Extra_BitGraph_t * Aut_SetupGraph( Aut_Auto_t * pAut );
static Aut_Auto_t *       Aut_ReduceStates( Aut_Auto_t * pAut, int * pColors, int nColors, Aut_State_t * pStateDcA, Aut_State_t * pStateDcN );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performes DC-based collapsing of states.]

  Description []
               
  SideEffects [Completes the automaton.]

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Auti_AutoStateDcMin( Aut_Auto_t * pAut, int fVerbose )
{
    Extra_BitGraph_t * pGraph;
    Aut_Auto_t * pAutR;
    Aut_State_t * pStateDcA, * pStateDcN;
    int * pColors;
    int nNonAccepting;
    int nColors, v, FirstColor;

    // check the size of the problem
    if ( pAut->nStates >= (1 << 14) )
    {
        printf( "Cannot color graphs with more than %d states.\n", (1 << 14) );
        return NULL;
    }

    // cannot process non-deterministic automata
    if ( Auti_AutoCheckIsNd( stdout, pAut, pAut->nVars, 0 ) > 0 )
    {
        printf( "Cannot process non-deterministic automaton.\n" );
        return NULL;
    }

    // check whether the automaton has the accepting DC state
    pStateDcA = Auti_AutoFindDcStateAccepting( pAut );
    if ( pStateDcA == NULL )
    {
        printf( "The automaton does not have the accepting DC state.\n" );
        return NULL;
    }

    // complete the automaton
    Auti_AutoComplete( pAut, pAut->nVars, 0 );

    // if the automaton as only one non-accepting state (DC state), it is okay
    // there should not be other non-accepting states
    pStateDcN = Auti_AutoFindDcStateNonAccepting( pAut );
    nNonAccepting = Aut_AutoCountNonAccepting(pAut);
    if ( !( pStateDcN == NULL && nNonAccepting == 0 ||
            pStateDcN         && nNonAccepting == 1 ) )
    {
        printf( "The automaton should have no non-accepting states,\n" );
        printf( "or it can have at most one non-accepting DC state.\n" );
        return NULL;
    }

    // compute the sums of conditions for each state, except the DC state
    Aut_AutoDerefSumCond( pAut );
    Aut_AutoComputeCareSets( pAut, pStateDcA );

    // create the graph coloring problem
    pGraph = Aut_SetupGraph( pAut );
//Extra_BitGraphPrint( pGraph );

    // solve the graph coloring problem
    pColors = ALLOC( int, pAut->nStates );
    nColors = Extra_BitGraphColor( pGraph, pColors, fVerbose );
    Extra_BitGraphFree( pGraph );

    // post-process the colors, replace the first one by the last one, and vice versa
    FirstColor = pColors[0];
    for ( v = 0; v < pAut->nStates; v++ )
        if ( pColors[v] == FirstColor )
            pColors[v] = 0;
        else if ( pColors[v] == 0 )
            pColors[v] = FirstColor;


    // create the new automaton
    if ( nColors < pAut->nStates - nNonAccepting )
        pAutR = Aut_ReduceStates( pAut, pColors, nColors, pStateDcA, pStateDcN );
    else
    {
        printf( "The DC-miminization did not reduce the number of states.\n" );
        pAutR = Aut_AutoDup( pAut, pAut->pMan );
    }
    FREE( pColors );

    // delete the sums of conditions on the states
    Aut_AutoDerefSumCond( pAut );

    // complete the resulting automaton with the accepting state
    if ( pAutR )
        Auti_AutoComplete( pAutR, pAutR->nVars, 1 );
    return pAutR;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoComputeCareSets( Aut_Auto_t * pAut, Aut_State_t * pStateDcA )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( pState->bCond )
            Cudd_RecursiveDeref( pAut->pMan->dd, pState->bCond );
        pState->bCond = Aut_StateComputeCareSet( pState, pStateDcA );
        Cudd_Ref( pState->bCond );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Aut_StateComputeCareSet( Aut_State_t * pState, Aut_State_t * pStateDcA )
{
    DdManager * dd = pState->pAut->pMan->dd;
    DdNode * bSum, * bTemp;
    Aut_Trans_t * pTrans;

    bSum = b0;      Cudd_Ref( bSum );
    Aut_StateForEachTransitionFrom_int( pState, pTrans )
    {
        if ( pTrans->pTo == pStateDcA )
            continue;
        bSum  = Cudd_bddOr( dd, bTemp = bSum, pTrans->bCond );    Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bSum );
    return bSum;
}

/**Function*************************************************************

  Synopsis    [Sets up the graph coloring problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitGraph_t * Aut_SetupGraph( Aut_Auto_t * pAut )
{
    Extra_BitGraph_t * pGraph;
    Aut_State_t * pState1, * pState2;
    DdManager * dd = pAut->pMan->dd; 

    // assign numbers to states
    Aut_AutoAssignNumbers( pAut );
    // create the graph
    pGraph = Extra_BitGraphAlloc( pAut->nStates );
    // iterate through the state pairs
    Aut_AutoForEachState_int( pAut, pState1 )
    {
        Aut_AutoForEachStateStart_int( pState1->pNext, pState2 )
        {
            // add an incompatibility edge to the graph is the states' care sets overlap
            if ( !Cudd_bddLeq( dd, pState1->bCond, Cudd_Not(pState2->bCond) ) ) // intersect
                Extra_BitGraphEdgeAdd( pGraph, pState1->uData, pState2->uData );
        }
    }
    return pGraph;
}

/**Function*************************************************************

  Synopsis    [Derives the reduced automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Aut_ReduceStates( Aut_Auto_t * pAut, 
    int * pColors, int nColors, Aut_State_t * pStateDcA, Aut_State_t * pStateDcN )
{
    DdManager * dd = pAut->pMan->dd;
    DdNode * bTemp;
    Aut_Auto_t * pAutNew;
    Aut_Trans_t * pTrans, * pTransNew;
    Aut_State_t * pState, * pStateNew;
    Aut_State_t * pStateNew1, * pStateNew2;
    Aut_State_t ** ppStatesNew;
    int i;

    // set the colors of states
    i = 0;
    Aut_AutoForEachState_int( pAut, pState )
        pState->uData = pColors[i++];

    // start the new automaton
    pAutNew = Aut_AutoClone( pAut, pAut->pMan );
    // set the automaton name
    pAutNew->pName = util_strsav(pAut->pName);

    // create and store the states of the new automaton
    ppStatesNew = ALLOC( Aut_State_t *, nColors );
    for ( i = 0; i < nColors; i++ )
    {
        // start the new state
        pStateNew       = Aut_StateAlloc( pAutNew );
        pStateNew->fAcc = 1;
        // find the first state with this color
        Aut_AutoForEachState_int( pAut, pState )
            if ( pState->uData == (unsigned)i )
                break;
        pStateNew->pName = Aut_AutoRegisterName( pAutNew, pState->pName );
        // add the new state
        Aut_AutoStateAddLast( pStateNew );
        ppStatesNew[i] = pStateNew;
    }

    // set the non-accepting state to be non-accepting in the new automaton
    if ( pStateDcN )
        ppStatesNew[pStateDcN->uData]->fAcc = 0;

    // set the initial state
    pAutNew->pInit[0] = ppStatesNew[pAut->pInit[0]->uData];

    // add the transitions
    Aut_AutoForEachState_int( pAut, pState )
    {
        // skip the DC state
        if ( pState == pStateDcA )
            continue;
        // go through the state's transitions and add them
        Aut_StateForEachTransitionFrom_int( pState, pTrans )
        {
            // check that the transition is valid
            assert( pTrans->pFrom == pState );
            // skip the transition if it goes into the DC state
            if ( pTrans->pTo == pStateDcA )
                continue;
            // get the new states, which should take this transition
            pStateNew1 = ppStatesNew[pTrans->pFrom->uData];
            pStateNew2 = ppStatesNew[pTrans->pTo->uData];
            // check whether transition among these states already exist
            Aut_StateForEachTransitionFrom_int( pStateNew1, pTransNew )
                if ( pTransNew->pTo == pStateNew2 )
                    break;
            if ( pTransNew )
            {   // the transition exists - add the condition to this transition
                pTransNew->bCond  = Cudd_bddOr( dd, bTemp = pTransNew->bCond, pTrans->bCond );    
                Cudd_Ref( pTransNew->bCond );
                Cudd_RecursiveDeref( dd, bTemp );
            }
            else
            {   // the transition does not exist - create a new transition
                pTransNew = Aut_TransAlloc( pAutNew );
                pTransNew->pFrom = pStateNew1;
                pTransNew->pTo   = pStateNew2;
                pTransNew->bCond = pTrans->bCond;  Cudd_Ref( pTransNew->bCond );
                // add the transition
                Aut_AutoAddTransition( pTransNew );
            }
        }
    }
    FREE( ppStatesNew );
    return pAutNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

