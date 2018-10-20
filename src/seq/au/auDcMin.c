/**CFile****************************************************************

  FileName    [auDcMin.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with automata and FSMs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auDcMin.c,v 1.1 2004/02/19 03:06:44 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Extra_BitGraphColorTest( int nVerts );

static Au_Auto_t *       Au_ReduceStates( Au_Auto_t * pAut, int * pColors, int nColors, int iDcState );
static Extra_BitGraph_t * Au_SetupGraph( DdManager * dd, Au_Auto_t * pAut, int iDcState );
static void               Au_DeleteConditionSums( DdManager * dd, Au_Auto_t * pAut );
static void               Au_ComputeConditionSums( DdManager * dd, Au_Auto_t * pAut, int iDcState );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performes DC-based collapsing of states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoStateDcMinimize( Au_Auto_t * pAut, int fVerbose )
{
    Au_Auto_t * pAutR;
    DdManager * dd;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int iDcState;
    int nIncompStates;
    Extra_BitGraph_t * pGraph;
    int * pColors;
    int nColors, s, v, FirstColor;
    int nNonAccepting;

    if ( pAut->nStates >= (1 << 14) )
    {
        printf( "Cannot color graphs with more than %d states.\n", (1 << 14) );
        return NULL;
    }

    // cannot process non-deterministic automata
    if ( Au_AutoCheckNd( stdout, pAut, pAut->nInputs, 0 ) > 0 )
    {
        printf( "Cannot process non-deterministic automaton.\n" );
        return NULL;
    }

    // check whether the automaton has the accepting DC state
    iDcState = Au_FindDcState( pAut );

    // check whether the automaton is complete
    nIncompStates = Au_AutoComplete( pAut, pAut->nInputs, 0, 1 );

    // if it is incomplete and has the DC state, we cannot proceed
    if ( nIncompStates && iDcState >= 0 )
    {
        printf( "The input automaton has the accepting DC state and is incomplete.\n" );
        printf( "It is completed with another DC state, which is non-accepting.\n" );
        Au_AutoComplete( pAut, pAut->nInputs, 0, 0 );
    }

    // make sure that there is no more than one non-accepting state
    nNonAccepting = 0;
    for ( s = 0; s < pAut->nStates; s++ )
        nNonAccepting += (int)(pAut->pStates[s]->fAcc == 0);
    if ( nNonAccepting > 1 )
    {
        printf( "Cannot process the automaton with non-accepting states.\n" );
        return NULL;
    }
    if ( nNonAccepting == 1 )
    {
        // find this non-acceptign state
        for ( s = 0; s < pAut->nStates; s++ )
            if ( !pAut->pStates[s]->fAcc )
                break;
        // make sure this a DC state
        if ( !pAut->pStates[s]->pHead || 
              pAut->pStates[s]->pHead->pNext ||
             !Mvc_CoverIsTautology(pAut->pStates[s]->pHead->pCond) )
        {
            printf( "The automaton has a non-accepting state that does not look like the non-accepting DC state.\n" );
            return NULL;
        }

    }



    // get the manager and expand it if necessary
    dd = Cudd_Init( pAut->nInputs, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // get the var maps
    pVm = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 0 );
    pVmx = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, -1, NULL );

    // get the conditions for each arch
    Au_ComputeConditions( dd, pAut, pVmx );

    // for each state, create the sums of conditions into the non-DC state
    Au_ComputeConditionSums( dd, pAut, iDcState );

    // create the graph coloring problem
    pGraph = Au_SetupGraph( dd, pAut, iDcState );
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


    if ( nColors < pAut->nStates - (int)(iDcState >= 0) )
    {
        // create the new automaton
        pAutR = Au_ReduceStates( pAut, pColors, nColors, iDcState );
    }
    else
    {
        printf( "The DC-miminization did not reduce the number of states.\n" );
        pAutR = Au_AutoDup( pAut );
    }

    // delete the sums of conditions on the states
//    Au_DeleteConditionSums( dd, pAut );

    // clean the current automaton
    Au_DeleteConditions( dd, pAut );

    Extra_StopManager( dd );
    FREE( pColors );

    // complete the resulting automaton with the accepting state
    Au_AutoComplete( pAutR, pAutR->nInputs, 1, 0 );
    return pAutR;
}


/**Function*************************************************************

  Synopsis    [Computes the sums of conditions for each state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_ComputeConditionSums( DdManager * dd, Au_Auto_t * pAut, int iDcState )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans;
    DdNode * bSum, * bTemp;
    int s;

    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the state
        pState = pAut->pStates[s];
        // go through all the transitions
        bSum = b0;        Cudd_Ref( bSum );
        Au_StateForEachTransition( pState, pTrans )
        {
            if ( pTrans->StateNext == iDcState )
                continue;
            bSum = Cudd_bddOr( dd, bTemp = bSum, pTrans->bCond );  Cudd_Ref( bSum );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        // set the condition
        pState->bTrans = bSum;  // takes ref
    }
}

/**Function*************************************************************

  Synopsis    [Sets up the graph coloring problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_DeleteConditionSums( DdManager * dd, Au_Auto_t * pAut )
{
    Au_State_t * pState;
    int s;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the state
        pState = pAut->pStates[s];
        // delete the condition
        if ( pState->bTrans )
            Cudd_RecursiveDeref( dd, pState->bTrans );
    }
}

/**Function*************************************************************

  Synopsis    [Sets up the graph coloring problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitGraph_t * Au_SetupGraph( DdManager * dd, Au_Auto_t * pAut, int iDcState )
{
    Extra_BitGraph_t * pGraph;
    Au_State_t * pState1;
    Au_State_t * pState2;
    int s1, s2;

    pGraph = Extra_BitGraphAlloc( pAut->nStates );
    for ( s1 = 0;      s1 < pAut->nStates; s1++ )
    for ( s2 = s1 + 1; s2 < pAut->nStates; s2++ )
    {
        if ( s1 == iDcState || s2 == iDcState )
            continue;
        pState1 = pAut->pStates[s1];
        pState2 = pAut->pStates[s2];
        if ( !Cudd_bddLeq( dd, pState1->bTrans, Cudd_Not(pState2->bTrans) ) ) // intersect
            Extra_BitGraphEdgeAdd( pGraph, s1, s2 );
    }
    return pGraph;
}

/**Function*************************************************************

  Synopsis    [Derives the reduced automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_ReduceStates( Au_Auto_t * pAut, int * pColors, int nColors, int iDcState )
{
    Au_Auto_t * pAutNew;
    Au_Trans_t * pTrans, * pTransNew;
    Au_State_t * pState, * pStateNew;
    int s1, s2, s, i;

    // start the new automaton
    pAutNew = Au_AutoClone( pAut );

    // set the new number of states
    pAutNew->nStates = nColors;
    // add the states
    pAutNew->nStatesAlloc = nColors + 1;
    pAutNew->pStates = ALLOC( Au_State_t *, pAutNew->nStatesAlloc );
    for ( i = 0; i < nColors; i++ )
    {
        // start the new state
        pStateNew        = Au_AutoStateAlloc();
        pStateNew->fAcc  = 1;
        // find the first state with this color
        for ( s = 0; s < pAut->nStates; s++ )
            if ( pColors[s] == i )
                break;
        pStateNew->pName = util_strsav( pAut->pStates[s]->pName );
        st_insert( pAutNew->tStateNames, pStateNew->pName, NULL );
        // add the new state
        pAutNew->pStates[i] = pStateNew;
    }

    // add the transitions
    for ( i = 0; i < pAut->nStates; i++ )
    {
        // skip the DC state
        if ( i == iDcState )
            continue;
        // get the state
        pState = pAut->pStates[i];
        // mark the non-accpeting state
        if ( pState->fAcc == 0 )
            pAutNew->pStates[pColors[i]]->fAcc = 0;

        // go through the state's transitions and add them
        Au_StateForEachTransition( pState, pTrans )
        {
            // skip the transition if it goes into the DC state
            if ( pTrans->StateNext == iDcState )
                continue;

            // check that the transition is valid
            assert( i == pTrans->StateCur );

            // get the new states
            s1 = pColors[pTrans->StateCur];
            s2 = pColors[pTrans->StateNext];

            // check whether transition among these states already exists
            Au_StateForEachTransition( pAutNew->pStates[s1], pTransNew )
                if ( pTransNew->StateNext == s2 )
                    break;
            // check whether it is found
            if ( pTransNew )
            {
                // add the condition to this transition
                Mvc_CoverCopyAndAppendCubes( pTransNew->pCond, pTrans->pCond );
            }
            else
            {
                // create the transition
                pTransNew = Au_AutoTransAlloc();
                pTransNew->pCond = Mvc_CoverDup( pTrans->pCond ); 
                pTransNew->StateCur  = s1;
                pTransNew->StateNext = s2;
                // add the transition
                Au_AutoTransAdd( pAutNew->pStates[s1], pTransNew );
            }
        }
    }
    return pAutNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


