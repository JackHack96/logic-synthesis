/**CFile****************************************************************

  FileName    [ntkTravId.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Exported procedures to work with traversal IDs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkTravId.c,v 1.11 2003/11/18 18:54:55 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

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
int Ntk_NetworkReadTravId( Ntk_Network_t * pNet )
{
    return pNet->nTravIds;
}

/**Function*************************************************************

  Synopsis    [Increments the current traversal ID of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkIncrementTravId( Ntk_Network_t * pNet )
{
    pNet->nTravIds++;
}

/**Function*************************************************************

  Synopsis    [Reads the traversal ID of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeReadTravId( Ntk_Node_t * pNode )
{
    return pNode->TravId;
}

/**Function*************************************************************

  Synopsis    [Sets the traversal ID of the node to the given traversal ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetTravId( Ntk_Node_t * pNode, int TravId )
{
    pNode->TravId = TravId;
}

/**Function*************************************************************

  Synopsis    [Sets the node's trav ID to be the current trav ID of the network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetTravIdCurrent( Ntk_Node_t * pNode )
{
    pNode->TravId = pNode->pNet->nTravIds;
}

/**Function*************************************************************

  Synopsis    [Sets the node's trav ID to be the current trav ID of the network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeIsTravIdCurrent( Ntk_Node_t * pNode )
{
    return (bool)(pNode->TravId == pNode->pNet->nTravIds);
}

/**Function*************************************************************

  Synopsis    [Sets the node's trav ID to be the current trav ID of the network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeIsTravIdPrevious( Ntk_Node_t * pNode )
{
    return (bool)(pNode->TravId == pNode->pNet->nTravIds - 1);
}

/**Function*************************************************************

  Synopsis    [Starts the specialized linked list of nodes in the network.]

  Description [After this procedure is called, the nodes can be added
  using Ntk_NetworkAddToSpecial().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkStartSpecial( Ntk_Network_t * pNet )
{
    pNet->pOrder = NULL;
    pNet->ppTail = &pNet->pOrder;
}

/**Function*************************************************************

  Synopsis    [Stops the specialized linked list of nodes in the network.]

  Description [When the last node is added using Ntk_NetworkAddToSpecial(),
  this procedure should be called to finalize the specialized linked
  list of nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkStopSpecial( Ntk_Network_t * pNet )
{
//    *(pNet->ppTail) = NULL;
}

/**Function*************************************************************

  Synopsis    [Adds one node to the specialized linked list of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkAddToSpecial( Ntk_Node_t * pNode )
{
    Ntk_Network_t * pNet;
    pNet = pNode->pNet;
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
void Ntk_NetworkMoveSpecial( Ntk_Node_t * pNode )
{
    pNode->pNet->ppTail = &pNode->pOrder;
}

/**Function*************************************************************

  Synopsis    [Resets the specialized linked list of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkResetSpecial( Ntk_Network_t * pNet )
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
int Ntk_NetworkCountSpecial( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int Counter;
    Counter = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Put the nodes in the array into the special internal list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkCreateSpecialFromArray( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[], int nNodes )
{
    int i;
    Ntk_NetworkStartSpecial( pNet );
    for ( i = 0; i < nNodes; i++ )
        Ntk_NetworkAddToSpecial( ppNodes[i] );
    Ntk_NetworkStopSpecial( pNet );
}

/**Function*************************************************************

  Synopsis    [Put the nodes in the special list into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkCreateArrayFromSpecial( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[] )
{
    Ntk_Node_t * pNode;
    int Counter;
    Counter = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        ppNodes[Counter++] = pNode;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Put the nodes in the special list into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
st_table * Ntk_NetworkCreateTableFromSpecial( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    st_table * tNodes;
    tNodes = st_init_table(st_ptrcmp, st_ptrhash);
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        st_insert( tNodes, (char *)pNode, (char *)NULL );
    return tNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintSpecial( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    // print the DFS order
    fprintf( Ntk_NetworkReadMvsisOut(pNet), "The nodes in the special linked list are:\n" );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
//        fprintf( Ntk_NetworkReadMvsisOut(pNet), "%s(%d) ", Ntk_NodeGetNamePrintable( pNode ), pNode->TravId );
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "%s ", Ntk_NodeGetNamePrintable( pNode ) );
    fprintf( Ntk_NetworkReadMvsisOut(pNet), "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintArray( Ntk_Node_t ** ppNodes, int nNodes )
{
    int i;
    if ( nNodes == 0 )
        return;
    // print the DFS order
    fprintf( Ntk_NodeReadMvsisOut(ppNodes[0]), "The nodes in the array are:\n" );
    for ( i = 0; i < nNodes; i++ )
        fprintf( Ntk_NodeReadMvsisOut(ppNodes[0]), "%s(%d) ", Ntk_NodeGetNamePrintable( ppNodes[i] ), ppNodes[i]->Level );
    fprintf( Ntk_NodeReadMvsisOut(ppNodes[0]), "\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


