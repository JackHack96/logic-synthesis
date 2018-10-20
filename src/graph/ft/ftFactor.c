/**CFile****************************************************************

  FileName    [ftFactor.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures for algebraic factoring.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ftFactor.c,v 1.4 2004/04/08 05:05:05 alanmi Exp $]

***********************************************************************/

#include "ft.h"
#include "string.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ft_Node_t * Ft_Factor_rec( Ft_Tree_t * pTree, Mvc_Cover_t * pCover );
static Ft_Node_t * Ft_FactorLF_rec( Ft_Tree_t * pTree, Mvc_Cover_t * pCover, Mvc_Cover_t * pSimple );
static Ft_Node_t * Ft_FactorComplementArray( Ft_Tree_t * pTree, Ft_Node_t ** pRoots, int nRoots );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Factors the given cover and returns the factored tree.]

  Description [The tree has only the roots and the leaves assigned. 
  The cover consists of cubes. When this procedure completes, 
  the cover is the same but the tree is filled with FF nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_Factor( Ft_Tree_t * pTree, Mvc_Cover_t * pCover )
{
    Ft_Node_t * pRoot;
    assert( pCover );

    // make sure the cover is CCS free
    // otherwise, factoring will have trouble
    // this should be done before CST!!!
    Mvc_CoverContain( pCover );

    // check for trivial functions
    if ( Mvc_CoverIsEmpty(pCover) )
        return Ft_TreeNodeCreate( pTree, FT_NODE_0, NULL, NULL );
    if ( Mvc_CoverIsTautology(pCover) )
        return Ft_TreeNodeCreate( pTree, FT_NODE_1, NULL, NULL );

    Mvc_CoverInverse( pCover ); // CST
    pRoot = Ft_Factor_rec( pTree, pCover );
    Mvc_CoverInverse( pCover ); // undo CST
    return pRoot;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_Factor_rec( Ft_Tree_t * pTree, Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pDiv, * pQuo, * pRem, * pCom;
    Ft_Node_t * pNodeDiv, * pNodeQuo, * pNodeRem;
    Ft_Node_t * pNodeAnd, * pNode;

    // make sure the cover contains some cubes
    assert( Mvc_CoverReadCubeNum(pCover) );

    // get the divisor
    pDiv = Mvc_CoverDivisor( pCover );
    if ( pDiv == NULL )
        return Ft_FactorTrivial( pTree, pCover );

    // divide the cover by the divisor
    Mvc_CoverDivideInternal( pCover, pDiv, &pQuo, &pRem );
    assert( Mvc_CoverReadCubeNum(pQuo) );

    Mvc_CoverFree( pDiv );
    Mvc_CoverFree( pRem );

    // check the trivial case
    if ( Mvc_CoverReadCubeNum(pQuo) == 1 )
    {
        pNode = Ft_FactorLF_rec( pTree, pCover, pQuo );
        Mvc_CoverFree( pQuo );
        return pNode;
    }

    // make the quotient cube free
    Mvc_CoverMakeCubeFree( pQuo );

    // divide the cover by the quotient
    Mvc_CoverDivideInternal( pCover, pQuo, &pDiv, &pRem );

    // check the trivial case
    if ( Mvc_CoverIsCubeFree( pDiv ) )
    {
        pNodeDiv = Ft_Factor_rec( pTree, pDiv );
        pNodeQuo = Ft_Factor_rec( pTree, pQuo );
        Mvc_CoverFree( pDiv );
        Mvc_CoverFree( pQuo );
        pNodeAnd = Ft_TreeNodeCreate( pTree, FT_NODE_AND, pNodeDiv, pNodeQuo );
        if ( Mvc_CoverReadCubeNum(pRem) == 0 )
        {
            Mvc_CoverFree( pRem );
            return pNodeAnd;
        }
        else
        {
            pNodeRem = Ft_Factor_rec( pTree, pRem );
            Mvc_CoverFree( pRem );
            return Ft_TreeNodeCreate( pTree, FT_NODE_OR,  pNodeAnd, pNodeRem );
        }
    }

    // get the common cube
    pCom = Mvc_CoverCommonCubeCover( pDiv );
    Mvc_CoverFree( pDiv );
    Mvc_CoverFree( pQuo );
    Mvc_CoverFree( pRem );

    // solve the simple problem
    pNode = Ft_FactorLF_rec( pTree, pCover, pCom );
    Mvc_CoverFree( pCom );
    return pNode;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorLF_rec( Ft_Tree_t * pTree, Mvc_Cover_t * pCover, Mvc_Cover_t * pSimple )
{
    Mvc_Cover_t * pDiv, * pQuo, * pRem;
    Ft_Node_t * pNodeDiv, * pNodeQuo, * pNodeRem;
    Ft_Node_t * pNodeAnd;

    // get the most often occurring literal
    pDiv = Mvc_CoverBestLiteralCover( pCover, pSimple );
    // divide the cover by the literal
    Mvc_CoverDivideByLiteral( pCover, pDiv, &pQuo, &pRem );
    // get the node pointer for the literal
    pNodeDiv = Ft_FactorTrivialCube( pTree, pDiv, Mvc_CoverReadCubeHead(pDiv) );
    Mvc_CoverFree( pDiv );
    // factor the quotient and remainder
    pNodeQuo = Ft_Factor_rec( pTree, pQuo );
    Mvc_CoverFree( pQuo );
    pNodeAnd = Ft_TreeNodeCreate( pTree, FT_NODE_AND, pNodeDiv, pNodeQuo );
    if ( Mvc_CoverReadCubeNum(pRem) == 0 )
    {
        Mvc_CoverFree( pRem );
        return pNodeAnd;
    }
    else
    {
        pNodeRem = Ft_Factor_rec( pTree, pRem );
        Mvc_CoverFree( pRem );
        return Ft_TreeNodeCreate( pTree, FT_NODE_OR,  pNodeAnd, pNodeRem );
    }
}


/**Function*************************************************************

  Synopsis    [Derive the factored form for NOT( OR(r0,r1,,,,,rk-1) ).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ft_Node_t * Ft_FactorComplementArray( Ft_Tree_t * pTree, Ft_Node_t ** pRoots, int nRoots )
{
    Ft_Node_t * pNode;
    int iFirst, Counter, i;

    // get the first entry in the array and
    // make sure that exactly one entry is NULL
    Counter = 0;
    pNode   = NULL;
    iFirst  = -1;
    for ( i = 0; i < nRoots; i++ )
        if ( pRoots[i] )
        {
            Counter++;
            if ( pNode == NULL )
            {
                pNode = pRoots[i];
                iFirst = i;
            }
        }
    assert( Counter == nRoots - 1 );

    if ( Counter > 1 )
    {   // create the OR of nodes
        for ( i = iFirst + 1; i < nRoots; i++ )
            if ( pRoots[i] ) // add to the OR-gate
                pNode = Ft_TreeNodeCreate( pTree, FT_NODE_OR, pNode, pRoots[i] );
    }
    // add the inv node
    return Ft_TreeNodeCreate( pTree, FT_NODE_INV, pNode, NULL );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


