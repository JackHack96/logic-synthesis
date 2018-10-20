/**CFile****************************************************************

  FileName    [ntkSort.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkSort.c,v 1.36 2005/07/08 01:01:21 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Compacts the node IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkCompactNodeIds( Ntk_Network_t * pNet )
{
    int iNewId, i;
    // start counting the new IDs
    iNewId = 1;
    for ( i = 1; i < pNet->nIds; i++ )
        if ( pNet->pId2Node[i] )
        {
            pNet->pId2Node[i]->Id = iNewId;
            pNet->pId2Node[iNewId++] = pNet->pId2Node[i];
        }
    // set the new node ID
    pNet->nIds = iNewId;
    // return the number of new IDs
    return iNewId;
}

/**Function*************************************************************

  Synopsis    [Sets proper ordering of fanins in all nodes.]

  Description [Should be called immediately after reading the network.
  The Cvr is freed, The Mvr is updated.
  When the new nodes are created, if their fanins are not ordered,
  it is important to call Ntk_NodeOrderFanins() AFTER we add the new 
  node to the network by Ntk_NodeReplace(), and AFTER all of its fanins 
  are already in the network.
  The reason is, the node ID is assigned when the node is added to the
  network. If the node ID is not assigned Ntk_NodeOrderFanins() does not
  know how to order the nodes. (When there is no name, it orders the 
  nodes by node ID.)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkOrderFanins( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;

    // assign node IDs according to the alphabetical ordering of node names
    Ntk_NetworkReassignIds( pNet );
    // order the fanins of each intenal node
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        Ntk_NodeOrderFanins( pNode );
//        if ( Ntk_NodeReadFaninNum( pNode ) > 10 )
//            continue;
//        if ( Ntk_NodeMakeMinimumBase( pNode ) )
//            fprintf( Ntk_NetworkReadMvsisOut(pNet), "Support of node \"%s\" has been minimized.\n", Ntk_NodeGetNameLong(pNode) );
    }

    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkReassignIds( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Ntk_Node_t ** ppNodes;
    int nNodes, iNode, i;
    int nNodesInt;

    // create the array of nodes by ID
    nNodes = Ntk_NetworkReadNodeTotalNum( pNet );
    ppNodes = ALLOC( Ntk_Node_t *, pNet->nIds );

    // copy the nodes into this array
    iNode = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( pNode )
            ppNodes[iNode++] = pNode;
    Ntk_NetworkForEachCo( pNet, pNode )
        if ( pNode )
            ppNodes[iNode++] = pNode;
    Ntk_NetworkForEachNode( pNet, pNode )
        if ( pNode )
            ppNodes[iNode++] = pNode;
    assert( iNode == nNodes );

    // sort the nodes alphabetically by name
    // make sure CIs are ordered first, then COs, then internal nodes
    qsort( (void *)ppNodes, nNodes, sizeof(Ntk_Node_t *), 
        (int (*)(const void *, const void *)) Ntk_NodeCompareByNameAndIdOrder );
    assert( Ntk_NodeCompareByNameAndIdOrder( ppNodes, ppNodes + nNodes - 1 ) < 0 );

    // reassign the IDs according to the alphabetical order
    // and put nodes into the Id2Node table
    pNet->nIds = 1;
    for ( i = 0; i < nNodes; i++ )
    {
        ppNodes[i]->Id             = pNet->nIds;
        pNet->pId2Node[pNet->nIds] = ppNodes[i];
        pNet->nIds++;
    }

    // compact the nodes by removing the PI and the PO nodes
    nNodesInt = 0;
    for ( i = 0; i < nNodes; i++ )
        if ( ppNodes[i]->Type == MV_NODE_INT )
            ppNodes[nNodesInt++] = ppNodes[i];
    // relink internal nodes according to this order
    if ( nNodesInt == 0 )
        pNet->lNodes.pHead = pNet->lNodes.pTail = NULL;
    else
    {
        pNet->lNodes.pHead = ppNodes[0];
        pNet->lNodes.pHead->pPrev = NULL;
        pNet->lNodes.pTail = ppNodes[nNodesInt-1];
        pNet->lNodes.pTail->pNext = NULL;
        for ( i = 1; i < nNodesInt; i++ )
        {
            ppNodes[i-1]->pNext = ppNodes[i];
            ppNodes[i]->pPrev   = ppNodes[i-1];
        }
    }
    FREE( ppNodes );
}

/**Function*************************************************************

  Synopsis    [Sets proper ordering of fanins in one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeOrderFanins( Ntk_Node_t * pNode )
{
    Mvr_Relation_t * pMvr, * pMvrNew;
    Cvr_Cover_t * pCvr, * pCvrNew;
    int * pPermuteInv;

    // relink the fanins and derive the tranlation map
    pPermuteInv = Ntk_NodeRelinkFanins( pNode );
    if ( pPermuteInv == NULL )
        return 0;

    if ( Ntk_NodeReadFuncMvr( pNode ) )
    { // the node's relation is present
        // get the node's relation
        pMvr = Ntk_NodeGetFuncMvr( pNode );
        // permute the node's relation using the permuation array
        pMvrNew = Mvr_RelationCreatePermuted( pMvr, pPermuteInv );

        // set the new variable map and the new relation
        Ntk_NodeSetFuncVm( pNode, Mvr_RelationReadVm(pMvrNew) );
        Ntk_NodeWriteFuncMvr( pNode, pMvrNew );
    }
    else
    { // the relation is not present -> the cover must be present
        pCvr = Ntk_NodeReadFuncCvr( pNode );
        assert( pCvr );
        // create the new cover
        pCvrNew = Cvr_CoverCreatePermuted( pCvr, pPermuteInv );

        // set the new variable map and the new cover
        Ntk_NodeSetFuncVm( pNode, Cvr_CoverReadVm(pCvrNew) );
        Ntk_NodeWriteFuncCvr( pNode, pCvrNew );
    }
    FREE( pPermuteInv );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Creates a new linked list of fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Ntk_NodeRelinkFanins( Ntk_Node_t * pNode )
{
    Ntk_Node_t ** ppFanins, * pFanin, * pFaninPrev;
    Ntk_Pin_t ** ppPins, * pPin;
    int * pPermuteInv;
    int fOrderingIsGood;
    int nFanins, i;

    // consider simple case, when sorting is trivial
    nFanins = Ntk_NodeReadFaninNum( pNode );
    if ( nFanins < 2 )
        return NULL;

    // consider the case when the fanins are already ordered
    fOrderingIsGood = 1;
    pFaninPrev = NULL;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        if ( pFaninPrev && Ntk_NodeCompareByNameAndId( &pFaninPrev, &pFanin ) >= 0 )
        { // the ordering should be changed
            fOrderingIsGood = 0;
            break;
        }
        pFaninPrev = pFanin;
    }
    // quit if there is no need to reorder
    if ( fOrderingIsGood )
        return NULL;

    // collect the fanins into an array
    ppFanins = pNode->pNet->pArray1;
    Ntk_NodeReadFanins( pNode, ppFanins );
    // order the fanins by node name (and node ID, if the name is absent)
    qsort( (void *)ppFanins, nFanins, sizeof(Ntk_Node_t *), 
        (int (*)(const void *, const void *)) Ntk_NodeCompareByNameAndId );
    assert( Ntk_NodeCompareByNameAndId( ppFanins, ppFanins + nFanins - 1 ) <= 0 );

    // detect the situation when there are duplicated fanins
    for ( i = 1; i < nFanins; i++ )
        if ( ppFanins[i-1] == ppFanins[i] )
        {
//            fprintf( Ntk_NodeReadMvsisOut(pNode), "Warning: removing duplicated fanin of node \"%s\".\n", Ntk_NodeGetNameLong(pNode) ); 
            Ntk_NodeCompactFanins( pNode, ppFanins[i] );
            return Ntk_NodeRelinkFanins( pNode );
        }

    // mark each fanin with its number in the new order
    for ( i = 0; i < nFanins; i++ )
        ppFanins[i]->pCopy = (Ntk_Node_t *)i;

    // create the new reordered array of pins
    ppPins = ALLOC( Ntk_Pin_t *, nFanins );
    // create variable permuation 
    // mapping a new fanin place into an old fanin place
    pPermuteInv = ALLOC( int, nFanins + 1 );
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
    {
        ppPins[ (int)pFanin->pCopy ] = pPin;
        pPermuteInv[ (int)pFanin->pCopy ] = i;
    }
    // set the fanout variable
    pPermuteInv[nFanins] = nFanins;

    // link the fanin pins in the new order
    pNode->lFanins.pHead = ppPins[0];
    pNode->lFanins.pHead->pPrev = NULL;
    pNode->lFanins.pTail = ppPins[nFanins-1];
    pNode->lFanins.pTail->pNext = NULL;
    for ( i = 1; i < nFanins; i++ )
    {
        ppPins[i-1]->pNext = ppPins[i];
        ppPins[i]->pPrev   = ppPins[i-1];
    }
    FREE( ppPins );
    return pPermuteInv;
}


/**Function*************************************************************

  Synopsis    [Compacts the fanins of the node.]

  Description [The node is known to have duplicated fanins. The given 
  fanin is one of the duplicated fanins. There may be other pairs.
  This procedure removes only the first redundancy encountered.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeCompactFanins( Ntk_Node_t * pNode, Ntk_Node_t * pFaninDup )
{
    Mvr_Relation_t * pMvr;
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int iFanin1, iFanin2, i;

    // detect the first pair of identical fanins
    iFanin1 = iFanin2 = -1;
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
        if ( pFanin == pFaninDup )
        {
            if ( iFanin1 == -1 )
                iFanin1 = i;
            else if ( iFanin2 == -1 )
            {
                iFanin2 = i;
                break;
            }
        }
    assert( iFanin1 >= 0 && iFanin2 >= 0 ); // the node should have dup fanins

    // get the relation of the node
    pMvr = Ntk_NodeGetFuncMvr( pNode );
    // quantify the variables in this relation
    Mvr_RelationMakeVarsEqual( pMvr, iFanin1, iFanin2 );
    // free the other representations
    Ntk_NodeSetFuncMvr( pNode, pMvr );
    // make the relation minimum base
    Ntk_NodeMakeMinimumBase( pNode );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


