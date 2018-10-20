/**CFile****************************************************************

  FileName    [autSpecial.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Manipulation of the special list of states.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: autSpecial.c,v 1.2 2004/04/08 04:48:25 alanmi Exp $]

***********************************************************************/

#include "autInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Starts the specialized linked list of states.]

  Description [After this procedure is called, the nodes can be added
  using Aut_AutoAddToSpecial().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStartSpecial( Aut_Auto_t * pAut )
{
    pAut->pOrder = NULL;
    pAut->ppTail = &pAut->pOrder;
}

/**Function*************************************************************

  Synopsis    [Stops the specialized linked list of states.]

  Description [When the last node is added using Aut_AutoAddToSpecial(),
  this procedure should be called to finalize the specialized linked
  list of nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoStopSpecial( Aut_Auto_t * pAut )
{
//    *(pAut->ppTail) = NULL;
}

/**Function*************************************************************

  Synopsis    [Adds one node to the specialized linked list of states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoAddToSpecial( Aut_State_t * pState )
{
    Aut_Auto_t * pAut;
    pAut = pState->pAut;
    pState->pOrder = *(pAut->ppTail);
    *(pAut->ppTail) = pState;
    pAut->ppTail = &pState->pOrder;
}

/**Function*************************************************************

  Synopsis    [Moves the insertion point to be after the specified state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoMoveSpecial( Aut_State_t * pState )
{
    pState->pAut->ppTail = &pState->pOrder;
}

/**Function*************************************************************

  Synopsis    [Resets the specialized linked list of states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoResetSpecial( Aut_Auto_t * pAut )
{
    pAut->pOrder = NULL;
    pAut->ppTail = NULL;
}

/**Function*************************************************************

  Synopsis    [Put the states in the array into the special internal list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aut_AutoCountSpecial( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    int Counter;
    Counter = 0;
    Aut_AutoForEachStateSpecial( pAut, pState )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Put the states in the array into the special internal list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoCreateSpecialFromArray( Aut_Auto_t * pAut, Aut_State_t * ppStates[], int nNodes )
{
    int i;
    Aut_AutoStartSpecial( pAut );
    for ( i = 0; i < nNodes; i++ )
        Aut_AutoAddToSpecial( ppStates[i] );
    Aut_AutoStopSpecial( pAut );
}

/**Function*************************************************************

  Synopsis    [Put the states in the special list into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aut_AutoCreateArrayFromSpecial( Aut_Auto_t * pAut, Aut_State_t * ppStates[] )
{
    Aut_State_t * pState;
    int Counter;
    Counter = 0;
    Aut_AutoForEachStateSpecial( pAut, pState )
        ppStates[Counter++] = pState;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Put the states in the special list into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
st_table * Aut_AutoCreateTableFromSpecial( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    st_table * tNodes;
    tNodes = st_init_table(st_ptrcmp, st_ptrhash);
    Aut_AutoForEachStateSpecial( pAut, pState )
        st_insert( tNodes, (char *)pState, (char *)NULL );
    return tNodes;
}

/**Function*************************************************************

  Synopsis    [Prints the states in the specialized list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_AutoPrintSpecial( Aut_Auto_t * pAut )
{
    Aut_State_t * pState;
    // print the DFS order
    fprintf( stdout, "The nodes in the special linked list are:\n" );
    Aut_AutoForEachStateSpecial( pAut, pState )
        fprintf( stdout, "%s ", pState->pName );
    fprintf( stdout, "\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


