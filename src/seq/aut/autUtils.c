/**CFile****************************************************************

  FileName    [autUtils.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autUtils.c,v 1.9 2004/04/08 04:48:25 alanmi Exp $]

***********************************************************************/

#include "autInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define AUTI_UNREACHABLE  10000000

static void Aut_AutoReachability_rec( Aut_Auto_t * pAut, Aut_State_t * pState, int Level );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Count transitions of the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aut_AutoCountTransitions( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    int Counter = 0;
    Aut_AutoForEachState_int( pAut, pState )
        Counter += pState->nTransFrom;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Computes the sum of transition conditions for each state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoComputeSumCond( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( pState->bCond )
            Cudd_RecursiveDeref( pAut->pMan->dd, pState->bCond );
        pState->bCond = Aut_StateComputeSumCond( pState );
        Cudd_Ref( pState->bCond );
    }
}

/**Function*************************************************************

  Synopsis    [Computes the sum of transition conditions for one state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Aut_StateComputeSumCond( Aut_State_t * pState )
{
    DdManager * dd = pState->pAut->pMan->dd;
    DdNode * bSum, * bTemp;
    Aut_Trans_t * pTrans;

    bSum = b0;      Cudd_Ref( bSum );
    Aut_StateForEachTransitionFrom_int( pState, pTrans )
    {
        bSum  = Cudd_bddOr( dd, bTemp = bSum, pTrans->bCond );    Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bSum );
    return bSum;
}

/**Function*************************************************************

  Synopsis    [Computes the sum of transition I-conditions for each state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoComputeSumCondI( Aut_Auto_t * pAut, int nInputs )
{
    Aut_State_t * pState;
    DdNode * bCube;

    // get the variables that are not used in the I conditions
    bCube = Aut_AutoGetCubeOutput( pAut, nInputs );  Cudd_Ref( bCube );
    // quantify these vars from sum of conditions for each state
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( pState->bCondI )
            Cudd_RecursiveDeref( pAut->pMan->dd, pState->bCondI );
        pState->bCondI = Cudd_bddExistAbstract( pAut->pMan->dd, pState->bCond, bCube );  
        Cudd_Ref( pState->bCondI );
    }
    Cudd_RecursiveDeref( pAut->pMan->dd, bCube );
}

/**Function*************************************************************

  Synopsis    [Derefs the sums of transition conditions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoDerefSumCond( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
        if ( pState->bCond )
        {
            Cudd_RecursiveDeref( pAut->pMan->dd, pState->bCond );
            pState->bCond = NULL;
        }
}

/**Function*************************************************************

  Synopsis    [Derefs the sums of transition I-conditions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoDerefSumCondI( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
        if ( pState->bCondI )
        {
            Cudd_RecursiveDeref( pAut->pMan->dd, pState->bCondI );
            pState->bCondI = NULL;
        }
}

/**Function*************************************************************

  Synopsis    [Computes the BDD-cube of all input variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Aut_AutoGetCubeInput( Aut_Auto_t * pAut, int nInputs )
{
    return Vmx_VarMapCharCubeRange( pAut->pMan->dd, pAut->pVmx, 0, nInputs );
}

/**Function*************************************************************

  Synopsis    [Computes the BDD-cube of all output variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Aut_AutoGetCubeOutput( Aut_Auto_t * pAut, int nInputs )
{
    return Vmx_VarMapCharCubeRange( pAut->pMan->dd, pAut->pVmx, nInputs, pAut->nVars - nInputs );
}

/**Function*************************************************************

  Synopsis    [Creates the DC state.]

  Description [The DC state has a name "DC" followed by an integer number.
  It has only one outgoing transition, which is the tautology self-loop.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t * Aut_AutoCreateDcState( Aut_Auto_t * pAut )
{
    DdManager * dd = Aut_AutoReadDd(pAut);
    char pDcStateName[100];
    Aut_State_t * pState;
    Aut_Trans_t * pTrans;
    int nIter;
    // find the DC state name
    sprintf( pDcStateName, "%s", "DC" );
    for ( nIter = 1; st_is_member( pAut->tName2State, pDcStateName ); nIter++ )
        sprintf( pDcStateName, "%s%d", "DC", nIter );
    // create the state
    pState = Aut_StateAlloc( pAut );
    // add the name
    pState->pName = Aut_AutoRegisterName( pAut, pDcStateName );

    // add the transition into the state itself
    pTrans = Aut_TransAlloc( pAut );
    pTrans->pFrom = pState;
    pTrans->pTo = pState;
    pTrans->bCond = b1;   Cudd_Ref( pTrans->bCond );
    Aut_AutoAddTransition( pTrans );
    return pState;
}

/**Function*************************************************************

  Synopsis    [Cleans the marks in all states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoCleanMark( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
        pState->fMark = 0;
}

/**Function*************************************************************

  Synopsis    [Sets the mark in all states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoSetMark( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
        pState->fMark = 1;
}

/**Function*************************************************************

  Synopsis    [Sets the mark to be equal to the accepting attribute.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoSetMarkAccepting( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
        pState->fMark = pState->fAcc;
}

/**Function*************************************************************

  Synopsis    [Sets the mark to be equal to the unaccepting attribute.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoSetMarkNonAccepting( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
        pState->fMark = !pState->fAcc;
}

/**Function*************************************************************

  Synopsis    [Cleans the second mark.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoCleanMark2( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
        pState->fMark2 = 0;
}

/**Function*************************************************************

  Synopsis    [Sets the second mark.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoSetMark2( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    Aut_AutoForEachState_int( pAut, pState )
        pState->fMark2 = 1;
}

/**Function*************************************************************

  Synopsis    [Assigns numbers to states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoAssignNumbers( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    int Counter = 0;
    Aut_AutoForEachState_int( pAut, pState )
        pState->uData = Counter++;
}

/**Function*************************************************************

  Synopsis    [Collects states into one array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_State_t ** Aut_AutoCollectStates( Aut_Auto_t * pAut )
{
    Aut_State_t ** ppStates;
    Aut_State_t * pState;
    int Counter = 0;
    ppStates = ALLOC( Aut_State_t *, pAut->nStates );
    Aut_AutoForEachState_int( pAut, pState )
        ppStates[Counter++] = pState;
    assert( Counter == pAut->nStates );
    return ppStates;
}

/**Function*************************************************************

  Synopsis    [Counts the number of accepting states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aut_AutoCountAccepting( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    int Counter = 0;
    Aut_AutoForEachState_int( pAut, pState )
        Counter += pState->fAcc;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of non-accepting states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aut_AutoCountNonAccepting( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    int Counter = 0;
    Aut_AutoForEachState_int( pAut, pState )
        Counter += !pState->fAcc;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Checks whether the automaton is binary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Aut_AutoCheckIsBinary( Aut_Auto_t * pAut )
{
    return Vm_VarMapIsBinary( Vmx_VarMapReadVm(pAut->pVmx) );
}


/**Function*************************************************************

  Synopsis    [Removes the automaton states that are marked.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoRemoveMarkedStates( Aut_Auto_t * pAut )
{
    Aut_State_t * pState, * pState2;
    // remove all the incomplete states
    Aut_AutoForEachStateSafe_int( pAut, pState, pState2 )
        if ( pState->fMark )
        {
            if ( pAut->pInit[0] == pState )
                pAut->pInit[0] = NULL;
            Aut_AutoStateDelete( pState );
            Aut_StateFree( pState );
        }
}


/**Function*************************************************************

  Synopsis    [Performs reachability analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aut_AutoReachability( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    unsigned nLevels;
    // set all the state as unreachable
    Aut_AutoForEachState_int( pAut, pState )
        pState->uData = AUTI_UNREACHABLE;
    // start from the initial state 
    Aut_AutoReachability_rec( pAut, pAut->pInit[0], 0 );
    // find the largest level
    nLevels = 0;
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( pState->uData == AUTI_UNREACHABLE )
            continue;
        if ( nLevels < pState->uData )
            nLevels = pState->uData;
    }
    return nLevels + 1;
}

/**Function*************************************************************

  Synopsis    [Performs reachability analysis recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoReachability_rec( Aut_Auto_t * pAut, Aut_State_t * pState, int Level )
{
    Aut_Trans_t * pTrans;
    if ( pState->uData <= (unsigned)Level )
        return;
    // set the level
    pState->uData = Level;
    // explore the rest of the graph
    Aut_StateForEachTransitionFrom( pState, pTrans )
        Aut_AutoReachability_rec( pAut, pTrans->pTo, Level + 1 );
}

/**Function*************************************************************

  Synopsis    [Renames the state of the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStateRename( Aut_Auto_t * pAut, char * pOldName, char * pNewName )
{
    Aut_State_t * pState;
    // get the state with this name
    pState = Aut_AutoFindState( pAut, pOldName );
    assert( pState );
    // delete the state name
    if ( !st_delete( pAut->tName2State, &pState->pName, (char **)&pState ) )
    {
        assert( 0 );
    }
//    FREE( pState->pName ); -- stored in the fixed manager
    // create the state name
    pState->pName = Aut_AutoRegisterName( pAut, pNewName );
    // add to the table
    if ( pState->pName )
        if ( st_insert( pAut->tName2State, pState->pName, (char *)pState ) )
        {
            assert( 0 );
        }
}

/**Function*************************************************************

  Synopsis    [Adds the state to the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStateAddToTable( Aut_State_t * pState )
{
    // add to the table
    if ( pState->pName )
        if ( st_insert( pState->pAut->tName2State, pState->pName, (char *)pState ) )
        {
            assert( 0 );
        }
}

/**Function*************************************************************

  Synopsis    [Encodes the MV automaton into binary for writing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoBinaryEncode( Aut_Auto_t * pAut )
{
    Vm_VarMap_t * pVm, * pVmIO;
    Vmx_VarMap_t * pVmxIO;
    Aut_Var_t ** ppVarsNew, * pVar;
    char Buffer[100];
    int * pBitsFirst, * pBitsOrder, * pBitsOrderR, * pBits;
    int nBits, iBit, i, k;

    assert( !Aut_AutoCheckIsBinary( pAut ) );

    // create the binary variable map
    // reverse the ordering of the binary bits
    pBits       = Vmx_VarMapReadBits( pAut->pVmx );
    pBitsFirst  = Vmx_VarMapReadBitsFirst( pAut->pVmx ); 
    pBitsOrder  = Vmx_VarMapReadBitsOrder( pAut->pVmx );
    pBitsOrderR = ALLOC( int, Vmx_VarMapReadBitsNum(pAut->pVmx) );
    nBits = 0;
    for ( i = 0; i < pAut->nVars; i++ )
        for ( k = 0; k < pBits[i]; k++ )
            pBitsOrderR[nBits++] =  pBitsOrder[pBitsFirst[i] + pBits[i] - 1 - k];
    pVm = Vmx_VarMapReadVm( pAut->pVmx );
    pVmIO = Vm_VarMapCreateBinary( Vm_VarMapReadMan(pVm), nBits, 0 );
    pVmxIO = Vmx_VarMapLookup( Vmx_VarMapReadMan(pAut->pVmx), pVmIO, nBits, pBitsOrderR );
    FREE( pBitsOrderR );

    // create the new variables
    iBit = 0;
    ppVarsNew = ALLOC( Aut_Var_t *, nBits );
    for ( i = 0; i < pAut->nVars; i++ )
    {
        if ( pBits[i] == 1 )
        {
            ppVarsNew[iBit++] = Aut_VarDup( pAut->pVars[i] );
            continue;
        }
        for ( k = 0; k < pBits[i]; k++ )
        {
            sprintf( Buffer, "%s_%d", pAut->pVars[i]->pName, pBits[i] - 1 - k );
            pVar = Aut_VarAlloc();
            pVar->nValues = 2;
            pVar->pName   = util_strsav( Buffer );
            ppVarsNew[iBit++] = pVar;
        }
        // free this var
        Aut_VarFree( pAut->pVars[i] );
    }
    assert( iBit == nBits );
    FREE( pAut->pVars );
    // set the new parameters
    pAut->nVars = nBits;
    pAut->pVars = ppVarsNew;
    pAut->pVmx  = pVmxIO;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



