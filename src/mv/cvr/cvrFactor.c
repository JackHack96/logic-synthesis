/**CFile****************************************************************

  FileName    [cvrFactor.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Performs factoring of the i-sets of the Cvr object.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cvrFactor.c,v 1.9 2003/05/27 23:14:57 alanmi Exp $]

***********************************************************************/

#include "cvrInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Tree_t * Cvr_CoverFactor( Cvr_Cover_t * pCvr )
{
    Ft_Tree_t * pTree;
    Vm_VarMap_t * pVm;
    Mvc_Cover_t ** pIsets;
    Mvc_Cover_t * pCover;
    int nIsets, i;

    if ( pCvr->pTree )
        return pCvr->pTree;

    pVm       = Cvr_CoverReadVm( pCvr );
    pIsets    = Cvr_CoverReadIsets( pCvr );
    nIsets    = Vm_VarMapReadValuesOutput( pVm );

    // get the first cover
    pCover = NULL;
    for ( i = 0; i < nIsets; i++ )
        if ( pIsets[i] )
        {
            pCover = pIsets[i];
            break;
        }

    // create the tree
    pTree = Ft_TreeCreate( pCover->pMem, pCover->nBits, nIsets );
    pTree->pVm = pVm;

    // factor each cover
    for ( i = 0; i < nIsets; i++ )
        if ( pIsets[i] )
            pTree->pRoots[i] = Ft_Factor( pTree, pIsets[i] );
        else
            pTree->pRoots[i] = NULL;

    // transform the tree if there are MV input variables
    if ( !Vm_VarMapIsBinaryInput( pVm ) )
        Ft_FactorTransform( pTree );

    pCvr->pTree = pTree;
    return pTree;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cvr_CoverReadLitFacNum( Cvr_Cover_t * p )
{
    Ft_Tree_t * pTree;
    pTree = Cvr_CoverFactor( p );
    return pTree->nNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cvr_CoverReadLeafValueNum( Cvr_Cover_t * p )
{
    Ft_Tree_t * pTree;
    pTree = Cvr_CoverFactor( p );
    return Ft_FactorCountLeafValues( pTree );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Tree_t * Cvr_CoverReadTree( Cvr_Cover_t * p )
{
    return p->pTree;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cvr_CoverFreeFactor( Cvr_Cover_t * p )
{
    Ft_TreeFree( p->pTree );
    p->pTree = NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


