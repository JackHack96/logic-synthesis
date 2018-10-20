/**CFile****************************************************************

  FileName    [mvcData.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Sets up additional data structures for MV cover processing.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcData.c,v 1.5 2003/09/01 04:56:51 alanmi Exp $]

***********************************************************************/

#include "mvc.h"
#include "vm.h"
#include "vmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the structure for the given cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Data_t * Mvc_CoverDataAlloc( Vm_VarMap_t * pVm, Mvc_Cover_t * pCover )
{
    Mvc_Data_t * p;
    int v, i;
    p = ALLOC( Mvc_Data_t, 1 );
    memset( p, 0, sizeof(Mvc_Data_t) );
    p->pVm = pVm;
    p->pMan = pCover->pMem;
    if ( pVm->nVarsIn )
    {
        // allocate the binary mask
        p->pMaskBin = Mvc_CubeAlloc( pCover );
        Mvc_CubeBitClean( p->pMaskBin );
        // allocate the storage for var masks
        p->ppMasks  = ALLOC( Mvc_Cube_t *, pVm->nVarsIn );
        for ( v = 0; v < pVm->nVarsIn; v++ )
        {
            // allocate the given var mask
            p->ppMasks[v] = Mvc_CubeAlloc( pCover );
            Mvc_CubeBitClean( p->ppMasks[v] );
            // fill up the given var mask
            for ( i = 0; i < pVm->pValues[v]; i++ )
                Mvc_CubeBitInsert( p->ppMasks[v], pVm->pValuesFirst[v] + i );
            // consider the case when this var is binary
            if ( pVm->pValues[v] == 2 )
            {
                Mvc_CubeBitOr( p->pMaskBin, p->pMaskBin, p->ppMasks[v] );
                p->nBinVars++;
            }
        }
    }
    // allocate the temporary cubes
    for ( i = 0; i < 3; i++ )
        p->ppTemp[i] = Mvc_CubeAlloc( pCover );
    return p;
}
 
/**Function*************************************************************

  Synopsis    [Deallocates the structure for the given cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDataFree( Mvc_Data_t * p, Mvc_Cover_t * pCover )
{
    int i;
    if ( p->pVm->nVarsIn )
    {
        for ( i = 0; i < p->pVm->nVarsIn; i++ )
            Mvc_CubeFree( pCover, p->ppMasks[i] );
        Mvc_CubeFree( pCover, p->pMaskBin );
        for ( i = 0; i < 3; i++ )
            Mvc_CubeFree( pCover, p->ppTemp[i] );
        FREE( p->ppMasks );
    }
    FREE( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


