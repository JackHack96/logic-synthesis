/**CFile****************************************************************

  FileName    [shNetwork.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shNetwork.c,v 1.6 2004/04/12 20:41:08 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Sh_NetworkDfs_rec( Sh_Network_t * pNet, Sh_Node_t * pNode );
static int Sh_NetworkInterleaveNodes_rec( Sh_Network_t * pNet, Sh_Node_t * pNode );
static void Sh_NodePrintTree_rec( Sh_Node_t * pNode );
static int Sh_NetworkGetNumLevels_rec( Sh_Network_t * pNet, Sh_Node_t * pNode );
static int Sh_NetworkCountLiterals_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode );
static void Sh_NetworkPrintAig_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode );
static int Sh_NodeCountNodes_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collects the set of internal nodes into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NodeCollectDfs( Sh_Manager_t * pMan, Sh_Node_t * pNode, Sh_Node_t *** pppNodes )
{
    Sh_Node_t ** ppNodes;
    Sh_Network_t * pNetwork;
    int nNodesRes, i;
    // create the network
    pNetwork = Sh_NetworkCreate( pMan, pMan->nVars, 1 );
    pNetwork->ppOutputs[0] = pNode;   shRef( pNode );
    // DFS order the nodes
    nNodesRes = Sh_NetworkDfs( pNetwork );
    // collect the nodes into the array
    ppNodes = ALLOC( Sh_Node_t *, nNodesRes );
    i = 0;
    Sh_NetworkForEachNodeSpecial( pNetwork, pNode )
        if ( shNodeIsAnd(pNode) )
            ppNodes[i++] = pNode;
    *pppNodes = ppNodes;
    // get rid of the network
    Sh_NetworkFree( pNetwork );
    return i;
}

/**Function*************************************************************

  Synopsis    [Collects the set of internal nodes into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkCollectInternal( Sh_Network_t * pNet )
{
    Sh_Node_t * pNode;
    int nNodesAll, nNodesInt;
    // collect all the nodes
    nNodesAll = Sh_NetworkDfs( pNet );
    pNet->ppNodes = ALLOC( Sh_Node_t *, nNodesAll );
    nNodesInt = 0;
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
        if ( shNodeIsAnd(pNode) )
            pNet->ppNodes[nNodesInt++] = pNode;
    assert( nNodesInt <= nNodesAll );
    pNet->nNodes = nNodesInt;
    return nNodesInt;
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkDfs( Sh_Network_t * pNet )
{
    int nNodesRes, i;
    // set the traversal ID for this DFS ordering
    Sh_ManagerIncrementTravId( pNet->pMan );
    // start the linked list
    Sh_NetworkStartSpecial( pNet );
    // traverse the TFI cones of all nodes in the array
    nNodesRes = 0;
    for ( i = 0; i < pNet->nOutputs; i++ )
        nNodesRes += Sh_NetworkDfs_rec( pNet, Sh_Regular(pNet->ppOutputs[i]) );
    for ( i = 0; i < pNet->nInputsCore; i++ )
        nNodesRes += Sh_NetworkDfs_rec( pNet, Sh_Regular(pNet->ppInputsCore[i]) );
    // finalize the linked list
    Sh_NetworkStopSpecial( pNet );
    pNet->nNodes = nNodesRes;
//    Sh_NetworkPrintSpecial( pNet );
    return nNodesRes;
}

/**Function*************************************************************

  Synopsis    [Counts the number of AND-INV nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NodeCountNodes( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    Sh_ManagerIncrementTravId( pMan );
    return Sh_NodeCountNodes_rec( pMan, Sh_Regular(pNode) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NodeCountNodes_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    assert( !Sh_IsComplement(pNode) );
    if ( !shNodeIsAnd( pNode ) )
        return 0;
    // if this node is already visited, skip
    if ( Sh_NodeIsTravIdCurrent( pMan, pNode ) )
        return 0;
    // mark the node as visited
    Sh_NodeSetTravIdCurrent( pMan, pNode );
    return Sh_NodeCountNodes_rec( pMan, Sh_Regular(pNode->pOne) ) +
           Sh_NodeCountNodes_rec( pMan, Sh_Regular(pNode->pTwo) ) + 1;
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes in the AIG rooted in this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkDfsNode( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    Sh_Network_t * pNetwork;
    int nNodesRes;
    pNetwork = Sh_NetworkCreate( pMan, pMan->nVars, 1 );
    pNetwork->ppOutputs[0] = pNode;   shRef( pNode );
    nNodesRes = Sh_NetworkDfs( pNetwork );
    Sh_NetworkFree( pNetwork );
    return nNodesRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkDfs_rec( Sh_Network_t * pNet, Sh_Node_t * pNode )
{
    int nNodes;

    assert( !Sh_IsComplement(pNode) );

    // if this node is already visited, skip
    if ( Sh_NodeIsTravIdCurrent(pNet->pMan, pNode) )
    {
        // move the insertion point to be after this node
//        Sh_NetworkMoveSpecial( pNet, pNode );
        return 0;
    }

    // mark the node as visited
    Sh_NodeSetTravIdCurrent( pNet->pMan, pNode );

    // visit the transitive fanin
    nNodes = 0;
    if ( shNodeIsAnd(pNode) )
    {
        nNodes += Sh_NetworkDfs_rec( pNet, Sh_Regular(pNode->pOne) );
        nNodes += Sh_NetworkDfs_rec( pNet, Sh_Regular(pNode->pTwo) );
    }

    // add the node to the list 
    Sh_NetworkAddToSpecial( pNet, pNode );
    nNodes++;
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
int Sh_NetworkInterleaveNodes( Sh_Network_t * pNet )
{
    int nNodesRes, i;
    // set the traversal ID for this DFS ordering
    Sh_ManagerIncrementTravId( pNet->pMan );
    // start the linked list
    Sh_NetworkStartSpecial( pNet );
    // traverse the TFI cones of all nodes in the array
    nNodesRes = 0;
    for ( i = 0; i < pNet->nOutputs; i++ )
        nNodesRes += Sh_NetworkInterleaveNodes_rec( pNet, Sh_Regular(pNet->ppOutputs[i]) );
    for ( i = 0; i < pNet->nInputsCore; i++ )
        nNodesRes += Sh_NetworkInterleaveNodes_rec( pNet, Sh_Regular(pNet->ppInputsCore[i]) );
    // finalize the linked list
    Sh_NetworkStopSpecial( pNet );
    pNet->nNodes = nNodesRes;
//    Sh_NetworkPrintSpecial( pNet );
    return nNodesRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkInterleaveNodes_rec( Sh_Network_t * pNet, Sh_Node_t * pNode )
{
    int nNodes;

    assert( !Sh_IsComplement(pNode) );

    // if this node is already visited, skip
    if ( Sh_NodeIsTravIdCurrent(pNet->pMan, pNode) )
    {
        // move the insertion point to be after this node
        Sh_NetworkMoveSpecial( pNet, pNode );
        return 0;
    }

    // mark the node as visited
    Sh_NodeSetTravIdCurrent( pNet->pMan, pNode );

    // visit the transitive fanin
    nNodes = 0;
    if ( shNodeIsAnd(pNode) )
    {
        nNodes += Sh_NetworkInterleaveNodes_rec( pNet, Sh_Regular(pNode->pOne) );
        nNodes += Sh_NetworkInterleaveNodes_rec( pNet, Sh_Regular(pNode->pTwo) );
    }

    // add the node to the list 
    Sh_NetworkAddToSpecial( pNet, pNode );
    nNodes++;
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the number of logic levels not counting PIs/POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkGetNumLevels( Sh_Network_t * pNet )
{
    int i, LevelsMax, LevelsCur;
    // set the traversal ID for this traversal
    Sh_ManagerIncrementTravId( pNet->pMan );
    // perform the traversal
    LevelsMax = -1;
    for ( i = 0; i < pNet->nOutputs; i++ )
    {
        LevelsCur = Sh_NetworkGetNumLevels_rec( pNet, Sh_Regular(pNet->ppOutputs[i]) );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
    }
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkGetNumLevels_rec( Sh_Network_t * pNet, Sh_Node_t * pNode )
{
    int LevelsMax, LevelsCur;
    // if this node is already visited, skip
    if ( Sh_NodeIsTravIdCurrent( pNet->pMan, pNode ) )
        return pNode->pData2;
    // mark the node as visited
    Sh_NodeSetTravIdCurrent( pNet->pMan, pNode );
    // visit the transitive fanin
    LevelsMax = -1;
    if ( shNodeIsAnd(pNode) )
    {
        LevelsCur = Sh_NetworkGetNumLevels_rec( pNet, Sh_Regular(pNode->pOne) );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
        LevelsCur = Sh_NetworkGetNumLevels_rec( pNet, Sh_Regular(pNode->pTwo) );
        if ( LevelsMax < LevelsCur )
            LevelsMax = LevelsCur;
        // set the number of levels
        pNode->pData2 = LevelsMax + 1;
    }
    else
        pNode->pData2 = 0;
    return pNode->pData2;
}

/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes in the AIG rooted in this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkCountLiterals( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    int nNodesRes;
    // set the traversal ID for this DFS ordering
    Sh_ManagerIncrementTravId( pMan );
    nNodesRes = Sh_NetworkCountLiterals_rec( pMan, Sh_Regular(pNode) );
    return nNodesRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkCountLiterals_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    int nNodes;

    assert( !Sh_IsComplement(pNode) );
    // if the node is a PI var, always add literal
    if ( !shNodeIsAnd(pNode) )
        return 1;

    // if this node is already visited, always add literal
    if ( Sh_NodeIsTravIdCurrent(pMan, pNode) )
    {
        pNode->pData2++;
        // if this node is not a PI var and is visited more than once, increment literal count by one
        return 1;
    }
    // mark the node as visited
    Sh_NodeSetTravIdCurrent( pMan, pNode );
    // clean its ref count
    pNode->pData2 = 1;
    // do not add literal for this node

    // visit the transitive fanin
    nNodes = 0;
    if ( shNodeIsAnd(pNode) )
    {
        nNodes += Sh_NetworkCountLiterals_rec( pMan, Sh_Regular(pNode->pOne) );
        nNodes += Sh_NetworkCountLiterals_rec( pMan, Sh_Regular(pNode->pTwo) );
    }
    return nNodes;
}



/**Function*************************************************************

  Synopsis    [Computes the DFS ordering of the nodes in the AIG rooted in this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkPrintAig( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    // set the traversal ID for this DFS ordering
    Sh_ManagerIncrementTravId( pMan );
    if ( Sh_IsComplement(pNode) )
        printf( "(" );
    Sh_NetworkPrintAig_rec( pMan, Sh_Regular(pNode) );
    if ( Sh_IsComplement(pNode) )
        printf( ")\'" );
    printf( " = F\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkPrintAig_rec( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    assert( !Sh_IsComplement(pNode) );
    // if the node is a PI var, always add literal
    if ( !shNodeIsAnd(pNode) )
    {
        printf( "%c", 'a' + (int)pNode->pTwo );
        return;
    }
    // if this node is already visited, add literal
    if ( Sh_NodeIsTravIdCurrent(pMan, pNode) )
    {
        // if this node is not a PI var and is visited more than once, increment literal count by one
        printf( "[%d]", Sh_NodeReadIndex(pNode) );
        return;
    }
    // mark the node as visited
    Sh_NodeSetTravIdCurrent( pMan, pNode );
    // visit the transitive fanin
    if ( shNodeIsAnd(pNode) )
    {
        if ( shNodeIsAnd(Sh_Regular(pNode->pOne)) && Sh_IsComplement(pNode->pOne) )
            printf( "(" );
        Sh_NetworkPrintAig_rec( pMan, Sh_Regular(pNode->pOne) );
        if ( shNodeIsAnd(Sh_Regular(pNode->pOne)) && Sh_IsComplement(pNode->pOne) )
            printf( ")\'" );

        printf( "*" );

        if ( shNodeIsAnd(Sh_Regular(pNode->pTwo)) && Sh_IsComplement(pNode->pTwo) )
            printf( "(" );
        Sh_NetworkPrintAig_rec( pMan, Sh_Regular(pNode->pTwo) );
        if ( shNodeIsAnd(Sh_Regular(pNode->pTwo)) && Sh_IsComplement(pNode->pTwo) )
            printf( ")\'" );

        if ( pNode->pData2 > 1 )
        {
            printf( " = [%d]\n", Sh_NodeReadIndex(pNode) );
            printf( "[%d]", Sh_NodeReadIndex(pNode) );
        }
    }
}



/**Function*************************************************************

  Synopsis    [Cleans the data fields of the nodes in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkCleanData( Sh_Network_t * pNet )
{
    Sh_Node_t * pNode;
    int i;

    for ( i = 0; i < pNet->nInputs; i++ )
    {
        pNode = pNet->pMan->pVars[i];
        pNode->pData = pNode->pData2 = 0;
    }
    for ( i = 0; i < pNet->nSpecials; i++ )
    {
        pNode = Sh_Regular(pNet->ppSpecials[i]);
        pNode->pData = pNode->pData2 = 0;
    }
    for ( i = 0; i < pNet->nOutputsCore; i++ )
    {
        pNode = Sh_Regular(pNet->ppOutputsCore[i]);
        pNode->pData = pNode->pData2 = 0;
    }
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
    {
        pNode->pData = pNode->pData2 = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Sets the numbers for all the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_NetworkSetNumbers( Sh_Network_t * pNet )
{
    Sh_Node_t * pNode;
    int Counter;
    Counter = 0;
//    for ( i = 0; i < pNet->nInputs; i++ )
//        pNet->pMan->pVars[i]->pData = Counter++;
    Sh_NetworkForEachNodeSpecial( pNet, pNode )
        pNode->pData = Counter++;
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Prints the array of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodePrintTrees( Sh_Node_t * ppNodes[], int nNodes )
{
    int i;
    for ( i = 0; i < nNodes; i++ )
    {
        printf( "Root %2d :  \n", i );
        Sh_NodePrintTree_rec( Sh_Regular(ppNodes[i]) );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the array of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodePrintTree_rec( Sh_Node_t * pNode )
{
    Sh_NodePrint( pNode );
    if ( shNodeIsAnd(pNode) )
    {
        Sh_NodePrintTree_rec( Sh_Regular(pNode->pOne) );
        Sh_NodePrintTree_rec( Sh_Regular(pNode->pTwo) );
    }
}


/**Function*************************************************************

  Synopsis    [Prints the array of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodePrintArray( Sh_Node_t * ppNodes[], int nNodes )
{
    int i;
    for ( i = 0; i < nNodes; i++ )
    {
        printf( "Node %2d :  ", i );
        Sh_NodePrint( ppNodes[i] );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the node and its "cofactors".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodePrint( Sh_Node_t * pNode )
{
    Sh_NodePrintOne( pNode );
    if ( shNodeIsAnd(pNode) )
    {
        Sh_NodePrintOne( Sh_Regular(pNode)->pOne );
        Sh_NodePrintOne( Sh_Regular(pNode)->pTwo );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodePrintOne( Sh_Node_t * pNode )
{
    Sh_Node_t * pNodeR;
    bool fCompl;

    pNodeR = Sh_Regular(pNode);
    fCompl = Sh_IsComplement(pNode);

    if ( (((unsigned)pNodeR) & ~0xffff) == 0 )
    {
        printf( "Error!  " );
        return;
    }

    printf( "num = %3d(refs=%d) %c   ", Sh_NodeReadIndex(pNodeR), pNodeR->nRefs, fCompl? 'c': ' ' );
    if ( shNodeIsConst(pNodeR) )
    {
        printf( "Const 1 " );
        return;
    }
    if ( !shNodeIsAnd(pNodeR) )
    {
        printf( "Var %3d ", Sh_NodeReadIndex(pNodeR) );
        return;
    }
    printf( "and     " );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


