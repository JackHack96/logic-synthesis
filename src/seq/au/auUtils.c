/**CFile****************************************************************

  FileName    [auUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various utilities working with STGs of automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auUtils.c,v 1.1 2004/02/19 03:06:48 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create the name of the product state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Au_UtilsCombineNames( char * pName1, char * pName2 )
{
    char * pName;
    pName = ALLOC( char, strlen(pName1) + strlen(pName2) + 2 );
    pName[0] = 0;
    strcat( pName, pName1 );
//    strcat( pName, "_" );
    strcat( pName, pName2 );
    return pName;
}

/**Function*************************************************************

  Synopsis    [Create the variable map for the given TR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Au_UtilsCreateVmx( Fnc_Manager_t * pMan, int nInputs, int nStates, int fUseNs )
{
    Vm_VarMap_t * pVm;
    int * pValues;
    int i;

    // get the var map
    pValues = ALLOC( int, nInputs + 2 );
    for ( i = 0; i < nInputs; i++ )
        pValues[i] = 2;
    pValues[nInputs  ] = nStates;
    if ( fUseNs )
        pValues[nInputs+1] = nStates;
    pVm = Vm_VarMapLookup( Fnc_ManagerReadManVm(pMan), nInputs + fUseNs, 1, pValues );
    FREE( pValues );

    // get the ext var map
    return Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pMan), pVm, -1, NULL );
}

/**Function*************************************************************

  Synopsis    [Create the permutation array to replace NS vars by CS vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Au_UtilsPermuteNs2Cs( Au_Rel_t * pTR )
{
    int * pBits, * pBitsFirst;
    int * pPermute, i;

    // create the permutation
    pPermute = ALLOC( int, pTR->dd->size );
    for ( i = 0; i < pTR->dd->size; i++ )
        pPermute[i] = i;
    pBits      = Vmx_VarMapReadBits( pTR->pVmx );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pTR->pVmx );
    assert( pBits[pTR->pAut->nInputs] == pBits[pTR->pAut->nInputs+1] );
    for ( i = 0; i < pBits[pTR->pAut->nInputs]; i++ )
    {
        pPermute[pBitsFirst[pTR->pAut->nInputs  ] + i] = pBitsFirst[pTR->pAut->nInputs+1] + i;
        pPermute[pBitsFirst[pTR->pAut->nInputs+1] + i] = pBitsFirst[pTR->pAut->nInputs  ] + i;
    }
    return pPermute;
}

/**Function*************************************************************

  Synopsis    [Sets the variable map to be used for permutations.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_UtilsSetVarMap( Au_Rel_t * pTR )
{
    DdManager * dd = pTR->dd;
    Vmx_VarMap_t * pVmx = pTR->pVmx;
    DdNode ** pbVarsX, ** pbVarsY;
    int * pBits, * pBitsFirst, * pBitsOrder;
    int iVarCS, iVarNS, nBits;
    int i;

    // set the variable mapping for Cudd_bddVarMap()
    pbVarsX = ALLOC( DdNode *, dd->size );
    pbVarsY = ALLOC( DdNode *, dd->size );
    pBits      = Vmx_VarMapReadBits( pVmx );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pVmx );
    pBitsOrder = Vmx_VarMapReadBitsOrder( pVmx );
    // the number of bits in the CS/NS var
    nBits  = pBits[pTR->pAut->nInputs];
    // the numbers of CS/NS vars
    iVarCS = pTR->pAut->nInputs;
    iVarNS = pTR->pAut->nInputs + 1;
    // remap the bits
    for ( i = 0; i < nBits; i++ )
    {
        pbVarsX[i] = dd->vars[ pBitsOrder[pBitsFirst[iVarCS] + i] ];
        pbVarsY[i] = dd->vars[ pBitsOrder[pBitsFirst[iVarNS] + i] ];
    }
    Cudd_SetVarMap( dd, pbVarsX, pbVarsY, nBits );
    FREE( pbVarsX );
    FREE( pbVarsY );
}


/**Function*************************************************************

  Synopsis    [Compute the condition for each transition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_ComputeConditions( DdManager * dd, Au_Auto_t * pAut, Vmx_VarMap_t * pVmx )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans;
    DdNode ** pbCodes;
    int s;

    // get the codes
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx );
    // compute the conditions for each transition
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
        Au_StateForEachTransition( pState, pTrans )
        {
            pTrans->bCond = Fnc_FunctionDeriveMddFromSop( dd, Vmx_VarMapReadVm(pVmx), pTrans->pCond, pbCodes );  
            Cudd_Ref( pTrans->bCond );
        }
    }
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );
}

/**Function*************************************************************

  Synopsis    [Rereferences the functions in states and transitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_DeleteConditions( DdManager * dd, Au_Auto_t * pAut )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans;
    int s;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the state
        pState = pAut->pStates[s];
        // deref the characterization
        if ( pState->bTrans )
            Cudd_RecursiveDeref( dd, pState->bTrans );
        pState->bTrans = NULL;
        // derefe the conditions
        Au_StateForEachTransition( pState, pTrans )
        {
            if ( pTrans->bCond )
                Cudd_RecursiveDeref( dd, pTrans->bCond );
            pTrans->bCond = NULL;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Returns the number of the accepting DC state if any.]

  Description [The DC state is the state with the only one
  transition into itself. The condition for this transition
  should be the autology function.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_FindDcState( Au_Auto_t * pAut )
{
    Au_State_t * pState;
    Au_Trans_t * pTrans;
    int s;
    for ( s = 0; s < pAut->nStates; s++ )
    {
        // get the state
        pState = pAut->pStates[s];
        // if it is non-accepting, skip it
        if ( !pState->fAcc )
            continue;
        // get the first transition
        pTrans = pState->pHead;
        // if there are more transitions, skip
        if ( pTrans->pNext != NULL )
            continue;
        // if the condition is the tautology, return it
        if ( Mvc_CoverIsTautology(pTrans->pCond) )
            return s;
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


