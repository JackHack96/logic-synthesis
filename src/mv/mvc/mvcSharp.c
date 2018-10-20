/**CFile****************************************************************

  FileName    [mvcSharp.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Computes a non-disjoint sharp for two covers.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcSharp.c,v 1.7 2003/09/01 04:56:54 alanmi Exp $]

***********************************************************************/

#include "mvc.h"
#include "vm.h"
#include "vmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

void Mvc_CoverSharpCube( Mvc_Data_t * p, Mvc_Cube_t * pCube, Mvc_Cover_t * pB, 
    Mvc_Cover_t * pSum, Mvc_Cover_t * pProd, Mvc_Cover_t * pOne );
void Mvc_CoverSharpCubeCube( Mvc_Data_t * p, Mvc_Cube_t * pCube, Mvc_Cube_t * pCubeB, Mvc_Cover_t * pRes );
bool Mvc_CoverDist0Cubes( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Non-disjoint sharp of a cover against a cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverSharp( Mvc_Data_t * p, Mvc_Cover_t * pA, Mvc_Cover_t * pB )
{
    Mvc_Cover_t * pSum;
    Mvc_Cover_t * pProd;
    Mvc_Cover_t * pOne;
    Mvc_Cube_t * pCube;

    // if one of the covers is empty, return the copy of the first cover
	if ( !Mvc_CoverReadCubeNum(pA) || !Mvc_CoverReadCubeNum(pB) )
        return Mvc_CoverDup( pA );

    // create temporary covers
    pSum  = Mvc_CoverAlloc( pA->pMem, pA->nBits );
    pProd = Mvc_CoverAlloc( pA->pMem, pA->nBits );
    pOne  = Mvc_CoverAlloc( pA->pMem, pA->nBits );
    // sharp each cube and union the results
    Mvc_CoverForEachCube( pA, pCube )
    {
        // sharp cube pCube with cover pB
        Mvc_CoverSharpCube( p, pCube, pB, pSum, pProd, pOne );
        // move the cubes from Prod to Sum
        Mvc_CoverAppendCubes( pSum, pProd );
        // if the number of cubes in the new cover is too large, compress
        if ( Mvc_CoverReadCubeNum( pSum ) > 500 )
            Mvc_CoverContain( pSum );
    }
    Mvc_CoverFree( pProd );
    Mvc_CoverFree( pOne );

    Mvc_CoverContain( pSum );
    return pSum;
}

/**Function*************************************************************

  Synopsis    [Non-disjoint sharp of a cube against a cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverSharpCube( Mvc_Data_t * p, Mvc_Cube_t * pCube, Mvc_Cover_t * pB, 
    Mvc_Cover_t * pSum, Mvc_Cover_t * pProd, Mvc_Cover_t * pOne )
{
    Mvc_Cube_t * pCubeB;
    assert( ( Mvc_CoverReadCubeNum(pProd) == 0 ) );
    assert( ( Mvc_CoverReadCubeNum(pOne) == 0 ) );
    // sharp each pair of cubes
    Mvc_CoverForEachCube( pB, pCubeB )
    {
        // sharp the cubes and put the result into pOne
        Mvc_CoverSharpCubeCube( p, pCube, pCubeB, pOne );
        // if the cube is completely covered, return const 0
        if ( Mvc_CoverReadCubeNum(pOne) == 0 )
        {
            Mvc_CoverRemoveCubes( pProd );
            return;
        }
        // if this is the first cube, simply transfer it to product
        if ( Mvc_CoverReadCubeNum(pProd) == 0 )
        {
            Mvc_CoverAppendCubes( pProd, pOne );
            continue;
        }
        // make the product of pProd with pOne
        Mvc_CoverIntersectCubes( p, pProd, pOne );
        Mvc_CoverRemoveCubes( pOne );
        if ( Mvc_CoverReadCubeNum(pProd) == 0 )
            return;
        // if the number of cubes in the new cover is too large, compress
        if ( Mvc_CoverReadCubeNum( pProd ) > 500 )
            Mvc_CoverContain( pProd );
    }
}


/**Function*************************************************************

  Synopsis    [Non-disjoint sharp of a cube against a cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverSharpCubeCube( Mvc_Data_t * p, Mvc_Cube_t * pCube, Mvc_Cube_t * pCubeB, Mvc_Cover_t * pRes )
{
    Mvc_Cube_t * pCubeNew;
    int i, Res;

	if ( Mvc_CoverDist0Cubes( p, pCube, pCubeB ) )
	{
        Mvc_CubeBitSharp( p->ppTemp[0], pCube, pCubeB );
		for ( i = 0; i < p->pVm->nVarsIn; i++ )
		{
            Mvc_CubeBitAnd( p->ppTemp[1], p->ppTemp[0], p->ppMasks[i] );
            Mvc_CubeBitEmpty( Res, p->ppTemp[1] );
            if ( !Res )
			{
                Mvc_CubeBitSharp( p->ppTemp[2], pCube, p->ppMasks[i] );
                pCubeNew = Mvc_CubeAlloc( pRes );
                Mvc_CubeBitOr( pCubeNew, p->ppTemp[1], p->ppTemp[2] );
                Mvc_CoverAddCubeTail( pRes, pCubeNew );
			}
		}
	}
	else
	{
        pCubeNew = Mvc_CubeDup( pRes, pCube );
        Mvc_CoverAddCubeTail( pRes, pCubeNew );
	}
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cubes are distance 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvc_CoverDist0Cubes( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB )
{
    int v, Res;
	for ( v = 0; v < pData->pVm->nVarsIn; v++ )
	{
        Mvc_CubeBitIntersectUnderMask( Res, pA, pB, pData->ppMasks[v] );
        if ( Res == 0 )
            return 0;
	}
	return 1;
}


/**Function*************************************************************

  Synopsis    [Intersects the cubes of C1 and C2.]

  Description [Writes the result into C1. Leaves C2 empty.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverIntersectCubes( Mvc_Data_t * pData, Mvc_Cover_t * pC1, Mvc_Cover_t * pC2 )
{
    Mvc_List_t List = { NULL, NULL, 0 };
    Mvc_List_t * pList = &List;
    Mvc_Cube_t * pCube1, * pCube2;
    Mvc_Cube_t * pCubeNew;

    Mvc_CoverForEachCube( pC1, pCube1 )
        Mvc_CoverForEachCube( pC2, pCube2 )
            if ( Mvc_CoverDist0Cubes( pData, pCube1, pCube2 ) )
            {
                pCubeNew = Mvc_CubeAlloc( pC1 );
                Mvc_CubeBitAnd( pCubeNew, pCube1, pCube2 );
                Mvc_ListAddCubeTail( pList, pCubeNew );
            }    
    // remove the cubes
    Mvc_CoverRemoveCubes( pC1 );
//    Mvc_CoverRemoveCubes( pC2 );
    pC1->lCubes = List;
}

/**Function*************************************************************

  Synopsis    [Intersects the cubes of C1 and C2.]

  Description [Returns 1 if they intersect.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvc_CoverIsIntersecting( Mvc_Data_t * pData, Mvc_Cover_t * pC1, Mvc_Cover_t * pC2 )
{
    Mvc_Cube_t * pCube1, * pCube2;
    Mvc_CoverForEachCube( pC1, pCube1 )
        Mvc_CoverForEachCube( pC2, pCube2 )
            if ( Mvc_CoverDist0Cubes( pData, pCube1, pCube2 ) )
                return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Moves the cubes from C2 to C1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverAppendCubes( Mvc_Cover_t * pC1, Mvc_Cover_t * pC2 )
{
    if ( pC2->lCubes.nItems == 0 )
        return;
    if ( pC1->lCubes.pHead == 0 )
        pC1->lCubes.pHead = pC2->lCubes.pHead;
    else
        Mvc_CubeSetNext( pC1->lCubes.pTail, pC2->lCubes.pHead ); 
    pC1->lCubes.pTail   = pC2->lCubes.pTail;
    pC1->lCubes.nItems += pC2->lCubes.nItems;

    pC2->lCubes.pHead  = NULL;
    pC2->lCubes.pTail  = NULL;
    pC2->lCubes.nItems = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverCopyAndAppendCubes( Mvc_Cover_t * pC1, Mvc_Cover_t * pC2 )
{
    Mvc_Cube_t * pCube, * pCubeCopy;
    // make sure the covers are compatible
    assert( pC1->nBits == pC2->nBits );
    // copy and append the cubes
    Mvc_CoverForEachCube( pC2, pCube )
    {
        pCubeCopy = Mvc_CubeDup( pC1, pCube );
        Mvc_CoverAddCubeTail( pC1, pCubeCopy );
    }
}

/**Function*************************************************************

  Synopsis    [Removes the cubes from the cover without removing the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverRemoveCubes( Mvc_Cover_t * pC )
{
    Mvc_Cube_t * pCube, * pCube2;
    Mvc_CoverForEachCubeSafe( pC, pCube, pCube2 )
        Mvc_CubeFree( pC, pCube );
    pC->lCubes.pHead  = NULL;
    pC->lCubes.pTail  = NULL;
    pC->lCubes.nItems = 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


