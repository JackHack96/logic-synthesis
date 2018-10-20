/**CFile****************************************************************

  FileName    [wnDfs.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [DFS ordering of the window's nodes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wnDfs.c,v 1.1 2003/11/18 18:55:02 alanmi Exp $]

***********************************************************************/

#include "wnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Wn_WindowDfs_rec( Ntk_Node_t * pNode );
static int  Wn_WindowInterleave_rec( Ntk_Node_t * pNode );
static int  Wn_WindowGetNumLevels_rec( Ntk_Node_t * pNode );
static int  Wn_WindowLevelizeFromInputs_rec( Ntk_Node_t * pNode );
static int  Wn_WindowLevelizeFromOutputs_rec( Ntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [DFS-order the nodes in the window.]

  Description [Roots are included, leaves are not included. Returns 0
  if the window is not specified correctly, that is, if there is a path
  from the roots to the CIs bypassing leaves.]
               
  SideEffects [Sets the trav IDs of the window nodes to the current one.]

  SeeAlso     []

***********************************************************************/
int Wn_WindowDfs( Wn_Window_t * pWnd )
{
    int RetValue, i;

    // mark the leaves - this way they will not be collected
    Ntk_NetworkIncrementTravId( pWnd->pNet );
    for ( i = 0; i < pWnd->nLeaves; i++ )
        Ntk_NodeSetTravIdCurrent( pWnd->ppLeaves[i] );

    // start the DFS and collect the nodes
    Ntk_NetworkStartSpecial( pWnd->pNet );
    RetValue = 1;
    for ( i = 0; i < pWnd->nRoots; i++ )
        if ( !Wn_WindowDfs_rec( pWnd->ppRoots[i] ) )
        {
            RetValue = 0;
            break; // detected window inconsistency  
        }
    Ntk_NetworkStopSpecial( pWnd->pNet );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs the recursive step of ordering the leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowDfs_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;

    if ( Ntk_NodeIsTravIdCurrent(pNode) ) // visited node
        return 1;
    // if the node is a CI, it is an inconsistency
    if ( Ntk_NodeIsCi(pNode) )
        return 0;

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent(pNode);

    // otherwise, call recursively
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        if ( !Wn_WindowDfs_rec(pFanin) )
            return 0;

    // collect this node
    Ntk_NetworkAddToSpecial( pNode );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Interleave-order the nodes in the window.]

  Description [Both roots and leaves are included. Returns 0
  if the window is not specified correctly, that is, if there is a path
  from the roots to the CIs bypassing leaves.]
               
  SideEffects [Sets the trav IDs of the window nodes to the current one.]

  SeeAlso     []

***********************************************************************/
int Wn_WindowInterleave( Wn_Window_t * pWnd )
{
    Ntk_Node_t * pNode;
    int RetValue, i;

    // collect the nodes and clean their Levels
    Wn_WindowDfs( pWnd );
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
        Ntk_NodeSetLevel( pNode, 0 );

    // mark the leaves
    for ( i = 0; i < pWnd->nLeaves; i++ )
        Ntk_NodeSetLevel( pWnd->ppLeaves[i], 1 );

    // start the DFS and collect the nodes in the interleaved ordering from roots
    Ntk_NetworkIncrementTravId( pWnd->pNet );
    Ntk_NetworkStartSpecial( pWnd->pNet );
    RetValue = 1;
    for ( i = 0; i < pWnd->nRoots; i++ )
        if ( !Wn_WindowInterleave_rec( pWnd->ppRoots[i] ) )
        {
            RetValue = 0;
            break; // detected window inconsistency  
        }
    Ntk_NetworkStopSpecial( pWnd->pNet );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs the recursive step of ordering the leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowInterleave_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;

    // detect a visited node
    if ( Ntk_NodeIsTravIdCurrent(pNode) ) // visited node
    {
        if ( Ntk_NodeReadLevel(pNode) ) // visited leaf - move insertion point
            Ntk_NetworkMoveSpecial( pNode );
        return 1;
    }
    // mark the node as visited
    Ntk_NodeSetTravIdCurrent(pNode);

    if ( Ntk_NodeReadLevel(pNode) ) // unvisited leaf
    {
        // collect this node
        Ntk_NetworkAddToSpecial( pNode );
        return 1;
    }

    // if the node is a CI, it is an inconsistency
    if ( Ntk_NodeIsCi(pNode) )
        return 0;

    // otherwise, call recursively
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        if ( !Wn_WindowInterleave_rec(pFanin) )
            return 0;

    // collect this node
    Ntk_NetworkAddToSpecial( pNode );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Collects the internal nodes of the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowCollectInternal( Wn_Window_t * pWnd )
{
    Ntk_Node_t * pNode;
    int RetValue, i;

    // DFS order the nodes in the window
    RetValue = Wn_WindowDfs( pWnd );
    assert( RetValue );
    FREE( pWnd->ppNodes );
    // get the number of internal nodes
    pWnd->nNodes = Ntk_NetworkCountSpecial( pWnd->pNet );
    pWnd->ppNodes = ALLOC( Ntk_Node_t *, pWnd->nNodes );
    // go through the nodes and collect them
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
    {
        assert( Ntk_NodeIsInternal(pNode) );
        pWnd->ppNodes[i++] = pNode;
    }
    assert( i == pWnd->nNodes );
}


/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowGetNumLevels( Wn_Window_t * pWnd )
{
    Ntk_Node_t * pNode;
    int LevelsMax, LevelsCur;
    int i;

    // set the traversal ID for this traversal
    Ntk_NetworkIncrementTravId( pWnd->pNet );
    // mark the leaves - this way we will stop at them
    for ( i = 0; i < pWnd->nLeaves; i++ )
    {
        pNode = pWnd->ppLeaves[i];
        Ntk_NodeSetTravIdCurrent( pNode );
        pNode->Level = 0;
    }

    // perform the traversal
    LevelsMax = -1;
    for ( i = 0; i < pWnd->nRoots; i++ )
    {
        pNode = pWnd->ppRoots[i];
        LevelsCur = Wn_WindowGetNumLevels_rec( pNode );
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
int Wn_WindowGetNumLevels_rec( Ntk_Node_t * pNode )
{
    int LevelsMax, LevelsCur;
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;

    if ( Ntk_NodeIsTravIdCurrent(pNode) ) // visited node
        return pNode->Level;
    assert( !Ntk_NodeIsCi(pNode) ); // otherwise, the window is wrong

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent(pNode);

    // visit the transitive fanin
    LevelsMax = -1;
    Ntk_NodeForEachFanin( pNode, pLink, pFanin )
    {
        LevelsCur = Wn_WindowGetNumLevels_rec( pFanin );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }

    // set the number of levels
    pNode->Level = LevelsMax + 1;
    return pNode->Level;
}



/**Function*************************************************************

  Synopsis    [Levelizes the TFI/TFO cone startign from the nodes.]

  Description [This procedure connects the nodes into linked lists 
  according to their levels. It returns the number of levels. The nodes
  in the levelized network can be traversed using iterators
  Wn_WindowForEachNodeByLevel.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowLevelizeNodes( Wn_Window_t * pWnd, int fFromRoots )
{
    Ntk_Network_t * pNet;
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    int i;

    // free the previous array of pointers to levels
    pNet = pWnd->pNet;
    if ( pNet->nLevels )
        FREE( pNet->ppLevels );
    // get the new number of levels
    pNet->nLevels = Wn_WindowGetNumLevels( pWnd );
    // allocate the array of pointers
    pNet->ppLevels = ALLOC( Ntk_Node_t *, pNet->nLevels + 2 );
    memset( pNet->ppLevels, 0, sizeof(Ntk_Node_t *) * (pNet->nLevels + 2) );

    // traverse the network and add the nodes to the levelized structure
    if ( fFromRoots )
    {
        // set the traversal ID for this traversal
        Ntk_NetworkIncrementTravId( pWnd->pNet );
        // mark the leaves - this way we will stop at them
        for ( i = 0; i < pWnd->nLeaves; i++ )
        {
            Ntk_NodeSetTravIdCurrent( pWnd->ppLeaves[i] );
            pWnd->ppLeaves[i]->Level = 0;
        }
        // visit all the nodes starting from the roots
        for ( i = 0; i < pWnd->nRoots; i++ )
            Wn_WindowLevelizeFromOutputs_rec( pWnd->ppRoots[i] );
    }
    else
    {
        // mark all the windows nodes
        Wn_WindowDfs( pWnd );
        // set the traversal ID for this traversal
        Ntk_NetworkIncrementTravId( pWnd->pNet );
        // mark the roots - this way we will stop at them
        for ( i = 0; i < pWnd->nRoots; i++ )
        {
            Ntk_NodeSetTravIdCurrent( pWnd->ppRoots[i] );
            pWnd->ppRoots[i]->Level = 0;
        }

        // start DFS from the fanouts of the leaves
        // visit only the nodes that are in the window
        // they have the previous trav ID set by Wn_WindowDfs()
        for ( i = 0; i < pWnd->nLeaves; i++ )
            Ntk_NodeForEachFanout( pWnd->ppLeaves[i], pPin, pFanout )
                Wn_WindowLevelizeFromInputs_rec( pFanout );
    }

    return pNet->nLevels;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowLevelizeFromOutputs_rec( Ntk_Node_t * pNode )
{
    int LevelsMax, LevelsCur;
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pLink;

    if ( Ntk_NodeIsTravIdCurrent(pNode) ) // visited node
        return pNode->Level;
    assert( !Ntk_NodeIsCi(pNode) ); // otherwise, the window is wrong

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent(pNode);

    // visit the transitive fanin
    LevelsMax = -1;
    Ntk_NodeForEachFanin( pNode, pLink, pFanin )
    {
        LevelsCur = Wn_WindowLevelizeFromOutputs_rec( pFanin );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    assert( LevelsMax >= 0 );
    // set the level of this node
    pNode->Level = LevelsMax + 1;

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
int Wn_WindowLevelizeFromInputs_rec( Ntk_Node_t * pNode )
{
    int LevelsMax, LevelsCur;
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pLink;

    if ( Ntk_NodeIsTravIdCurrent(pNode) ) // visited node
        return pNode->Level;
    if ( !Ntk_NodeIsTravIdPrevious(pNode) ) // the node is not in the window
        return -1;
    assert( !Ntk_NodeIsCo(pNode) ); // otherwise, the window is wrong

    // mark the node as visited
    Ntk_NodeSetTravIdCurrent(pNode);

    // visit the transitive fanout
    LevelsMax = -1;
    Ntk_NodeForEachFanout( pNode, pLink, pFanout )
    {
        LevelsCur = Wn_WindowLevelizeFromInputs_rec( pFanout );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    assert( LevelsMax >= 0 );
    // set the level of this node
    pNode->Level = LevelsMax + 1;

    // attach the node to the level number "pNode->Level"
    pNode->pOrder = pNode->pNet->ppLevels[pNode->Level];
    pNode->pNet->ppLevels[pNode->Level] = pNode;
    return pNode->Level;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


