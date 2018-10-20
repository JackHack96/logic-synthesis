/**CFile****************************************************************

  FileName    [auCompl.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Completion and complementation procedures for automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auCompl.c,v 1.1 2004/02/19 03:06:44 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Mvc_Cover_t * Au_AutoStateComplementCondsSop( Au_Auto_t * pAut, Au_State_t * pState, Mvc_Data_t * pData );
static Mvc_Cover_t * Au_AutoStateComplementConds( Au_Auto_t * pAut, Au_State_t * pState, 
    DdNode * bCube, Vm_VarMap_t * pVm, DdManager * dd, DdNode ** pbCodes, Mvr_Relation_t * pMvr );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Completes the automaton or the underlying FSM).]

  Description [Returns the number of incomplete states; 0 if the
  automaton is complete.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoComplete( Au_Auto_t * pAut, int nInputs, int fAccepting, int fCheckOnly )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans;
    Mvc_Cover_t * pCompl;
    char pDcStateName[100];
    int nStatesIncomp, nIter, s;

    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    Mvc_Data_t * pData;
    Mvc_Cover_t * pCover;
    DdNode ** pbCodes;
    DdManager * dd;
    Mvr_Relation_t * pMvrTemp;
    DdNode * bCube;

    if ( pAut->nStates == 0 )
    {
        printf( "Trying to complete the automaton with no states.\n" );
        return 0;
    }

    // create the MVC data to compute the intersection of conditions
    pCover = pAut->pStates[0]->pHead->pCond;
    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 0 );
    pData  = Mvc_CoverDataAlloc( pVm, pCover );

    // prepare to the BDD-based computation of the complemented conditions
    pVmx     = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, -1, NULL );
    dd       = Fnc_ManagerReadDdLoc( pAut->pMan );
    pbCodes  = Vmx_VarMapEncodeMap( dd, pVmx );
    pMvrTemp = Mvr_RelationCreate( Fnc_ManagerReadManMvr(pAut->pMan), pVmx, b1 );
    bCube    = Vmx_VarMapCharCubeRange( dd, pVmx, nInputs, pAut->nInputs - nInputs );  Cudd_Ref( bCube );

    
    // go through the transitions and compute the complements
    nStatesIncomp = 0;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
//        pCompl = Au_AutoStateComplementCondsSop( pAut, pState, pData );
        pCompl = Au_AutoStateComplementConds( pAut, pState, bCube, pVm, dd, pbCodes, pMvrTemp );
        if ( Mvc_CoverReadCubeNum(pCompl) )
            nStatesIncomp++;
        // store the complemented cover
        pState->State1 = (int)pCompl;
    }

    // free useless data stuctures
    Mvc_CoverDataFree( pData, pCover );
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );
    Mvr_RelationFree( pMvrTemp );
    Cudd_RecursiveDeref( dd, bCube );



    // complete the automaton if necessary
    if ( nStatesIncomp == 0 )
    {
        // remove the covers
        for ( s = 0; s < pAut->nStates; s++ )
        {
            pState = pAut->pStates[s];
            if ( pState->State1 )
            {
                pCompl = (Mvc_Cover_t *)pState->State1;
                Mvc_CoverFree( pCompl );
                pState->State1 = 0;
            }
        }
        return nStatesIncomp;  // the automaton is complete
    }

    if ( fCheckOnly )
    {
        // remove the covers
        for ( s = 0; s < pAut->nStates; s++ )
        {
            pState = pAut->pStates[s];
            if ( pState->State1 )
            {
                pCompl = (Mvc_Cover_t *)pState->State1;
                Mvc_CoverFree( pCompl );
                pState->State1 = 0;
            }
        }
        return nStatesIncomp;  // the automaton is incomplete
    }

    // start with "DC" and create the name that does not exist among the state names
    sprintf( pDcStateName, "%s", "DC" );
    for ( nIter = 1; st_is_member( pAut->tStateNames, pDcStateName ); nIter++ )
        sprintf( pDcStateName, "%s%d", "DC", nIter );

    // add the dummy state to the automaton
    pAut->nStates++;
    if ( pAut->nStatesAlloc < pAut->nStates )
    {
        pAut->pStates = REALLOC( Au_State_t *, pAut->pStates, pAut->nStates );
        pAut->nStatesAlloc = pAut->nStates;
    }
    pAut->pStates[pAut->nStates - 1] = Au_AutoStateAlloc();
    pAut->pStates[pAut->nStates - 1]->pName = util_strsav( pDcStateName );
    pAut->pStates[pAut->nStates - 1]->fAcc = fAccepting;
    if ( st_insert( pAut->tStateNames, pDcStateName, NULL ) )
    {
        assert( 0 );
    }

    // go through the states and add transitions to the dummy state if necessary
    for ( s = 0; s < pAut->nStates - 1; s++ )
    {
        pState = pAut->pStates[s];
        pCompl = (Mvc_Cover_t *)pState->State1;
        if ( Mvc_CoverReadCubeNum(pCompl) == 0 )
        {
            Mvc_CoverFree( pCompl );
            pState->State1 = 0;
            continue;
        }
        // add the transition
        pTrans = Au_AutoTransAlloc();
        pTrans->pCond = pCompl;
        pTrans->StateCur  = s;
        pTrans->StateNext = pAut->nStates - 1;
        // add the transition
        Au_AutoTransAdd( pState, pTrans );
        pState->State1 = 0;
    }

    // add the transition from the dummy state into itself
    pTrans = Au_AutoTransAlloc();
    pTrans->pCond = Mvc_CoverCreateTautology(pCover);
    pTrans->StateCur  = pAut->nStates - 1;
    pTrans->StateNext = pAut->nStates - 1;
    // add the transition
    pState = pAut->pStates[pAut->nStates - 1];
    Au_AutoTransAdd( pState, pTrans );

    return nStatesIncomp;
}


/**Function*************************************************************

  Synopsis    [Computes the complements of conditions for the given state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Au_AutoStateComplementCondsSop( Au_Auto_t * pAut, Au_State_t * pState, Mvc_Data_t * pData )
{
    Mvc_Cover_t * pCompl, * pSum;
    pSum   = Au_AutoStateSumConds( pAut, pState );
    Mvc_CoverContain( pSum );
    pCompl = Mvc_CoverComplement( pData, pSum );
    Mvc_CoverFree( pSum );
    return pCompl;
}

/**Function*************************************************************

  Synopsis    [Computes the complements of conditions for the given state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Au_AutoStateComplementConds( Au_Auto_t * pAut, Au_State_t * pState, 
    DdNode * bCube, Vm_VarMap_t * pVm, DdManager * dd, DdNode ** pbCodes, Mvr_Relation_t * pMvr )
{
    Au_Trans_t * pTrans;
    Mvc_Cover_t * pCompl;
    DdNode * bCond, * bTemp, * bSum;
    // go through the condition and add the functions
    bSum = b0;    Cudd_Ref( bSum );
    Au_StateForEachTransition( pState, pTrans )
    {
        // derive the condition
        bCond = Fnc_FunctionDeriveMddFromSop( dd, pVm, pTrans->pCond, pbCodes );  Cudd_Ref( bCond );
        bSum  = Cudd_bddOr( dd, bTemp = bSum, bCond );  Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCond );
    }
    // quontify the additional variables
    bSum  = Cudd_bddExistAbstract( dd, bTemp = bSum, bCube );  Cudd_Ref( bSum );
    Cudd_RecursiveDeref( dd, bTemp );
    // derive the complement of the cover
    pCompl = Fnc_FunctionDeriveSopFromMdd( Fnc_ManagerReadManMvc(pAut->pMan), 
        pMvr, Cudd_Not(bSum), Cudd_Not(bSum), pAut->nInputs );
    Cudd_RecursiveDeref( dd, bSum );
    return pCompl;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the state is complete or Moore.]

  Description [If the flag fUseMark is set to 1, checks completion only 
  w.r.t. the states that are marked. If the flag fMoore is set to 1,
  checks the Mooreness rather than the completion.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Au_AutoStateIsCcmplete( Au_Auto_t * pAut, Au_State_t * pState, DdNode * bCube, 
    Vm_VarMap_t * pVm, DdManager * dd, DdNode ** pbCodes, bool fUseMark, int fMoore )
{
    Au_Trans_t * pTrans;
    DdNode * bCond, * bTemp, * bSum;
    bool RetValue;
    // go through the condition and add the functions
    bSum = b0;    Cudd_Ref( bSum );
    Au_StateForEachTransition( pState, pTrans )
    {
        if ( fUseMark && !pAut->pStates[pTrans->StateNext]->fMark )
            continue;

        // derive the condition
        bCond = Fnc_FunctionDeriveMddFromSop( dd, pVm, pTrans->pCond, pbCodes );  Cudd_Ref( bCond );
        bSum  = Cudd_bddOr( dd, bTemp = bSum, bCond );  Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCond );
    }
    // quantify the additional variables
    if ( fMoore )
    {
        // if we are trying to make the machine Moore. 
        // universally quantify the input variables
        // and make sure the result is not empty
        bSum  = Cudd_bddUnivAbstract( dd, bTemp = bSum, bCube );    Cudd_Ref( bSum ); 
        Cudd_RecursiveDeref( dd, bTemp );
        // get the return value
        RetValue = (int)(bSum != b0);
        Cudd_RecursiveDeref( dd, bSum );
    }
    else
    {
        // if we are trying to achieve completeness
        // existentially quantify the output variables
        // and make sure the result is constant 1
        bSum  = Cudd_bddExistAbstract( dd, bTemp = bSum, bCube );   Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
        // get the return value
        RetValue = (int)(bSum == b1);
        Cudd_RecursiveDeref( dd, bSum );
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoComplement( Au_Auto_t * pAut )
{
    int s;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        assert( pAut->pStates[s]->fAcc == 0 || pAut->pStates[s]->fAcc == 1 );
        pAut->pStates[s]->fAcc ^= 1;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


