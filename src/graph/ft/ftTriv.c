/**CFile****************************************************************

  FileName    [ftTriv.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Trivial case of factoring.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ftTriv.c,v 1.5 2005/06/02 03:34:10 alanmi Exp $]

***********************************************************************/

#include "ft.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ft_Node_t * Ft_FactorTrivialTree_rec( Ft_Tree_t * pTree, Ft_Node_t ** ppNodes, int nNodes, int fAnd );
static Ft_Node_t * Ft_FactorTrivialCascade( Ft_Tree_t * pTree, Mvc_Cover_t * pCover );
static Ft_Node_t * Ft_FactorTrivialCubeCascade( Ft_Tree_t * pTree, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Factoring the cover, which has no algebraic divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorTrivial( Ft_Tree_t * pTree, Mvc_Cover_t * pCover )
{
    Ft_Node_t ** ppNodes;
    Ft_Node_t * pNode;
    Mvc_Cube_t * pCube;
    int i, nNodes;

    // create space to put the cubes
    nNodes = Mvc_CoverReadCubeNum(pCover);
    assert( nNodes > 0 );
    ppNodes = ALLOC( Ft_Node_t *, nNodes );
    // create the factored form for each cube
    i = 0;
    Mvc_CoverForEachCube( pCover, pCube )
        ppNodes[i++] = Ft_FactorTrivialCube( pTree, pCover, pCube );
    assert( i == nNodes );
    // balance the factored forms
    pNode = Ft_FactorTrivialTree_rec( pTree, ppNodes, nNodes, 0 );
    free( ppNodes );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Factoring the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorTrivialCube( Ft_Tree_t * pTree, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    Ft_Node_t ** ppNodes;
    Ft_Node_t * pNode;
    int iBit, Value, i;

    // create space to put each literal
    ppNodes = ALLOC( Ft_Node_t *, pCover->nBits );
    // create the factored form for each literal
    i = 0;
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
    {
        if ( Value )
            ppNodes[i++] = Ft_FactorTrivialNode( pTree, iBit );
    }
    assert( i > 0 && i < pCover->nBits );
    // balance the factored forms
    pNode = Ft_FactorTrivialTree_rec( pTree, ppNodes, i, 1 );
    free( ppNodes );
    return pNode;
}
 
/**Function*************************************************************

  Synopsis    [Create the well-balanced tree of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorTrivialTree_rec( Ft_Tree_t * pTree, Ft_Node_t ** ppNodes, int nNodes, int fAnd )
{
    Ft_Node_t * pNode1, * pNode2;
    int nNodes1, nNodes2;

    if ( nNodes == 1 )
        return ppNodes[0];

    // split the nodes into two parts
    nNodes1 = nNodes/2;
    nNodes2 = nNodes - nNodes1;

    // recursively construct the tree for the parts
    pNode1 = Ft_FactorTrivialTree_rec( pTree, ppNodes,           nNodes1, fAnd );
    pNode2 = Ft_FactorTrivialTree_rec( pTree, ppNodes + nNodes1, nNodes2, fAnd );

    return Ft_TreeNodeCreate( pTree, fAnd? FT_NODE_AND : FT_NODE_OR, pNode1, pNode2 );
}



/**Function*************************************************************

  Synopsis    [Factoring the cover, which has no algebraic divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorTrivialCascade( Ft_Tree_t * pTree, Mvc_Cover_t * pCover )
{
    Ft_Node_t * pNode;
    Mvc_Cube_t * pCube;

    // iterate through the cubes
    pNode = NULL;
    Mvc_CoverForEachCube( pCover, pCube )
    {
        if ( pNode == NULL )
            pNode = Ft_FactorTrivialCube( pTree, pCover, pCube );
        else
            pNode = Ft_TreeNodeCreate( pTree, FT_NODE_OR, pNode, Ft_FactorTrivialCubeCascade( pTree, pCover, pCube ) );
    }
    assert( pNode ); // if this assertion fails, the input cover is not SCC-free
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Factoring the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorTrivialCubeCascade( Ft_Tree_t * pTree, Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    Ft_Node_t * pNode;
    int iBit, Value;

    // iterate through the literals
    pNode = NULL;
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
    {
        if ( Value )
        {
            if ( pNode == NULL )
                pNode = Ft_FactorTrivialNode( pTree, iBit );
            else
                pNode = Ft_TreeNodeCreate( pTree, FT_NODE_AND, pNode, Ft_FactorTrivialNode( pTree, iBit ) );
        }
    }
    assert( pNode ); // if this assertion fails, the input cover is not SCC-free
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Factoring the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorTrivialNode( Ft_Tree_t * pTree, int iLit )
{
    Vm_VarMap_t * pVm;
    int * pValuesFirst, * pValues;
    int nValuesIn, nVarsIn;
    Ft_Node_t * pNode;
    int iVar;
    pVm = pTree->pVm;
    pValues      = Vm_VarMapReadValuesArray(pVm);
    pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);
    nValuesIn    = Vm_VarMapReadValuesInNum(pVm);
    nVarsIn      = Vm_VarMapReadVarsInNum(pVm);
    assert( iLit < nValuesIn );
    for ( iVar = 0; iVar < nVarsIn; iVar++ )
        if ( iLit < pValuesFirst[iVar] + pValues[iVar] )
            break;
    assert( iVar < nVarsIn );
    pNode = Ft_TreeNodeCreate( pTree, FT_NODE_LEAF, NULL, NULL );
    pNode->VarNum  = iVar;
    pNode->nValues = pValues[iVar];
    pNode->uData   = FT_MV_MASK(pNode->nValues) ^ (1 << (iLit - pValuesFirst[iVar]));
    return pNode;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


