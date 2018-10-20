/**CFile****************************************************************

  FileName    [mvcMerge.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various cube merging procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcMerge.c,v 1.4 2003/09/01 04:56:53 alanmi Exp $]

***********************************************************************/

#include "mvc.h"
#include "vm.h"
#include "vmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Mvc_CoverDist1MergeOne( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Performs distance 1 merge.]

  Description [Returns 1 if the cover has changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDist1Merge( Mvc_Data_t * p, Mvc_Cover_t * pCover )
{
    int i;
    if ( Mvc_CoverReadCubeNum(pCover) < 2 )
        return;
    for ( i = 0; i < p->pVm->nVarsIn; i++ )
        Mvc_CoverDist1MergeOne( pCover, p->ppMasks[i] );
}


/**Function*************************************************************

  Synopsis    [Performs distance 1 merge for one variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDist1MergeOne( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask )
{
    Mvc_Cube_t * pPrev, * pCube, * pCube2;
    bool fEqual;

    // sort
    Mvc_CoverSort( pCover, pMask, Mvc_CubeCompareIntOutsideMask );
    // set the first cube of the cover
    pPrev = Mvc_CoverReadCubeHead(pCover);
    // go through all the cubes after this one
    Mvc_CoverForEachCubeStartSafe( Mvc_CubeReadNext(pPrev), pCube, pCube2 )
    {
        // compare the current cube with the prev cube
        Mvc_CubeBitEqualOutsideMask( fEqual, pPrev, pCube, pMask );
        if ( fEqual )
        { // they are equal under the mask
            // transfer the values from the second cube to the first one
            Mvc_CubeBitOr( pPrev, pPrev, pCube );
            // remove the cube
            Mvc_CoverDeleteCube( pCover, pPrev, pCube );
            Mvc_CubeFree( pCover, pCube );
            // don't change the previous cube cube
        }
        else
        { // they are not equal - update the previous cube
            pPrev = pCube;
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


