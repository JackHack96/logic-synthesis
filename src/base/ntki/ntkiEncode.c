/**CFile****************************************************************

  FileName    [ntkiEncode.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Performs the arbitrary encoding of the node.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 15, 2003.]

  Revision    [$Id: ntkiEncode.c,v 1.2 2003/09/01 04:56:05 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Encodes the code to reduce the number of values.]

  Description [The node is the node to be encoded. The two numbers are
  the numbers of values of the two nodes after from the encoding.
  The product of these two number should not be less than the number
  of values of the given node. 
  The encoding looks like merging in reverse order. Originally we have
  pNode -> {Fanout1, Fanout2,...}. As an intermediate step we introduce
  pNode -> {pNodeEnc1, pNodeEnc2} -> pNodeDec -> {Fanout1, Fanout2,...}.
  The encoders transform the pNode->nValues into {nValues1, nValues2}. 
  The decoder transforms {nValues1, nValues2} back into pNode->nValues.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkEncodeNode( Ntk_Node_t * pNode, int nValues1, int nValues2 )
{
    unsigned Pols[32];
    Ntk_Node_t * pNodeEnc1, * pNodeEnc2, * pNodeDec;
    Ntk_Node_t * pNode1, * pNode2;
    Ntk_Network_t * pNet;
    Ntk_Node_t ** ppFanouts;
    Ntk_Node_t * pTrans1, * pTrans2;
    int nFanouts, i;

    // get the pointer to the network
    pNet = pNode->pNet;

    // makes sure that the node to be encoded is internal and the number of values is okay
    assert( pNode->Type == MV_NODE_INT );
    assert( nValues1 * nValues2 <= 32 );
    if ( pNode->nValues > nValues1 * nValues2 )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkReduceNodeValue(): Cannot encode the node with the given values.\n" );
        return;
    }

    // pad the i-sets of pNode with the empty i-sets corresponding to 
    // the extra values (nValues1 * nValues2 - pNode->nValues)
    if ( pNode->nValues < nValues1 * nValues2 )
    {
        // create the first transformer
        for ( i = 0; i < pNode->nValues; i++ )
            Pols[i] = ( 1 << i );
        for (      ; i < nValues1 * nValues2; i++ )
            Pols[i] = 0;
        pTrans1 = Ntk_NodeCreateOneInputNode( pNet, pNode, pNode->nValues, nValues1 * nValues2, Pols );
        // create the second transformer
        for ( i = 0; i < pNode->nValues; i++ )
            Pols[i] = ( 1 << i );
        pTrans2 = Ntk_NodeCreateOneInputNode( pNet, pTrans1, nValues1 * nValues2, pNode->nValues, Pols );

        // transform the node and dispose of the transformers
        Ntk_NetworkSweepTransformNode( pNode, pTrans1, pTrans2 );
        // update the node pointer
        pNode = pTrans1;
    }
    assert( pNode->nValues == nValues1 * nValues2 ); 


    // derive the pair of encoders transforming pNode into pNode1 and pNode2
    Ntk_NodeCreateEncoders( pNet, pNode, nValues1, nValues2, &pNodeEnc1, &pNodeEnc2 );
    // collapse pNode into the encoders to derive pNode1 and pNode2
    pNode1 = Ntk_NodeCollapse( pNodeEnc1, pNode );
    pNode2 = Ntk_NodeCollapse( pNodeEnc2, pNode );
    // make the nodes minimum-base
    Ntk_NodeMakeMinimumBase( pNode1 );
    Ntk_NodeMakeMinimumBase( pNode2 );
    // add the new nodes to the network
    Ntk_NetworkAddNode( pNet, pNode1, 1 );
    Ntk_NetworkAddNode( pNet, pNode2, 1 );
    // delete the encoders
    Ntk_NodeDelete( pNodeEnc1 );
    Ntk_NodeDelete( pNodeEnc2 );
    // at this point pNode1 and pNode2 are in the network w/o fanouts
    // their fanouts are the fanouts of pNode


    // derive the decoder transforming pNode1 and pNode2 into pNode
    pNodeDec = Ntk_NodeCreateDecoder( pNet, pNode1, pNode2 );
    Ntk_NodeOrderFanins( pNodeDec );
    assert( pNodeDec->nValues == pNode->nValues );

    // replace pNode by the decoder
    Ntk_NodeReplace( pNode, pNodeDec );
    // at this point pNodeDec is in the network and has the same fanouts as pNode

    // collapse the decoder into its fanouts
    // store the fanouts 
    nFanouts = Ntk_NodeReadFanoutNum( pNode );
    ppFanouts = ALLOC( Ntk_Node_t *, nFanouts );
    nFanouts = Ntk_NodeReadFanouts( pNode, ppFanouts );
    // for each fanout, patch fanin corresponding to pNode 
    // with pNodeDec and collapse pNodeDec into the fanout
    for ( i = 0; i < nFanouts; i++ )
    {
        // patch the fanin of the fanout to point to the transformer node
//        Ntk_NodePatchFanin( ppFanouts[i], pNode, pNodeDec );
        // there is no need to patch the fanin because the node is the same

        // consider the case when the fanout is a PO
        if ( ppFanouts[i]->Type == MV_NODE_CO )
            continue;
        // collapse pTrans2 into ppFanouts[i]
        Ntk_NetworkCollapseNodes( ppFanouts[i], pNode );
    }

    // if pNodeDec has some CO fanouts, pNodeDec stays in the network
    if ( Ntk_NodeReadFanoutNum( pNode ) == 0 )
        Ntk_NetworkDeleteNode( pNet, pNode, 1, 1 );

    if ( !Ntk_NetworkCheck( pNet ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkReduceNodeValue(): Network check has failed.\n" );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


