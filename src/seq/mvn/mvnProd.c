/**CFile****************************************************************

  FileName    [mvnProd.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Compute the MV network representing the product of two FSMs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnProd.c,v 1.5 2004/02/19 03:11:18 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Computes the product of two automata.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Mvn_NetAutomataProduct( 
    Ntk_Network_t * pNet1, Ntk_Network_t * pNet2,
    char * ppNames1[], char * ppNames2[], int nPairs )
{
    Ntk_Node_t * pNode1, * pNode2, * pNode;
    Ntk_Node_t * pDriver1, * pDriver2, * pNodePseudo;
    Ntk_Node_t * ppNodes[2];
    int i;

    // make sure they have one output
    if ( Ntk_NetworkReadCoTrueNum(pNet1) != 1 )
    {
        fprintf( stdout, "The first network has more than one PO.\n" );
        return NULL;
    }
    if ( Ntk_NetworkReadCoTrueNum(pNet2) != 1 )
    {
        fprintf( stdout, "The second network has more than one PO.\n" );
        return NULL;
    }

    // get the true outputs
    Ntk_NetworkForEachCo( pNet1, pNode1 )
        if ( pNode1->Subtype != MV_BELONGS_TO_LATCH )
            break;
    Ntk_NetworkForEachCo( pNet2, pNode2 )
        if ( pNode2->Subtype != MV_BELONGS_TO_LATCH )
            break;

    // patch the name of the outputs
    Ntk_NetworkChangeNodeName( pNode1, "Out1" );
    Ntk_NetworkChangeNodeName( pNode2, "Out2" );

    // combine the networks together
    Ntk_NetworkAppend( pNet1, pNet2 );

    // patch the nodes according to the spec
    for ( i = 0; i < nPairs; i++ )
    {
        pNode1 = Ntk_NetworkFindNodeByName( pNet1, ppNames1[i] );
        pNode2 = Ntk_NetworkFindNodeByName( pNet1, ppNames2[i] );
        assert( pNode1->nValues == pNode2->nValues );
        assert( pNode1->Type == MV_NODE_CI );
        assert( pNode2->Type == MV_NODE_CI );
        // create the pseudo-input
        pNodePseudo = Ntk_NodeCreateConstant( pNet1, pNode1->nValues, FT_MV_MASK(pNode1->nValues) );
        // add it to the network
        Ntk_NetworkAddNode( pNet1, pNodePseudo, 1 );
        // transform the CIs to point to the pseudo-input
        Ntk_NetworkTransformCi( pNode1, pNodePseudo );
        Ntk_NetworkTransformCi( pNode2, pNodePseudo );
    }

    // make the single output
    assert( Ntk_NetworkReadCoTrueNum(pNet1) == 2 );
    pNode1 = pNode2 = NULL;
    Ntk_NetworkForEachCo( pNet1, pNode )
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
        {
            if ( pNode1 == NULL )
                pNode1 = pNode;
            else if ( pNode2 == NULL )
                pNode2 = pNode;
            else
            {
                assert( 0 );
            }
        }
    assert( pNode1 && pNode2 );

    // remove the output nodes
    pDriver1 = Ntk_NodeReadFaninNode( pNode1, 0 );
    pDriver2 = Ntk_NodeReadFaninNode( pNode2, 0 );
    Ntk_NetworkDeleteNode( pNet1, pNode1, 1, 1 );
    Ntk_NetworkDeleteNode( pNet1, pNode2, 1, 1 );

    // add the node that is the product of the two nodes
    ppNodes[0] = pDriver1;
    ppNodes[1] = pDriver2;
    pNode = Ntk_NodeCreateTwoInputBinary( pNet1, ppNodes, 8 );
    pNode->pName = Ntk_NetworkRegisterNewName( pNet1, "Out" );
    Ntk_NetworkAddNode( pNet1, pNode, 1 );

    // add the output node
    Ntk_NetworkAddNodeCo( pNet1, pNode, 1 );

    if ( !Ntk_NetworkCheck( pNet1 ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet1), "Fsm_NetAutomataProduct(): Network check has failed.\n" );
    return pNet1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


