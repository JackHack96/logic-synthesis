/**CFile****************************************************************

  FileName    [auReduce.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with automata and FSMs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auReduce.c,v 1.1 2004/02/19 03:06:48 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


static void Au_EssentialTransitions( DdManager * dd, Au_Auto_t * pAut );
static int  Au_FindCoreStates( Au_Auto_t * pAut );
static void Au_FindCoreStates_rec( Au_Auto_t * pAut, Au_State_t * pState );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Finds one deterministic reduction of the FSM-ND automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoReduction( Au_Auto_t * pAut, int nInputs, bool fVerbose )
{
    Au_Auto_t * pAutR;
    DdManager * dd;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    int nStatesReduced;

    if ( nInputs > pAut->nInputs )
    {
        printf( "The number of FSM inputs is larger than the number of all inputs.\n" );
        return NULL;
    }

    // get the manager and expand it if necessary
    dd = Cudd_Init( pAut->nInputs, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // get the var maps
    pVm = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), nInputs, 0 );
    pVmx = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, -1, NULL );

    // get the conditions for each arch
    Au_ComputeConditions( dd, pAut, pVmx );

    // find essential transitions for each state
    Au_EssentialTransitions( dd, pAut );
    // extract the core
    nStatesReduced = Au_FindCoreStates( pAut );
    // extract the automaton with marked states and transitions
    pAutR = Au_AutoExtract( pAut, 1 );

    // clean the current automaton
    Au_DeleteConditions( dd, pAut );

    Extra_StopManager( dd );
    return pAutR;
}

/**Function*************************************************************

  Synopsis    [Finds the esential transitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_EssentialTransitions( DdManager * dd, Au_Auto_t * pAut )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans1, * pTrans2;
    DdNode * bSum, * bProd, * bTemp;
    int s;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the state
        pState = pAut->pStates[s];
        // for each pair of transitions, compute the overlaps
        bSum = b0;    Cudd_Ref( bSum );
        Au_StateForEachTransition( pState, pTrans1 )
        {
            Au_StateForEachTransitionStart( pTrans1->pNext, pTrans2 )
            {
                // create the overlap
                bProd = Cudd_bddAnd( dd, pTrans1->bCond, pTrans2->bCond ); Cudd_Ref( bProd );
                // add this to the sum of characterizations
                bSum  = Cudd_bddOr( dd, bTemp = bSum, bProd );             Cudd_Ref( bSum );
                Cudd_RecursiveDeref( dd, bTemp );
                Cudd_RecursiveDeref( dd, bProd );
            }
        }
        // the essential condition is the complement of overlaps
        pState->bTrans = Cudd_Not(bSum); // takes ref
        // mark essential transitions
        Au_StateForEachTransition( pState, pTrans1 )
        {
            // if the condition for this transition overlaps with ess. condition
            // then this is an essential transition
            if ( !Cudd_bddLeq( dd, pTrans1->bCond, Cudd_Not(pState->bTrans) ) ) // overlap
                pTrans1->uCond = 1;
            else
                pTrans1->uCond = 0;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Computes the core of the automaton.]

  Description [The core includes the states reachable from the initial
  state through the essential transitions. The core includes the transitions 
  among the core states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_FindCoreStates( Au_Auto_t * pAut )
{
    Au_State_t * pState, * pStateN;
    Au_Trans_t * pTrans;
    int nStates, s;
    int nTransAdd;
    // mark all states as not belonging to the core
    for ( s = 0; s < pAut->nStates; s++ )
        pAut->pStates[s]->fMark = 0;
    // perform reachability analisys using essential transitions only
    // start from the intial state
    Au_FindCoreStates_rec( pAut, pAut->pStates[0] );

    // mark the transitions, which go into the core states
    nTransAdd = 0;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
        // skip the non-core states
        if ( pState->fMark == 0 )
            continue;
        // explore the transitions
        Au_StateForEachTransition( pState, pTrans )
        {
            pStateN = pAut->pStates[pTrans->StateNext];
            if ( pStateN->fMark == 0 )
                continue;
            // if the transition is not marked, mark it
            if ( pTrans->uCond == 0 )
            {
                pTrans->uCond = 1;
                nTransAdd++;
            }
        }
    }

    // count how many states we included
    nStates = 0;
    for ( s = 0; s < pAut->nStates; s++ )
        nStates += pAut->pStates[s]->fMark;
    printf( "Total states = %d. Core states = %d. Added trans = %d.\n",
        pAut->nStates, nStates, nTransAdd );

    // return if there are no new states
//    if ( nStates == pAut->nStates )
        return nStates;
}

/**Function*************************************************************

  Synopsis    [Computes the states reachable from the given one in the core automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_FindCoreStates_rec( Au_Auto_t * pAut, Au_State_t * pState )
{
    Au_State_t * pStateN;
    Au_Trans_t * pTrans;
    // mark the current state
    pState->fMark = 1;
    // explore transitions from this state
    Au_StateForEachTransition( pState, pTrans )
    {
        pStateN = pAut->pStates[pTrans->StateNext];
        // if the next state if already marked, no need to explore it
        if ( pStateN->fMark )
            continue;
        if ( pTrans->uCond == 0 ) // non-essential
            continue;
        Au_FindCoreStates_rec( pAut, pStateN );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


