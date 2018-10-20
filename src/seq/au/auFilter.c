/**CFile****************************************************************

  FileName    [auFilter.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various state filtering procedures (prefix-closed, progressive).]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auFilter.c,v 1.1 2004/02/19 03:06:46 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Au_AutoReachability_rec( Au_Auto_t * pAut, int State, int Level, int fUseMark );

static bool Au_AutoMakeMooreState( Au_Auto_t * pAut, Au_State_t * pState, DdNode * bCube, 
    Vmx_VarMap_t * pVmx, DdManager * dd, DdNode ** pbCodes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Makes the automaton prefix closed.]

  Description [Produces a new automaton, that is different from the
  current one in the following respects. The non-accepting states 
  are removed together with all the transitions. Only the reachable
  states of the resulting machine are collected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoPrefixClose( Au_Auto_t * pAut )
{
    int s;
    // mark the non-accepting states as special
    for ( s = 0; s < pAut->nStates; s++ )
        pAut->pStates[s]->fMark = pAut->pStates[s]->fAcc;
    // filter the states
    return Au_AutoFilter( pAut );
}
 
/**Function*************************************************************

  Synopsis    [Makes the automaton progressive.]

  Description [Produces a new automaton, that is different from the
  current one in the following respects. The non-accepting states 
  and the state that are not complete in the FSM sense are removed
  together with all the transitions. Only the reachable
  states of the resulting automaton are collected.]


  states of the resulting machine are collected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoProgressive( Au_Auto_t * pAut, int nInputs, int fMoore, int fVerbose )
{
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    DdNode ** pbCodes;
    DdManager * dd;
    DdNode * bCube;
    int s, fChanges;
    int nIter = 0;
    
    // create the MVC data to compute the intersection of conditions
    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 0 );

    // prepare to the BDD-based computation of the complemented conditions
    pVmx     = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, -1, NULL );
    dd       = Fnc_ManagerReadDdLoc( pAut->pMan );
    pbCodes  = Vmx_VarMapEncodeMap( dd, pVmx );

    // if we are doing Moore filtering, collect the input variables
    // if we are doing progressive, collect the output variables
    if ( fMoore )
        bCube = Vmx_VarMapCharCubeRange( dd, pVmx, 0, nInputs );  // inputs
    else
        bCube = Vmx_VarMapCharCubeRange( dd, pVmx, nInputs, pAut->nInputs - nInputs );  // outputs
    Cudd_Ref( bCube );

    // originally mark the accepting states
    for ( s = 0; s < pAut->nStates; s++ )
        pAut->pStates[s]->fMark = pAut->pStates[s]->fAcc;

    // additionally mark the incomplete states 
    // (only accepting and complete states should be taken)
    nIter = 1;
    do 
    {
        if ( fVerbose )
            printf( "Iteration %d\n", nIter );
        fChanges = 0;
        for ( s = 0; s < pAut->nStates; s++ )
            if ( pAut->pStates[s]->fMark == 1 )
            {
                pAut->pStates[s]->fMark = Au_AutoStateIsCcmplete( pAut, pAut->pStates[s], 
                        bCube, pVm, dd, pbCodes, 1, fMoore );
                if ( pAut->pStates[s]->fMark == 0 )
                {
                    if ( fVerbose )
                        printf( "State %s (%2d), originally accepting, is now removed.\n",
                            pAut->pStates[s]->pName, s );
                    fChanges = 1;
                }
            }
        nIter++;
    } while ( fChanges );
    
    // if we are doing Moore,
    // for each remaning state, reduce the transitions to only those that satisty Mooreness
    if ( fMoore )
    {
        // if the initial state is gone, quit
        if ( pAut->pStates[0]->fMark == 0 )
        {
            printf( "The result of Moore reduction is empty.\n" );
            // free useless data stuctures
            Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );
            Cudd_RecursiveDeref( dd, bCube );
            return Au_AutoClone( pAut );
        }

        for ( s = 0; s < pAut->nStates; s++ )
            if ( pAut->pStates[s]->fMark == 1 )
                Au_AutoMakeMooreState( pAut, pAut->pStates[s], bCube, pVmx, dd, pbCodes );
    }


    // free useless data stuctures
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );
    Cudd_RecursiveDeref( dd, bCube );

    // filter the states
    return Au_AutoFilter( pAut );
}

/**Function*************************************************************

  Synopsis    [Filters certain states and transitions.]

  Description [Produces a new automaton, that is different from the
  current one in the following respects. The non-accepting states 
  and (all the associated transitions) are removed. Only the reachable
  states of the resulting machine are collected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoFilter( Au_Auto_t * pAut )
{
    Au_Auto_t * pAutR;
    Au_State_t * pStateR;
    Au_Trans_t * pTrans, * pTransR;
    int nStatesNew, s, iState;

    // if the initial state is not marked, return empty
    if ( pAut->pStates[0]->fMark == 0 )
        return Au_AutoClone( pAut );

    // mark the states reachable through the accepting states
    Au_AutoReachability( pAut, 1 );

    // get the number of reachable states
    nStatesNew = 0;
    for ( s = 0; s < pAut->nStates; s++ )
        if ( pAut->pStates[s]->Level != UNREACHABLE )
            nStatesNew++;

    // create the new automaton
    pAutR = Au_AutoClone( pAut );
    pAutR->nStates = nStatesNew;
    pAutR->nStatesAlloc = nStatesNew;
    pAutR->pStates = ALLOC( Au_State_t *, pAutR->nStatesAlloc );

    // create the states
    iState = 0;
    for ( s = 0; s < pAut->nStates; s++ )
        if ( pAut->pStates[s]->Level != UNREACHABLE )
        {
            pStateR = Au_AutoStateAlloc();
            pStateR->fAcc = 1;
            pStateR->pName = util_strsav( pAut->pStates[s]->pName );
            pStateR->State1 = s; // remember the producing state
            pStateR->State2 = 0;
            pAut->pStates[s]->State1 = iState; // remember this state in the producing state

            pAutR->pStates[iState++] = pStateR;
            // add the state names to the table
            st_insert( pAutR->tStateNames, pStateR->pName, NULL );
        }

    // add the transitions
    for ( s = 0; s < pAutR->nStates; s++ )
    {
        pStateR = pAutR->pStates[s];
        // look at the transitions in the original automaton
        assert( pAut->pStates[pStateR->State1]->Level != UNREACHABLE );
        Au_StateForEachTransition( pAut->pStates[pStateR->State1], pTrans )
            if ( pAut->pStates[pTrans->StateNext]->Level != UNREACHABLE )
            { // add the transition from the current state to the corresponding state
                pTransR = Au_AutoTransAlloc();
                pTransR->pCond = Mvc_CoverDup( pTrans->pCond );
                pTransR->StateCur  = s;
                pTransR->StateNext = pAut->pStates[pTrans->StateNext]->State1;
                // add the transition
                Au_AutoTransAdd( pStateR, pTransR );
            }
    }

    // detect the trivial case when the product automaton is empty
    if ( pAutR->nStates == 1 && pAutR->pStates[0]->pHead == NULL )
    {
        // there are no state and no transitions
        FREE( pAutR->pStates[0] );
        pAutR->nStates = 0;
        return pAutR;
    }
    return pAutR;
}

/**Function*************************************************************

  Synopsis    [Removes the marked states and transitions into them.]

  Description [Produces a new automaton, that is different from the
  current one in the following respects. The marked states of the original
  automaton are preserved. All other states and (all the associated 
  transitions) are removed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoFilterMarked( Au_Auto_t * pAut )
{
    Au_Auto_t * pAutR;
    Au_State_t * pStateR;
    Au_Trans_t * pTrans, * pTransR;
    int nStatesNew, s, iState;

    // get the number of reachable states
    nStatesNew = 0;
    for ( s = 0; s < pAut->nStates; s++ )
        if ( pAut->pStates[s]->fMark )
            nStatesNew++;

    // create the new automaton
    pAutR = Au_AutoClone( pAut );
    pAutR->nStates = nStatesNew;
    pAutR->nStatesAlloc = nStatesNew;
    pAutR->pStates = ALLOC( Au_State_t *, pAutR->nStatesAlloc );

    // create the states
    iState = 0;
    for ( s = 0; s < pAut->nStates; s++ )
        if ( pAut->pStates[s]->fMark )
        {
            pStateR = Au_AutoStateAlloc();
            pStateR->fAcc = pAut->pStates[s]->fAcc;
            pStateR->pName = util_strsav( pAut->pStates[s]->pName );
            pStateR->State1 = s; // remember the producing state
            pStateR->State2 = 0;
            pAut->pStates[s]->State1 = iState; // remember this state in the producing state

            pAutR->pStates[iState++] = pStateR;
            // add the state names to the table
            st_insert( pAutR->tStateNames, pStateR->pName, NULL );
        }

    // add the transitions
    for ( s = 0; s < pAutR->nStates; s++ )
    {
        pStateR = pAutR->pStates[s];
        // look at the transitions in the original automaton
        assert( pAut->pStates[pStateR->State1]->fMark );
        Au_StateForEachTransition( pAut->pStates[pStateR->State1], pTrans )
            if ( pAut->pStates[pTrans->StateNext]->fMark )
            { // add the transition from the current state to the corresponding state
                pTransR = Au_AutoTransAlloc();
                pTransR->pCond = Mvc_CoverDup( pTrans->pCond );
                pTransR->StateCur  = s;
                pTransR->StateNext = pAut->pStates[pTrans->StateNext]->State1;
                // add the transition
                Au_AutoTransAdd( pStateR, pTransR );
            }
    }

    // detect the trivial case when the product automaton is empty
    if ( pAutR->nStates == 1 && pAutR->pStates[0]->pHead == NULL )
    {
        // there are no state and no transitions
        FREE( pAutR->pStates[0] );
        pAutR->nStates = 0;
        return pAutR;
    }
    return pAutR;
}

/**Function*************************************************************

  Synopsis    [Performs reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoReachability( Au_Auto_t * pAut, int fUseMark )
{
    int nLevels, s;
    int fThereAreUnreachable;
    // set all the state as unreachable
    for ( s = 0; s < pAut->nStates; s++ )
        pAut->pStates[s]->Level = UNREACHABLE;
    // start from the initial state 
    Au_AutoReachability_rec( pAut, 0, 0, fUseMark );
    // find the largest level
    nLevels = 0;
    fThereAreUnreachable = 0;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        if ( pAut->pStates[s]->Level == UNREACHABLE )
        {
            fThereAreUnreachable = 1;
            continue;
        }
        if ( nLevels < pAut->pStates[s]->Level )
            nLevels = pAut->pStates[s]->Level;
    }
    return nLevels + 1;
}

/**Function*************************************************************

  Synopsis    [Performs reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoReachability_rec( Au_Auto_t * pAut, int State, int Level, int fUseMark )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans;

    pState = pAut->pStates[State];
    if ( pState->Level <= Level )
        return;
    // set the level
    pState->Level = Level;
    // explore the rest of the graph
    Au_StateForEachTransition( pState, pTrans )
        if ( !fUseMark || (pState->fMark && pAut->pStates[pTrans->StateNext]->fMark) )
            Au_AutoReachability_rec( pAut, pTrans->StateNext, Level + 1, fUseMark );
}



/**Function*************************************************************

  Synopsis    [Returns 1 if the state is complete or Moore.]

  Description [If the flag fUseMark is set to 1, checks completion only 
  w.r.t. the states that are marked. If the flag fMoore is set to 1,
  checks the Mooreness rather than the completion.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Au_AutoMakeMooreState( Au_Auto_t * pAut, Au_State_t * pState, DdNode * bCube, 
    Vmx_VarMap_t * pVmx, DdManager * dd, DdNode ** pbCodes )
{
    Vm_VarMap_t * pVm = Vmx_VarMapReadVm( pVmx );
    Au_Trans_t * pPrev, * pTrans, * pTrans2;
    DdNode * bCond, * bTemp, * bSum;
    bool RetValue;
    int nBitsIO;

    nBitsIO = Vmx_VarMapReadBitsNum( pVmx );

    // go through the condition and add the functions
    bSum = b0;    Cudd_Ref( bSum );
    Au_StateForEachTransition( pState, pTrans )
    {
        if ( !pAut->pStates[pTrans->StateNext]->fMark )
            continue;

        // derive the condition
        bCond = Fnc_FunctionDeriveMddFromSop( dd, pVm, pTrans->pCond, pbCodes );  Cudd_Ref( bCond );
        bSum  = Cudd_bddOr( dd, bTemp = bSum, bCond );  Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
//        Cudd_RecursiveDeref( dd, bCond );
        pTrans->bCond = bCond; // takes ref
    }
    // if we are trying to make the machine Moore. 
    // universally quantify the input variables
    // and make sure the result is not empty
    bSum  = Cudd_bddUnivAbstract( dd, bTemp = bSum, bCube );    Cudd_Ref( bSum ); 
    Cudd_RecursiveDeref( dd, bTemp );
    // get the return value
    RetValue = (int)(bSum != b0);
    assert( RetValue );

    // remove the transitions that do not intersect with the sum,
    // update the transitions that change after intersecting with the sum
    pPrev = NULL;
    Au_StateForEachTransitionSafe( pState, pTrans, pTrans2 )
    {
        // if the transition goes in an unmarked state, skip it
        // because it will be deleted later
        if ( !pAut->pStates[pTrans->StateNext]->fMark )
        {
            pPrev = pTrans;
            continue;
        }

        // get the condition after modification
        bCond = Cudd_bddAnd( dd, bSum, pTrans->bCond );   Cudd_Ref( bCond );
        Cudd_RecursiveDeref( dd, pTrans->bCond );
        pTrans->bCond = NULL;

        if ( bCond == b0 )
        {
            // remove the transition
            Au_AutoTransFree( pTrans );
            if ( pPrev == NULL )
                pState->pHead = pTrans2;
            else
                pPrev->pNext = pTrans2;
        }
        else
        {
            if ( bCond != pTrans->bCond )
            { // update the condition
                Mvc_CoverFree( pTrans->pCond );
                pTrans->pCond = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pAut->pMan), 
                    dd, bCond, bCond, pVmx, nBitsIO );
            }
            pPrev = pTrans;
        }
        Cudd_RecursiveDeref( dd, bCond );
    }

    Cudd_RecursiveDeref( dd, bSum );
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


