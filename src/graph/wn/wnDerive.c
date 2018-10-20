/**CFile****************************************************************

  FileName    [wnDerive.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Computes the window centered about one node.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wnDerive.c,v 1.4 2004/04/08 05:05:07 alanmi Exp $]

***********************************************************************/

#include "wnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Wn_Window_t * Wn_WindowInit( Wn_Window_t * pWndCore, int nLevelsTfi, int nLevelsTfo );
static int           Wn_WindowSelectLeaves( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[] );
static int           Wn_WindowSelectRoots( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[] );
static int           Wn_WindowComputeTfiMarked( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes, int Depth, bool fExistPath );
static int           Wn_WindowComputeTfiMarked_rec( Ntk_Node_t * pNode, int Depth, bool fExistPath );
static int           Wn_WindowComputeLeaves( Ntk_Network_t * pNet );
static void          Wn_WindowVerifyWindow( Wn_Window_t * pWnd );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the window centered about one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wn_Window_t * Wn_WindowDeriveForNodeOld( Ntk_Node_t * pNode, int nLevelsTfi, int nLevelsTfo )
//Wn_Window_t * Wn_WindowDeriveForNode( Ntk_Node_t * pNode, int nLevelsTfi, int nLevelsTfo )
{
    Wn_Window_t * pWnd, * pWndCore;
    // create the core window from the linked list of node in pNet
    pWndCore = Wn_WindowCreateFromNode( pNode );
    // derive the container window for the core window
    pWnd     = Wn_WindowDerive( pWndCore, nLevelsTfi, nLevelsTfo );
    return pWnd;

}

/**Function*************************************************************

  Synopsis    [Computes the window centered about one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wn_Window_t * Wn_WindowDerive( Wn_Window_t * pWndCore, int nLevelsTfi, int nLevelsTfo )
{
    Wn_Window_t * p;
    Ntk_Node_t ** pNodesStart;
    int nNodesStart, nNodesTfo, nNodesAll;
    int nLeaves, nLevels, i;
    bool fExistPath = 0; // when it is 1, theoretically the algorithm may not work...
    bool fVerbose = 0;

    // the window can be derived only if the core window is given
    assert( pWndCore );
    assert( pWndCore->pNet );

    // start a new window with the given window as its core
    p = Wn_WindowInit( pWndCore, nLevelsTfi, nLevelsTfo );

    // (1) find the window roots
    // collect the nodes of the core window
    Wn_WindowCollectInternal( pWndCore );
    // link the nodes in the limited TFO
    nNodesTfo = Ntk_NetworkComputeNodeTfo( p->pNet, pWndCore->ppNodes, pWndCore->nNodes, nLevelsTfo, 0, fExistPath );
    if ( fVerbose ) Ntk_NetworkPrintSpecial( p->pNet );
    // select the nodes exactly nLevelsTfo away with fanouts outside themselves
    p->ppRoots = ALLOC( Ntk_Node_t *, nNodesTfo + Ntk_NetworkReadCoNum(p->pNet) );
    p->nRoots = Wn_WindowSelectRoots( p->pNet, p->ppRoots );
    assert( p->nRoots <= nNodesTfo + Ntk_NetworkReadCoNum(p->pNet) );
    if ( fVerbose ) Ntk_NetworkPrintArray( p->ppRoots, p->nRoots );

    // (2) find part of starting leaves
    // link the nodes in the limited TFI
    nNodesStart = Ntk_NetworkComputeNodeTfi( p->pNet, pWndCore->ppNodes, pWndCore->nNodes, nLevelsTfi, 0, fExistPath );
    if ( fVerbose ) Ntk_NetworkPrintSpecial( p->pNet );
    // select the nodes exactly nLevelsTfi away with fanins outside themselves
    pNodesStart = ALLOC( Ntk_Node_t *, nNodesStart + Ntk_NetworkReadCiNum(p->pNet) );
    nNodesStart = Wn_WindowSelectLeaves( p->pNet, pNodesStart );
    if ( fVerbose ) Ntk_NetworkPrintArray( pNodesStart, nNodesStart );

    // (3) mark the nodes in the limited TFO of the leaves
    nLevels = Wn_WindowGetNumLevels( p->pWndCore );
    Ntk_NetworkComputeNodeTfo( p->pNet, pNodesStart, nNodesStart, nLevelsTfi + nLevelsTfo + nLevels - 1, 0, fExistPath );
    // make sure the current roots are marked
    for ( i = 0; i < p->nRoots; i++ )
        assert( Ntk_NodeIsTravIdCurrent(p->ppRoots[i]) );
    if ( fVerbose ) Ntk_NetworkPrintSpecial( p->pNet );

    // (4) collect the nodes in the limited TFI of the roots 
    // that overlap with the limited TFO of the leaves
    nNodesAll = Wn_WindowComputeTfiMarked( p->pNet, p->ppRoots, p->nRoots, nLevelsTfi + nLevelsTfo + nLevels - 1, fExistPath );
    // make sure the starting leaves are marked
    for ( i = 0; i < nNodesStart; i++ )
        assert( Ntk_NodeIsCi(pNodesStart[i]) || Ntk_NodeIsTravIdCurrent(pNodesStart[i]) );
    FREE( pNodesStart );
    // save these nodes
    p->ppNodes = ALLOC( Ntk_Node_t *, nNodesAll );
    p->nNodes = Ntk_NetworkCreateArrayFromSpecial( p->pNet, p->ppNodes );
    assert( p->nNodes == nNodesAll );
    if ( fVerbose ) Ntk_NetworkPrintSpecial( p->pNet );

    // (5) select the fanins of these nodes that are outside these nodes
    nLeaves = Wn_WindowComputeLeaves( p->pNet );
    // put these nodes into the array of leaves
    p->ppLeaves = ALLOC( Ntk_Node_t *, nLeaves );
    p->nLeaves = Ntk_NetworkCreateArrayFromSpecial( p->pNet, p->ppLeaves );
    if ( fVerbose ) Ntk_NetworkPrintSpecial( p->pNet );

    // (6) verify that the leaves are indeed the leaves of the window
    // make sure that the leaves fanout into nodes of the set
    // make sure that the roots fanin into nodes of the set
    Wn_WindowVerifyWindow( p );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wn_Window_t * Wn_WindowInit( Wn_Window_t * pWndCore, int nLevelsTfi, int nLevelsTfo )
{
    Wn_Window_t * p;
    p = ALLOC( Wn_Window_t, 1 );
    memset( p, 0, sizeof(Wn_Window_t) );
    p->nLevelsTfi = nLevelsTfi;
    p->nLevelsTfo = nLevelsTfo;
    p->pWndCore   = pWndCore;
    if ( pWndCore )
    p->pNet       = pWndCore->pNet;
    return p;
}


/**Function*************************************************************

  Synopsis    [Select the roots from the list of TFO nodes.]

  Description [The roots are the nodes belonging to the limited TFO cone
  that (1) fanout outside of this cone (possibly into COs), and 
  (2) have at least one fanin that is not the root itself.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowSelectRoots( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[] )
{
    Ntk_Node_t * pNode, * pFanout;
    Ntk_Pin_t * pPin;
    int nRoots;

    // mark all the nodes that belong to the limited TFO cone
    Ntk_NetworkIncrementTravId( pNet );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        Ntk_NodeSetTravIdCurrent( pNode );

    // go through these nodes and collect those of them that fanout outside
    nRoots = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        // check the node's fanins
        Ntk_NodeForEachFanout( pNode, pPin, pFanout )
            if ( !Ntk_NodeIsTravIdCurrent(pFanout) )
            {
                ppNodes[ nRoots++ ] = pNode;
                break;
            }
    }
    return nRoots;
}

/**Function*************************************************************

  Synopsis    [Select the leaves from the list of TFI nodes.]

  Description [The leaves are the nodes that (1) are exactly the given
  distance away and (2) have at least one fanout that is not a leaf itself.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowSelectLeaves( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[] )
{
    Ntk_Node_t * pNode, * pFanin, * pFanout;
    Ntk_Pin_t * pPin;
    int nLeaves;

    // mark all the nodes that have level 0
    Ntk_NetworkIncrementTravId( pNet );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( pNode->Level == 0 )
            Ntk_NodeSetTravIdCurrent( pNode );

    // go through these nodes and collect those
    // that have at least one fanout that not marked
    nLeaves = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        // if the node has CI fanins that are not added yet, add them
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            if ( Ntk_NodeIsCi(pFanin) )
            { // the fanin is CI
                if ( !Ntk_NodeIsTravIdCurrent(pFanin) ) // is not visited
                {
                     ppNodes[ nLeaves++ ] = pFanin; // add
                     Ntk_NodeSetTravIdCurrent( pFanin ); // mark as visited
                }
            }

        if ( pNode->Level == 0 )
        {   // check the node's fanins
            Ntk_NodeForEachFanout( pNode, pPin, pFanout )
                if ( !Ntk_NodeIsTravIdCurrent(pFanout) )
                {
                    ppNodes[ nLeaves++ ] = pNode;
                    break;
                }
        }
        else
        { 
        }
    }
    return nLeaves;
}

/**Function*************************************************************

  Synopsis    [Collects the marked limited TFI of the set of nodes.]

  Description [Collects only those nodes in the TFI of the given set of 
  nodes that are marked by the current value of the traversal ID.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowComputeTfiMarked( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes, int Depth, bool fExistPath )
{
    Ntk_Node_t * pNode, * pNode2;
    int nNodesRes, DepthReal, i;
    bool fFirst;

    // set the new traversal ID
    // (now the marked nodes are marked with the previous traversal ID)
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    // start DFS from the primary outputs
    DepthReal = Depth + (!fExistPath) * 1000;
    nNodesRes = 0;
    for ( i = 0; i < nNodes; i++ )
        nNodesRes += Wn_WindowComputeTfiMarked_rec( pNodes[i], DepthReal, fExistPath );
    // finalize the linked list
    Ntk_NetworkStopSpecial( pNet );

    // remove from the list those nodes have the large real number of levels
    if ( !fExistPath )
    {
        fFirst = 1;
        nNodesRes = 0;
        Ntk_NetworkForEachNodeSpecialSafe( pNet, pNode, pNode2 )
        {
            if ( fFirst )
            { // restart the specialized list
                Ntk_NetworkStartSpecial( pNet );
                fFirst = 0;
            }
            if ( pNode->Level >= 1000 )
            { // if the node satisfies the criteria, add it to the specialized list
                Ntk_NetworkAddToSpecial( pNode );
                pNode->Level -= 1000;
                nNodesRes++;
            }
        }
        // finalize the linked list
        Ntk_NetworkStopSpecial( pNet );
    }
    return nNodesRes;
}

/**Function*************************************************************

  Synopsis    [Performs the recursive step of Wn_WindowComputeTfiMarked().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowComputeTfiMarked_rec( Ntk_Node_t * pNode, int Depth, bool fExistPath )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;
    int nNodes;

    // if the depth is out, skip
    if ( Depth == -1 )
        return 0;
    // if the node is CI, skip
    if ( Ntk_NodeIsCi(pNode) )
        return 0;

    // if this node is already visited, check the level
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
    {
        nNodes = 0;
        if ( fExistPath && pNode->Level < Depth || (!fExistPath) && pNode->Level > Depth )
        { // we have found a shorter path to the same node
            // update the level to be the shortest path
            pNode->Level = Depth;
            // visit the transitive fanin
            Ntk_NodeForEachFanin( pNode, pLink, pFanin )
                nNodes += Wn_WindowComputeTfiMarked_rec( pFanin, Depth - 1, fExistPath );
        }
        return nNodes;
    }

    // visit the transitive fanin
    nNodes = 0;
    Ntk_NodeForEachFanin( pNode, pLink, pFanin )
        nNodes += Wn_WindowComputeTfiMarked_rec( pFanin, Depth - 1, fExistPath );

    // add the node after the fanins are added
    // if the node belongs to the set of marked nodes
    if ( Ntk_NodeIsTravIdPrevious( pNode ) )
    {
        Ntk_NetworkAddToSpecial( pNode );
        nNodes++;
    }

    // mark the node as visited (this should be done after we check it for being previous)
    Ntk_NodeSetTravIdCurrent( pNode );
    // set the depth
    pNode->Level = Depth;
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Select nodes that have fanins outside the set.]

  Description [The current set of nodes is linked in the internal list.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowComputeLeaves( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode, * pFanin;
    Ntk_Node_t * pList, ** ppTail;
    Ntk_Pin_t * pPin;
    int nNodes;

    // mark the nodes currently in the list
    Ntk_NetworkIncrementTravId( pNet );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        Ntk_NodeSetTravIdCurrent( pNode );

    // start the linked list
    ppTail = &pList;
    nNodes = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        {
            // make sure the fanin is not yet in the list
            if ( !Ntk_NodeIsTravIdCurrent( pFanin ) )
            { // add this node
                *ppTail = pFanin;
                ppTail = &pFanin->pOrder;
                nNodes++;
                // mark that the fanin has been added to the list
                Ntk_NodeSetTravIdCurrent( pFanin );
            }
        }
    // finalize the linked list
    *ppTail = NULL;
    // set this new list as a specialize internal list
    pNet->pOrder = pList;
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Verifies the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowVerifyWindow( Wn_Window_t * pWnd )
{
    Ntk_Node_t * pNode;
//    Ntk_Node_t * pFanout, * pFanin;
//    Ntk_Pin_t * pPin;
//    bool fHasFanoutInside;
//    bool fHasFaninInside;
//    int i;
    int RetValue;

    // connect the nodes in the DFS way
    RetValue = Wn_WindowDfs( pWnd );
    assert( RetValue );
    if ( RetValue == 0 )
        fail( "Wn_WindowVerifyWindow(): Serious windowing error.\n" );

    // mark the internal nodes
    Ntk_NetworkIncrementTravId( pWnd->pNet );
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
        Ntk_NodeSetTravIdCurrent( pNode );
/*
    {
    int i;
    for ( i = 0; i < pWnd->nLeaves; i++ )
    {
        assert( pWnd->ppLeaves[i]->Type == MV_NODE_CI );
    }
    for ( i = 0; i < pWnd->nRoots; i++ )
    {
        assert( Ntk_NodeIsCoFanin(pWnd->ppRoots[i]) );
    }
    }
*/

/*
    // look at each leaf and make sure 
    // it has at least one fanout into internal nodes
    for ( i = 0; i < pWnd->nLeaves; i++ )
    {
        fHasFanoutInside = 0;
        Ntk_NodeForEachFanout( pWnd->ppLeaves[i], pPin, pFanout )
            if ( Ntk_NodeIsTravIdCurrent(pFanout) )
            {
                fHasFanoutInside = 1;
                break;
            }
        assert( fHasFanoutInside );
    }
*/

/*
    // look at each root and make sure 
    // it has at least one fanin among the internal nodes
    for ( i = 0; i < pWnd->nRoots; i++ )
    {
        fHasFaninInside = 0;
        Ntk_NodeForEachFanin( pWnd->ppRoots[i], pPin, pFanin )
            if ( Ntk_NodeIsTravIdCurrent(pFanin) )
            {
                fHasFaninInside = 1;
                break;
            }
        assert( fHasFaninInside );
    }
*/
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


