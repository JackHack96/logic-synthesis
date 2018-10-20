/**CFile****************************************************************

  FileName    [mvc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Reshapes the dist-2 cubes and tries to perform dist-1 merge.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcReshape.c,v 1.1 2003/09/01 04:56:54 alanmi Exp $]

***********************************************************************/

#include "mvc.h"
#include "vm.h"
#include "vmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

void Mvc_CoverMinimizeByReshape( Mvc_Data_t * pData, Mvc_Cover_t * pCover );
int  Mvc_CoverReshape( Mvc_Data_t * pData, Mvc_Cover_t * pCover );

bool Mvc_CoverDiff2Cubes( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB, int * pDiff1, int * pDiff2 );
bool Mvc_CoverReshapeCubes( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB, int Var1, int Var2 );

void Mvc_CoverSharpOnePart( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB, int Var );
void Mvc_CoverOrOnePart( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB, int Var );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Minimizes the cover by reshaping pairs of dist-2 cubes.]

  Description [Performs the following procedure: Goes through the cubes, 
  detects each pair of dist-2 cubes, reshapes the cubes if possible. 
  In the end, performs distance-1 merge. Iterates this procedure
  until there is some reduction.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverMinimizeByReshape( Mvc_Data_t * pData, Mvc_Cover_t * pCover )
{
    int nCubesPrev, nCubesCur;

    // start with merge
    Mvc_CoverDist1Merge( pData, pCover );
    Mvc_CoverContain( pCover );
    // consider the case when the cover is too small
    if ( Mvc_CoverReadCubeNum( pCover ) < 2 )
        return;
    // make sure that the number of vars is large enough
    if ( pData->pVm->nVarsIn < 2 )
        return;

//Mvc_CoverPrintMv( pData, pCover );

    do 
    {
        // go through the cubes, detect dist-2 and reshape them
        // mark the reshaped cubes to prevent doing the same work twice
        nCubesPrev = Mvc_CoverReadCubeNum( pCover );
        // if reshaping is not possible, no need to merge
        if ( Mvc_CoverReshape( pData, pCover ) == 0 )
            return;

        Mvc_CoverDist1Merge( pData, pCover );
        Mvc_CoverContain( pCover );
        nCubesCur = Mvc_CoverReadCubeNum( pCover );
    }
//    while ( nCubesPrev != nCubesCur );
    while ( 0 );

//Mvc_CoverPrintMv( pData, pCover );
}

/**Function*************************************************************

  Synopsis    [Reshapes all the pairs of dist-2 cubes.]

  Description [Returns the number of reshaped cubes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverReshape( Mvc_Data_t * pData, Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube1, * pCube2;
    int Var1, Var2;
    int Counter = 0;

    // unmark the cubes
    Mvc_CoverForEachCube( pCover, pCube1 )
        Mvc_CubeSetSize( pCube1, 0 );

    // find the diff-2 cube pairs
    Mvc_CoverForEachCube( pCover, pCube1 )
    {
        Mvc_CoverForEachCubeStart( Mvc_CubeReadNext(pCube1), pCube2 )
        {
            // check the number of different parts in the two cubes
            if ( Mvc_CoverDiff2Cubes( pData, pCube1, pCube2, &Var1, &Var2 ) )
            {
                assert( Var1 >= 0 && Var2 >= 0 );
                Counter += Mvc_CoverReshapeCubes( pData, pCube1, pCube2, Var1, Var2 );
                // this procedure marks the reshaped cube by setting its size to 1
                // this procedure may not be able to reshape the cubes
                // in the following cases: (1) if they are diff-2 but their
                // value sets are not contained, (2) the cube to be reshaped is already reshaped
            }
        }
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Reshapes one pair of dist-2 cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvc_CoverReshapeCubes( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB, 
    int Var1, int Var2 )
{
    bool A1impliesB1, B1impliesA1, A2impliesB2, B2impliesA2, Res;
    int fModifyA, fModifyB;

    Mvc_CubeBitNotImplUnderMask( Res, pA, pB, pData->ppMasks[Var1] );
    A1impliesB1 = !Res;
    Mvc_CubeBitNotImplUnderMask( Res, pB, pA, pData->ppMasks[Var1] );
    B1impliesA1 = !Res;
    Mvc_CubeBitNotImplUnderMask( Res, pA, pB, pData->ppMasks[Var2] );
    A2impliesB2 = !Res;
    Mvc_CubeBitNotImplUnderMask( Res, pB, pA, pData->ppMasks[Var2] );
    B2impliesA2 = !Res;
    // consider the case when there is no containment
    // in this case, reshaping is not possible
    if ( !A1impliesB1 && !B1impliesA1 && !A2impliesB2 && !B2impliesA2 )
        return 0;
    assert( A1impliesB1 == 0 || B1impliesA1 == 0 );
    assert( A2impliesB2 == 0 || B2impliesA2 == 0 );

//    assert( A1impliesB1 == 0 || A2impliesB2 == 0 );
//    assert( B1impliesA1 == 0 || B2impliesA2 == 0 );
    if ( A1impliesB1 && A2impliesB2 || B1impliesA1 && B2impliesA2 )
        return 0;


    // otherwise, reshaping is possible
    fModifyA = 0;
    fModifyB = 0;
    if ( (A1impliesB1 || B1impliesA1) && (A2impliesB2 || B2impliesA2) )
    { // both of the cubes are expanded and intersect
        if ( Mvc_CubeReadSize(pA) && !Mvc_CubeReadSize(pB) )
            fModifyB = 1;
        else if ( Mvc_CubeReadSize(pB) && !Mvc_CubeReadSize(pA) )
            fModifyA = 1;
        else if ( !Mvc_CubeReadSize(pB) && !Mvc_CubeReadSize(pA) )
            fModifyA = (rand() & 1);
        // perform the changes
        if ( fModifyA )
        { // modify pA
            if ( B1impliesA1 )
                Mvc_CoverSharpOnePart( pData, pA, pB, Var1 );
            else if ( B2impliesA2 )
                Mvc_CoverSharpOnePart( pData, pA, pB, Var2 );
            else
            {
                assert( 0 );
            }
            Mvc_CubeSetSize(pA, 1);
            return 1;
        }
        if ( fModifyB )
        { // modify pB
            if ( A1impliesB1 )
                Mvc_CoverSharpOnePart( pData, pB, pA, Var1 );
            else if ( A2impliesB2 )
                Mvc_CoverSharpOnePart( pData, pB, pA, Var2 );
            else
            {
                assert( 0 );
            }
            Mvc_CubeSetSize(pB, 1);
            return 1;
        }
    }
    else if ( A1impliesB1 )
    { // should be B2impliesA2 - modify pA in Var2 if possible
/*
        if ( !Mvc_CubeReadSize(pA) )
        {
            Mvc_CoverSharpOnePart( pData, pA, pB, Var2 );
            Mvc_CubeSetSize(pA, 1);
            return 1;
        }
*/

        if ( !Mvc_CubeReadSize(pB) )
        {
            Mvc_CoverOrOnePart( pData, pA, pB, Var2 );
            Mvc_CoverSharpOnePart( pData, pB, pA, Var1 );
            Mvc_CubeSetSize(pB, 1);
            return 1;
        }

    }
    else if ( B1impliesA1 )
    { // should be A2impliesB2 - modify pB in Var2 if possible
/*
        if ( !Mvc_CubeReadSize(pB) )
        {
            Mvc_CoverSharpOnePart( pData, pB, pA, Var2 );
            Mvc_CubeSetSize(pB, 1);
            return 1;
        }
*/

        if ( !Mvc_CubeReadSize(pA) )
        {
            Mvc_CoverOrOnePart( pData, pB, pA, Var2 );
            Mvc_CoverSharpOnePart( pData, pA, pB, Var1 );
            Mvc_CubeSetSize(pA, 1);
            return 1;
        }
    }
    else if ( A2impliesB2 )
    { // should be B1impliesA1 - modify pA in Var1 if possible
/*
        if ( !Mvc_CubeReadSize(pA) )
        {
            Mvc_CoverSharpOnePart( pData, pA, pB, Var1 );
            Mvc_CubeSetSize(pA, 1);
            return 1;
        }
*/

        if ( !Mvc_CubeReadSize(pB) )
        {
            Mvc_CoverOrOnePart( pData, pA, pB, Var1 );
            Mvc_CoverSharpOnePart( pData, pB, pA, Var2 );
            Mvc_CubeSetSize(pB, 1);
            return 1;
        }
    }
    else if ( B2impliesA2 )
    { // should be A1impliesB1 - modify pA in Var1 if possible
/*        
        if ( !Mvc_CubeReadSize(pB) )
        {
            Mvc_CoverSharpOnePart( pData, pB, pA, Var1 );
            Mvc_CubeSetSize(pB, 1);
            return 1;
        }
*/

        if ( !Mvc_CubeReadSize(pA) )
        {
            Mvc_CoverOrOnePart( pData, pB, pA, Var1 );
            Mvc_CoverSharpOnePart( pData, pA, pB, Var2 );
            Mvc_CubeSetSize(pA, 1);
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sharps the given part of pA with pB.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverSharpOnePart( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB, int Var )
{
    // create the cube for selective sharping
    Mvc_CubeBitAnd( pData->ppTemp[0], pB, pData->ppMasks[Var] );
    // sharp the cube pA
    Mvc_CubeBitSharp( pA, pA, pData->ppTemp[0] );
}

/**Function*************************************************************

  Synopsis    [ORs the given part of pA using the corresponding part of pB.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverOrOnePart( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB, int Var )
{
    // create the cube for selective sharping
    Mvc_CubeBitAnd( pData->ppTemp[0], pB, pData->ppMasks[Var] );
    // sharp the cube pA
    Mvc_CubeBitOr( pA, pA, pData->ppTemp[0] );
}



/**Function*************************************************************

  Synopsis    [Returns 1 if the cubes are different in the value sets of two literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvc_CoverDiff2Cubes( Mvc_Data_t * pData, Mvc_Cube_t * pA, Mvc_Cube_t * pB, 
    int * pDiff1, int * pDiff2 )
{
    int Res, v;

    // set the number of the diff parts to -1
    *pDiff1 = -1;
    *pDiff2 = -1;

    // go through the parts
	for ( v = 0; v < pData->pVm->nVarsIn; v++ )
	{
        Mvc_CubeBitEqualUnderMask( Res, pA, pB, pData->ppMasks[v] );
        if ( Res == 0 )
        {
            // remember the number of the diff parts
            if ( *pDiff1 == -1 )
                *pDiff1 = v;
            else if ( *pDiff2 == -1 )
                *pDiff2 = v;
            else
            {
                *pDiff1 = -1;
                *pDiff2 = -1;
                return 0;
            }
        }
	}
    if ( *pDiff1 >= 0 && *pDiff2 >= 0 )
        return 1;

    *pDiff1 = -1;
    *pDiff2 = -1;
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


