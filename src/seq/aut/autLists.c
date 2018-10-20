/**CFile****************************************************************

  FileName    [autLists.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Manipulation of linked lists and rings of states and transitions.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autLists.c,v 1.2 2004/04/08 04:48:25 alanmi Exp $]

***********************************************************************/

#include "autInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Adding the state at the beginning of the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStateAddFirst( Aut_State_t * pState )
{
    Aut_Auto_t * pAut = pState->pAut;
    if ( pAut->pHead == NULL )
    {
        pAut->pHead = pState;
        pAut->pTail = pState;
        pState->pPrev = NULL;
        pState->pNext = NULL;
    }
    else
    {
        pState->pPrev = NULL;
        pAut->pHead->pPrev = pState;
        pState->pNext = pAut->pHead;
        pAut->pHead = pState;
    }
    pAut->nStates++;
    // add to the table
    if ( pState->pName )
        if ( st_insert( pAut->tName2State, pState->pName, (char *)pState ) )
        {
            assert( 0 );
        }
}

/**Function*************************************************************

  Synopsis    [Adding the state at the end of the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStateAddLast( Aut_State_t * pState )
{
    Aut_Auto_t * pAut = pState->pAut;
    if ( pAut->pHead == NULL )
    {
        pAut->pHead = pState;
        pAut->pTail = pState;
        pState->pPrev = NULL;
        pState->pNext = NULL;
    }
    else
    {
        pState->pNext = NULL;
        pAut->pTail->pNext = pState;
        pState->pPrev = pAut->pTail;
        pAut->pTail = pState;
    }
    pAut->nStates++;
    // add to the table
    if ( pState->pName )
        if ( st_insert( pAut->tName2State, pState->pName, (char *)pState ) )
        {
            assert( 0 );
        }
}

/**Function*************************************************************

  Synopsis    [Deleting the state from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStateDelete( Aut_State_t * pState )
{
    Aut_Auto_t * pAut = pState->pAut;
    if ( pAut->pHead == pState )
         pAut->pHead = pState->pNext;
    if ( pAut->pTail == pState )
         pAut->pTail = pState->pPrev;
    if ( pState->pPrev )
         pState->pPrev->pNext = pState->pNext;
    if ( pState->pNext )
         pState->pNext->pPrev = pState->pPrev;
    pAut->nStates--;
    // remove from the table
    assert ( pState->pName );
    if ( !st_delete( pAut->tName2State, &pState->pName, (char **)&pState ) )
    {
        assert( 0 );
    }
}

/**Function*************************************************************

  Synopsis    [Adding the outgoing transition to the state's transition list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_StateAddLastTransFrom( Aut_State_t * pState, Aut_Trans_t * pTrans )
{
    if ( pState->pHeadFrom == NULL )
    {
        pState->pHeadFrom = pTrans;
        pState->pTailFrom = pTrans;
        pTrans->pPrevFrom = NULL;
        pTrans->pNextFrom = NULL;
    }
    else
    {
        pTrans->pNextFrom = NULL;
        pState->pTailFrom->pNextFrom = pTrans;
        pTrans->pPrevFrom = pState->pTailFrom;
        pState->pTailFrom = pTrans;
    }
    pState->nTransFrom++;
}

/**Function*************************************************************

  Synopsis    [Adding the incoming transition to the state's transition list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_StateAddLastTransTo( Aut_State_t * pState, Aut_Trans_t * pTrans )
{
    if ( pState->pHeadTo == NULL )
    {
        pState->pHeadTo = pTrans;
        pState->pTailTo = pTrans;
        pTrans->pPrevTo = NULL;
        pTrans->pNextTo = NULL;
    }
    else
    {
        pTrans->pNextTo = NULL;
        pState->pTailTo->pNextTo = pTrans;
        pTrans->pPrevTo = pState->pTailTo;
        pState->pTailTo = pTrans;
    }
    pState->nTransTo++;
}

/**Function*************************************************************

  Synopsis    [Deleting the outgoing transition from the state's transition list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_StateDeleteTransFrom( Aut_Trans_t * pTrans )
{
    Aut_State_t * pState = pTrans->pFrom;
    if ( pState->pHeadFrom == pTrans )
         pState->pHeadFrom = pTrans->pNextFrom;
    if ( pState->pTailFrom == pTrans )
         pState->pTailFrom = pTrans->pPrevFrom;
    if ( pTrans->pPrevFrom )
         pTrans->pPrevFrom->pNextFrom = pTrans->pNextFrom;
    if ( pTrans->pNextFrom )
         pTrans->pNextFrom->pPrevFrom = pTrans->pPrevFrom;
    pState->nTransFrom--;
}

/**Function*************************************************************

  Synopsis    [Delecting the incoming transition from the state's transition list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_StateDeleteTransTo( Aut_Trans_t * pTrans )
{
    Aut_State_t * pState = pTrans->pTo;
    if ( pState->pHeadTo == pTrans )
         pState->pHeadTo = pTrans->pNextTo;
    if ( pState->pTailTo == pTrans )
         pState->pTailTo = pTrans->pPrevTo;
    if ( pTrans->pPrevTo )
         pTrans->pPrevTo->pNextTo = pTrans->pNextTo;
    if ( pTrans->pNextTo )
         pTrans->pNextTo->pPrevTo = pTrans->pPrevTo;
    pState->nTransTo--;
}





#if 0  // should not be compiled

// there experimental procedures store the states 
// in the ring of states instead of the list of states

#define Aut_AutoForEachState_int( Auto, State, i )\
    if ( (Auto)->pRing )\
        for ( State  = (Auto)->pRing, i = 0;\
              State != (Auto)->pRing || i == 0;\
              State  = State->pNext, i++ )

#define Aut_AutoForEachStateSafe_int( Auto, State, State2, i )\
    if ( (Auto)->pRing )\
        for ( State  = (Auto)->pRing, i = 0;\
              State != (Auto)->pRing || i == 0;\
              State  = State->pNext, i++ )

#define Aut_AutoForEachStateSafe_int( Auto, State, State2, i )\
    if ( (Auto)->pRing )\
        for ( State  = (Auto)->pRing, State2 = State->pNext, i = 0;\
              State != (Auto)->pRing || i == 0;\
              State  = State2, State2 = State->pNext, i++ )

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStateAddFirst( Aut_State_t * pState )
{
    Aut_Auto_t * pAut = pState->pAut;
    if ( pAut->pRing == NULL )
    {
        pAut->pRing   = pState;
        pState->pPrev = pState;
        pState->pNext = pState;
    }
    else
    {
        pState->pNext = pAut->pRing;
        pState->pPrev = pAut->pRing->pPrev;
        pState->pNext->pPrev = pState;
        pState->pPrev->pNext = pState;
        pAut->pRing   = pState;
    }
    pAut->nStates++;
    // add to the table
    if ( st_insert( pAut->tName2State, pState->pName, (char *)pState ) )
    {
        assert( 0 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStateAddLast( Aut_State_t * pState )
{
    Aut_Auto_t * pAut = pState->pAut;
    if ( pAut->pRing == NULL )
    {
        pAut->pRing   = pState;
        pState->pPrev = pState;
        pState->pNext = pState;
    }
    else
    {
        pState->pNext = pAut->pRing;
        pState->pPrev = pAut->pRing->pPrev;
        pState->pNext->pPrev = pState;
        pState->pPrev->pNext = pState;
    }
    pAut->nStates++;
    // add to the table
    if ( st_insert( pAut->tName2State, pState->pName, (char *)pState ) )
    {
        assert( 0 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStateDelete( Aut_State_t * pState )
{
    Aut_Auto_t * pAut = pState->pAut;
    if ( pState->pNext == pState )
        pAut->pRing = NULL;
    else 
    {
        pState->pNext->pPrev = pState->pPrev;
        pState->pPrev->pNext = pState->pNext;
        if ( pAut->pRing == pState )
            pAut->pRing = pState->pNext;
    }
    pAut->nStates--;
    // add to the table
    if ( !st_delete( pAut->tName2State, &pState->pName, (char **)&pState ) )
    {
        assert( 0 );
    }
}

#endif


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


