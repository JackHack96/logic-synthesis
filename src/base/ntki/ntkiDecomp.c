/**CFile****************************************************************

  FileName    [ntkiDecomp.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Decomposition of the network using factored forms.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiDecomp.c,v 1.5 2003/11/18 18:54:55 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Node_t * Ntk_NetworkDecompOne( Ntk_Node_t * pNode, Ft_Tree_t * pTree );
static Ntk_Node_t * Ntk_NetworkDecompOne_rec( Ntk_Node_t * pNodeInit, Ft_Node_t * pNodeFt );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkDecomp( Ntk_Network_t * pNet )
{
    Ntk_Node_t * ppFanins[32];
    Ntk_Node_t * pNode, * pNodeNew;
    Ft_Tree_t * pTree;
    int nFanins, i;

    // go through the internal nodes in the DFS order
    Ntk_NetworkDfs( pNet, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        // factor the node
        pTree = Cvr_CoverFactor( Ntk_NodeGetFuncCvr( pNode ) );
        // transform the FF into a cone of two-input nodes
        pNodeNew = Ntk_NetworkDecompOne( pNode, pTree );
        // replace the old node
        Ntk_NodeReplace( pNode, pNodeNew ); // disposes of pNodeNew
        // eliminate the fanins of this node into it
        if ( pNode->nValues > 2 )
        {
            // collect the fanins of this node
            nFanins = Ntk_NodeReadFanins( pNode, ppFanins );
            for ( i = 0; i < nFanins; i++ )
                Ntk_NetworkCollapseNodes( pNode, ppFanins[i] );
        }
    }

    Ntk_NetworkSweep( pNet, 1, 1, 0, 0 );
    if ( !Ntk_NetworkCheck( pNet ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkDecomp(): Network check has failed.\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkDecompOne( Ntk_Node_t * pNode, Ft_Tree_t * pTree )
{
    Ntk_Node_t * pNodeIsets[32];
    Ntk_Node_t * pCollector;
    int DefValue;
    int i;
    // get the default value if any
    DefValue = -1;
    for ( i = 0; i < pTree->nRoots; i++ )
        if ( pTree->pRoots[i] == NULL )
        {
            pNodeIsets[i] = NULL;
            DefValue = i;
        }
        else
            pNodeIsets[i] = Ntk_NetworkDecompOne_rec( pNode, pTree->pRoots[i] );

    // get the collector
    pCollector = Ntk_NodeCreateCollector( pNode->pNet, pNodeIsets, pTree->nRoots, DefValue );
    // note that the collector fanins are ordered
    // do not add the collector to the network
    return pCollector;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkDecompOne_rec( Ntk_Node_t * pNodeInit, Ft_Node_t * pNodeFt )
{
    Ntk_Node_t * pNode, * pFanin;
    Ntk_Node_t * ppNodes[2];
    unsigned Pols[2];

    assert( pNodeFt->Type != FT_NODE_NONE );
    if ( pNodeFt->Type == FT_NODE_0 )
        pNode = Ntk_NodeCreateConstant( pNodeInit->pNet, 2, 1 );
    else if ( pNodeFt->Type == FT_NODE_1 )
        pNode = Ntk_NodeCreateConstant( pNodeInit->pNet, 2, 2 );
    else if ( pNodeFt->Type == FT_NODE_LEAF )
    {
        pFanin = Ntk_NodeReadFaninNode( pNodeInit, pNodeFt->VarNum );
        assert( pFanin->nValues == pNodeFt->nValues );
        Pols[0] = pNodeFt->uData ^ FT_MV_MASK( pFanin->nValues );
        Pols[1] = pNodeFt->uData;
        pNode = Ntk_NodeCreateOneInputNode( pNodeInit->pNet, pFanin, pFanin->nValues, 2, Pols );
    }
    else
    {
        ppNodes[0] = Ntk_NetworkDecompOne_rec( pNodeInit, pNodeFt->pOne );
        ppNodes[1] = Ntk_NetworkDecompOne_rec( pNodeInit, pNodeFt->pTwo );
        if ( pNodeFt->Type == FT_NODE_AND )
            pNode = Ntk_NodeCreateTwoInputBinary( pNodeInit->pNet, ppNodes, 8 );
        else if ( pNodeFt->Type == FT_NODE_OR )
            pNode = Ntk_NodeCreateTwoInputBinary( pNodeInit->pNet, ppNodes, 14 );
        else
        {
            assert( 0 );
        }
    }
    // add this node to the network
    Ntk_NetworkAddNode( pNodeInit->pNet, pNode, 1 );
    return pNode;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


