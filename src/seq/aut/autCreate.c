/**CFile****************************************************************

  FileName    [autCreate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autCreate.c,v 1.8 2004/04/08 04:48:24 alanmi Exp $]

***********************************************************************/

#include "autInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Aut_AutoAlloc( Vmx_VarMap_t * pVmx, Aut_Man_t * pMan )
{
    Aut_Auto_t * p;

    p = ALLOC( Aut_Auto_t, 1 );
    memset( p, 0, sizeof(Aut_Auto_t) );

    p->pVmx = pVmx;
    p->nVars = Vm_VarMapReadVarsNum( Vmx_VarMapReadVm( pVmx ) );
    p->pVars = ALLOC( Aut_Var_t *, p->nVars );
    memset( p->pVars, 0, sizeof(Aut_Var_t *) * p->nVars );

    p->tName2State = st_init_table(strcmp, st_strhash);
    p->pInit = ALLOC( Aut_State_t *, 1 );
    p->pInit[0] = NULL;

    if ( pMan == NULL )
        p->pMan = Aut_ManAlloc( Vmx_VarMapReadBitsNum(pVmx) );
    else
        p->pMan = pMan;
    Aut_ManRef( p->pMan );
    Aut_ManResize( p->pMan, Vmx_VarMapReadBitsNum(pVmx) );

    p->pMemState = Extra_MmFixedStart( sizeof(Aut_State_t), 1000, 100 );
    p->pMemTrans = Extra_MmFixedStart( sizeof(Aut_Trans_t), 1000, 100 );
    p->pMemNames = Extra_MmFlexStart( 10000, 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Clones the automaton.]

  Description ["Clone" means duplicate the variable data (names and
  numbers of values) but do not copy states and transitions.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Aut_AutoClone( Aut_Auto_t * pAut, Aut_Man_t * pMan )
{
    Aut_Auto_t * p;
    int i;

    p = Aut_AutoAlloc( pAut->pVmx, pMan );
    for ( i = 0; i < p->nVars; i++ )
        p->pVars[i] = Aut_VarDup( pAut->pVars[i] );
    for ( i = 0; i < p->nVarsL; i++ )
        p->pVarsL[i] = Aut_VarDup( pAut->pVarsL[i] );

    p->nInit = pAut->nInit;
    FREE( p->pInit );
    p->pInit = ALLOC( Aut_State_t *, p->nInit );
    memset( p->pInit, 0, sizeof(Aut_State_t *) * p->nInit );

    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the automaton.]

  Description [If the BDD manager (pMan) is given, uses this manager.
  Otherwise (if pMan is NULL), allocates a new BDD manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Aut_AutoDup( Aut_Auto_t * pAut, Aut_Man_t * pMan )
{
    Aut_Auto_t * pAutNew;
    Aut_Trans_t * pTrans, * pTransNew;
    Aut_State_t * pState, * pStateNew;
    int i;

    // clone the automaton
    pAutNew = Aut_AutoClone( pAut, pMan );
    // copy the name
    pAutNew->pName = util_strsav( pAut->pName );

    // duplicate states
    Aut_AutoForEachState_int( pAut, pState )
    {
        pStateNew = Aut_StateDup( pAutNew, pState );
        Aut_AutoStateAddLast( pStateNew );
        pState->pData = (char *)pStateNew;
    }
    // copy the initial states
    assert( pAutNew->nInit == pAut->nInit );   
    for ( i = 0; i < pAutNew->nInit; i++ )
        pAutNew->pInit[i] = (Aut_State_t *)pAut->pInit[i]->pData;

    // duplicate the transitions
    Aut_AutoForEachState_int( pAut, pState )
    {
        Aut_StateForEachTransitionFrom_int( pState, pTrans )
        {
            pTransNew = Aut_TransDup( pAutNew, pTrans, pAutNew->pMan->dd );
            pTransNew->pFrom = (Aut_State_t *)pTrans->pFrom->pData;
            pTransNew->pTo   = (Aut_State_t *)pTrans->pTo->pData;
            Aut_AutoAddTransition( pTransNew );
        }
    }
    return pAutNew;
}   

/**Function*************************************************************

  Synopsis    [Deallocates the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoFree( Aut_Auto_t * pAut )
{
    DdManager * dd;
    Aut_Trans_t * pTrans;
    Aut_State_t * pState;
    int i;

    if ( pAut == NULL )
        return;
    dd = pAut->pMan->dd;

    FREE( pAut->pName );
    if ( pAut->tName2State )
        st_free_table( pAut->tName2State );

    if ( pAut->pVars )
    {
        for ( i = 0; i < pAut->nVars; i++ )
            Aut_VarFree( pAut->pVars[i] );
        FREE( pAut->pVars );
    }
    if ( pAut->pVarsL )
    {
        for ( i = 0; i < pAut->nVarsL; i++ )
            Aut_VarFree( pAut->pVarsL[i] );
        FREE( pAut->pVarsL );
    }
    FREE( pAut->pInit );

    // dereference the BDDs
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( pState->bCond )
            Cudd_RecursiveDeref( dd, pState->bCond );
        if ( pState->bCondI )
            Cudd_RecursiveDeref( dd, pState->bCondI );
        Aut_StateForEachTransitionFrom_int( pState, pTrans )
        {
            if ( pTrans->bCond )
                Cudd_RecursiveDeref( dd, pTrans->bCond );
        }
    }
//    Extra_StopManager( pAut->dd );
    Aut_ManDeref( pAut->pMan );

    Extra_MmFixedStop( pAut->pMemState, 0 );
    Extra_MmFixedStop( pAut->pMemTrans, 0 );
    Extra_MmFlexStop( pAut->pMemNames, 0 );
    FREE( pAut );
}


/**Function*************************************************************

  Synopsis    [Registers the state name with the internal memory manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aut_AutoRegisterName( Aut_Auto_t * pAut, char * pName )
{
    char * pRegisteredName;
    pRegisteredName = Extra_MmFlexEntryFetch( pAut->pMemNames, strlen(pName) + 1 );
    strcpy( pRegisteredName, pName );
    return pRegisteredName;
}



/**Function*************************************************************

  Synopsis    [Allocates the state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t * Aut_StateAlloc( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    pState = (Aut_State_t *)Extra_MmFixedEntryFetch( pAut->pMemState );
    memset( pState, 0, sizeof(Aut_State_t) );
    pState->pAut = pAut;
    return pState;
}

/**Function*************************************************************

  Synopsis    [Duplicates the state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t * Aut_StateDup( Aut_Auto_t * pAut, Aut_State_t * pState )
{
    Aut_State_t * pStateNew;
    pStateNew = Aut_StateAlloc( pAut );
    pStateNew->pName = Aut_AutoRegisterName( pAut, pState->pName );
    pStateNew->fAcc  = pState->fAcc;
    pStateNew->pAut  = pAut;
    return pStateNew;
}

/**Function*************************************************************

  Synopsis    [Frees the state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_StateFree( Aut_State_t * pState )
{
    Aut_Auto_t * pAut = pState->pAut;
    Aut_Trans_t * pTrans, * pTrans2;
    // recycle memory for the state name - no need to
    // deref the conditions
    if ( pState->bCond )
        Cudd_RecursiveDeref( pAut->pMan->dd, pState->bCond );
    if ( pState->bCondI )
        Cudd_RecursiveDeref( pAut->pMan->dd, pState->bCondI );
    // free the outgoing transitions
    Aut_StateForEachTransitionFromSafe_int( pState, pTrans, pTrans2 )
        Aut_TransFree( pAut, pTrans );
    // free the incoming transitions
    Aut_StateForEachTransitionToSafe_int( pState, pTrans, pTrans2 )
        Aut_TransFree( pAut, pTrans );
    // recycle the memory
    Extra_MmFixedEntryRecycle( pAut->pMemState, (char *)pState );
}


/**Function*************************************************************

  Synopsis    [Allocates the transition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Trans_t * Aut_TransAlloc( Aut_Auto_t * pAut )
{
    Aut_Trans_t * pTrans;
    pTrans = (Aut_Trans_t *)Extra_MmFixedEntryFetch( pAut->pMemTrans );
    memset( pTrans, 0, sizeof(Aut_Trans_t) );
    return pTrans;
}

/**Function*************************************************************

  Synopsis    [Duplicates the transition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Trans_t * Aut_TransDup( Aut_Auto_t * pAut, Aut_Trans_t * pTrans, DdManager * ddOld )
{
    Aut_Trans_t * pTransNew;
    pTransNew = Aut_TransAlloc( pAut );
    if ( pTrans->bCond )
    {
        if ( ddOld )
            pTransNew->bCond = Cudd_bddTransfer( ddOld, pAut->pMan->dd, pTrans->bCond );
        else
            pTransNew->bCond = pTrans->bCond;
        Cudd_Ref( pTransNew->bCond );
    }
    return pTransNew;
}

/**Function*************************************************************

  Synopsis    [Frees the transition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_TransFree( Aut_Auto_t * pAut, Aut_Trans_t * pTrans )
{
    if ( pTrans->bCond )
        Cudd_RecursiveDeref( pAut->pMan->dd, pTrans->bCond );
    // remove this transition
    Aut_AutoDeleteTransition( pTrans );
    // recycle memory
    Extra_MmFixedEntryRecycle( pAut->pMemTrans, (char *)pTrans );
}


/**Function*************************************************************

  Synopsis    [Adds the transition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoAddTransition( Aut_Trans_t * pTrans )
{
    Aut_StateAddLastTransFrom( pTrans->pFrom, pTrans );
    Aut_StateAddLastTransTo  ( pTrans->pTo,   pTrans );
}

/**Function*************************************************************

  Synopsis    [Deletes the transition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoDeleteTransition( Aut_Trans_t * pTrans )
{
    Aut_StateDeleteTransFrom( pTrans );
    Aut_StateDeleteTransTo  ( pTrans );
}


/**Function*************************************************************

  Synopsis    [Finds the transition among the two states.]

  Description [Returns NULL, if the transition does not exist.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Trans_t * Aut_AutoFindTransition( Aut_State_t * pStateFrom, Aut_State_t * pStateTo )
{
    Aut_Trans_t * pTrans;
    Aut_StateForEachTransitionFrom_int( pStateFrom, pTrans )
        if ( pTrans->pTo == pStateTo )
            return pTrans;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Finds the state with the given name.]

  Description [Returns NULL if the state does not exist.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t *   Aut_AutoFindState( Aut_Auto_t * pAut, char * pStateName )
{
    Aut_State_t * pState;
    if ( st_lookup( pAut->tName2State, pStateName, (char **)&pState ) )
        return pState;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the conditions of the two transitions overlap.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Aut_AutoTransOverlap( Aut_Trans_t * pTrans1, Aut_Trans_t * pTrans2 )
{
    return !Cudd_bddLeq( pTrans1->pTo->pAut->pMan->dd, pTrans1->bCond, Cudd_Not(pTrans2->bCond) );
}
 
/**Function*************************************************************

  Synopsis    [Allocates a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Var_t * Aut_VarAlloc()
{
    Aut_Var_t * pVar;
    pVar = ALLOC( Aut_Var_t, 1 );
    memset( pVar, 0, sizeof(Aut_Var_t) );
    return pVar;
}

/**Function*************************************************************

  Synopsis    [Duplicates a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Var_t * Aut_VarDup( Aut_Var_t * pVar )
{
    Aut_Var_t * pVarNew;
    int i;
    pVarNew = Aut_VarAlloc();
    pVarNew->pName = util_strsav( pVar->pName );
    pVarNew->nValues = pVar->nValues;
    if ( pVar->pValueNames )
    {
        pVarNew->pValueNames = ALLOC( char *, pVar->nValues );
        for ( i = 0; i < pVar->nValues; i++ )
            pVarNew->pValueNames[i] = util_strsav( pVar->pValueNames[i] );
    }
    return pVarNew;
}

/**Function*************************************************************

  Synopsis    [Frees a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_VarFree( Aut_Var_t * pVar )
{
    int i;
    if ( pVar->pValueNames )
    {
        for ( i = 0; i < pVar->nValues; i++ )
            FREE( pVar->pValueNames[i] );
        FREE( pVar->pValueNames );
    }
    FREE( pVar->pName );
    FREE( pVar );
}

/**Function*************************************************************

  Synopsis    [Allocates a reusable BDD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Man_t * Aut_ManAlloc( int nVars )
{
    Aut_Man_t * pMan;
    pMan = ALLOC( Aut_Man_t, 1 );
    memset( pMan, 0, sizeof(Aut_Man_t) );
    pMan->dd = Cudd_Init( nVars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Creates the reusable manager given an ordinary BDD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Man_t * Aut_ManCreate( DdManager * dd )
{
    Aut_Man_t * pMan;
    pMan = ALLOC( Aut_Man_t, 1 );
    memset( pMan, 0, sizeof(Aut_Man_t) );
    pMan->dd = dd;
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Increments the reference count of the reusable manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_ManRef( Aut_Man_t * pMan )
{
    pMan->nRefs++;
}

/**Function*************************************************************

  Synopsis    [Decrements the reference count of the reusable manager. ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_ManDeref( Aut_Man_t * pMan )
{
    if ( --pMan->nRefs == 0 )
    {
        Extra_StopManager( pMan->dd );
        FREE( pMan );
    }
}

/**Function*************************************************************

  Synopsis    [Resizes the underlying BDD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_ManResize( Aut_Man_t * pMan, int nSizeNew )
{
    int i;
    if ( pMan->dd->size >= nSizeNew )
        return;
    for ( i = pMan->dd->size; i < nSizeNew; i++ )
        Cudd_bddNewVar(pMan->dd);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


