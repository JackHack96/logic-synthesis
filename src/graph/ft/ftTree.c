/**CFile****************************************************************

  FileName    [ftTree.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to manipulate factored trees.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ftTree.c,v 1.3 2005/06/02 03:34:10 alanmi Exp $]

***********************************************************************/

#include "ft.h"
#include "string.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Ft_NodeFree_rec( Ft_Tree_t * pTree, Ft_Node_t * pNode );
static void Ft_TreeCountLeafRefs_rec( Ft_Tree_t * pTree, Ft_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Tree_t * Ft_TreeCreate( Mvc_Manager_t * pMem, int nLeaves, int nRoots )
{
    Ft_Tree_t * pTree;
    pTree           = MEM_ALLOC( pMem,  Ft_Tree_t, 1 );
    memset( pTree, 0, sizeof(Ft_Tree_t) );
    // allocate room for roots and leaves
    pTree->pMem    = pMem;
    pTree->nRoots  = nRoots;
    pTree->nLeaves = nLeaves;
    // create roots
    pTree->pRoots    = MEM_ALLOC( pMem,  Ft_Node_t *, nRoots );
    memset( pTree->pRoots, 0, sizeof(Ft_Node_t *) * nRoots );
    // create additional data members
    pTree->uLeafData = MEM_ALLOC( pMem, unsigned, nLeaves );
    pTree->uRootData = MEM_ALLOC( pMem, unsigned, nRoots );
    memset( pTree->uLeafData, 0, sizeof(unsigned) * nLeaves );
    memset( pTree->uRootData, 0, sizeof(unsigned) * nRoots );
    return pTree;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_TreeFree( Ft_Tree_t * pTree )
{
    int i;
    if ( !pTree )
        return;
    // free the branches
    for ( i = 0; i < pTree->nRoots; i++ )
        if ( pTree->pRoots[i] )
            Ft_NodeFree_rec( pTree, pTree->pRoots[i] );
    if ( pTree->pDefault )
        Ft_NodeFree_rec( pTree, pTree->pDefault );
    // free the tree
    MEM_FREE( pTree->pMem, Ft_Node_t *, pTree->nRoots,  pTree->pRoots );
    MEM_FREE( pTree->pMem, unsigned,    pTree->nLeaves, pTree->uLeafData );
    MEM_FREE( pTree->pMem, unsigned,    pTree->nRoots,  pTree->uRootData );
    MEM_FREE( pTree->pMem, Ft_Tree_t *, 1,              pTree );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_TreeFreeRoot( Ft_Tree_t * pTree, int iRoot )
{
    if ( pTree->pRoots[iRoot] )
    {
        Ft_NodeFree_rec( pTree, pTree->pRoots[iRoot] );
        pTree->pRoots[iRoot] = NULL;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_NodeFree_rec( Ft_Tree_t * pTree, Ft_Node_t * pNode )
{
    assert( pNode->Type != FT_NODE_NONE );
    if ( pNode->Type == FT_NODE_AND || pNode->Type == FT_NODE_OR )
    {
        Ft_NodeFree_rec( pTree, pNode->pOne );
        Ft_NodeFree_rec( pTree, pNode->pTwo );
    }
    Ft_TreeNodeFree( pTree, pNode );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_TreeNodeCreate( Ft_Tree_t * pTree, int Type, Ft_Node_t * pNode1, Ft_Node_t * pNode2 )
{
    Ft_Node_t * pNode;
    assert( pNode1 && pNode2 || !pNode1 && !pNode2 );
    pNode = MEM_ALLOC( pTree->pMem, Ft_Node_t, 1 );
    memset( pNode, 0, sizeof(Ft_Node_t) );
    pNode->Type = Type;
    pNode->pOne = pNode1;
    pNode->pTwo = pNode2;
    // update FF statistics
    if ( pNode->Type == FT_NODE_LEAF )
        pTree->nNodes++;
    return pNode;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_TreeNodeFree( Ft_Tree_t * pTree, Ft_Node_t * pNode )
{
    MEM_FREE( pTree->pMem, Ft_Node_t, 1, pNode );
}

/**Function*************************************************************

  Synopsis    [Counts the number of references of the leaf nodes.]

  Description [This procedure counts the number of times each leaf 
  node is referenced from the non-leaf nodes of the factored tree.
  The resulting counters are stored in pTree->pLeaves->pData.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_TreeCountLeafRefs( Ft_Tree_t * pTree )
{
    int i;
    // clean the data fields of the leaves
    for ( i = 0; i < pTree->nLeaves; i++ )
        pTree->uLeafData[i] = 0;
    // recursively count the references
    for ( i = 0; i < pTree->nRoots; i++ )
        if ( pTree->pRoots[i] )
            Ft_TreeCountLeafRefs_rec( pTree, pTree->pRoots[i] );
}

/**Function*************************************************************

  Synopsis    [Performs the recursive step of reference counting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ft_TreeCountLeafRefs_rec( Ft_Tree_t * pTree, Ft_Node_t * pNode )
{
    int v;
    int * pValuesFirst;
    assert( pNode->Type != FT_NODE_NONE );
    if ( pNode->Type == FT_NODE_0 || pNode->Type == FT_NODE_1 )
        return;
    if ( pNode->Type != FT_NODE_LEAF )
    {
        Ft_TreeCountLeafRefs_rec( pTree, pNode->pOne );
        Ft_TreeCountLeafRefs_rec( pTree, pNode->pTwo );
    }
    else
    {
        pValuesFirst = Vm_VarMapReadValuesFirstArray(pTree->pVm);
        for ( v = 0; v < pNode->nValues; v++ )
            if ( pNode->uData & (1<<v) )
                pTree->uLeafData[ pValuesFirst[pNode->VarNum] + v ]++;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////