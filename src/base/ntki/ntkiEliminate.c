/**CFile****************************************************************

  FileName    [ntkiEliminate.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiEliminate.c,v 1.6 2004/04/08 05:05:04 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define NTK_ELIMINATE_SET_WEIGHT( Node, Weight )   (((Node)->pCopy) = (Ntk_Node_t *)Weight)
#define NTK_ELIMINATE_READ_WEIGHT( Node )          ((int)((Node)->pCopy))

static int              Ntk_EliminateNodeOkay( Ntk_Node_t * pNode, bool fUseCostBdd, int nCubeLimit, int nFaninLimit );
static void             Ntk_EliminateNode( Ntk_Network_t * pNet, Ntk_NodeHeap_t * pHeap, Ntk_Node_t * pNode, bool fUseCostBdd );
static void             Ntk_EliminateUpdate( Ntk_Network_t * pNet, Ntk_NodeHeap_t * pHeap, Ntk_Node_t ** ppFanouts, int nFanouts, bool fUseCostBdd );
static Ntk_NodeHeap_t * Ntk_EliminateSetUp( Ntk_Network_t * pNet, bool fUseCostBdd );
static void             Ntk_EliminateCleanUp( Ntk_Network_t * pNet, Ntk_NodeHeap_t * pHeap );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [The core of the elimination procedure.]

  Description [Eliminates all nodes those weight is above the limit. 
  The weight is defined as an inverse of the classical elimination value.
  The classical elimination value says how much the literal count of
  the network increases after elimination, i.e. Lits(After) - Lits(Before). 
  The weight is Lits(Before) - Lits(After). The weight is used because
  it is seems to be more natural and works well with the priority queue.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NetworkEliminate( Ntk_Network_t * pNet, int nNodesElim, int nCubeLimit, int nFaninLimit, int WeightLimit, bool fUseCostBdd, bool fVerbosity )
{
    Ntk_NodeHeap_t * pHeap;
    Ntk_Node_t * pNodeBest;
    ProgressBar * pProgress;
    int Weight, nIters;
    int nNodesToEliminate;
    bool fChanges;

    // perform sweep first to make eliminate more efficient
    Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );
    // prepare the network for elimination
    pHeap = Ntk_EliminateSetUp( pNet, fUseCostBdd );

    // start the progress bar
    nNodesToEliminate = Ntk_NodeHeapCountNodes(pHeap,WeightLimit);
    pProgress = Extra_ProgressBarStart( stdout, nNodesToEliminate );

    // assume there will be no changes
    fChanges = 0;
    nIters   = 0;
    while ( 1 )
    {
        // get the best node from the queque
        pNodeBest = Ntk_NodeHeapGetMax( pHeap );
        if ( !pNodeBest )
            break;
        // quit if the node's weight gets below the limit
        Weight = NTK_ELIMINATE_READ_WEIGHT( pNodeBest );
        if ( Weight < WeightLimit )
            break;
        // skip this node if the elimination is likely to be slow
        if ( !Ntk_EliminateNodeOkay( pNodeBest, fUseCostBdd, nCubeLimit, nFaninLimit ) )
            continue;
        // eliminate this node
        Ntk_EliminateNode( pNet, pHeap, pNodeBest, fUseCostBdd );
        // mark the change
        fChanges = 1;
        // count the number of iterations
        if ( ++nIters == nNodesElim )
            break;

        if ( nIters % 10 == 0 )
            Extra_ProgressBarUpdate( pProgress, nIters, NULL );
    }
    Extra_ProgressBarStop( pProgress );

    Ntk_EliminateCleanUp( pNet, pHeap );
    if ( !Ntk_NetworkCheck( pNet ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkEliminate(): Network check has failed.\n" );
    return fChanges;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if it is okay to eliminate this node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_EliminateNodeOkay( Ntk_Node_t * pNode, bool fUseCostBdd, int nCubeLimit, int nFaninLimit )
{
    Cvr_Cover_t * pCvr;
    Mvr_Relation_t * pMvr;
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    int nNodeCubes, nFanoutCubes;
    int nNodeBdds, nFanoutBdds;

    if ( Ntk_NodeReadFaninNum(pNode) > nFaninLimit )
        return 0;
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        if ( Ntk_NodeReadFaninNum(pFanout) > nFaninLimit )
            return 0;

    if ( fUseCostBdd )
    {
        // get the node cubes
        pMvr      = Ntk_NodeGetFuncMvr( pNode );
        nNodeBdds = Mvr_RelationGetNodes( pMvr );
        if ( nNodeBdds > nCubeLimit )
            return 0;
        // print the number of cubes in the fanouts
        Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        {
            if ( Ntk_NodeIsCo(pFanout) )
                continue;
            pMvr = Ntk_NodeGetFuncMvr( pFanout );
            nFanoutBdds = Mvr_RelationGetNodes( pMvr );
            if ( nFanoutBdds > nCubeLimit ) 
                return 0;
        }
    }
    else 
    {
        // get the node cubes
        pCvr       = Ntk_NodeGetFuncCvr( pNode );
        nNodeCubes = Cvr_CoverReadCubeNum( pCvr );
        if ( nNodeCubes > nCubeLimit )
            return 0;
        // print the number of cubes in the fanouts
        Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        {
            if ( Ntk_NodeIsCo(pFanout) )
                continue;
            pCvr = Ntk_NodeGetFuncCvr( pFanout );
            nFanoutCubes = Cvr_CoverReadCubeNum( pCvr );
            if ( nFanoutCubes > nCubeLimit ) 
                return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Eliminates one node and updates the cost of affected nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_EliminateNode( Ntk_Network_t * pNet, Ntk_NodeHeap_t * pHeap, Ntk_Node_t * pNode, bool fUseCostBdd )
{
    Ntk_Node_t ** ppFanouts;
    Ntk_Node_t * pNodeCol, * pFanout;
    Ntk_Pin_t * pPin, * pPin2;
    int nFanouts;

    // store the fanouts of the best node
    nFanouts  = Ntk_NodeReadFanoutNum(pNode);
    ppFanouts = ALLOC( Ntk_Node_t *, nFanouts );
    Ntk_NodeReadFanouts( pNode, ppFanouts );

    // for each fanout of the node, collapse the node into the fanout
    Ntk_NodeForEachFanoutSafe( pNode, pPin, pPin2, pFanout )
    {
        if ( Ntk_NodeIsCo( pFanout ) )
            continue;
        // collapse the node
        pNodeCol = Ntk_NodeCollapse( pFanout, pNode );
//printf( "N = %5s (%d)  ",  Ntk_NodeGetNamePrintable(pNode), Ntk_NodeReadFaninNum(pNode) );
//printf( "F = %5s (%d)  ",  Ntk_NodeGetNamePrintable(pFanout), Ntk_NodeReadFaninNum(pFanout) );
//printf( "N = %3d. ",  Mvr_RelationGetNodes( Ntk_NodeGetFuncMvr(pNode) ) );
//printf( "F = %3d. ",  Mvr_RelationGetNodes( Ntk_NodeGetFuncMvr(pFanout) ) );
//printf( "C = %6d. ",  Mvr_RelationGetNodes( Ntk_NodeGetFuncMvr(pNodeCol) ) );
        // reorder the relation
        Ntk_NodeReorderMvr( pNodeCol );
//printf( "R = %3d.\n",  Mvr_RelationGetNodes( Ntk_NodeGetFuncMvr(pNodeCol) ) );
        // make the node minimum-base
        Ntk_NodeMakeMinimumBase( pNodeCol );
        // replace the old node with the new one
        Ntk_NodeReplace( pFanout, pNodeCol ); // disposes of pNodeCol
    }

    // delete the node if it has no fanouts
    if ( Ntk_NodeReadFanoutNum( pNode ) == 0 )
        Ntk_NetworkDeleteNode( pNet, pNode, 1, 1 );

    // work on the surrounding nodes after collapsing
    Ntk_EliminateUpdate( pNet, pHeap, ppFanouts, nFanouts, fUseCostBdd );
    free( ppFanouts );
}


/**Function*************************************************************

  Synopsis    [Performs actions before elimination starts.]

  Description [For each pair fanin-fanout, generates the cost of collapsing
  them and attach the cost to the fanin pin for future reference.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_NodeHeap_t * Ntk_EliminateSetUp( Ntk_Network_t * pNet, bool fUseCostBdd )
{
    Ntk_Node_t * pNode, * pFanin;
    Ntk_Pin_t * pPin;
    Ntk_NodeHeap_t * pHeap;
    int Weight;

    // clean the pin data members for all fanins in the network
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            pPin->pData = NULL;
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            pPin->pData = NULL;
    }

    // prepare the internal nodes
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            if ( fUseCostBdd )
                Ntk_EliminateSetCollapsingWeightBdd( pNode, pPin, pFanin );
            else
                Ntk_EliminateSetCollapsingWeightSop( pNode, pPin, pFanin );
    }

    // start the heap
    pHeap = Ntk_NodeHeapStart();
    // put all the eliminatable nodes into the heap
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        // skip the nodes that cannot be eliminated
        if ( Ntk_NodeIsCoDriver(pNode) )
            continue;
        // skip the nodes that are too large
//        if ( Ntk_NodeReadFaninNum(pNode) > 10 )
//            continue;

        if ( fUseCostBdd )
            Weight = Ntk_EliminateGetNodeWeightBdd( pNode );
        else
            Weight = Ntk_EliminateGetNodeWeightSop( pNode );
        NTK_ELIMINATE_SET_WEIGHT( pNode, Weight );

        // clean the heap number
        pNode->pData = NULL; 
        // insert the node into the heap
        Ntk_NodeHeapInsert( pHeap, pNode );
        // check that the heap number is assigned
        assert( pNode->pData );
    }
//    Ntk_NodeHeapCheck( pHeap );
//    Ntk_NodeHeapPrint( stdout, pHeap );
    return pHeap;
}

/**Function*************************************************************

  Synopsis    [Performs actions when the elimination is completed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_EliminateCleanUp( Ntk_Network_t * pNet, Ntk_NodeHeap_t * pHeap )
{
//    Ntk_NodeHeapCheck( pHeap );
    Ntk_NodeHeapStop( pHeap );
}


/**Function*************************************************************

  Synopsis    [Work on the node after collapsing.]

  Description [Recalculates the collapsed nodes after the change.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_EliminateUpdate( Ntk_Network_t * pNet, Ntk_NodeHeap_t * pHeap, Ntk_Node_t ** ppFanouts, int nFanouts, bool fUseCostBdd )
{
    Ntk_Node_t * pNode, * pFanin, * pFanout;
    Ntk_Pin_t * pPin;
    int Weight,WeightOld;
    int i;

    // recalculate the cost of collapsing the affected nodes
    for ( i = 0; i < nFanouts; i++ )
    {
        // get the node
        pNode = ppFanouts[i];
        // try collapsing the fanins into this node
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            if ( fUseCostBdd )
                Ntk_EliminateSetCollapsingWeightBdd( pNode, pPin, pFanin );
            else
                Ntk_EliminateSetCollapsingWeightSop( pNode, pPin, pFanin );
        // try collapsing this node into its fanout
        Ntk_NodeForEachFanout( pNode, pPin, pFanout )
            if ( fUseCostBdd )
                Ntk_EliminateSetCollapsingWeightBdd( pFanout, pPin->pLink, pNode );
            else
                Ntk_EliminateSetCollapsingWeightSop( pFanout, pPin->pLink, pNode );
    }

    // update the queque
    for ( i = 0; i < nFanouts; i++ )
    {
        // get the node
        pNode = ppFanouts[i];
        if ( Ntk_NodeIsCo(pNode) )
            continue;
        // update the node
        if ( !Ntk_NodeIsCoDriver(pNode) )
        {
            WeightOld = NTK_ELIMINATE_READ_WEIGHT( pNode );
            if ( fUseCostBdd )
                Weight = Ntk_EliminateGetNodeWeightBdd( pNode );
            else
                Weight = Ntk_EliminateGetNodeWeightSop( pNode );
            NTK_ELIMINATE_SET_WEIGHT( pNode, Weight );
            // update the node if it is in the heap
            if ( pNode->pData )
                Ntk_NodeHeapUpdate( pHeap, pNode );
        }
        // update the fanins
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        {
            if ( pFanin->Type == MV_NODE_CI )
                continue;

            WeightOld = NTK_ELIMINATE_READ_WEIGHT( pFanin );
            if ( fUseCostBdd )
                Weight = Ntk_EliminateGetNodeWeightBdd( pFanin );
            else
                Weight = Ntk_EliminateGetNodeWeightSop( pFanin );
            NTK_ELIMINATE_SET_WEIGHT( pFanin, Weight );
            // update the node if it is in the heap
            if ( pFanin->pData )
                Ntk_NodeHeapUpdate( pHeap, pFanin );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Computes the weight of the current node.]

  Description [The weight is measured in terms of how many BDD
  nodes can be saved if the node is collapsed into its fanouts.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_EliminateGetNodeWeightBdd( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    int CostBefore, CostAfter;

    if ( Ntk_NodeIsCo(pNode) )
        return 0;

    // get the cost before elimination of this node
    CostBefore = Mvr_RelationGetNodes( Ntk_NodeGetFuncMvr(pNode) );
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        if ( pFanout->Type != MV_NODE_CO )
            CostBefore += Mvr_RelationGetNodes( Ntk_NodeGetFuncMvr(pFanout) );

    // get the cost after elimination of this node
    // if the node fans out into a CO, it cannot be eliminated
    if ( Ntk_NodeIsCoFanin(pNode) )
        CostAfter = Mvr_RelationGetNodes( Ntk_NodeGetFuncMvr(pNode) );
    else
        CostAfter = 0;
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        if ( pFanout->Type != MV_NODE_CO )
            CostAfter += (int)pPin->pLink->pData;
    return CostBefore - CostAfter;
}

/**Function*************************************************************

  Synopsis    [Computes the cost of two nodes after collapsing.]

  Description [Assigns the cost to the fanin pin of the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_EliminateSetCollapsingWeightBdd( Ntk_Node_t * pNode, Ntk_Pin_t * pPin, Ntk_Node_t * pFanin )
{
    Ntk_Node_t * pCollapsed;
    int nNodes;
    assert( Ntk_NodeReadFaninIndex( pNode, pFanin ) != -1 );
    pCollapsed = Ntk_NodeCollapse( pNode, pFanin );
    if ( pCollapsed )
    {   // reorder the relation
//        Ntk_NodeReorderMvr( pCollapsed );
        // get the number of nodes after collapsing
        nNodes = Mvr_RelationGetNodes( Ntk_NodeGetFuncMvr(pCollapsed) );
        // set the cost of these nodes after collapsing
        pPin->pData = (char *)nNodes;
        Ntk_NodeDelete( pCollapsed );
        return nNodes;
    }
    else
    {
        pPin->pData = NULL;
        return 1000000;
    }
}


/**Function*************************************************************

  Synopsis    [Computes the weight of the current node.]

  Description [The weight is the inverse of the classical
  elimination value of the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_EliminateGetNodeWeightSop( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    Cvr_Cover_t * pCvrNode;
    Ft_Tree_t * pTreeNode;
    int CostBefore, CostAfter;

    if ( Ntk_NodeIsCo(pNode) )
        return 0;

    // get the node's factored form
    pCvrNode = Ntk_NodeGetFuncCvr( pNode );
    // get the factored tree
    pTreeNode = Cvr_CoverFactor( pCvrNode );

    // get the cost before elimination of this node
    CostBefore = pTreeNode->nNodes;

    // get the cost after elimination of this node
    // if the node fans out into a CO, it cannot be eliminated
    if ( Ntk_NodeIsCoFanin(pNode) )
        CostAfter = pTreeNode->nNodes;
    else
        CostAfter = 0;
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        if ( pFanout->Type != MV_NODE_CO )
            CostAfter += (int)pPin->pLink->pData;

//    assert( CostBefore - CostAfter <= 1 );
    return CostBefore - CostAfter;
}

/**Function*************************************************************

  Synopsis    [Computes the cost of two nodes after collapsing.]

  Description [Assigns the cost to the fanin pin of the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_EliminateSetCollapsingWeightSop( Ntk_Node_t * pNode, Ntk_Pin_t * pPin, Ntk_Node_t * pFanin )
{
    Vm_VarMap_t * pVm;
    Cvr_Cover_t * pCvrNode;
    Cvr_Cover_t * pCvrFanin;
    Ft_Tree_t * pTreeNode;
    Ft_Tree_t * pTreeFanin;
    int iFanin, nValues, iValueFirst;
    int nFanoutLitOccurs, nFaninLits, Cost, i;

    iFanin = Ntk_NodeReadFaninIndex( pNode, pFanin );
    assert( iFanin != -1 );
    if ( pNode->Type != MV_NODE_INT || pFanin->Type != MV_NODE_INT )
    {
        pPin->pData = NULL;
        return 1000000;
    }

    // get the fanin's factored form
    pCvrFanin = Ntk_NodeGetFuncCvr( pFanin );
    // get the factored tree
    pTreeFanin = Cvr_CoverFactor( pCvrFanin );

    // get the node's factored form
    pCvrNode = Ntk_NodeGetFuncCvr( pNode );
    // get the factored tree
    pTreeNode = Cvr_CoverFactor( pCvrNode );
    // count the number of terminal literals in the FF
    if ( pTreeNode->fMark == 0 )
    {
        Ft_TreeCountLeafRefs( pTreeNode );
        pTreeNode->fMark = 1;
    }
 
    // count how many times the literals corresponding 
    // to pFanin occur in pNode's factored form
    pVm = Ntk_NodeReadFuncVm( pNode );
    nValues = Vm_VarMapReadValues( pVm, iFanin );
    iValueFirst = Vm_VarMapReadValuesFirst( pVm, iFanin );

    // get the cost of collapsing
    if ( nValues == 2 )
    {
        nFanoutLitOccurs = pTreeNode->uLeafData[iValueFirst] +
                           pTreeNode->uLeafData[iValueFirst + 1];
        nFaninLits = pTreeFanin->nNodes;
        Cost = nFanoutLitOccurs * (nFaninLits - 1);
    }
    else
    {
        Cost = 0;
        for ( i = iValueFirst; i < iValueFirst + nValues; i++ )
            if ( pTreeFanin->pRoots[i-iValueFirst] )
            {
                nFanoutLitOccurs = pTreeNode->uLeafData[i];
                nFaninLits = Ft_FactorCountLeavesOne(pTreeFanin->pRoots[i-iValueFirst]);
                Cost += nFanoutLitOccurs * (nFaninLits - 1);
            }
    }

    // set the cost
    pPin->pData = (char *)Cost;
    return Cost;
}

/**Function*************************************************************

  Synopsis    [Computes the cost of two nodes after collapsing.]

  Description [Assigns the cost to the fanin pin of the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkGetNodeValueSop( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;
    int Value;
    // precompute the elimination value for collapsing
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        Ntk_EliminateSetCollapsingWeightSop( pFanout, pPin->pLink, pNode );
    // get the elimination value for the node
    Value = - Ntk_EliminateGetNodeWeightSop( pNode );
    return Value;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


