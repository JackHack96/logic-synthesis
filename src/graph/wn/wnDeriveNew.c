/**CFile****************************************************************

  FileName    [wnDerive.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Computes the window centered about one node.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wnDeriveNew.c,v 1.4 2005/03/31 01:29:46 casem Exp $]

***********************************************************************/

#include "wnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef Ntk_Node_t    Ntk_Ring_t;

// these are iterators through the nodes in the ring
// each node can only be in one ring
#define Ntk_RingForEachNode( pRing, pNode, i )\
    for ( pNode = pRing, i = 0;\
        pNode && (pNode != pRing || i == 0);\
        pNode = pNode->pLink, i++ )
#define Ntk_RingForEachNodeSafe( pRing, pPrev, pNode, i )\
    for ( pPrev = NULL, pNode = pRing, i = 0;\
        pNode && (pNode != pRing || i == 0);\
        pPrev = pNode, pNode = pNode->pLink, i++ )

static Wn_Window_t * Wn_WindowInitForNode( Ntk_Node_t * pNode, int nLevelsTfi, int nLevelsTfo );
static void          Wn_WindowVerify( Wn_Window_t * pWnd );

static Ntk_Ring_t * Ntk_NetworkCollectLimitedTFI( Ntk_Ring_t * pRing, int Limit, bool fCollectCis );
static Ntk_Ring_t * Ntk_NetworkCollectSliceTFI( Ntk_Ring_t * pRing, bool fCollectCis );
static Ntk_Ring_t * Ntk_NetworkCollectLimitedTFO( Ntk_Ring_t * pRing, int Limit, bool fIncludeFirst, bool fSkipUnmarked );
static Ntk_Ring_t * Ntk_NetworkCollectSliceTFO( Ntk_Ring_t * pRing, bool fSkipUnmarked );

static Ntk_Ring_t * Ntk_NetworkRestrictToRoots( Ntk_Ring_t * pRing );
static Ntk_Ring_t * Ntk_NetworkRestrictToLeaves( Ntk_Ring_t * pRing );
static Ntk_Ring_t * Ntk_NetworkRestrictToMarked( Ntk_Ring_t * pRing );

static void         Ntk_NetworkSaveRoots( Wn_Window_t * pWnd, Ntk_Ring_t * pRing );
static void         Ntk_NetworkSaveLeaves( Wn_Window_t * pWnd, Ntk_Ring_t * pRing );
static void         Ntk_NetworkSaveInternal( Wn_Window_t * pWnd );
static void         Ntk_NetworkFilterLeaves( Wn_Window_t * pWnd );

extern Ntk_Ring_t * Ntk_NodeRingCreateFromNode( Ntk_Node_t * pNode );
extern Ntk_Ring_t * Ntk_NodeRingAdd( Ntk_Ring_t * pRing, Ntk_Node_t * pNode );
extern Ntk_Ring_t * Ntk_NodeRingDelete( Ntk_Ring_t * pRing, Ntk_Node_t * pNode );
extern Ntk_Ring_t * Ntk_NodeRingAppend( Ntk_Ring_t * pRing1, Ntk_Ring_t * pRing2 );
extern void         Ntk_NodeRingDissolve( Ntk_Ring_t * pRing );
extern void         Ntk_NodeRingMark( Ntk_Ring_t * pRing );
extern void         Ntk_NodeRingPrint( Ntk_Ring_t * pRing, char * pComment );

extern Ntk_Ring_t * Ntk_NetworkRingCreateFromSpecial( Ntk_Network_t * pNet );
extern int          Ntk_NetworkRingCheck( Ntk_Network_t * pNet );

int clock1 = 0;
int clock2 = 0;
int clock3 = 0;
int clock4 = 0;
int clock5 = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Shows the window using DOT software.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowShow( Wn_Window_t * pWnd )
{
    char NewNetName[500];
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNodeNew;
    // collect the internal nodes
    Wn_WindowCollectInternal( pWnd );
    // put the nodes into the list
    Ntk_NetworkCreateSpecialFromArray( pWnd->pNet, pWnd->ppNodes, pWnd->nNodes );
    sprintf( NewNetName, "%s_%s.blif", pWnd->pNet->pName, Ntk_NodeGetNamePrintable(pWnd->pNode) );
    // extract the subnetwork 
    pNetNew = Ntk_SubnetworkExtract( pWnd->pNet );
    // change the network's name
//    pNetNew->pName = NULL;
//    Ntk_NetworkSetName( pNetNew, Ntk_NetworkRegisterNewName(pNetNew, NewNetName) );
    // find the new node with the same name
    pNodeNew = Ntk_NetworkFindNodeByName( pNetNew, Ntk_NodeGetNamePrintable(pWnd->pNode) );
    // show this network
    Ntk_NetworkShow( pNetNew, &pNodeNew, 1, 0 );
    // delete the subnetwork
    Ntk_NetworkDelete( pNetNew );
}

/**Function*************************************************************

  Synopsis    [Computes the window centered about one node.]

  Description [This procedure works correctly after the constants
  are propagates and dangling nodes removed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wn_Window_t * Wn_WindowDeriveForNode( Ntk_Node_t * pNode, int nLevelsTfi, int nLevelsTfo )
{
    Wn_Window_t * p;
    Ntk_Ring_t * pRingNode, * pRingTfo, * pRingTfi;
    Ntk_Ring_t * pRingRoots, * pRingStarts, * pRingLeaves;
    int nLevelsExtra = 2; // this parameter determines the overlap of TFI(roots) and TFO(leaves)
    // it cannot be less than 1; otherwise the algorithm will not work
    int fVerbose = 0;
    int RetValue;
//    int clk;

    if ( nLevelsTfi > 8 )
        nLevelsTfi = 8;
    if ( nLevelsTfo > 8 )
        nLevelsTfo = 8;

    // the window can be derived only if the core window is given
    assert( pNode );
    assert( pNode->pNet );
    assert( pNode->Type == MV_NODE_INT );
    // check that the ring pointers are clean (otherwise, rings will not work)
//    Ntk_NetworkRingCheck( pNode->pNet );

    // start a new window with the given window as its core
    p = Wn_WindowInitForNode( pNode, nLevelsTfi, nLevelsTfo );

    // (1) find the window roots
//clk = clock();
    pRingNode  = Ntk_NodeRingCreateFromNode( pNode );
    pRingTfo   = Ntk_NetworkCollectLimitedTFO( pRingNode, nLevelsTfo, 1, 0 );
    pRingRoots = Ntk_NetworkRestrictToRoots( pRingTfo );
    if ( fVerbose ) Ntk_NodeRingPrint( pRingRoots, "Window roots" );
    Ntk_NetworkSaveRoots( p, pRingRoots );
//clock1 += clock() - clk;

    // (2) find the TFI cone of roots
//clk = clock();
    pRingTfi = Ntk_NetworkCollectLimitedTFI( pRingRoots, nLevelsTfi + nLevelsTfo + nLevelsExtra, 1 );
    if ( fVerbose ) Ntk_NodeRingPrint( pRingTfi, "Root TFI" );
    Ntk_NetworkIncrementTravId( pNode->pNet );
    Ntk_NodeRingMark( pRingTfi );
    Ntk_NodeRingDissolve( pRingTfi );
//clock2 += clock() - clk;

    // (3) find the starting window leaves
//clk = clock();
    pRingNode   = Ntk_NodeRingCreateFromNode( pNode );
    pRingTfi    = Ntk_NetworkCollectLimitedTFI( pRingNode, nLevelsTfi, 0 );
    pRingStarts = Ntk_NetworkRestrictToLeaves( pRingTfi );
    if ( fVerbose ) Ntk_NodeRingPrint( pRingStarts, "Windows leaves" );
//clock3 += clock() - clk;

    // (4) mark the TFO cone of the starting leaves
//clk = clock();
    pRingTfo = Ntk_NetworkCollectLimitedTFO( pRingStarts, nLevelsTfi + nLevelsTfo + nLevelsExtra, 0, 1 );
    if ( fVerbose ) Ntk_NodeRingPrint( pRingTfo, "Window internal" );
    pRingLeaves = Ntk_NetworkRestrictToLeaves( pRingTfo );
    if ( fVerbose ) Ntk_NodeRingPrint( pRingLeaves, "Windows leaves (all)" );
    Ntk_NetworkSaveLeaves( p, pRingLeaves );
    Ntk_NodeRingDissolve( pRingLeaves );
//clock4 += clock() - clk;

    // (5) collect internal nodes and filter the leaves
//clk = clock();
    RetValue = Wn_WindowDfs( p );
    assert( RetValue );
    if ( RetValue == 0 )
        fail( "Wn_WindowVerify(): Serious windowing error.\n" );
    Ntk_NetworkSaveInternal( p );
    assert( p->nNodes > 0 );
    Ntk_NetworkFilterLeaves( p );
//clock5 += clock() - clk;

    // (6) verify that the leaves are indeed the leaves of the window
    // make sure that the leaves fanout into nodes of the set
//    Wn_WindowVerify( p );

    // consider the special case when the TFI cone (and its leaves) are
    // completely disjoint-support from the rest of the window
    // in this case, the don't-cares can only be produced by the TFI cone
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Wn_Window_t * Wn_WindowInitForNode( Ntk_Node_t * pNode, int nLevelsTfi, int nLevelsTfo )
{
    Wn_Window_t * p;
    p = ALLOC( Wn_Window_t, 1 );
    memset( p, 0, sizeof(Wn_Window_t) );
    p->nLevelsTfi = nLevelsTfi;
    p->nLevelsTfo = nLevelsTfo;
    p->pWndCore   = NULL;
    p->pNode      = pNode;
    if ( pNode )
    p->pNet       = pNode->pNet;
    return p;
}

/**Function*************************************************************

  Synopsis    [Verifies the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowVerify( Wn_Window_t * pWnd )
{
    Ntk_Node_t * pNode;
    Ntk_Node_t * pFanout;//, * pFanin;
    Ntk_Pin_t * pPin;
    bool fHasFanoutInside;
//    bool fHasFaninInside;
    int i, nNodes;
    int RetValue;

    // connect the nodes in the DFS way
    RetValue = Wn_WindowDfs( pWnd );
    assert( RetValue );
    if ( RetValue == 0 )
        fail( "Wn_WindowVerify(): Serious windowing error.\n" );

    // mark the internal nodes
    Ntk_NetworkIncrementTravId( pWnd->pNet );
    nNodes = 0;
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
    {
        Ntk_NodeSetTravIdCurrent( pNode );
        nNodes++;
    }
    assert( nNodes > 0 );

    // look at each leaf and make sure 
    // it has at least one fanout among the internal nodes
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
}


/**Function*************************************************************

  Synopsis    [Collects the nodes in the TFI of the given nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NetworkCollectLimitedTFI( Ntk_Ring_t * pRing, int Limit, bool fCollectCis )
{
    Ntk_Ring_t * pRingTotal;
    Ntk_Ring_t * pRings[21];
    int i, k;
    assert( pRing );
    assert( Limit >= 0 );
    assert( Limit <= 20 );
    // collect the onion rings of nodes
    pRings[0] = pRing;
    for ( i = 1; i <= Limit; i++ )
    {
        pRings[i] = Ntk_NetworkCollectSliceTFI( pRings[i-1], fCollectCis );
        if ( pRings[i] == NULL )
            break;
    }
    pRingTotal = pRings[0];
    for ( k = 1; k < i; k++ )
        pRingTotal = Ntk_NodeRingAppend( pRingTotal, pRings[k] );
    return pRingTotal;
}

/**Function*************************************************************

  Synopsis    [Collects the next layer of nodes.]

  Description [Given the nodes that are distance-k removed from the given set,
  this procedure collects the fanins of the nodes in the set that are
  (1) not in the set and (2) whose fanouts are *all* in the set.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NetworkCollectSliceTFI( Ntk_Ring_t * pRing, bool fCollectCis )
{
    Ntk_Ring_t * pRingNext;
    Ntk_Node_t * pFanin, * pNode;
    Ntk_Pin_t * pPin;
    int i;
    // go through the nodes
    pRingNext = NULL;
    Ntk_RingForEachNode( pRing, pNode, i )
    {
        // go through the node's fanins
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        {
            // skip the fanin if it is in the ring already
            if ( pFanin->pLink )
                continue;
            // skip the fanin if it is a CI
            if ( pFanin->Type == MV_NODE_CI && !fCollectCis )
                continue;
            // add the node to the list
            pRingNext = Ntk_NodeRingAdd( pRingNext, pFanin );
        }
    }
    return pRingNext;
}

/**Function*************************************************************

  Synopsis    [Collects the nodes in the TFI of the given nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NetworkCollectLimitedTFO( Ntk_Ring_t * pRing, int Limit, bool fIncludeFirst, bool fSkipUnmarked )
{
    Ntk_Ring_t * pRingTotal;
    Ntk_Ring_t * pRings[21];
    int i, k;
    assert( pRing );
    assert( Limit >= 0 );
    assert( Limit <= 20 );
    // collect the onion rings of nodes
    pRings[0] = pRing;
    for ( i = 1; i <= Limit; i++ )
    {
        pRings[i] = Ntk_NetworkCollectSliceTFO( pRings[i-1], fSkipUnmarked );
        if ( pRings[i] == NULL )
            break;
    }
    pRingTotal = NULL;
    if ( fIncludeFirst )
    {
        for ( k = 0; k < i; k++ )
            pRingTotal = Ntk_NodeRingAppend( pRingTotal, pRings[k] );
    }
    else
    {
        Ntk_NodeRingDissolve( pRings[0] );
        for ( k = 1; k < i; k++ )
            pRingTotal = Ntk_NodeRingAppend( pRingTotal, pRings[k] );
    }
    return pRingTotal;
}

/**Function*************************************************************

  Synopsis    [Collects the next layer of nodes.]

  Description [Given the nodes that are distance-k removed from the given set,
  this procedure collects the fanouts of the nodes in the set that are
  (1) not in the set, and (2) whose fanins are *all* in the set.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NetworkCollectSliceTFO( Ntk_Ring_t * pRing, bool fSkipUnmarked )
{
    Ntk_Ring_t * pRingNext;
    Ntk_Node_t * pFanout, * pNode;
    Ntk_Pin_t * pPin;
    int i;
    // go through the nodes
    pRingNext = NULL;
    Ntk_RingForEachNode( pRing, pNode, i )
    {
        // go through the node's fanouts
        Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        {
            // skip the fanout if it is in the ring already
            if ( pFanout->pLink )
                continue;
            // skip the fanout if it is a CO
            if ( pFanout->Type == MV_NODE_CO )
                continue;
            // skip unmarked nodes
            if ( fSkipUnmarked && !Ntk_NodeIsTravIdCurrent(pFanout) )
                continue;
            // add the node to the list
            pRingNext = Ntk_NodeRingAdd( pRingNext, pFanout );
        }
    }
    return pRingNext;
}





/**Function*************************************************************

  Synopsis    [Returns the roots of the set of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NetworkRestrictToRoots( Ntk_Ring_t * pRing )
{
    Ntk_Ring_t * pRingRoots = NULL;
    Ntk_Node_t * pFanout, * pNode, * pPrev;
    Ntk_Pin_t * pPin;
    int i;
    // go through the node's fanins
    Ntk_RingForEachNodeSafe( pRing, pPrev, pNode, i )
    {
        if ( pPrev == NULL )
            continue;
        // remove the fanout from the ring
        pPrev->pLink = NULL;
        // go through the fanouts
        Ntk_NodeForEachFanout( pPrev, pPin, pFanout )
            if ( pFanout->pLink == NULL ) // the fanout is not in the set
                break;
        if ( pPin == NULL ) // cound not find a fanout that is not in the set
            continue;
        // there is a fanout that is not in the set
        // add the node to the list
        pRingRoots = Ntk_NodeRingAdd( pRingRoots, pPrev );
    }

    if ( pPrev == NULL )
        return pRingRoots;
    // remove the fanout from the ring
    pPrev->pLink = NULL;
    // go through the fanouts
    Ntk_NodeForEachFanout( pPrev, pPin, pFanout )
        if ( pFanout->pLink == NULL ) // the fanout is not in the set
            break;
    if ( pPin == NULL ) // cound not find a fanout that is not in the set
        return pRingRoots;
    // there is a fanout that is not in the set
    // add the node to the list
    pRingRoots = Ntk_NodeRingAdd( pRingRoots, pPrev );
    return pRingRoots;
}

/**Function*************************************************************

  Synopsis    [Returns the leaves of the set of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NetworkRestrictToLeaves( Ntk_Ring_t * pRing )
{
    Ntk_Ring_t * pRingLeaves;
    pRingLeaves = Ntk_NetworkCollectSliceTFI( pRing, 1 );
    Ntk_NodeRingDissolve( pRing );
    return pRingLeaves;
}

/**Function*************************************************************

  Synopsis    [Returns the marked nodes in the set of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NetworkRestrictToMarked( Ntk_Ring_t * pRing )
{
    Ntk_Ring_t * pRingMarked;
    Ntk_Node_t * pNode, * pPrev;
    int i;
    // go through the node's fanins
    pRingMarked = NULL;
    Ntk_RingForEachNodeSafe( pRing, pPrev, pNode, i )
    {
        if ( pPrev == NULL )
            continue;
        // remove the fanout from the ring
        pPrev->pLink = NULL;
        // check if the node is marked
        if ( !Ntk_NodeIsTravIdCurrent(pPrev) )
            continue;
        // add the node to the list
        pRingMarked = Ntk_NodeRingAdd( pRingMarked, pPrev );
    }
    if ( pPrev == NULL )
        return pRingMarked;
    // remove the fanout from the ring
    pPrev->pLink = NULL;
    // check if the node is marked
    if ( !Ntk_NodeIsTravIdCurrent(pPrev) )
        return pRingMarked;
    // add the node to the list
    pRingMarked = Ntk_NodeRingAdd( pRingMarked, pPrev );
    return pRingMarked;
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Save the leaves.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSaveLeaves( Wn_Window_t * pWnd, Ntk_Ring_t * pRing )
{
    Ntk_Node_t * pNode;
    int i;
    // allocate room for the leaves
    Ntk_RingForEachNode( pRing, pNode, i );
    pWnd->nLeaves = i;
    pWnd->ppLeaves = ALLOC( Ntk_Node_t *, pWnd->nLeaves );
    // save starting from the next one
    pRing = pRing->pLink;
    Ntk_RingForEachNode( pRing, pNode, i )
        pWnd->ppLeaves[i] = pNode;
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Save the roots.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSaveRoots( Wn_Window_t * pWnd, Ntk_Ring_t * pRing )
{
    Ntk_Node_t * pNode;
    int i;
    // allocate room for the roots
    Ntk_RingForEachNode( pRing, pNode, i );
    pWnd->nRoots = i;
    pWnd->ppRoots = ALLOC( Ntk_Node_t *, pWnd->nRoots );
    // save starting from the next one
    pRing = pRing->pLink;
    Ntk_RingForEachNode( pRing, pNode, i )
        pWnd->ppRoots[i] = pNode;
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Save the roots.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSaveInternal( Wn_Window_t * pWnd )
{
    Ntk_Node_t * pNode;
    int i = 0;
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
        i++;
    pWnd->nNodes = i;
    pWnd->ppNodes = ALLOC( Ntk_Node_t *, pWnd->nNodes );
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
        pWnd->ppNodes[i++] = pNode;
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Save the roots.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFilterLeaves( Wn_Window_t * pWnd )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    int fHasFanoutInside;
    int i, k;
    // mark the internal nodes
    Ntk_NetworkIncrementTravId( pWnd->pNet );
    for ( i = 0; i < pWnd->nNodes; i++ )
        Ntk_NodeSetTravIdCurrent( pWnd->ppNodes[i] );
    // look at each leaf and make sure 
    // it has at least one fanout among the internal nodes
    for ( i = 0, k = 0; i < pWnd->nLeaves; i++ )
    {
        fHasFanoutInside = 0;
        Ntk_NodeForEachFanout( pWnd->ppLeaves[i], pPin, pFanout )
            if ( Ntk_NodeIsTravIdCurrent(pFanout) )
            {
                fHasFanoutInside = 1;
                break;
            }
        if ( fHasFanoutInside )
            pWnd->ppLeaves[k++] = pWnd->ppLeaves[i];
    }
    pWnd->nLeaves = k;
}



/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Creates the ring composed of one node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NodeRingCreateFromNode( Ntk_Node_t * pNode )
{
    assert( pNode->pLink == NULL );
    return pNode->pLink = pNode;
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Adds to the end of the ring.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NodeRingAdd( Ntk_Ring_t * pRing, Ntk_Node_t * pNode )
{
    // check that the node is not in a ring
    assert( pNode->pLink == NULL );
    // if the ring is empty, make the node point to itself
    if ( pRing == NULL )
    {
        pNode->pLink = pNode;
        return pNode;
    }
    // if the ring is not empty, add it to the ring after pRing
    pNode->pLink = pRing->pLink;
    pRing->pLink = pNode;
    // return the new ring
    return pNode;
}


/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Deletes from the ring.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NodeRingDelete( Ntk_Ring_t * pRing, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pTemp, * pPrev, * pStop;
    int i;
    // check that the ring and the node are non-trivial
    assert( pRing );
    assert( pNode );
    // check that the node is in the ring
    assert( pNode->pLink );
    // consider the case when the ring contains only one node
    if ( pRing->pLink == pRing )
    {
        assert( pNode->pLink == pNode );
        assert( pNode == pRing );
        pNode->pLink = NULL;
        return NULL;
    }
    // find the node in the ring and its previous node
    for ( pPrev = pRing, pTemp = pStop = pRing->pLink, i = 0; 
          pTemp != pNode && (pTemp != pStop || i == 0); 
          pPrev = pTemp, pTemp = pTemp->pLink, i++ );
    if ( pTemp != pNode )
    { // the node is not in the ring
        assert( 0 );
        return NULL;
    }
    // pPrev is the previous, pNode is the link to be removed
    pPrev->pLink = pNode->pLink;
    pNode->pLink = NULL;
    if ( pRing == pNode )
        return pPrev;
    return pRing;
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Concatenates two rings. Assume that the second ring is shorter.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NodeRingAppend( Ntk_Ring_t * pRing1, Ntk_Ring_t * pRing2 )
{
    Ntk_Node_t * pTemp;
    if ( pRing1 == NULL )
        return pRing2;
    if ( pRing2 == NULL )
        return pRing1;
    // insert the second ring into the first ring
    pTemp = pRing2->pLink;
    pRing2->pLink = pRing1->pLink;
    pRing1->pLink = pTemp;
    return pRing2;
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Concatenates two rings. Assume that the second ring is shorter.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeRingDissolve( Ntk_Ring_t * pRing )
{
    Ntk_Node_t * pNode, * pPrev;
    int i;
    Ntk_RingForEachNodeSafe( pRing, pPrev, pNode, i )
        if ( pPrev ) 
        {
            assert( pPrev->pLink );
            pPrev->pLink = NULL;
        }
    if ( pPrev ) 
    {
        assert( pPrev->pLink );
        pPrev->pLink = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Mark all the nodes in the ring with the current trav ID.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeRingMark( Ntk_Ring_t * pRing )
{
    Ntk_Node_t * pNode;
    int i;
    Ntk_RingForEachNode( pRing, pNode, i )
        Ntk_NodeSetTravIdCurrent( pNode );
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Make sure all the nodes in the network are ringless.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeRingPrint( Ntk_Ring_t * pRing, char * pComment )
{
    Ntk_Node_t * pNode;
    int i;
    fprintf( stdout, "%s: {", pComment );
    if ( pRing )
    {
        pRing = pRing->pLink;
        Ntk_RingForEachNode( pRing, pNode, i )
            fprintf( stdout, " %s", Ntk_NodeGetNamePrintable( pNode ) );
    }
    fprintf( stdout, " }\n" );
}



/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Create the ring from the nodes in the current list.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Ring_t * Ntk_NetworkRingCreateFromSpecial( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Ntk_Ring_t * pRing = NULL;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        Ntk_NodeRingAdd( pRing, pNode );
    return pRing;
}

/**Function*************************************************************

  Synopsis    [Ring manipulation procedures.]

  Description [Make sure all the nodes in the network are ringless.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkRingCheck( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Ntk_NetworkForEachNode( pNet, pNode )
        assert( pNode->pLink == NULL );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


