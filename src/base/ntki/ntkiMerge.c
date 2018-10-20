/**CFile****************************************************************

  FileName    [ntkiMerge.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Performs merging of two nodes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiMerge.c,v 1.4 2003/09/01 04:56:07 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Merges a number of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkMerge( Ntk_Network_t * pNet, int nNodes )
{
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode, * pNodeTemp;
    Ntk_Node_t * pNodeMerged;
    int nValuesRes;
    int i, k, Index;
    bool fChange;


    assert( nNodes > 1 );
    // collect the nodes into the list
    ppNodes = ALLOC( Ntk_Node_t *, nNodes );
    i = 0;
    nValuesRes = 1;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( Ntk_NodeIsCo(pNode) )
            ppNodes[i] = Ntk_NodeReadFaninNode( pNode, 0 );
        else
            ppNodes[i] = pNode;
        // add to the product
        nValuesRes *= ppNodes[i]->nValues;
        // increment the counter of nodes
        i++;
    }
    assert( i == nNodes );
    if ( nValuesRes > 32 )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkMerge(): Cannot create a node with more than 32 values.\n" );
        return;
    }

    // make sure the list contains no duplicates
    for ( i = 0; i < nNodes; i++ )
        for ( k = i + 1; k < nNodes; k++ )
            if ( ppNodes[i] == ppNodes[k] )
            {
                fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkMerge(): Cannot merge the node \"%s\" with itself.\n", 
                    Ntk_NodeGetNamePrintable(ppNodes[i]) );
                return;
            }


    // if the nodes are fanin/fanout of each other, collapse them first
    do 
    {
        fChange = 0;
        for ( i = 0; i < nNodes; i++ )
        for ( k = i + 1; k < nNodes; k++ )
        {
            Index = Ntk_NodeReadFaninIndex( ppNodes[i], ppNodes[k] );
            if ( Index >= 0 )
            {
                Ntk_NetworkCollapseNodes( ppNodes[i], ppNodes[k] );
                fChange = 1;
            }
            Index = Ntk_NodeReadFaninIndex( ppNodes[k], ppNodes[i] );
            if ( Index >= 0 )
            {
                Ntk_NetworkCollapseNodes( ppNodes[k], ppNodes[i] );
                fChange = 1;
            }
        }
    }
    while ( fChange );

    // merge nodes one by one
    pNodeMerged = ppNodes[0];
    for ( i = 1; i < nNodes; i++ )
    {
        if ( !Ntk_NodesCanBeMerged( pNodeMerged, ppNodes[i] ) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkMerge(): Cannot merge node \"%s\" with the rest.\n", 
                Ntk_NodeGetNamePrintable(ppNodes[i]) );
            continue;
        }

        pNodeMerged = Ntk_NetworkMergeNodes( pNodeTemp = pNodeMerged, ppNodes[i] );
        assert ( pNodeMerged != NULL );
    }
    FREE( ppNodes );

    if ( !Ntk_NetworkCheck( pNet ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkMerge(): Network check has failed.\n" );
}



/**Function*************************************************************

  Synopsis    [Merges two nodes.]

  Description [Derives the node, which is the result of merging of 
  pNode1 and pNode2. Makes this node minimum base and sorts the nodes
  fanins to be in the accepted fanin order. Introduces the new node
  into the network and modifies the fanouts of pNode1 and pNode2
  accordingly. Leaves pNode1 and pNode2 unchanged and does not remove 
  them from the network, even if they do not fanout after merging.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkMergeNodes( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 )
{
    Ntk_Node_t * pNode1Enc, * pNode2Enc;
    Ntk_Node_t * pNodeMerged;

    // derive the merged node 
    // this node is not minimum-base and is not in the network
    // but it contains the relation of the merged node
    pNodeMerged = Ntk_NodeMerge( pNode1, pNode2 );
    if ( pNodeMerged == NULL )
        return NULL;
    // make the node minimum-base
    Ntk_NodeMakeMinimumBase( pNodeMerged );
    // add the merged node to the network
    Ntk_NetworkAddNode( pNode1->pNet, pNodeMerged, 1 );

    // derive the encoder nodes from pNode1 and pNode2 into pNodeMerged
    Ntk_NodeCreateEncoders( pNode1->pNet, pNodeMerged, 
        pNode1->nValues, pNode2->nValues, &pNode1Enc, &pNode2Enc );

    // collapse pNode1Enc into the fanouts of pNode1
    Ntk_NetworkSweepTransformFanouts( pNode1, pNode1Enc );
    // if the node has some CO fanouts, pNode1Enc stays in the network
    if ( Ntk_NodeReadFanoutNum( pNode1Enc ) )
        Ntk_NetworkAddNode( pNode1->pNet, pNode1Enc, 1 );
    else
        Ntk_NodeDelete( pNode1Enc );

    // collapse pNode2Enc into the fanouts of pNode2
    Ntk_NetworkSweepTransformFanouts( pNode2, pNode2Enc );
    // if the node has some CO fanouts, pNode2Enc stays in the network
    if ( Ntk_NodeReadFanoutNum( pNode2Enc ) )
        Ntk_NetworkAddNode( pNode2->pNet, pNode2Enc, 1 );
    else
        Ntk_NodeDelete( pNode2Enc );

    // remove the original nodes from the network
    Ntk_NetworkDeleteNode( pNode1->pNet, pNode1, 1, 1 );
    Ntk_NetworkDeleteNode( pNode1->pNet, pNode2, 1, 1 );
    return pNodeMerged;
}

/**Function*************************************************************

  Synopsis    [Derives the merged node.]

  Description [Derives the merged node without making in minimum base
  and without adding it to the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeMerge( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 )
{
    Ntk_Node_t * pNodeDec;
    Ntk_Node_t * pNodeCol1, * pNodeCol2;

    // cannot merge if the nodes are not internal
    if ( pNode1->Type != MV_NODE_INT && pNode2->Type != MV_NODE_INT )
        return NULL;
    // cannot merge when nodes are fanin/fanout of each other
    if ( Ntk_NodeReadFaninIndex( pNode1, pNode2 ) != -1 )
        return NULL;
    if ( Ntk_NodeReadFaninIndex( pNode2, pNode1 ) != -1 )
        return NULL;
    // derive the decoder
    pNodeDec = Ntk_NodeCreateDecoder( pNode1->pNet, pNode1, pNode2 );
    Ntk_NodeOrderFanins( pNodeDec );
    // collapse the first node into the decoder
    pNodeCol1 = Ntk_NodeCollapse( pNodeDec, pNode1 );
    Ntk_NodeDelete( pNodeDec );
    // collapse the second node into the decoder
    pNodeCol2 = Ntk_NodeCollapse( pNodeCol1, pNode2 );
    Ntk_NodeDelete( pNodeCol1 );
    return pNodeCol2;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


