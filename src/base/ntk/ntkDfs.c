/**CFile****************************************************************

  FileName    [ntkDfs.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures to put/get/traverse nodes in the DFS order.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkDfs.c,v 1.37 2004/10/15 05:41:02 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ntk_NetworkDfsFromOutputs_rec( Ntk_Node_t * pNode );
static void Ntk_NetworkDfsFromInputs_rec( Ntk_Node_t * pNode );
static void Ntk_NetworkMarkFanouts_rec( Ntk_Node_t * pNode );
static void Ntk_NetworkDfsNodeTfo_rec( Ntk_Node_t * pNode );
static int  Ntk_NetworkLevelizeFromOutputs_rec( Ntk_Node_t * pNode );
static int  Ntk_NetworkLevelizeFromInputs_rec( Ntk_Node_t * pNode );
static void Ntk_NetworkCountVisits_rec( Ntk_Node_t * pNode );
static int  Ntk_NetworkGetNumLevels_rec( Ntk_Node_t * pNode );
static bool Ntk_NetworkIsAcyclic1_rec( Ntk_Node_t * pNode, st_table * tPath );
static bool Ntk_NetworkIsAcyclic2_rec( Ntk_Node_t * pNode, Ntk_Node_t ** pPath, int Step );
static bool Ntk_NetworkIsAcyclic_rec( Ntk_Node_t * pNode );
static int  Ntk_NetworkComputeNodeSupport_rec( Ntk_Node_t * pNode );
static int  Ntk_NetworkInterleaveNodes_rec( Ntk_Node_t * pNode );
static int  Ntk_NetworkComputeNodeTfi_rec( Ntk_Node_t * pNode, int Depth, bool fIncludeCis, bool fExistPath );
static int  Ntk_NetworkComputeNodeTfo_rec( Ntk_Node_t * pNode, int Depth, bool fIncludeCos, bool fExistPath );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Links the nodes of the network in the DFS order.]

  Description [The nodes that do not fanout into the COs are not collected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDfs( Ntk_Network_t * pNet, bool fFromOutputs )
{
    Ntk_Node_t * pNode;

    // set the traversal ID
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    // traverse and link the nodes
    if ( fFromOutputs )
    { // start DFS from the primary outputs
        Ntk_NetworkForEachCo( pNet, pNode )
            Ntk_NetworkDfsFromOutputs_rec( pNode );
    }
    else
    { // start DFS from the primary inputs
        Ntk_NetworkForEachCi( pNet, pNode )
            Ntk_NetworkDfsFromInputs_rec( pNode );
    }
    // finalize the linked list
    Ntk_NetworkStopSpecial( pNet );
}

/**Function*************************************************************

  Synopsis    [Links the nodes of the network in the DFS order.]

  Description [The nodes that do not fanout into the COs are not collected.
  Only consideres the nodes in the TFI/TFO cones of latches.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDfsLatches( Ntk_Network_t * pNet, bool fFromOutputs )
{
    Ntk_Node_t * pNode;

    // set the traversal ID
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    // traverse and link the nodes
    if ( fFromOutputs )
    { // start DFS from the primary outputs
        Ntk_NetworkForEachCo( pNet, pNode )
            if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
                Ntk_NetworkDfsFromOutputs_rec( pNode );
    }
    else
    { // start DFS from the primary inputs
        Ntk_NetworkForEachCi( pNet, pNode )
            if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
                Ntk_NetworkDfsFromInputs_rec( pNode );
    }
    // finalize the linked list
    Ntk_NetworkStopSpecial( pNet );
}

/**Function*************************************************************

  Synopsis    [Links the nodes reachable in the DFS order from the given nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDfsNodes( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[], int nNodes, int fFromOutputs )
{
    int i;

    // set the traversal ID for this DFS ordering
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    // traverse and link the nodes
    if ( fFromOutputs )
    {
        for ( i = 0; i < nNodes; i++ )
            Ntk_NetworkDfsFromOutputs_rec( ppNodes[i] );
    }
    else
    {
        for ( i = 0; i < nNodes; i++ )
            Ntk_NetworkDfsFromInputs_rec( ppNodes[i] );
    }
    // finalize the linked list
    Ntk_NetworkStopSpecial( pNet );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDfsFromOutputs_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;

    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return;

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );

    // visit the transitive fanin
    if ( pNode->Type != MV_NODE_CI )
        Ntk_NodeForEachFanin( pNode, pLink, pFanin )
            Ntk_NetworkDfsFromOutputs_rec( pFanin );

    // add the node after the fanins have been added
    Ntk_NetworkAddToSpecial( pNode );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDfsFromInputs_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pLink;

    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return;

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );

    // visit the transitive fanout
    if ( pNode->Type != MV_NODE_CO )
        Ntk_NodeForEachFanout( pNode, pLink, pFanout )
            Ntk_NetworkDfsFromInputs_rec( pFanout );

    // add the node after the fanouts have been added
    Ntk_NetworkAddToSpecial( pNode );
}


/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of nodes in the TFO of the node.]

  Description [This procedure collects all the nodes in the TFO cone
  of the given node. The result is the list of these nodes ordered
  in the DFS fashion from the given node to the POs that belong to the
  TFO cone. The nodes ordered this way are useful to update the global
  BDDs of the network after the given node has been changed. This procedure
  gracefully treats the case when the TFO cone includes the nodes that
  do not fanout into the POs. The BDDs of such nodes are updated.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDfsFromNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    Ntk_Node_t ** ppNodes;
    int nNodes, i;

    assert( pNode->Type == MV_NODE_INT );
    // set the new traversal ID
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list to collect the nodes that do not fanout
    Ntk_NetworkStartSpecial( pNet );
    // mark the TFO and collect the nodes that do not fanout
    Ntk_NetworkMarkFanouts_rec( pNode );

    // save these nodes into array
    nNodes = Ntk_NetworkCountSpecial(pNet);
//    ppNodes = ALLOC( Ntk_Node_t *, nNodes );
    assert( nNodes <= pNet->nArraysAlloc );
    ppNodes = pNode->pNet->pArray1;
    Ntk_NetworkCreateArrayFromSpecial( pNet, ppNodes );

    // set the new traversal ID
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list to collect the TFO nodes in the DFS order
    Ntk_NetworkStartSpecial( pNet );
    // starting from the nodes in the TFO that do not fanout
    // collect the TFO in the DFS order
    for ( i = 0; i < nNodes; i++ )
        Ntk_NetworkDfsNodeTfo_rec( ppNodes[i] );
//    FREE( ppNodes );
}


/**Function*************************************************************

  Synopsis    [Links the nodes of the network in the DFS order by level.]

  Description [The nodes that do not fanout into the COs are not collected.
  The nodes are ordered by level.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDfsByLevel( Ntk_Network_t * pNet, bool fFromOutputs )
{
    Ntk_Node_t * pNode;
    Ntk_Node_t ** ppNodes;
    int nNodes, Level, iNode, k;
    // levelize
    Ntk_NetworkLevelize( pNet, fFromOutputs );
    // put the nodes into an array
    nNodes = Ntk_NetworkReadNodeIntNum(pNet) + Ntk_NetworkReadCiNum(pNet) + Ntk_NetworkReadCoNum(pNet);
    ppNodes = ALLOC( Ntk_Node_t *, nNodes );
    iNode = 0;
    for ( Level = 0; Level <= pNet->nLevels + 1; Level++ )
        Ntk_NetworkForEachNodeSpecialByLevel( pNet, Level, pNode )
//            if ( Ntk_NodeIsInternal(pNode) )
                ppNodes[iNode++] = pNode;
    assert( iNode <= nNodes );

    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    for ( k = 0; k < iNode; k++ )
        Ntk_NetworkAddToSpecial( ppNodes[k] );
    // finalize the linked list
    Ntk_NetworkStopSpecial( pNet );
    FREE( ppNodes );
}


/**Function*************************************************************

  Synopsis    [Collects the nodes in the TFO of the node.]

  Description [Assumes that the nodes are not marked. Marks them.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkMarkFanouts_rec( Ntk_Node_t * pNode )
{
	Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );
    // add the node if it has no fanouts
    if ( Ntk_NodeReadFanoutNum(pNode) == 0 )
    {
        Ntk_NetworkAddToSpecial( pNode );
        return;
    }
    // visit the fanouts
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        Ntk_NetworkMarkFanouts_rec( pFanout );
}


/**Function*************************************************************

  Synopsis    [Collects the marked nodes in the DFS order]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDfsNodeTfo_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return;
    // if this node is not visited in the previous traversal, skip it
    if ( !Ntk_NodeIsTravIdPrevious( pNode ) )
        return;
    assert( pNode->Type != MV_NODE_CI );
    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );
    // visit the fanins
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        Ntk_NetworkDfsNodeTfo_rec( pFanin );
    // add the node after the fanins have been added
    Ntk_NetworkAddToSpecial( pNode );
}



/**Function*************************************************************

  Synopsis    [Levelizes the network.]

  Description [This procedure connects the nodes into linked lists 
  according to their levels. It returns the number of levels. The nodes
  in the levelized network can be traversed using iterators
  Ntk_NetworkForEachNodeByLevel.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkLevelize( Ntk_Network_t * pNet, int fFromOutputs )
{
    Ntk_Node_t * pNode;
    // free the previous array of pointers to levels
    if ( pNet->nLevels )
        FREE( pNet->ppLevels );
    // get the new number of levels
    pNet->nLevels = Ntk_NetworkGetNumLevels( pNet );
    // allocate the array of pointers
    pNet->ppLevels = ALLOC( Ntk_Node_t *, pNet->nLevels + 2 );
    memset( pNet->ppLevels, 0, sizeof(Ntk_Node_t *) * (pNet->nLevels + 2) );

    // set the traversal ID for this traversal
    Ntk_NetworkIncrementTravId( pNet );

    // traverse the network and add the nodes to the levelized structure
    if ( fFromOutputs )
    {
        Ntk_NetworkForEachCo( pNet, pNode )
            Ntk_NetworkLevelizeFromOutputs_rec( pNode );
    }
    else
    {
        Ntk_NetworkForEachCi( pNet, pNode )
            Ntk_NetworkLevelizeFromInputs_rec( pNode );
    }

    return pNet->nLevels;
}

/**Function*************************************************************

  Synopsis    [Levelizes the TFI/TFO cone starting from the nodes.]

  Description [This procedure connects the nodes into linked lists 
  according to their levels. It returns the number of levels. The nodes
  in the levelized network can be traversed using iterators
  Ntk_NetworkForEachNodeByLevel.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkLevelizeNodes( Ntk_Node_t * ppNodes[], int nNodes, int fFromOutputs )
{
    Ntk_Network_t * pNet;
    int i;

    assert( nNodes > 0 );
    pNet = ppNodes[0]->pNet;
    // free the previous array of pointers to levels
    if ( pNet->nLevels )
        FREE( pNet->ppLevels );
    // get the new number of levels
    pNet->nLevels = Ntk_NetworkGetNumLevels( pNet );
    // allocate the array of pointers
    pNet->ppLevels = ALLOC( Ntk_Node_t *, pNet->nLevels + 2 );
    memset( pNet->ppLevels, 0, sizeof(Ntk_Node_t *) * (pNet->nLevels + 2) );

    // set the traversal ID for this traversal
    Ntk_NetworkIncrementTravId( pNet );

    // traverse the network and add the nodes to the levelized structure
    if ( fFromOutputs )
    {
        for ( i = 0; i < nNodes; i++ )
            Ntk_NetworkLevelizeFromOutputs_rec( ppNodes[i] );
    }
    else
    {
        for ( i = 0; i < nNodes; i++ )
            Ntk_NetworkLevelizeFromInputs_rec( ppNodes[i] );
    }

    return pNet->nLevels;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkLevelizeFromOutputs_rec( Ntk_Node_t * pNode )
{
    int LevelsMax, LevelsCur;
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;

    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return pNode->Level;

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );

    // if this is a PI node, attach it and return 0 levels
    if ( pNode->Type == MV_NODE_CI )
    {  // set the level of this node
        pNode->Level = 0;
    }
    else
    {  // visit the transitive fanin
        LevelsMax = -1;
        Ntk_NodeForEachFanin( pNode, pLink, pFanin )
        {
            LevelsCur = Ntk_NetworkLevelizeFromOutputs_rec( pFanin );
            if ( LevelsMax < LevelsCur )
                LevelsMax = LevelsCur;
        }
        // set the level of this node
        pNode->Level = LevelsMax + 1;
    }

    // attach the node to the level number "pNode->Level"
    pNode->pOrder = pNode->pNet->ppLevels[pNode->Level];
    pNode->pNet->ppLevels[pNode->Level] = pNode;

    return pNode->Level;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkLevelizeFromInputs_rec( Ntk_Node_t * pNode )
{
    int LevelsMax, LevelsCur;
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pLink;

    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return pNode->Level;

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );

    // if this is a PI node, attach it and return 0 levels
    if ( pNode->Type == MV_NODE_CO )
    {  // set the level of this node
        pNode->Level = 0;
    }
    else
    {  // visit the transitive fanout
        LevelsMax = -1;
        Ntk_NodeForEachFanout( pNode, pLink, pFanout )
        {
            LevelsCur = Ntk_NetworkLevelizeFromInputs_rec( pFanout );
            if ( LevelsMax < LevelsCur )
                LevelsMax = LevelsCur;
        }
        // set the level of this node
        pNode->Level = LevelsMax + 1;
    }

    // attach the node to the level number "pNode->Level"
    pNode->pOrder = pNode->pNet->ppLevels[pNode->Level];
    pNode->pNet->ppLevels[pNode->Level] = pNode;

    return pNode->Level;
}


/**Function*************************************************************

  Synopsis    [Counts the number of visits to each node from the COs.]

  Description [This procedure sets pNode->Level counter to be equal
  to the number of visits of this node from its fanouts, starting
  from the CO. If the fanouts contain nodes that do not fanout, this
  procedure will only count the number of visits from the fanouts
  that fanout.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkCountVisits( Ntk_Network_t * pNet, int fLatchOnly )
{
    Ntk_Node_t * pNode;

    // get the counter of visits of each node to 0
    Ntk_NetworkForEachCi( pNet, pNode )
        pNode->Level = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        pNode->Level = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        pNode->Level = 0;

    // perform the traversal
    Ntk_NetworkForEachCo( pNet, pNode )
        if ( !fLatchOnly || pNode->Subtype == MV_BELONGS_TO_LATCH )
            Ntk_NetworkCountVisits_rec( pNode );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkCountVisits_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;
    // skip the node that was already visited
    if ( pNode->Level++ )
        return;
    // visit the transitive fanin
    Ntk_NodeForEachFanin( pNode, pLink, pFanin )
        Ntk_NetworkCountVisits_rec( pFanin );
}

/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkGetNumLevels( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int LevelsMax, LevelsCur;

    // set the traversal ID for this traversal
    Ntk_NetworkIncrementTravId( pNet );

    // perform the traversal
    LevelsMax = -1;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        LevelsCur = Ntk_NetworkGetNumLevels_rec( Ntk_NodeReadFaninNode(pNode,0) );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
        pNode->Level = LevelsCur;
    }
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkGetNumLevels_rec( Ntk_Node_t * pNode )
{
    int LevelsMax, LevelsCur;
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;

    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return pNode->Level;

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );

    // if this is a PI node, return 0 levels
    if ( pNode->Type == MV_NODE_CI )
    {
        pNode->Level = 0;
        return 0;
    }

    // visit the transitive fanin
    LevelsMax = -1;
    Ntk_NodeForEachFanin( pNode, pLink, pFanin )
    {
        LevelsCur = Ntk_NetworkGetNumLevels_rec( pFanin );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }

    // set the number of levels
    pNode->Level = LevelsMax + 1;
    return pNode->Level;
}

/**Function*************************************************************

  Synopsis    [Detects combinational loops.]

  Description [This procedure is based on the idea suggested by Donald Chai. 
  As we traverse the network and visit the nodes, we need to distinquish 
  three types of nodes: (1) those that are visited for the first time, 
  (2) those that have been visited in this traversal but are currently not 
  on the traversal path, (3) those that have been visited and are currently 
  on the travesal path. When the node of type (3) is encountered, it means 
  that there is a combinational loop. To mark the three types of nodes, 
  two new values of the traversal IDs are used.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkIsAcyclic( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int fAcyclic;

    // set the traversal ID for this DFS ordering
    pNet->nTravIds += 2;   
    // pNode->TravId == pNet->nTravIds      means "pNode is on the path"
    // pNode->TravId == pNet->nTravIds - 1  means "pNode is visited but is not on the path"
    // pNode->TravId < pNet->nTravIds - 1   means "pNode is not visited"

    // traverse the network to detect cycles
    fAcyclic = 1;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( Ntk_NodeReadFanoutNum( pNode ) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkIsAcyclic(): Combinational output \"%s\" has fanouts! This should not happen.\n", pNode->pName );
            continue;

        }
        // traverse the output logic cone to detect the combinational loops
        if ( (fAcyclic = Ntk_NetworkIsAcyclic_rec( pNode )) == 0 ) 
        { // stop as soon as the first loop is detected
            fprintf( Ntk_NetworkReadMvsisOut(pNet), " (the output node)\n" );
            break;
        }
    }
    return fAcyclic;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkIsAcyclic_rec( Ntk_Node_t * pNode )
{
    Ntk_Network_t * pNet = pNode->pNet;
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;
    int fAcyclic;

    // make sure the node is not visited
    assert( pNode->TravId != pNet->nTravIds - 1 );
    // check if the node is part of the combinational loop
    if ( pNode->TravId == pNet->nTravIds )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Network \"%s\" contains combinational loop!\n", pNet->pName );
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Node \"%s\" is encountered twice on the following path:\n", Ntk_NodeGetNameLong(pNode) );
        fprintf( Ntk_NetworkReadMvsisOut(pNet), " %s", Ntk_NodeGetNameLong(pNode) );
        return 0;
    }
    // mark this node as a node on the current path
    pNode->TravId = pNet->nTravIds;
    // visit the transitive fanin
    Ntk_NodeForEachFanin( pNode, pLink, pFanin )
    {
        // make sure there is no mixing of networks
        assert( pFanin->pNet == pNode->pNet );

        // check if the fanin is visited
        if ( pFanin->TravId == pNet->nTravIds - 1 ) 
            continue;
        // traverse searching for the loop
        fAcyclic = Ntk_NetworkIsAcyclic_rec( pFanin );
        // return as soon as the loop is detected
        if ( fAcyclic == 0 ) 
        {
            if ( !Ntk_NodeHasCoName(pNode) )
                fprintf( Ntk_NetworkReadMvsisOut(pNet), "  <--  %s", Ntk_NodeGetNameLong(pNode) );
            return 0;
        }
    }
    // mark this node as a visited node
    pNode->TravId = pNet->nTravIds - 1;
    return 1;
}



/**Function*************************************************************

  Synopsis    [Detects combinational loops.]

  Description [This procedure uses the hash table similar to SIS.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkIsAcyclic1( Ntk_Network_t * pNet )
{
    st_table * tPath;
    Ntk_Node_t * pNode;
    int fAcyclic;

    tPath = st_init_table( st_ptrcmp, st_ptrhash );
    // set the traversal ID for this DFS ordering
    Ntk_NetworkIncrementTravId( pNet );
    // traverse the network to detect cycles
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        fAcyclic = Ntk_NetworkIsAcyclic1_rec( pNode, tPath );
        if ( fAcyclic == 0 ) // return as soon as the loop is detected
            break;
    }
    st_free_table( tPath );

    return fAcyclic;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkIsAcyclic1_rec( Ntk_Node_t * pNode, st_table * tPath )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;
    int fAcyclic;

    // if this node is already visited, make sure it is not on the path
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
    {
        if ( st_is_member( tPath, (char*)pNode ) )
            return 0;
        return 1;
    }

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );

    // if this is a PI node, return 1 because a PI cannot be in the loop
    if ( pNode->Type == MV_NODE_CI )
        return 1;

    // put the current node on the path
    st_insert( tPath, (char *)pNode, NULL );

    // visit the transitive fanin
    Ntk_NodeForEachFanin( pNode, pLink, pFanin )
    {
        // traverse searching for the loop
        fAcyclic = Ntk_NetworkIsAcyclic1_rec( pFanin, tPath );
        // return as soon as the loop is detected
        if ( fAcyclic == 0 ) 
            return 0;
    }

    // remove the current node from the path
    st_delete( tPath, (char **)&pNode, NULL );

    return 1;
}

/**Function*************************************************************

  Synopsis    [Detects combinational loops.]

  Description [This procedure does not use the hash table (like the one in SIS).
  It uses the array instead. Several hash table lookups per call may be worth 
  iterating through the array if the array is not that large. Even if the number
  of logic level is substantial, this procedure should not be very slow.
  The above conclusion is confirmed experimentally. Even for very "deep" 
  networks (many logic levels) this procedure is not much slower, but for
  some shallow networks it can even be faster than Ntk_NetworkIsAcyclic1().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkIsAcyclic2( Ntk_Network_t * pNet )
{
    Ntk_Node_t ** pPath;
    Ntk_Node_t * pNode;
    int fAcyclic;

    // allocate place where the nodes on the current path are stored
    pPath = ALLOC( Ntk_Node_t *, Ntk_NetworkGetNumLevels(pNet) + 2 );
    // set the traversal ID for this DFS ordering
    Ntk_NetworkIncrementTravId( pNet );

    // traverse the network to detect cycles
    fAcyclic = 1;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        fAcyclic = Ntk_NetworkIsAcyclic2_rec( pNode, pPath, 0 );
        if ( fAcyclic == 0 ) // return as soon as the loop is detected
            break;
    }
    free( pPath );
    return fAcyclic;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkIsAcyclic2_rec( Ntk_Node_t * pNode, Ntk_Node_t ** pPath, int Step )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;
    int i;

    // if this node is already visited, make sure it is not on the path
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
    {
        for ( i = 0; i < Step; i++ )
            if ( pNode == pPath[i] )
                return 0;
        return 1;
    }

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );

    // if this is a PI node, return 1 because a PI cannot be in the loop
    if ( pNode->Type == MV_NODE_CI )
        return 1;

    // put the current node on the path
    pPath[Step] = pNode;

    // visit the transitive fanin
    Ntk_NodeForEachFanin( pNode, pLink, pFanin )
    {
        // return as soon as the loop is detected
        if ( !Ntk_NetworkIsAcyclic2_rec( pFanin, pPath, Step + 1 ) ) 
            return 0;
    }

    // remove the current node from the path
    pPath[Step] = pNode;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Computes the combinational support of the given nodes.]

  Description [Computes the combinational support by the DFS searh
  from the given nodes. The nodes are linked into the list pNet->pOrder,
  which can be traversed Ntk_NetworkForEachNodeSpecial. The return values 
  in the number of CI nodes in the combinational support.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkComputeNodeSupport( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes )
{
    int nNodesRes, i;

    // set the traversal ID for this DFS ordering
    Ntk_NetworkIncrementTravId( pNet );

    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    // traverse the TFI cones of all nodes in the array
    nNodesRes = 0;
    for ( i = 0; i < nNodes; i++ )
        nNodesRes += Ntk_NetworkComputeNodeSupport_rec( pNodes[i] );
    // finalize the linked list
    Ntk_NetworkStopSpecial( pNet );

    return nNodesRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkComputeNodeSupport_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;
    int nNodes;

    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
        return 0;

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );

    // visit the transitive fanin
    nNodes = 0;
    if ( pNode->Type != MV_NODE_CI )
        Ntk_NodeForEachFanin( pNode, pLink, pFanin )
            nNodes += Ntk_NetworkComputeNodeSupport_rec( pFanin );

    // add the node to the list only if it a combinational input
    if ( pNode->Type == MV_NODE_CI )
    {
        Ntk_NetworkAddToSpecial( pNode );
        // increment the number of nodes in the list
        nNodes++;
    }
    return nNodes;
}


/**Function*************************************************************

  Synopsis    [Computes the interleaved ordering of nodes in the network.]

  Description [Interleaves the nodes using the algorithmm described
  in the paper: Fujii et al., "Interleaving Based Variable Ordering 
  Methods for OBDDs", ICCAD 1993.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkInterleave( Ntk_Network_t * pNet )
{
    Ntk_Node_t ** ppNodesCo;
    Ntk_Node_t * pNode;
    int nNodesRes, nNodes, i;
    // order COs by size
    nNodes = Ntk_NetworkReadCoNum(pNet);
    ppNodesCo = Ntk_NetworkOrderCoNodesBySize( pNet );
    // set the traversal ID for this DFS ordering
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    // traverse the TFI cones of all nodes in the network
    nNodesRes = 0;
    for ( i = 0; i < nNodes; i++ )
        nNodesRes += Ntk_NetworkInterleaveNodes_rec( ppNodesCo[i] );
//    Ntk_NetworkForEachCo( pNet, pNode )
//        nNodesRes += Ntk_NetworkInterleaveNodes_rec( pNode );
    FREE( ppNodesCo );
    // add the CI nodes that are not reachable from the COs
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        // if this node is already visited, skip
        if ( Ntk_NodeIsTravIdCurrent(pNode) )
            continue;
        // add the node to the list 
        Ntk_NetworkAddToSpecial( pNode );
        nNodesRes++;
    }
    // finalize the linked list
    Ntk_NetworkStopSpecial( pNet );
//    Ntk_NetworkPrintSpecial( pNet );
    return nNodesRes;
}

/**Function*************************************************************

  Synopsis    [Computes the interleaved ordering of nodes in the network.]

  Description [Interleaves the nodes using the algorithmm described
  in the paper: Fujii et al., "Interleaving Based Variable Ordering 
  Methods for OBDDs", ICCAD 1993.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkInterleaveNodes( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes )
{
    int nNodesRes, i;
    // set the traversal ID for this DFS ordering
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    // traverse the TFI cones of all nodes in the array
    nNodesRes = 0;
    for ( i = 0; i < nNodes; i++ )
        nNodesRes += Ntk_NetworkInterleaveNodes_rec( pNodes[i] );
    // finalize the linked list
    Ntk_NetworkStopSpecial( pNet );
//    Ntk_NetworkPrintSpecial( pNet );
    return nNodesRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkInterleaveNodes_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;
    int nNodes;

    // if this node is already visited, skip
    if ( Ntk_NodeIsTravIdCurrent(pNode) )
    {
        // move the insertion point to be after this node
        Ntk_NetworkMoveSpecial( pNode );
        return 0;
    }
    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );

    // visit the transitive fanin
    nNodes = 0;
    if ( pNode->Type != MV_NODE_CI )
        Ntk_NodeForEachFanin( pNode, pLink, pFanin )
            nNodes += Ntk_NetworkInterleaveNodes_rec( pFanin );

    // add the node to the list 
    Ntk_NetworkAddToSpecial( pNode );
    nNodes++;
    return nNodes;
}


/**Function*************************************************************

  Synopsis    [Computes the transitive fanin nodes of the given node.]

  Description [Computes the transitive fanin nodes by the DFS search
  from the given nodes. The return value in the number of nodes in the 
  TFI cones. The depth 0 returns only the nodes in the array. The depth 1 
  returns the first logic level of fanins of the given nodes, and so on. 
  The nodes are linked into the internal list, which can be traversed by 
  Ntk_NetworkForEachNodeSpecial. The CI nodes are included if the flag 
  fIncludeCis is set to 1. If the flag fExistPath is 1, the set of all nodes 
  is computed such that there is exist a path of the given length (Depth) 
  or shorter, If this flag is 0, the set of all node is computes such that 
  any path to them is the given length (Depth) or shorter. Note that
  if fExistPath is 0 the complexity of this procedure is higher. Therefore
  in those applications where it makes no difference, fExistPath = 1 is 
  preferable.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkComputeNodeTfi( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes, int Depth, bool fIncludeCis, bool fExistPath )
{
    Ntk_Node_t * pNode, * pNode2;
    int nNodesRes, DepthReal, i;
    bool fFirst;
    // set the traversal ID
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    // start DFS from the primary outputs
    DepthReal = Depth + (!fExistPath) * 1000;
    nNodesRes = 0;
    for ( i = 0; i < nNodes; i++ )
        nNodesRes += Ntk_NetworkComputeNodeTfi_rec( pNodes[i], DepthReal, fIncludeCis, fExistPath );
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

  Synopsis    [Performs the recursive step of Ntk_NetworkComputeNodeTfi().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkComputeNodeTfi_rec( Ntk_Node_t * pNode, int Depth, bool fIncludeCis, bool fExistPath )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;
    int nNodes;

    // if the depth is out, skip
    if ( Depth == -1 )
        return 0;

    // if this node is already visited, check the level
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
    {
        nNodes = 0;
        if ( fExistPath && pNode->Level < Depth || (!fExistPath) && pNode->Level > Depth )
        {
            // update the path to be shortest possible
            pNode->Level = Depth;
            // visit the transitive fanin
            if ( pNode->Type != MV_NODE_CI )
                Ntk_NodeForEachFanin( pNode, pLink, pFanin )
                    nNodes += Ntk_NetworkComputeNodeTfi_rec( pFanin, Depth - 1, fIncludeCis, fExistPath );
        }
        return nNodes;
    }

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );
    // set the depth
    pNode->Level = Depth;

    // visit the transitive fanin
    nNodes = 0;
    if ( pNode->Type != MV_NODE_CI )
        Ntk_NodeForEachFanin( pNode, pLink, pFanin )
            nNodes += Ntk_NetworkComputeNodeTfi_rec( pFanin, Depth - 1, fIncludeCis, fExistPath );

    // add the node after the fanins are added
    if ( pNode->Type != MV_NODE_CI || fIncludeCis )
    {
        Ntk_NetworkAddToSpecial( pNode );
        nNodes++;
    }
    return nNodes;
}


/**Function*************************************************************

  Synopsis    [Computes the transitive fanout nodes of the given node.]

  Description [Computes the transitive fanout nodes by the DFS search
  from the given nodes. The return value in the number of nodes in the 
  TFI cones. The depth 0 returns only the nodes in the array. The depth 1 
  returns the first logic level of fanouts of the given nodes, and so on. 
  The nodes are linked into the internal list, which can be traversed by 
  Ntk_NetworkForEachNodeSpecial. The CO nodes are included if the flag 
  fIncludeCos is set to 1. If the flag fExistPath is 1, the set of all nodes 
  is computed such that there is exist a path of the given length (Depth) 
  or shorter, If this flag is 0, the set of all node is computes such that 
  any path to them is the given length (Depth) or shorter. Note that
  if fExistPath is 0 the complexity of this procedure is higher. Therefore
  in those applications where it makes no difference, fExistPath = 1 is 
  preferable.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkComputeNodeTfo( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes, int Depth, bool fIncludeCos, bool fExistPath )
{
    Ntk_Node_t * pNode, * pNode2;
    int nNodesRes, DepthReal, i;
    bool fFirst;
    // set the traversal ID
    Ntk_NetworkIncrementTravId( pNet );
    // start the linked list
    Ntk_NetworkStartSpecial( pNet );
    // start DFS from the primary outputs
    DepthReal = Depth + (!fExistPath) * 1000;
    nNodesRes = 0;
    for ( i = 0; i < nNodes; i++ )
        nNodesRes += Ntk_NetworkComputeNodeTfo_rec( pNodes[i], DepthReal, fIncludeCos, fExistPath );
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

  Synopsis    [Performs the recursive step of Ntk_NetworkComputeNodeTfo().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkComputeNodeTfo_rec( Ntk_Node_t * pNode, int Depth, bool fIncludeCos, bool fExistPath )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pLink;
    int nNodes;

    // if the depth is out, skip
    if ( Depth == -1 )
        return 0;

    // if this node is already visited, check the level
    if ( Ntk_NodeIsTravIdCurrent( pNode ) )
    {
        nNodes = 0;
        if ( fExistPath && pNode->Level < Depth || (!fExistPath) && pNode->Level > Depth )
        {
            // update the path to be shortest possible
            pNode->Level = Depth;
            // visit the transitive fanin
            if ( pNode->Type != MV_NODE_CO )
                Ntk_NodeForEachFanout( pNode, pLink, pFanout )
                    nNodes += Ntk_NetworkComputeNodeTfo_rec( pFanout, Depth - 1, fIncludeCos, fExistPath );
        }
        return nNodes;
    }

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent( pNode );
    // set the depth
    pNode->Level = Depth;

    // visit the transitive fanin
    nNodes = 0;
    if ( pNode->Type != MV_NODE_CO )
        Ntk_NodeForEachFanout( pNode, pLink, pFanout )
            nNodes += Ntk_NetworkComputeNodeTfo_rec( pFanout, Depth - 1, fIncludeCos, fExistPath );

    // add the node after the fanouts are added
    if ( pNode->Type != MV_NODE_CO || fIncludeCos )
    {
        Ntk_NetworkAddToSpecial( pNode );
        nNodes++;
    }
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node has a CO in its TFO logic cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeHasCoInTfo( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;

    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
    {
        if ( Ntk_NodeIsCo( pFanout ) )
            return 1;
        if ( Ntk_NodeHasCoInTfo( pFanout ) )
            return 1;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


