/**CFile****************************************************************

  FileName    [wnStrash.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Performs structural hashing on the logic in the window.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wnStrash.c,v 1.4 2004/04/12 20:41:09 alanmi Exp $]

***********************************************************************/

#include "wnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void        Wn_WindowStrashCollect( Sh_Manager_t * pMan, Vm_VarMap_t * pVm, Ntk_Node_t * ppNodes[], int nNodes, Sh_Node_t * ppShInputs[], int nOutputs  );
static void        Wn_WindowStrashNode( Sh_Manager_t * pMan, Ntk_Node_t * pNode, Sh_Node_t * ppShInputs[] );
static Sh_Node_t * Wn_WindowStrashNode_rec( Sh_Manager_t * pMan, Ft_Tree_t * pTree, Ft_Node_t * pFtNode, Sh_Node_t * ppShInputs[] );
static Sh_Node_t * Wn_WindowStrashNodeOr( Sh_Manager_t * pMan, Ft_Tree_t * pTree, Sh_Node_t * ppShInputs[], unsigned Signature, int nNodes );
static void        Wn_WindowStrashDeref( Sh_Manager_t * pMan, Ntk_Node_t * pNode );
static void        Wn_WindowStrashAddUnique( Sh_Manager_t * pMan, Ntk_Node_t * pNode );

static void        Wn_WindowStrashPrint( Sh_Manager_t * pMan, Wn_Window_t * pWnd );
static void        Wn_WindowStrashPrintOne( Sh_Manager_t * pMan, Ntk_Node_t * pNode );


static void        Wn_WindowStrashCoreConstruct_rec( Sh_Manager_t * pMan, Ntk_Node_t * pNode, Wn_Window_t * pWnd );
static void        Wn_WindowStrashCoreClean( Sh_Manager_t * pMan, Ntk_Node_t * pNode );
static void        Wn_WindowStrashCoreClean_rec( Ntk_Node_t * pNode );
static bool        Wn_WindowStrashCoreNodeIsStrashed( Ntk_Node_t * pNode );
static void        Wn_WindowStrashCoreCleanStrashings( Ntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives strashed network SS-equivalent to nodes in the window.]

  Description [If the original window has a core, this core is marked.
  If the flag fUseUnique is set, we mark the special nodes and introduce
  unique AND gates at the outputs of these nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Wn_WindowStrash( Sh_Manager_t * pMan, Wn_Window_t * pWnd, bool fUseUnique )
{
    Sh_Network_t * pShNet;
    Sh_Node_t ** ppShInputs;
    Sh_Node_t ** ppShOutputs;
    Sh_Node_t ** ppShNodes;
    Ntk_Node_t * pNode, ** ppNodes;
    int nNodes, nValuesIn;
    int nShInputs, nShOutputs;
    int RetValue, i;

    // to be fixed later
//    ppNodes    = ALLOC( Ntk_Node_t *, 1000 );
//    ppShInputs = ALLOC( Sh_Node_t *,  1000 );
    ppNodes    = pWnd->pNet->pArray1;
    ppShInputs = (Sh_Node_t **)pWnd->pNet->pArray2;

    // create the MV var maps for the window
    Wn_WindowCreateVarMaps( pWnd );

    // start the network
    nShInputs  = Vm_VarMapReadValuesInNum( pWnd->pVmL );
    nShOutputs = Vm_VarMapReadValuesInNum( pWnd->pVmR );
    pShNet     = Sh_NetworkCreate( pMan, nShInputs, nShOutputs );
    Sh_NetworkSetVmL( pShNet, pWnd->pVmL );
    Sh_NetworkSetVmR( pShNet, pWnd->pVmR );
    Sh_NetworkSetVmS( pShNet, pWnd->pVmS );

    // DFS order the nodes in the window
    RetValue = Wn_WindowDfs( pWnd );
    assert( RetValue );

    // mark the special nodes and the root nodes
    if ( fUseUnique )
    {
        Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
            pNode->pData = (char*)0;
        for ( i = 0; i < pWnd->pWndCore->nRoots; i++ )
            pWnd->pWndCore->ppRoots[i]->pData = (char*)1;
        for ( i = 0; i < pWnd->nSpecials; i++ )
            pWnd->ppRoots[i]->pData = (char*)1;
    }

    // mark the leaves of the window and set their numbers
    Ntk_NetworkIncrementTravId( pWnd->pNet );
    for ( i = 0; i < pWnd->nLeaves; i++ )
    {
        Ntk_NodeSetTravIdCurrent( pWnd->ppLeaves[i] );
        pWnd->ppLeaves[i]->pCopy = (Ntk_Node_t *)i;
    }

    // go through the nodes in the window
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
    {
        // collect fanins into array
        nNodes = Ntk_NodeReadFanins( pNode, ppNodes );
        // set the input Sh_Nodes
        nValuesIn = Vm_VarMapReadValuesInNum( Ntk_NodeReadFuncVm(pNode) );
        Wn_WindowStrashCollect( pMan, pWnd->pVmL, ppNodes, nNodes, ppShInputs, nValuesIn );
        // strash the node
        Wn_WindowStrashNode( pMan, pNode, ppShInputs );

        // add the unique AND-gates
        if ( fUseUnique && pNode->pData )
            Wn_WindowStrashAddUnique( pMan, pNode );
    }

    // collect the pointers to the roots after strashing
    ppShOutputs = Sh_NetworkReadOutputs( pShNet );
    Wn_WindowStrashCollect( pMan, pWnd->pVmL, pWnd->ppRoots, pWnd->nRoots, ppShOutputs, nShOutputs );
    // reference them nodes
    for ( i = 0; i < nShOutputs; i++ )
        Sh_Ref( ppShOutputs[i] );

    // if the core is defined, collect the pointers to the leaves/roots after strashing
    if ( pWnd->pWndCore )
    {
        // create the MV var maps for the core window
        Wn_WindowCreateVarMaps( pWnd->pWndCore );
        Sh_NetworkSetVmLC( pShNet, pWnd->pWndCore->pVmL );
        Sh_NetworkSetVmRC( pShNet, pWnd->pWndCore->pVmR );
 
        // collect the core leaves
        nShInputs = Vm_VarMapReadValuesInNum( pWnd->pWndCore->pVmL );
        ppShNodes = Sh_NetworkAllocInputsCore( pShNet, nShInputs );
        Wn_WindowStrashCollect( pMan, pWnd->pVmL, 
            pWnd->pWndCore->ppLeaves, pWnd->pWndCore->nLeaves, ppShNodes, nShInputs );
        for ( i = 0; i < nShInputs; i++ )
            Sh_Ref( ppShNodes[i] );

        // collect the core roots
        nShOutputs = Vm_VarMapReadValuesInNum( pWnd->pWndCore->pVmR );
        ppShNodes  = Sh_NetworkAllocOutputsCore( pShNet, nShOutputs );
        Wn_WindowStrashCollect( pMan, pWnd->pVmL, 
            pWnd->pWndCore->ppRoots, pWnd->pWndCore->nRoots, ppShNodes, nShOutputs );
        for ( i = 0; i < nShOutputs; i++ )
            Sh_Ref( ppShNodes[i] );
    }

    // if the window has special nodes, collect the pointers after strashing
    if ( pWnd->nSpecials > 0 )
    {
        // collect the special nodes
        nShInputs = Vm_VarMapReadValuesInNum( pWnd->pVmS );
        ppShNodes = Sh_NetworkAllocSpecials( pShNet, nShInputs );
        Wn_WindowStrashCollect( pMan, pWnd->pVmL, 
            pWnd->pWndCore->ppSpecials, pWnd->pWndCore->nSpecials, ppShNodes, nShInputs );
        for ( i = 0; i < nShInputs; i++ )
            Sh_Ref( ppShNodes[i] );
    }
//    Wn_WindowStrashPrint( pMan, pWnd );

    // dereference the temporary pointers in the nodes
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
        Wn_WindowStrashDeref( pMan, pNode );

//    FREE( ppNodes );
//    FREE( ppShInputs );
    return pShNet;
}

/**Function*************************************************************

  Synopsis    [Derives strashed network SS-equivalent to nodes in the window.]

  Description [If the original window has a core, this core is marked.
  If the flag fUseUnique is set, we mark the special nodes and introduce
  unique AND gates at the outputs of these nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Wn_WindowStrashCore( Sh_Manager_t * pMan, Wn_Window_t * pWnd, Wn_Window_t * pWndC )
{
    Sh_Network_t * pShNet;
    Sh_Node_t ** ppShInputs;
    Sh_Node_t ** ppShOutputs;
    Sh_Node_t ** ppShNodes;
    Ntk_Node_t ** ppNodes;
    int nShInputs, nShOutputs;
//    Ntk_Node_t * pNode;
    int i;

    // to be fixed later
//    ppNodes    = ALLOC( Ntk_Node_t *, 1000 );
//    ppShInputs = ALLOC( Sh_Node_t *,  1000 );
    ppNodes    = pWnd->pNet->pArray1;
    ppShInputs = (Sh_Node_t **)pWnd->pNet->pArray2;

    // create the MV var maps for the window
    Wn_WindowCreateVarMaps( pWndC );

    assert( pWnd->pVmL );
    assert( pWndC->pVmL );

    // start the network
    nShInputs  = Vm_VarMapReadValuesInNum( pWnd->pVmL );
    nShOutputs = Vm_VarMapReadValuesInNum( pWnd->pVmR );
    pShNet     = Sh_NetworkCreate( pMan, nShInputs, nShOutputs );
    Sh_NetworkSetVmL( pShNet, pWnd->pVmL );
    Sh_NetworkSetVmR( pShNet, pWnd->pVmR );
    Sh_NetworkSetVmS( pShNet, pWnd->pVmS );
    Sh_NetworkSetVmLC( pShNet, pWndC->pVmL );
    Sh_NetworkSetVmRC( pShNet, pWndC->pVmR );

    // set the numbers of the leaves
    for ( i = 0; i < pWnd->nLeaves; i++ )
        pWnd->ppLeaves[i]->pCopy = (Ntk_Node_t *)i;

//    i = 0;
//    Ntk_NetworkForEachCi( pWndC->pNet, pNode )
//        pNode->pCopy = (Ntk_Node_t *)i;

//Ntk_NetworkForEachNode( pWnd->pNet, pNode )
//    Wn_WindowStrashCoreCleanStrashings( pNode );

    // clean the TFOs of roots
    for ( i = 0; i < pWndC->nRoots; i++ )
        Wn_WindowStrashCoreClean( pMan, pWndC->ppRoots[i] );

    // increment trav id to prevent clashes with Wn_WindowStrashCollect()
    Ntk_NetworkIncrementTravId( pWnd->pNet );

    // construct the Sh_Node_t pointers for the outputs of the node
    for ( i = 0; i < pWndC->nRoots; i++ )
        Wn_WindowStrashCoreConstruct_rec( pMan, pWndC->ppRoots[i], pWnd );

    // set the unique outputs for these nodes
    for ( i = 0; i < pWndC->nRoots; i++ )
        Wn_WindowStrashAddUnique( pMan, pWndC->ppRoots[i] );

    // construct the Sh_Node_t pointers for the outputs of the network
    for ( i = 0; i < pWnd->nRoots; i++ )
        Wn_WindowStrashCoreConstruct_rec( pMan, pWnd->ppRoots[i], pWnd );

    // collect the pointers to the roots after strashing
    ppShOutputs = Sh_NetworkReadOutputs( pShNet );
    Wn_WindowStrashCollect( pMan, pWnd->pVmL, pWnd->ppRoots, pWnd->nRoots, ppShOutputs, nShOutputs );
 
    // collect the core leaves
    nShInputs = Vm_VarMapReadValuesInNum( pWndC->pVmL );
    ppShNodes = Sh_NetworkAllocInputsCore( pShNet, nShInputs );
    Wn_WindowStrashCollect( pMan, pWnd->pVmL, 
        pWndC->ppLeaves, pWndC->nLeaves, ppShNodes, nShInputs );

    // collect the core roots
    nShOutputs = Vm_VarMapReadValuesInNum( pWndC->pVmR );
    ppShNodes  = Sh_NetworkAllocOutputsCore( pShNet, nShOutputs );
    Wn_WindowStrashCollect( pMan, pWnd->pVmL, 
        pWndC->ppRoots, pWndC->nRoots, ppShNodes, nShOutputs );

    // clean the PFO of the core roots from the temporary Sh_Node_t pointers
    for ( i = 0; i < pWndC->nRoots; i++ )
        Wn_WindowStrashCoreClean( pMan, pWndC->ppRoots[i] );

//    FREE( ppNodes );
//    FREE( ppShInputs );
    return pShNet;
}


/**Function*************************************************************

  Synopsis    [Collects the result of strashing of the nodes.]

  Description [This procedure assumes that strashing for nodes
  in the array (ppNodes, nNodes) has been finished. It collects the
  entries saved in pTree->uRootData or takes the elementary vars
  if the node is a leaf. The result is returned in ppShInputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashCollect( Sh_Manager_t * pMan, Vm_VarMap_t * pVm, 
    Ntk_Node_t * ppNodes[], int nNodes, Sh_Node_t * ppShInputs[], int nOutputs )
{
    Sh_Node_t * pShNode;
    Ntk_Node_t * pFanin;
    Ft_Tree_t * pTree;
    Cvr_Cover_t * pCvr;
    int * pValues, * pValuesFirst;
    int iValue, iLeaf, i, k;

    pValues = Vm_VarMapReadValuesArray( pVm );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( pVm );

    // prepare the input data for strashing
    iValue = 0;
    for ( i = 0; i < nNodes; i++ )
    {
        pFanin = ppNodes[i];
//        if ( Ntk_NodeIsTravIdCurrent(pFanin) ) // the node is the leaf
        if ( Ntk_NodeIsCi(pFanin) || Ntk_NodeIsTravIdCurrent(pFanin) ) // the node is the leaf
        {
            // get the leaf number
            iLeaf = (int)pFanin->pCopy;
            assert( pValues[iLeaf] == pFanin->nValues );

            // treat the binary case
            if ( pFanin->nValues == 2 )
            {
                // get the positive literal
                pShNode = Sh_ManagerReadVar( pMan, pValuesFirst[iLeaf] + 1 );
                assert( pShNode );
                ppShInputs[iValue++] = Sh_Not(pShNode);
                ppShInputs[iValue++] = pShNode;        
            }
            else
            {
                // set the corresponding Sh_Nodes into uLeafData
                for ( k = 0; k < pValues[iLeaf]; k++ )
                {
                    pShNode = Sh_ManagerReadVar( pMan, pValuesFirst[iLeaf] + k );
                    assert( pShNode );
                    ppShInputs[iValue++] = pShNode;    
                }
            }
        }
        else // the node has already been processed as part of this window
        {
            pCvr = Ntk_NodeGetFuncCvr( pFanin );
            pTree = Cvr_CoverFactor( pCvr );
            assert( pTree->nRoots == pFanin->nValues );

            for ( k = 0; k < pFanin->nValues; k++ )
            {
                // get the node from the root data
                pShNode = (Sh_Node_t *)pTree->uRootData[k];
                assert( pShNode );
                ppShInputs[iValue++] = pShNode;  
            }
        }
    }
    assert( iValue == nOutputs );
}

/**Function*************************************************************

  Synopsis    [Strashes the node and sets the result in root data.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashNode( Sh_Manager_t * pMan, Ntk_Node_t * pNode, Sh_Node_t * ppShInputs[] )
{
    Ft_Tree_t * pTree;
    Cvr_Cover_t * pCvr;
    Sh_Node_t * pShNode;
    unsigned Signature;
    int i, DefValue;


    // get the factored form of this node
    pCvr = Ntk_NodeGetFuncCvr( pNode );
/*
    DefValue = Cvr_CoverReadDefault(pCvr);
    if ( DefValue >= 0 )
    {
        Mvc_Cover_t ** ppIsets;
        ppIsets = Cvr_CoverReadIsets( pCvr );
        ppIsets[DefValue] = Ntk_NodeComputeDefault(pNode);

        Cvr_CoverFreeFactor(pCvr);
    }
*/

    pTree = Cvr_CoverFactor( pCvr );

/*
    DefValue = Cvr_CoverReadDefault(pCvr);
    if ( DefValue >= 0 )
    {
        Mvc_Cover_t ** ppIsets;
        ppIsets = Cvr_CoverReadIsets( pCvr );
        ppIsets[DefValue] = Ntk_NodeComputeDefault(pNode);

        pTree->pRoots[DefValue] = Ft_Factor( pTree, ppIsets[DefValue] );

//       Mvc_CoverFree( ppIsets[DefValue] );
//        ppIsets[DefValue] = NULL;

    }
*/

    DefValue = -1;
    if ( pNode->fNdTfi ) // means that the node is in the TFO of an ND node
    {
        assert( pNode->fNdTfi == 1 );
        DefValue = Cvr_CoverReadDefault(pCvr);
        if ( DefValue >= 0 )
            pTree->pRoots[DefValue] = Ntk_NodeFactorDefault( pNode );
    }

    for ( i = 0; i < pTree->nRoots; i++ )
        if ( pTree->pRoots[i] )
        {
            pShNode = Wn_WindowStrashNode_rec( pMan, pTree, pTree->pRoots[i], ppShInputs );
            shRef( pShNode );
            pTree->uRootData[i] = (unsigned)pShNode;
        }
        else
            pTree->pRoots[i] = NULL;

    // check if there is a default
    for ( i = 0; i < pTree->nRoots; i++ )
        if ( pTree->pRoots[i] == NULL )
        { // build NOR of other i-sets
            // get the signature of this value set
            Signature = (FT_MV_MASK(pTree->nRoots) ^ (1<<i));
            // build OR gate
            pShNode = Wn_WindowStrashNodeOr( pMan, pTree, 
                (Sh_Node_t **)pTree->uRootData, Signature, pTree->nRoots );
            shRef( pShNode );
            // complement
            pShNode = Sh_Not( pShNode );
            // save
            pTree->uRootData[i] = (unsigned)pShNode;
        }

    // remove the temporary default value FF 
    // it is still stored internally in the tree
    if ( DefValue >= 0 )
        pTree->pRoots[DefValue] = NULL;
}

/**Function*************************************************************

  Synopsis    [Recursive implementation of the above.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Wn_WindowStrashNode_rec( Sh_Manager_t * pMan, 
    Ft_Tree_t * pTree, Ft_Node_t * pFtNode, Sh_Node_t * ppShInputs[] )
{
    Sh_Node_t * pNode1, * pNode2, * pNode;
    
    if ( pFtNode->Type == FT_NODE_LEAF )
    {
        int iValueFirst;  
        // find the first value of this var
        iValueFirst = Vm_VarMapReadValuesFirst( pTree->pVm, pFtNode->VarNum );
        if ( pFtNode->nValues == 2 )
        {
            if ( pFtNode->uData == 1 )
            {
                pNode = ppShInputs[iValueFirst];
                assert( pNode );
//                assert( Sh_Regular(pNode)->Num != 1000000 );
                assert( Sh_Regular(pNode)->pOne != Sh_Regular(pNode)->pTwo || Sh_Regular(pNode)->pOne == Sh_Regular(pNode)->pTwo );
                return pNode;
            }
            else if ( pFtNode->uData == 2 )
            {
                pNode = ppShInputs[iValueFirst + 1];
                assert( pNode );
//                assert( Sh_Regular(pNode)->Num != 1000000 );
                assert( Sh_Regular(pNode)->pOne != Sh_Regular(pNode)->pTwo || Sh_Regular(pNode)->pOne == Sh_Regular(pNode)->pTwo );
                return pNode;
            }
            assert( 0 );
        }
        else // MV literal (signature is in pFtNode->uData)
            return Wn_WindowStrashNodeOr( pMan, pTree, 
                ppShInputs + iValueFirst, pFtNode->uData, pFtNode->nValues );
    }
    if ( pFtNode->Type == FT_NODE_AND )
    {
        pNode1 = Wn_WindowStrashNode_rec( pMan, pTree, pFtNode->pOne, ppShInputs ); 
        shRef( pNode1 );
        pNode2 = Wn_WindowStrashNode_rec( pMan, pTree, pFtNode->pTwo, ppShInputs ); 
        shRef( pNode2 );
        pNode = Sh_NodeAnd( pMan, pNode1, pNode2 ); shRef( pNode );
        Sh_RecursiveDeref( pMan, pNode1 );
        Sh_RecursiveDeref( pMan, pNode2 );
        shDeref( pNode );
//        assert( Sh_Regular(pNode)->Num != 1000000 );
        assert( Sh_Regular(pNode)->pOne != Sh_Regular(pNode)->pTwo || Sh_Regular(pNode)->pOne == Sh_Regular(pNode)->pTwo );
        return pNode;
    }
    if ( pFtNode->Type == FT_NODE_OR )
    {
        pNode1 = Wn_WindowStrashNode_rec( pMan, pTree, pFtNode->pOne, ppShInputs ); 
        shRef( pNode1 );
        pNode2 = Wn_WindowStrashNode_rec( pMan, pTree, pFtNode->pTwo, ppShInputs ); 
        shRef( pNode2 );
        pNode = Sh_NodeOr( pMan, pNode1, pNode2 ); shRef( pNode );
        Sh_RecursiveDeref( pMan, pNode1 );
        Sh_RecursiveDeref( pMan, pNode2 );
        shDeref( pNode );
//        assert( Sh_Regular(pNode)->Num != 1000000 );
        assert( Sh_Regular(pNode)->pOne != Sh_Regular(pNode)->pTwo || Sh_Regular(pNode)->pOne == Sh_Regular(pNode)->pTwo );
        return pNode;
    }
    if ( pFtNode->Type == FT_NODE_0 )
        return Sh_Not( Sh_ManagerReadConst1( pMan ) );
    if ( pFtNode->Type == FT_NODE_1 )
        return Sh_ManagerReadConst1( pMan );
    assert( 0 ); // unknown node type
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Creates the cascade of OR gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Wn_WindowStrashNodeOr( Sh_Manager_t * pMan, Ft_Tree_t * pTree, 
    Sh_Node_t * ppShInputs[], unsigned Signature, int nNodes )
{
    Sh_Node_t * pNode1, * pNode2, * pTemp;
    int i;

    assert( nNodes >= 2 );

    pNode1 = NULL;
    for ( i = 0; i < nNodes; i++ )
        if ( Signature & (1<<i) )
        {
            if ( pNode1 == NULL )
            {
                pNode1 = ppShInputs[i];   shRef( pNode1 );
                assert( pNode1 );
            }
            else
            {
                pNode2 = ppShInputs[i];   shRef( pNode2 );
                assert( pNode2 );
                pNode1 = Sh_NodeOr( pMan, pTemp = pNode1, pNode2 ); shRef( pNode1 );
                Sh_RecursiveDeref( pMan, pTemp );
                Sh_RecursiveDeref( pMan, pNode2 );
            }
        }
    assert( pNode1 );
//    assert( Sh_Regular(pNode1)->Num != 1000000 );
    assert( Sh_Regular(pNode1)->pOne != Sh_Regular(pNode1)->pTwo || Sh_Regular(pNode1)->pOne == Sh_Regular(pNode1)->pTwo );
    shDeref( pNode1 );
    return pNode1;
}

/**Function*************************************************************

  Synopsis    [Creates the cascade of OR gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashDeref( Sh_Manager_t * pMan, Ntk_Node_t * pNode )
{
    Ft_Tree_t * pTree;
    Cvr_Cover_t * pCvr;
    int i;
    // get the factored form of this node
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );
    // check if there is a default
    for ( i = 0; i < pTree->nRoots; i++ )
    {
        Sh_RecursiveDeref( pMan, (Sh_Node_t *)pTree->uRootData[i] );
        pTree->uRootData[i] = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Adds a unique AND gate for each output of each FF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashAddUnique( Sh_Manager_t * pMan, Ntk_Node_t * pNode )
{
    Cvr_Cover_t * pCvr;
    Ft_Tree_t * pTree;
    Sh_Node_t * pShNode;
    int k;

    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );
    assert( pTree->nRoots == pNode->nValues );

    for ( k = 0; k < pNode->nValues; k++ )
    {
        // get the node from the root data
        pShNode = (Sh_Node_t *)pTree->uRootData[k];
        assert( pShNode );
        // duplicate the node by inserting a unique one into the table
        pTree->uRootData[k] = (unsigned)Sh_TableInsertUnique( pMan, pShNode ); 
        Sh_Ref( (Sh_Node_t *)pTree->uRootData[k] );
        Sh_RecursiveDeref( pMan, pShNode );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the roots of the nodes after strashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashPrint( Sh_Manager_t * pMan, Wn_Window_t * pWnd )
{
    Ntk_Node_t * pNode;
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
    {
        printf( "NODE %3s : \n", Ntk_NodeGetNamePrintable(pNode) );
        printf( "\n" );
        Wn_WindowStrashPrintOne( pMan, pNode );
        printf( "\n" );
    }
}


/**Function*************************************************************

  Synopsis    [Prints the roots of the nodes after strashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashPrintOne( Sh_Manager_t * pMan, Ntk_Node_t * pNode )
{
    Sh_Node_t * ppNodes[32];
    Cvr_Cover_t * pCvr;
    Ft_Tree_t * pTree;
    int k;

    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );
    assert( pTree->nRoots == pNode->nValues );

    for ( k = 0; k < pNode->nValues; k++ )
        ppNodes[k] = (Sh_Node_t *)pTree->uRootData[k];

    Sh_NodePrintArray( ppNodes, pNode->nValues );
}




/**Function*************************************************************

  Synopsis    [Recursively strashs the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashCoreConstruct_rec( Sh_Manager_t * pMan, Ntk_Node_t * pNode, Wn_Window_t * pWnd )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    Ntk_Node_t ** ppNodes;
    Sh_Node_t ** ppShInputs;
    int nNodes, nValuesIn;

    if ( Wn_WindowStrashCoreNodeIsStrashed(pNode) )
        return;

    // makes sure that the fanins are already strashed
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        if ( !Wn_WindowStrashCoreNodeIsStrashed(pFanin) )
            Wn_WindowStrashCoreConstruct_rec( pMan, pFanin, pWnd );

    // collect fanins into array
    ppNodes = pNode->pNet->pArray1;
    nNodes = Ntk_NodeReadFanins( pNode, ppNodes );
    // set the input Sh_Nodes
    nValuesIn = Vm_VarMapReadValuesInNum( Ntk_NodeReadFuncVm(pNode) );
    ppShInputs = (Sh_Node_t **)pNode->pNet->pArray2;
    Wn_WindowStrashCollect( pMan, pWnd->pVmL, ppNodes, nNodes, ppShInputs, nValuesIn );
    // strash the node
    Wn_WindowStrashNode( pMan, pNode, ppShInputs );
}

/**Function*************************************************************

  Synopsis    [Recursively cleans the TFO of the traces of strashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashCoreClean( Sh_Manager_t * pMan, Ntk_Node_t * pNode )
{
    // start the trav-id for this traversal
    Ntk_NetworkIncrementTravId( pNode->pNet );
    // start from the node
    Wn_WindowStrashCoreClean_rec( pNode );
}


/**Function*************************************************************

  Synopsis    [Recursively cleans the TFO of the traces of strashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashCoreClean_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;

    if ( Ntk_NodeIsCo(pNode) )
        return;
    if ( Ntk_NodeIsTravIdCurrent(pNode) ) // visited node
        return;
    // mark the node as visited
    Ntk_NodeSetTravIdCurrent(pNode);
    // call recursively for the fanouts
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
        Wn_WindowStrashCoreClean_rec( pFanout );
    // clean the node
    Wn_WindowStrashCoreCleanStrashings( pNode );
}

/**Function*************************************************************

  Synopsis    [Cleans the traces of strashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashCoreCleanStrashings( Ntk_Node_t * pNode )
{
    Cvr_Cover_t * pCvr;
    Ft_Tree_t * pTree;
    int k;
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );
    assert( pTree->nRoots == pNode->nValues );
    for ( k = 0; k < pNode->nValues; k++ )
        pTree->uRootData[k] = 0;
}

/**Function*************************************************************

  Synopsis    [Detect if the traces of strashing are present.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Wn_WindowStrashCoreNodeIsStrashed( Ntk_Node_t * pNode )
{
    Cvr_Cover_t * pCvr;
    Ft_Tree_t * pTree;
    int k;

    if ( Ntk_NodeIsCi(pNode) )
        return 1;

    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );
    assert( pTree->nRoots == pNode->nValues );
    for ( k = 0; k < pNode->nValues; k++ )
        if ( pTree->uRootData[k] == 0 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Cleans strashing that may have been attached to the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowStrashClean( Wn_Window_t * pWnd )
{
    Ntk_Node_t * pNode;
    Ntk_NetworkForEachNode( pWnd->pNet, pNode )
        Wn_WindowStrashCoreCleanStrashings( pNode );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


