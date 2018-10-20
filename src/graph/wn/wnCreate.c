/**CFile****************************************************************

  FileName    [wnCreate.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Creating windows.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wnCreate.c,v 1.4 2004/04/12 03:42:31 alanmi Exp $]

***********************************************************************/

#include "wnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the window from one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wn_Window_t * Wn_WindowCreateFromNode( Ntk_Node_t * pNode )
{
    Wn_Window_t * p;
    p = ALLOC( Wn_Window_t, 1 );
    memset( p, 0, sizeof(Wn_Window_t) );
    p->nLevelsTfi = 0;
    p->nLevelsTfo = 0;
    p->pNet = Ntk_NodeReadNetwork(pNode);
    // create the leaves
    p->nLeaves = Ntk_NodeReadFaninNum( pNode );
    p->ppLeaves = ALLOC( Ntk_Node_t *, p->nLeaves );
    Ntk_NodeReadFanins( pNode, p->ppLeaves );
    // create the root
    p->nRoots = 1;
    p->ppRoots = ALLOC( Ntk_Node_t *, p->nRoots );
    p->ppRoots[0] = pNode;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the window from the specialized linked list of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wn_Window_t * Wn_WindowCreateFromNodeArray( Ntk_Network_t * pNet )
{
    Wn_Window_t * p;
    Ntk_Node_t * pFanin, * pFanout, * pNode;
    Ntk_Pin_t * pPin;
    int nNodes;
    int nFaninsTotal;

    // count the number of nodes in the internal list
    nNodes = Ntk_NetworkCountSpecial( pNet );

    // shortcut if there is only one node
    if ( nNodes == 1 )
        return Wn_WindowCreateFromNode( Ntk_NetworkReadOrder(pNet) );

    // mark the nodes in the list and count the total number of fanins
    Ntk_NetworkIncrementTravId( pNet );
    nFaninsTotal = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        Ntk_NodeSetTravIdCurrent( pNode );
        nFaninsTotal += Ntk_NodeReadFaninNum(pNode);
    }

    // start the window
    p = ALLOC( Wn_Window_t, 1 );
    memset( p, 0, sizeof(Wn_Window_t) );
    p->nLevelsTfi = 0;
    p->nLevelsTfo = 0;
    p->pNet       = pNet;

    // collect those nodes that have fanouts outside the set
    p->ppRoots = ALLOC( Ntk_Node_t *, nNodes );
    p->nRoots = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        Ntk_NodeForEachFanout( pNode, pPin, pFanout )
            if ( !Ntk_NodeIsTravIdCurrent(pFanout) )
            {
                // collect the corresponding node
                p->ppRoots[ p->nRoots++ ] = pNode;
                break;
            }
    }
    assert( p->nRoots <= nNodes );

    // collect the fanins of the nodes
    // as the nodes that fanin into the given set of nodes
    p->ppLeaves = ALLOC( Ntk_Node_t *, nFaninsTotal );
    p->nLeaves = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            if ( !Ntk_NodeIsTravIdCurrent(pFanin) )
            {
                // collect this fanin
                p->ppLeaves[ p->nLeaves++ ] = pFanin;
                // mark this fanin
                Ntk_NodeSetTravIdCurrent( pFanin );
            }
    }
    assert( p->nLeaves <= nFaninsTotal );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the window from the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wn_Window_t * Wn_WindowCreateFromNetwork( Ntk_Network_t * pNet )
{
    Wn_Window_t * p;
    Ntk_Node_t * pNode, * pDriver;
    int i;
    p = ALLOC( Wn_Window_t, 1 );
    memset( p, 0, sizeof(Wn_Window_t) );
    p->nLevelsTfi = 0;
    p->nLevelsTfo = 0;
    p->pNet = pNet;
    // create the leaves
    p->nLeaves = Ntk_NetworkReadCiNum( pNet );
    p->ppLeaves = ALLOC( Ntk_Node_t *, p->nLeaves );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        p->ppLeaves[i++] = pNode;
    // create the roots
    p->ppRoots = ALLOC( Ntk_Node_t *, Ntk_NetworkReadCoNum(pNet) );
    p->nRoots = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
        if ( pDriver->Type != MV_NODE_CI )
            p->ppRoots[p->nRoots++] = pDriver;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the window from the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wn_Window_t * Wn_WindowCreateFromNetworkNames( Ntk_Network_t * pNet, char ** pNamesIn, char ** pNamesOut )
{
    Wn_Window_t * p;
    Ntk_Node_t * pNode;
    int i;
    p = ALLOC( Wn_Window_t, 1 );
    memset( p, 0, sizeof(Wn_Window_t) );
    p->nLevelsTfi = 0;
    p->nLevelsTfo = 0;
    p->pNet = pNet;
    // create the leaves
    p->nLeaves = Ntk_NetworkReadCiNum( pNet );
    p->ppLeaves = ALLOC( Ntk_Node_t *, p->nLeaves );
    for ( i = 0; i < p->nLeaves; i++ )
    {
        pNode = Ntk_NetworkFindNodeByName( pNet, pNamesIn[i] );
        assert( pNode );
        p->ppLeaves[i] = pNode;
    }
    // create the roots
    p->nRoots = Ntk_NetworkReadCoNum( pNet );
    p->ppRoots = ALLOC( Ntk_Node_t *, p->nRoots );
    for ( i = 0; i < p->nRoots; i++ )
    {
        pNode = Ntk_NetworkFindNodeByName( pNet, pNamesOut[i] );
        assert( pNode );
        p->ppRoots[i] = pNode;
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes the window and its associated data structures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowDelete( Wn_Window_t * p )
{
    if ( p == NULL )
        return;
    Wn_WindowDelete( p->pWndCore );
    FREE( p->ppLeaves );
    FREE( p->ppRoots );
    FREE( p->ppNodes );
    FREE( p->ppSpecials );

    Ntk_NodeVecFree( p->pVecLeaves ); 
    Ntk_NodeVecFree( p->pVecRoots ); 
    Ntk_NodeVecFree( p->pVecNodes ); 
    FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


