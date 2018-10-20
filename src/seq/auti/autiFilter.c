/**CFile****************************************************************

  FileName    [autiFilter.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Peforms filtering operations (such as progressive, moore, etc)]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autiFilter.c,v 1.2 2004/05/12 04:30:16 alanmi Exp $]

***********************************************************************/

#include "autiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Auti_AutoReachable( Aut_Auto_t * pAut );
static void Auti_AutoReachable_rec( Aut_State_t * pState, int fMarkAccepting );
static bool Auti_AutoFilter( Aut_Auto_t * pAut, int nInputs, int fMoore, int fVerbose );
static bool Auti_StateCheckIsComplete( Aut_State_t * pState, DdNode * bCube );
static bool Auti_StateCheckIsMoore( Aut_State_t * pState, DdNode * bCube );
static bool Auti_StateCheckIsMooreFeasible( Aut_State_t * pState, DdNode * bCube );
static void Auti_AutoMakeMoore( Aut_Auto_t * pAut, int nInputs );
static void Auti_StateMakeMoore( Aut_State_t * pState, DdNode * bCube );
static int  Auti_AutoDetectIncomplete( Aut_Auto_t * pAut, DdNode * bCube, bool fMoore, bool fVerbose );
static void Auti_AutoRemoveIncomplete( Aut_Auto_t * pAut, bool fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Makes the automaton prefix closed.]

  Description [Produces an automaton, that is different from the
  current one in the following respects. The non-accepting states 
  are removed together with all the transitions. Only the reachable
  states of the resulting machine are collected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoPrefixClose( Aut_Auto_t * pAut )
{
    int i;
    // set the mark
    Aut_AutoSetMark( pAut );
    // unmark reachable accepting states
    for ( i = 0; i < pAut->nInit; i++ )
        Auti_AutoReachable_rec( pAut->pInit[i], 1 );
    // remove marked states
    Aut_AutoRemoveMarkedStates( pAut );
}

/**Function*************************************************************

  Synopsis    [Perform reachability starting from the given state.]

  Description [Only reaches through the reachable states. If a state is
  unmarked, it means that we already visited it before.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoReachable_rec( Aut_State_t * pState, int fMarkAccepting )
{
    Aut_Trans_t * pTrans;
    if ( pState->fMark == 0 || (fMarkAccepting && pState->fAcc == 0) )
        return;
    pState->fMark = 0;
    Aut_StateForEachTransitionFrom( pState, pTrans )
        Auti_AutoReachable_rec( pTrans->pTo, fMarkAccepting );
}

/**Function*************************************************************

  Synopsis    [Leaves only reachable states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoReachable( Aut_Auto_t * pAut )
{
    int i;
    // set the mark
    Aut_AutoSetMark( pAut );
    // unmark reachable accepting states
    for ( i = 0; i < pAut->nInit; i++ )
        Auti_AutoReachable_rec( pAut->pInit[i], 0 );
    // remove marked states
    Aut_AutoRemoveMarkedStates( pAut );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoProgressive( Aut_Auto_t * pAut, int nInputs, int fVerbose, int fPrefix )
{
    // remove unreachable states and non-accepting states
    if ( fPrefix )
    {
        Auti_AutoPrefixClose( pAut );
        if ( pAut->nStates == 0 )
            return;
    }
    else
    {
        Auti_AutoReachable( pAut );
        if ( pAut->nStates == 0 )
            return;
    }
    // find the incomplete states
    if ( Auti_AutoFilter( pAut, nInputs, 0, fVerbose ) ) 
    { // some states are left
        // remove the unreachable states
        if ( fPrefix )
            Auti_AutoPrefixClose( pAut );
        else
            Auti_AutoReachable( pAut );
    }
    else
    { // remove all states
        Aut_AutoSetMark( pAut );
        Aut_AutoRemoveMarkedStates( pAut );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoMoore( Aut_Auto_t * pAut, int nInputs, int fVerbose )
{
    Auti_AutoPrefixClose( pAut );
    if ( pAut->nStates == 0 )
        return;
    // find the incomplete states
    if ( Auti_AutoFilter( pAut, nInputs, 1, fVerbose ) )
    { // some states are left
        Auti_AutoMakeMoore( pAut, nInputs );
        // remove the unreachable states
        Auti_AutoPrefixClose( pAut );
    }
    else
    { // remove all states
        Aut_AutoSetMark( pAut );
        Aut_AutoRemoveMarkedStates( pAut );
    }
}


/**Function*************************************************************

  Synopsis    [Detects all the states that do not satisfy the condition.]

  Description [Returns 1 if some states are left. Returns 0 if the resulting
  automaton is empty.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Auti_AutoFilter( Aut_Auto_t * pAut, int nInputs, int fMoore, int fVerbose )
{
    DdManager * dd = pAut->pMan->dd;
    DdNode * bCube;

    // remove the conditions if present
    Aut_AutoDerefSumCond( pAut );
    Aut_AutoDerefSumCondI( pAut );

    // get the quantification cube
    if ( fMoore )
        bCube = Aut_AutoGetCubeInput( pAut, nInputs );   // inputs
    else
        bCube = Aut_AutoGetCubeOutput( pAut, nInputs );  // outputs
    Cudd_Ref( bCube );

    // the first mark is used to mark the states that should be checked
    // the second mark is used to mark the incomplete states
    // mark all the states for checking
    Aut_AutoSetMark( pAut );
    Aut_AutoCleanMark2( pAut );
    
    // detect the incomplete states
    // check only states marked with Mark1, clear Mark1 as we go
    while ( Auti_AutoDetectIncomplete( pAut, bCube, fMoore, fVerbose ) )
    {   // there are such states - they are marked with Mark2
        // go through the states marked with Mark2 and delete them
        // before deleting, mark those that point to them with Mark1
        // these should be checked again later
        Auti_AutoRemoveIncomplete( pAut, fVerbose );
    }
    Cudd_RecursiveDeref( dd, bCube );
    return Aut_AutoReadStateNum(pAut) > 0;
}

/**Function*************************************************************

  Synopsis    [Detect the incomplete states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_AutoDetectIncomplete( Aut_Auto_t * pAut, DdNode * bCube, bool fMoore, bool fVerbose ) 
{
    Aut_State_t * pState;
    int Counter = 0;
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( pState->fMark == 0 )
            continue;
        pState->fMark = 0;
        if (  fMoore && Auti_StateCheckIsMooreFeasible( pState, bCube ) || 
             !fMoore && Auti_StateCheckIsComplete( pState, bCube ) )
            continue;
        pState->fMark2 = 1;
        Counter++;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Detect the incomplete states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoRemoveIncomplete( Aut_Auto_t * pAut, bool fVerbose )
{
    Aut_State_t * pState, * pState2;
    Aut_Trans_t * pTrans;
    Aut_AutoForEachStateSafe_int( pAut, pState, pState2 )
    {
        if ( pState->fMark2 == 0 )
            continue;
        // mark the incoming states
        Aut_StateForEachTransitionTo_int( pState, pTrans )
            pTrans->pFrom->fMark = 1;
        // delete this state
        Aut_AutoStateDelete( pState );
        Aut_StateFree( pState );
    }
}

/**Function*************************************************************

  Synopsis    [Check the condition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Auti_StateCheckIsComplete( Aut_State_t * pState, DdNode * bCube )
{
    DdManager * dd = pState->pAut->pMan->dd;
    DdNode * bCond, * bCondI;
    bool RetValue;

    // derive the sum of conditions
    bCond = Aut_StateComputeSumCond( pState );           Cudd_Ref( bCond );
    // get the I-condition
    bCondI = Cudd_bddExistAbstract( dd, bCond, bCube );  Cudd_Ref( bCondI );
    RetValue = (int)(bCondI == b1);
    Cudd_RecursiveDeref( dd, bCond );
    Cudd_RecursiveDeref( dd, bCondI );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Check the condition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Auti_StateCheckIsMoore( Aut_State_t * pState, DdNode * bCube )
{
    DdManager * dd = pState->pAut->pMan->dd;
    DdNode * bCond, * bCondI;
    bool RetValue;

    // derive the sum of conditions
    bCond = Aut_StateComputeSumCond( pState );          Cudd_Ref( bCond );
    // get the I-condition
    bCondI = Cudd_bddUnivAbstract( dd, bCond, bCube );  Cudd_Ref( bCondI );
    RetValue = (int)(bCondI == bCond);
    Cudd_RecursiveDeref( dd, bCond );
    Cudd_RecursiveDeref( dd, bCondI );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Check the condition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Auti_StateCheckIsMooreFeasible( Aut_State_t * pState, DdNode * bCube )
{
    DdManager * dd = pState->pAut->pMan->dd;
    DdNode * bCond, * bCondI;
    bool RetValue;

    // derive the sum of conditions
    bCond = Aut_StateComputeSumCond( pState );          Cudd_Ref( bCond );
    // get the I-condition
    bCondI = Cudd_bddUnivAbstract( dd, bCond, bCube );  Cudd_Ref( bCondI );
    RetValue = (int)(bCondI != b0);
    Cudd_RecursiveDeref( dd, bCond );
    Cudd_RecursiveDeref( dd, bCondI );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Reduces the transition to be Moore.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_AutoMakeMoore( Aut_Auto_t * pAut, int nInputs )
{
    Aut_State_t * pState;
    DdNode * bCube;
    bCube = Aut_AutoGetCubeInput( pAut, nInputs );  Cudd_Ref( bCube );
    Aut_AutoForEachState_int( pAut, pState )
        Auti_StateMakeMoore( pState, bCube );
    Cudd_RecursiveDeref( pAut->pMan->dd, bCube );
}

/**Function*************************************************************

  Synopsis    [Reduces the transition to be Moore.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_StateMakeMoore( Aut_State_t * pState, DdNode * bCube )
{
    Aut_Trans_t * pTrans, * pTrans2;
    DdManager * dd = pState->pAut->pMan->dd;
    DdNode * bCond, * bCondI, * bTemp;
    // derive the sum of conditions
    bCond = Aut_StateComputeSumCond( pState );  Cudd_Ref( bCond );
    // get the Moore condition
    bCondI = Cudd_bddUnivAbstract( dd, bCond, bCube );  Cudd_Ref( bCondI );
    assert( bCondI != b0 );
    // reduce the transition condition to Moore condition
    Aut_StateForEachTransitionFromSafe_int( pState, pTrans, pTrans2 )
    {
        pTrans->bCond = Cudd_bddAnd( dd, bTemp = pTrans->bCond, bCondI );    
        Cudd_Ref( pTrans->bCond );
        Cudd_RecursiveDeref( dd, bTemp );
        // remove the transition if the condition is empty
        if ( pTrans->bCond == b0 )
            Aut_TransFree( pState->pAut, pTrans );
    }
    Cudd_RecursiveDeref( dd, bCond );
    Cudd_RecursiveDeref( dd, bCondI );
}


/**Function*************************************************************

  Synopsis    [Returns the number of non-progressive (non-Moore) states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_AutoCheckIsComplete( Aut_Auto_t * pAut, int nInputs )
{
    DdManager * dd = pAut->pMan->dd;
    Aut_State_t * pState;
    DdNode * bCube;
    int Counter = 0;
    // get the quantification cube
    bCube = Aut_AutoGetCubeOutput( pAut, nInputs );  Cudd_Ref( bCube );
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( !Auti_StateCheckIsComplete( pState, bCube ) )
            Counter++;
    }
    Cudd_RecursiveDeref( dd, bCube );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the number of non-progressive (non-Moore) states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_AutoCheckIsMoore( Aut_Auto_t * pAut, int nInputs )
{
    DdManager * dd = pAut->pMan->dd;
    Aut_State_t * pState;
    DdNode * bCube;
    int Counter = 0;
    // get the quantification cube
    bCube = Aut_AutoGetCubeInput( pAut, nInputs );  Cudd_Ref( bCube );
    Aut_AutoForEachState_int( pAut, pState )
    {
        if ( !Auti_StateCheckIsMoore( pState, bCube ) )
            Counter++;
    }
    Cudd_RecursiveDeref( dd, bCube );
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


