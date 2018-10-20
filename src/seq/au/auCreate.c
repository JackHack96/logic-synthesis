/**CFile****************************************************************

  FileName    [auCreate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Data structure manipulation for automata, states, and transitions.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auCreate.c,v 1.1 2004/02/19 03:06:44 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoAlloc()
{
    Au_Auto_t * p;
    p = ALLOC( Au_Auto_t, 1 );
    memset( p, 0, sizeof(Au_Auto_t) );
    p->tStateNames = st_init_table(strcmp, st_strhash);
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoClone( Au_Auto_t * pAut )
{
    Au_Auto_t * p;
    int i;
    p = Au_AutoAlloc();
    p->pMan     = pAut->pMan;
    p->nInputs  = pAut->nInputs;
    p->nOutputs = pAut->nOutputs;
    p->nInitial = pAut->nInitial;
    p->ppNamesInput = ALLOC( char *, pAut->nInputs );
    for ( i = 0; i < pAut->nInputs; i++ )
        p->ppNamesInput[i] = util_strsav( pAut->ppNamesInput[i] );
//    p->ppNamesOutput = ALLOC( char *, pAut->nOutputs );
//    for ( i = 0; i < pAut->nOutputs; i++ )
//        p->ppNamesOutput[i] = util_strsav( pAut->ppNamesOutput[i] );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoDup( Au_Auto_t * pAut )
{
    Au_Auto_t * pAutNew;
    Au_Trans_t * pTrans, * pTransNew;
    int i;
    pAutNew = Au_AutoClone( pAut );
    pAutNew->nStates = pAut->nStates;
    pAutNew->nStatesAlloc = pAut->nStates;
    // add the states
    pAutNew->pStates = ALLOC( Au_State_t *, pAut->nStates );
    for ( i = 0; i < pAut->nStates; i++ )
    {
        pAutNew->pStates[i] = Au_AutoStateAlloc();
        pAutNew->pStates[i]->pName = util_strsav( pAut->pStates[i]->pName );
        st_insert( pAutNew->tStateNames, pAutNew->pStates[i]->pName, NULL );
        pAutNew->pStates[i]->fAcc = pAut->pStates[i]->fAcc;
    }
    // add the transitions
    for ( i = 0; i < pAut->nStates; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            // create the transition
            pTransNew = Au_AutoTransAlloc();
            pTransNew->pCond = Mvc_CoverDup( pTrans->pCond ); 
            pTransNew->StateCur  = pTrans->StateCur ;
            pTransNew->StateNext = pTrans->StateNext;
            // add the transition
            Au_AutoTransAdd( pAutNew->pStates[i], pTransNew );
        }
    }
    return pAutNew;
}   

/**Function*************************************************************

  Synopsis    [Create the automaton with the reversed transitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoReverse( Au_Auto_t * pAut )
{
    Au_Auto_t * pAutNew;
    Au_Trans_t * pTrans, * pTransNew;
    int i;
    pAutNew = Au_AutoClone( pAut );
    pAutNew->nStates = pAut->nStates;
    pAutNew->nStatesAlloc = pAut->nStates;
    // add the states
    pAutNew->pStates = ALLOC( Au_State_t *, pAut->nStates );
    for ( i = 0; i < pAut->nStates; i++ )
    {
        pAutNew->pStates[i] = Au_AutoStateAlloc();
        pAutNew->pStates[i]->pName = util_strsav( pAut->pStates[i]->pName );
        st_insert( pAutNew->tStateNames, pAutNew->pStates[i]->pName, NULL );
        pAutNew->pStates[i]->fAcc = pAut->pStates[i]->fAcc;
    }
    // add the transitions
    for ( i = 0; i < pAut->nStates; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            // create the transition
            pTransNew = Au_AutoTransAlloc();
            pTransNew->pCond = Mvc_CoverDup( pTrans->pCond ); 
            pTransNew->StateCur  = pTrans->StateNext;
            pTransNew->StateNext = pTrans->StateCur;
            // add the transition
            Au_AutoTransAdd( pAutNew->pStates[pTrans->StateNext], pTransNew );
        }
    }
    return pAutNew;
}

/**Function*************************************************************

  Synopsis    [Extract a subautomaton with marked states and transitions.]

  Description [States are marked by pState->fMark. Transitions are marked
  by pTrans->uCond.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoExtract( Au_Auto_t * pAut, int fUseTransitions )
{
    Au_Auto_t * pAutNew;
    Au_Trans_t * pTrans, * pTransNew;
    Au_State_t * pState, * pStateNew;
    int * pMapping, i, iState;

    // start the new automaton
    pAutNew = Au_AutoClone( pAut );

    // set up mapping of old states into new states
    pMapping = ALLOC( int, pAut->nStates );
    for ( i = 0; i < pAut->nStates; i++ )
        pMapping[i] = -1;
    // add the states
    pAutNew->nStatesAlloc = pAut->nStates;
    pAutNew->pStates = ALLOC( Au_State_t *, pAutNew->nStatesAlloc );
    iState = 0;
    for ( i = 0; i < pAut->nStates; i++ )
    {
        // skip the dropped state
        pState = pAut->pStates[i];
        if ( pState->fMark == 0 )
            continue;
        // start the new state
        pStateNew        = Au_AutoStateAlloc();
        pStateNew->fAcc  = pState->fAcc;
        pStateNew->pName = util_strsav( pState->pName );
        st_insert( pAutNew->tStateNames, pStateNew->pName, NULL );
        // add the new state
        pAutNew->pStates[iState] = pStateNew;
        // reflect this in the mapping
        pMapping[i] = iState++;
    }
    // set the new number of states
    pAutNew->nStates = iState;

    // add the transitions
    for ( i = 0; i < pAut->nStates; i++ )
    {
        // skip the dropped state
        pState = pAut->pStates[i];
        if ( pState->fMark == 0 )
            continue;
        Au_StateForEachTransition( pState, pTrans )
        {
            // skip the transition if it goes into the dropped state
            if ( pAut->pStates[pTrans->StateNext]->fMark == 0 )
            {
//                assert( pTrans->uCond == 0 );
                continue;
            }
            // skip the unmarked transition
            if ( fUseTransitions && pTrans->uCond == 0 )
                continue;
            // check that the transition is valid
            assert( pMapping[pTrans->StateCur]  >= 0 );
            assert( pMapping[pTrans->StateNext] >= 0 );
            assert( i == pTrans->StateCur );

            // create the transition
            pTransNew = Au_AutoTransAlloc();
            pTransNew->pCond = Mvc_CoverDup( pTrans->pCond ); 
            pTransNew->StateCur  = pMapping[pTrans->StateCur];
            pTransNew->StateNext = pMapping[pTrans->StateNext];
            // add the transition
            Au_AutoTransAdd( pAutNew->pStates[pMapping[i]], pTransNew );
        }
    }
    FREE( pMapping );
    return pAutNew;
}

/**Function*************************************************************

  Synopsis    [Creates a single state automaton with one self-loop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoSingleState( Au_Auto_t * pAut, DdManager * dd, DdNode * bCombFunc, Vmx_VarMap_t * pVmx )
{
    Au_Auto_t * pAutNew;
    Au_State_t * pStateNew;
    Au_Trans_t * pTransNew;

    // start the new automaton
    pAutNew = Au_AutoClone( pAut );
    // add the states
    pAutNew->nStates = 1;
    pAutNew->nStatesAlloc = 1;
    pAutNew->pStates = ALLOC( Au_State_t *, pAutNew->nStatesAlloc );

      // start the new state
    pStateNew        = Au_AutoStateAlloc();
    pStateNew->fAcc  = 1;
    pStateNew->pName = util_strsav( pAut->pStates[0]->pName );
    st_insert( pAutNew->tStateNames, pStateNew->pName, NULL );
    // add the new state
    pAutNew->pStates[0] = pStateNew;

    // create the transition
    pTransNew = Au_AutoTransAlloc();
    pTransNew->pCond = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pAut->pMan), 
            dd, bCombFunc, bCombFunc, pVmx, Vmx_VarMapReadBitsNum(pVmx) );
    pTransNew->StateCur  = 0;
    pTransNew->StateNext = 0;
    // add the transition
    Au_AutoTransAdd( pStateNew, pTransNew );
    return pAutNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoFree( Au_Auto_t * pAut )
{
    int i;
    if ( pAut->tStateNames )
        st_free_table( pAut->tStateNames );
    FREE( pAut->pName );
    if ( pAut->pTR )
        Au_AutoRelFree( pAut->pTR );
    if ( pAut->ppNamesInput )
        for ( i = 0; i < pAut->nInputs; i++ )
            FREE( pAut->ppNamesInput[i] );
    if ( pAut->ppNamesOutput )
        for ( i = 0; i < pAut->nOutputs; i++ )
            FREE( pAut->ppNamesOutput[i] );
    for ( i = 0; i < pAut->nStates; i++ )
        Au_AutoStateFree( pAut->pStates[i] );
    FREE( pAut->pStates );
    FREE( pAut->ppNamesInput );
    FREE( pAut->ppNamesOutput );
    FREE( pAut );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Rel_t * Au_AutoGetTR( DdManager * dd, Au_Auto_t * pAut, bool fReorder )
{
    if ( pAut->pTR )
        return pAut->pTR;
    return Au_AutoRelCreate( dd, pAut, fReorder );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoFreeTR( Au_Auto_t * pAut )
{
    if ( pAut->pTR )
    {
        Au_AutoRelFree( pAut->pTR );
        pAut->pTR = NULL;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoCountTransitions( Au_Auto_t * pAut )
{
    Au_Trans_t * pTrans;
    int i, nTrans = 0;
    for ( i = 0; i < pAut->nStates; i++ )
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
            nTrans++;
    return nTrans;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoCountProducts( Au_Auto_t * pAut )
{
    Au_Trans_t * pTrans;
    int i, nCubes = 0;
    for ( i = 0; i < pAut->nStates; i++ )
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
            nCubes += Mvc_CoverReadCubeNum( pTrans->pCond );
    return nCubes;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_State_t * Au_AutoStateAlloc()
{
    Au_State_t * p;
    p = ALLOC( Au_State_t, 1 );
    memset( p, 0, sizeof(Au_State_t) );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoStateFree( Au_State_t * pState )
{
    Au_Trans_t * pTrans, * pTrans2;
    Au_StateForEachTransitionSafe( pState, pTrans, pTrans2 )
        Au_AutoTransFree( pTrans );
    FREE( pState->pName );
    FREE( pState );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Au_AutoStateSumConds( Au_Auto_t * pAut, Au_State_t * pState )
{
    Au_Trans_t * pTrans;
    Mvc_Cover_t * pMvc;
    pMvc = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(pAut->pMan), 2 * pAut->nInputs );
    Au_StateForEachTransition( pState, pTrans )
        Mvc_CoverCopyAndAppendCubes( pMvc, pTrans->pCond );
    return pMvc;
}

/**Function*************************************************************

  Synopsis    [Get the set of accepting states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Au_AutoStateSumAccepting( Au_Auto_t * pAut, DdManager * dd, DdNode * pbCodes[] )
{
    DdNode * bRes, * bTemp;
    int s;
    bRes = b0;  Cudd_Ref( bRes );
    for ( s = 0; s < pAut->nStates; s++ )
        if ( pAut->pStates[s]->fAcc )
        {
            bRes = Cudd_bddOr( dd, bTemp = bRes, pbCodes[s] );  Cudd_Ref( bRes );
            Cudd_RecursiveDeref( dd, bTemp );
        }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Trans_t * Au_AutoTransAlloc()
{
    Au_Trans_t * p;
    p = ALLOC( Au_Trans_t, 1 );
    memset( p, 0, sizeof(Au_Trans_t) );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []
`
  SeeAlso     []

***********************************************************************/
void Au_AutoTransFree( Au_Trans_t * pTrans )
{
    if ( pTrans->pCond )
        Mvc_CoverFree( pTrans->pCond );
    FREE( pTrans );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoTransAdd( Au_State_t * pState, Au_Trans_t * pTrans )
{
    if ( pState->pHead == NULL )
        pState->pHead = pTrans;
    else
        pState->pTail->pNext = pTrans;
    pState->pTail    = pTrans;
    pTrans->pNext = NULL;
    pState->nTrans++;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


