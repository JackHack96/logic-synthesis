/**CFile****************************************************************

  FileName    [wnStrash.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Performs structural hashing on the logic in the window.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wnStrashBin.c,v 1.3 2004/05/12 04:30:11 alanmi Exp $]

***********************************************************************/

#include "wnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define WN_STRASH_READ1(pNode)      ((Sh_Node_t *)pNode->pCopy)
#define WN_STRASH_READ2(pNode)      ((Sh_Node_t *)pNode->pData)
#define WN_STRASH_WRITE1(pNode,p)   (pNode->pCopy = (Ntk_Node_t *)p)
#define WN_STRASH_WRITE2(pNode,p)   (pNode->pData = (char *)p)

static void Wn_WindowStrashBinaryNode( Sh_Manager_t * pMan, Ntk_Node_t * pNode, Ntk_Node_t * pNodeCore );
static void Wn_WindowStrashBinaryNode_old( Sh_Manager_t * pMan, Ntk_Node_t * pNode, Ntk_Node_t * pNodeCore );
static Sh_Node_t * Wn_WindowStrashBinaryForm_rec( Sh_Manager_t * pMan, Ft_Tree_t * pFtTree, Ft_Node_t * pFtNode );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Strashes the binary window with one special node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Wn_WindowStrashBinaryMiter( Sh_Manager_t * pMan, Wn_Window_t * pWnd )
{
    Sh_Network_t * pShNet;
    Sh_Node_t ** pgInputs;
    Sh_Node_t ** pgOutputs;
    Sh_Node_t * gNode, * gNode1, * gNode2, * gExor, * gTemp;
    Ntk_Node_t * pNode, * pNodeCore, * pFanin;
    Ntk_Pin_t * pPin;
    int nFanins, i, k;
//    int RetValue;

    if ( pWnd->nLevelsTfi == 0 && pWnd->nLevelsTfo == 0 )
        return NULL;

    // make sure that there is exactly one core node
//    assert( pWnd->pWndCore );
//    assert( pWnd->pWndCore->nRoots == 1 );
//    pNodeCore = pWnd->pWndCore->ppRoots[0];
    pNodeCore = pWnd->pNode;
    nFanins = Ntk_NodeReadFaninNum( pNodeCore );

    // start the network
    pShNet  = Sh_NetworkCreate( pMan, pWnd->nLeaves, 1 + nFanins );

    // DFS order the nodes in the window
//    RetValue = Wn_WindowDfs( pWnd );
//    assert( RetValue );

    // clean the data of the nodes in the window
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode ){pNode->pCopy = NULL;pNode->pData = NULL;}

    // set the leaves
    pgInputs = Sh_ManagerReadVars( pMan );
    for ( i = 0; i < pWnd->nLeaves; i++ )
    {
        WN_STRASH_WRITE1( pWnd->ppLeaves[i], pgInputs[i] );  shRef( pgInputs[i] );
        WN_STRASH_WRITE2( pWnd->ppLeaves[i], pgInputs[i] );  shRef( pgInputs[i] );
    }

    // build the network for the roots taking the core node into account
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
    {
        if ( pNode->pCopy )
            continue;
        Wn_WindowStrashBinaryNode( pMan, pNode, pNodeCore );
    }

    // derive the miter for the output nodes
    gNode = Sh_Not( Sh_ManagerReadConst1( pMan ) );   shRef( gNode );
    for ( i = 0; i < pWnd->nRoots; i++ )
    {
        gNode1 = WN_STRASH_READ1( pWnd->ppRoots[i] );
        gNode2 = WN_STRASH_READ2( pWnd->ppRoots[i] );
        if ( gNode1 != gNode2 )
        {
            gExor = Sh_NodeExor( pMan, gNode1, gNode2 );      shRef( gExor );
            gNode = Sh_NodeOr( pMan, gTemp = gNode, gExor );  shRef( gNode );
            Sh_RecursiveDeref( pMan, gExor );
            Sh_RecursiveDeref( pMan, gTemp );
        }
    }
    // write the fanin outputs first, then miter
    pgOutputs = Sh_NetworkReadOutputs( pShNet );
    Ntk_NodeForEachFaninWithIndex( pNodeCore, pPin, pFanin, k )
    {
        pgOutputs[k] = WN_STRASH_READ1( pFanin ); shRef( pgOutputs[k] );
    }
    pgOutputs[nFanins] = gNode;   // takes ref
//if ( gNode != pMan->pConst1 )
//    Sh_NodeWriteBlif( pMan, gNode, "sat_cdc_aig.blif" );

    // dereference intermediate Sh_Node's
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
    {
        gNode1 = WN_STRASH_READ1( pNode );
        gNode2 = WN_STRASH_READ2( pNode );
        Sh_RecursiveDeref( pMan, gNode2 );
        Sh_RecursiveDeref( pMan, gNode1 );
    }
    return pShNet;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashBinaryNode( Sh_Manager_t * pMan, Ntk_Node_t * pNode, Ntk_Node_t * pNodeCore )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    Sh_Node_t * gNode, * gNode1, * gNode2;
    Cvr_Cover_t * pCvr;
    Ft_Tree_t * pTree;
    int fSkipSecond, i;

    // get the factored form of this node
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );

    // init the leaves of the factored form
    // and determine if the second part is present
    fSkipSecond = 1;
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
    {
        gNode1 = WN_STRASH_READ1( pFanin );
        gNode2 = WN_STRASH_READ2( pFanin );
        pTree->uLeafData[i] = (unsigned)gNode1;
        if ( fSkipSecond && gNode1 != gNode2 )
            fSkipSecond = 0;
    }
    // strash the factored form
    if ( pTree->pRoots[0] == NULL )
    {
        gNode = Wn_WindowStrashBinaryForm_rec( pMan, pTree, pTree->pRoots[1] );
        WN_STRASH_WRITE1( pNode, gNode );   shRef( gNode );
        if ( fSkipSecond )
        {
            WN_STRASH_WRITE2( pNode, gNode );   shRef( gNode );
        }
    }
    else
    {
        gNode = Wn_WindowStrashBinaryForm_rec( pMan, pTree, pTree->pRoots[0] );
        WN_STRASH_WRITE1( pNode, Sh_Not(gNode) );   shRef( gNode );
        if ( fSkipSecond )
        {
            WN_STRASH_WRITE2( pNode, Sh_Not(gNode) );   shRef( gNode );
        }
    }
    // if this is the core node, permute it
    if ( pNode == pNodeCore )
    {
        assert( fSkipSecond );
        gNode1 = WN_STRASH_READ1( pNode );
        gNode2 = WN_STRASH_READ2( pNode );
        assert( gNode1 == gNode2 );
        WN_STRASH_WRITE2( pNode, Sh_Not(gNode1) );
    }
    // quit if the second part is not needed
    if ( fSkipSecond )
        return;

    // init the leaves the factored form
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
        pTree->uLeafData[i] = (unsigned)WN_STRASH_READ2( pFanin );

    // strash the factored form
    if ( pTree->pRoots[0] == NULL )
    {
        gNode = Wn_WindowStrashBinaryForm_rec( pMan, pTree, pTree->pRoots[1] );
        WN_STRASH_WRITE2( pNode, gNode );   shRef( gNode );
    }
    else
    {
        gNode = Wn_WindowStrashBinaryForm_rec( pMan, pTree, pTree->pRoots[0] );
        WN_STRASH_WRITE2( pNode, Sh_Not(gNode) );   shRef( gNode );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashBinaryNode_old( Sh_Manager_t * pMan, Ntk_Node_t * pNode, Ntk_Node_t * pNodeCore )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    Sh_Node_t * gNode;
    Cvr_Cover_t * pCvr;
    Ft_Tree_t * pTree;
    int i;

    // get the factored form of this node
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );

    // init the leaves the factored form
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
        pTree->uLeafData[i] = (unsigned)WN_STRASH_READ1( pFanin );
    // strash the factored form
    if ( pTree->pRoots[0] == NULL )
    {
        gNode = Wn_WindowStrashBinaryForm_rec( pMan, pTree, pTree->pRoots[1] );
        WN_STRASH_WRITE1( pNode, gNode );   shRef( gNode );
    }
    else
    {
        gNode = Wn_WindowStrashBinaryForm_rec( pMan, pTree, pTree->pRoots[0] );
        WN_STRASH_WRITE1( pNode, Sh_Not(gNode) );   shRef( gNode );
    }

    // init the leaves the factored form
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
        pTree->uLeafData[i] = (unsigned)WN_STRASH_READ2( pFanin );
    // strash the factored form
    if ( pTree->pRoots[0] == NULL )
    {
        gNode = Wn_WindowStrashBinaryForm_rec( pMan, pTree, pTree->pRoots[1] );
        WN_STRASH_WRITE2( pNode, gNode );   shRef( gNode );
    }
    else
    {
        gNode = Wn_WindowStrashBinaryForm_rec( pMan, pTree, pTree->pRoots[0] );
        WN_STRASH_WRITE2( pNode, Sh_Not(gNode) );   shRef( gNode );
    }

    // if this is the core node, permute it
    if ( pNode == pNodeCore )
        WN_STRASH_WRITE2( pNode, Sh_Not(WN_STRASH_READ1(pNode)) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Wn_WindowStrashBinaryForm_rec( Sh_Manager_t * pMan, Ft_Tree_t * pFtTree, Ft_Node_t * pFtNode )
{
    Sh_Node_t * pNode1, * pNode2, * pNode;    
    if ( pFtNode->Type == FT_NODE_LEAF )
    {
        if ( pFtNode->uData == 1 )
            return Sh_Not( pFtTree->uLeafData[pFtNode->VarNum] );
        if ( pFtNode->uData == 2 )
            return (Sh_Node_t *)pFtTree->uLeafData[pFtNode->VarNum];
        assert( 0 );
        return NULL;
    }
    if ( pFtNode->Type == FT_NODE_AND )
    {
        pNode1 = Wn_WindowStrashBinaryForm_rec( pMan, pFtTree, pFtNode->pOne );     shRef( pNode1 );
        pNode2 = Wn_WindowStrashBinaryForm_rec( pMan, pFtTree, pFtNode->pTwo );     shRef( pNode2 );
        pNode = Sh_NodeAnd( pMan, pNode1, pNode2 );                                 shRef( pNode );
        Sh_RecursiveDeref( pMan, pNode1 );
        Sh_RecursiveDeref( pMan, pNode2 );
        shDeref( pNode );
        return pNode;
    }
    if ( pFtNode->Type == FT_NODE_OR )
    {
        pNode1 = Wn_WindowStrashBinaryForm_rec( pMan, pFtTree, pFtNode->pOne );     shRef( pNode1 );
        pNode2 = Wn_WindowStrashBinaryForm_rec( pMan, pFtTree, pFtNode->pTwo );     shRef( pNode2 );
        pNode = Sh_NodeOr( pMan, pNode1, pNode2 );                                  shRef( pNode );
        Sh_RecursiveDeref( pMan, pNode1 );
        Sh_RecursiveDeref( pMan, pNode2 );
        shDeref( pNode );
        return pNode;
    }
    if ( pFtNode->Type == FT_NODE_0 )
        return Sh_Not( Sh_ManagerReadConst1( pMan ) );
    if ( pFtNode->Type == FT_NODE_1 )
        return Sh_ManagerReadConst1( pMan );
    assert( 0 ); // unknown node type
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


