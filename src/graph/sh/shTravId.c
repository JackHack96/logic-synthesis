/**CFile****************************************************************

  FileName    [shTravId.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Manipulating trav IDs and special lists of strashed nodes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shTravId.c,v 1.4 2004/04/12 20:41:09 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the current traversal ID of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_ManagerReadTravId( Sh_Manager_t * pMan )
{
    return pMan->nTravIds;
}

/**Function*************************************************************

  Synopsis    [Increments the current traversal ID of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_ManagerIncrementTravId( Sh_Manager_t * pMan )
{
    pMan->nTravIds++;
}

/**Function*************************************************************

  Synopsis    [Reads the traversal ID of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NodeReadTravId( Sh_Node_t * pNode )
{
    return pNode->TravId;
}

/**Function*************************************************************

  Synopsis    [Sets the traversal ID of the node to the given traversal ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeSetTravId( Sh_Node_t * pNode, int TravId )
{
    pNode->TravId = TravId;
}

/**Function*************************************************************

  Synopsis    [Sets the node's trav ID to be the current trav ID of the network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeSetTravIdCurrent( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    pNode->TravId = pMan->nTravIds;
}

/**Function*************************************************************

  Synopsis    [Sets the node's trav ID to be the current trav ID of the network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_NodeIsTravIdCurrent( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    return (bool)(pNode->TravId == pMan->nTravIds);
}

/**Function*************************************************************

  Synopsis    [Sets the node's trav ID to be the current trav ID of the network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Sh_NodeIsTravIdPrevious( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    return (bool)(pNode->TravId == pMan->nTravIds - 1);
}

/**Function*************************************************************

  Synopsis    [Starts the specialized linked list of nodes in the network.]

  Description [After this procedure is called, the nodes can be added
  using Sh_NetworkAddToSpecial().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkStartSpecial( Sh_Network_t * pNet )
{
    pNet->pOrder = NULL;
    pNet->ppTail = &pNet->pOrder;
}

/**Function*************************************************************

  Synopsis    [Stops the specialized linked list of nodes in the network.]

  Description [When the last node is added using Sh_NetworkAddToSpecial(),
  this procedure should be called to finalize the specialized linked
  list of nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkStopSpecial( Sh_Network_t * pNet )
{
//    *(pNet->ppTail) = NULL;
}

/**Function*************************************************************

  Synopsis    [Adds one node to the specialized linked list of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkAddToSpecial( Sh_Network_t * pNet, Sh_Node_t * pNode )
{
    pNode->pOrder = *(pNet->ppTail);
    *(pNet->ppTail) = pNode;
    pNet->ppTail = &pNode->pOrder;
}

/**Function*************************************************************

  Synopsis    [Moves the insertion point to be after the specified node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkMoveSpecial( Sh_Network_t * pNet, Sh_Node_t * pNode )
{
    pNet->ppTail = &pNode->pOrder;
}

/**Function*************************************************************

  Synopsis    [Resets the specialized linked list of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkResetSpecial( Sh_Network_t * pNet )
{
    pNet->pOrder = NULL;
    pNet->ppTail = NULL;
}

/**Function*************************************************************

  Synopsis    [Put the nodes in the array into the special internal list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkCountSpecial( Sh_Network_t * pNet )
{
    Sh_Node_t * pNode;
    int Counter;
    Counter = 0;
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Put the nodes in the array into the special internal list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkCreateSpecialFromArray( Sh_Network_t * pNet, Sh_Node_t * ppNodes[], int nNodes )
{
    int i;
    Sh_NetworkStartSpecial( pNet );
    for ( i = 0; i < nNodes; i++ )
        Sh_NetworkAddToSpecial( pNet, ppNodes[i] );
    Sh_NetworkStopSpecial( pNet );
}

/**Function*************************************************************

  Synopsis    [Put the nodes in the special list into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkCreateArrayFromSpecial( Sh_Network_t * pNet, Sh_Node_t * ppNodes[] )
{
    Sh_Node_t * pNode;
    int Counter;
    Counter = 0;
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
        ppNodes[Counter++] = pNode;
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkPrintSpecial( Sh_Network_t * pNet )
{
    Sh_Node_t * pNode;
//    int Num;

    // assign numbers
//    Num = 0;
//    Sh_NetworkForEachNodeSpecial( pNet, pNode )
//        pNode->pData = Num++;

    // print the DFS order
    printf( "The nodes in the special linked list are:\n" );
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
    {
        printf( "Node %3d : ", Sh_NodeReadIndex(pNode) );
        if ( pNode->pOne == NULL )
        {
            printf( " var %3d ", pNode->pTwo );
            printf( "\n" );
            continue;
        }

        if ( Sh_IsComplement(pNode->pOne) )
            printf( " (%3d) ", Sh_NodeReadIndex(pNode->pOne) );
        else
            printf( "  %3d  ", Sh_NodeReadIndex(pNode->pOne) );

        if ( Sh_IsComplement(pNode->pTwo) )
            printf( " (%3d) ", Sh_NodeReadIndex(pNode->pTwo) );
        else
            printf( "  %3d  ", Sh_NodeReadIndex(pNode->pTwo) );

        if ( shNodeIsVar(pNode) )
            printf( "  special  " );

        printf( "\n" );
    }
    printf( "\n" );

    // clean the numbers
//    Sh_NetworkForEachNodeSpecial( pNet, pNode )
//        pNode->pData = 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


