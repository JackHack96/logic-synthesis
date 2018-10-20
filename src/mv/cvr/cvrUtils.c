/**CFile****************************************************************

  FileName    [cvrUtils.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cvrUtils.c,v 1.7 2003/05/27 23:15:02 alanmi Exp $]

***********************************************************************/

#include "cvrInt.h"


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [return a new cube]
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
Mvc_Cube_t *
Cvr_CoverCreateCube( Cvr_Cover_t *pCvr )
{
    int i, nValues;
    nValues = Vm_VarMapReadValuesOutNum( pCvr->pVm );
    for ( i=0; i<nValues; ++i ) {
        if ( pCvr->ppCovers[i] )
            return Mvc_CubeAlloc( pCvr->ppCovers[i] );
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [delete a cube]
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
void
Cvr_CoverDestroyCube( Cvr_Cover_t *pCvr, Mvc_Cube_t *pMvcCube )
{
    int i, nValues;
    nValues = Vm_VarMapReadValuesOutNum( pCvr->pVm );
    for ( i=0; i<nValues; ++i ) {
        if ( pCvr->ppCovers[i] ) {
            Mvc_CubeFree( pCvr->ppCovers[i], pMvcCube );
            return;
        }
    }
}


/**Function*************************************************************

  Synopsis    [return the number of variables in the support]
  Description [Takes an MV relation and the array where the resulting 
  support is written. Entry 1 means var is in the support. Entry 0 means
  var is not in the support. This procedure returns the total number of 
  MV variables in the support.]

  SideEffects []
  SeeAlso     []

***********************************************************************/
int
Cvr_CoverSupport( Cvr_Cover_t *pCvr, int *pSupp ) 
{
    bool         fEmpty;
    int          i, iBit, nValues, nVars, nSupp;
    int         *pValues, *pValuesFirst;

    Mvc_Cube_t  *pMvcCube, *pComCube;
    
    nVars         = Vm_VarMapReadVarsInNum( pCvr->pVm );
    if ( nVars == 0 ) return 0;
    
    /* start with full support */
    for ( i=0; i<nVars; ++i ) {
        pSupp[i] = 1;
    }
    pSupp[nVars] = 1;
    
    nValues       = Vm_VarMapReadValuesOutNum( pCvr->pVm );
    pValues       = Vm_VarMapReadValuesArray( pCvr->pVm );
    pValuesFirst  = Vm_VarMapReadValuesFirstArray( pCvr->pVm );
    pMvcCube      = Cvr_CoverCreateCube( pCvr );
    pComCube      = Cvr_CoverCreateCube( pCvr );
    Mvc_CubeBitFill( pComCube );
    
    /* find common columns */
    for ( i=0; i<nValues; ++i ) {
        if ( pCvr->ppCovers[i] ) {
            Mvc_CoverCommonCube( pCvr->ppCovers[i], pMvcCube );
            Mvc_CubeBitAnd( pComCube, pComCube, pMvcCube );
        }
    }
    
    /* if full support then return */
    Mvc_CubeBitEmpty( fEmpty, pComCube );
    if ( fEmpty ) {
        Cvr_CoverDestroyCube( pCvr, pMvcCube );
        Cvr_CoverDestroyCube( pCvr, pComCube );
        return (nVars+1);
    }
    
    /* find vars not in support */
    nSupp = nVars + 1;
    for ( i=0; i<nVars; i++ ) {
        for ( iBit=0; iBit<pValues[i]; iBit++ ) {
            if ( !Mvc_CubeBitValue( pComCube, pValuesFirst[i] + iBit ) ) {
                break;
            }
        }
        /* if all bits are one, then not in the support */
        if ( iBit == pValues[i] ) {
            pSupp[i] = 0;
            nSupp--;
        }
    }
    Cvr_CoverDestroyCube( pCvr, pMvcCube );
    Cvr_CoverDestroyCube( pCvr, pComCube );
    
    return nSupp;
}


/**Function*************************************************************

  Synopsis    []
  Description [a new cvr is created]
  SideEffects []
  SeeAlso     []

***********************************************************************/
Cvr_Cover_t *
Cvr_CoverCreateMinimumBase( Cvr_Cover_t *pCvr, int *pSupp )
{
    int i, nVars, nBits, iBit, iVar, nValues;
    int *pnFirst, *pnSizes, *pBitPerm, *pnFirstNew;

    Vm_VarMap_t *pVmNew;
    Cvr_Cover_t *pCvrNew;
    
    nVars   = Vm_VarMapReadVarsInNum( pCvr->pVm );
    nValues = Vm_VarMapReadValuesOutNum( pCvr->pVm );
    pnFirst = Vm_VarMapReadValuesFirstArray( pCvr->pVm );
    pnSizes = Vm_VarMapReadValuesArray( pCvr->pVm );
    
    pVmNew     = Vm_VarMapCreateReduced( pCvr->pVm, pSupp );
    nBits      = Vm_VarMapReadValuesInNum( pVmNew );
    pnFirstNew = Vm_VarMapReadValuesFirstArray( pVmNew );
    
    pCvrNew = ALLOC( Cvr_Cover_t, 1 );
    pCvrNew->pVm   = pVmNew;
    pCvrNew->pTree = NULL;
    pCvrNew->ppCovers = ALLOC( Mvc_Cover_t *, nValues );
    
    // pBitPerm[new position] = old position 
    pBitPerm = ALLOC( int, nBits );
    iVar = 0;
    for ( i=0; i<nVars; ++i ) {
        if ( pSupp[i] > 0 ) {
            for ( iBit=0; iBit<pnSizes[i]; ++iBit ) {
                pBitPerm[pnFirstNew[iVar] + iBit] = pnFirst[i] + iBit;
            }
            iVar ++;
        }
    }
    assert( iVar == Vm_VarMapReadVarsInNum( pVmNew ) );
    
    // now do the permutation
    for ( i=0; i<nValues; ++i ) {
        if ( pCvr->ppCovers[i] ) {
            pCvrNew->ppCovers[i] = Mvc_CoverRemap( pCvr->ppCovers[i], pBitPerm, nBits );
        }
        else {
            pCvrNew->ppCovers[i] = NULL;
        }
    }
    
    FREE(pBitPerm);
    return pCvrNew;
}

/**Function*************************************************************

  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []

***********************************************************************/
void
Cvr_CoverPrint( Cvr_Cover_t *pCvr )
{
    int i, nValues;
    nValues = Vm_VarMapReadValuesOutNum( pCvr->pVm );
    for ( i=0; i<nValues; ++i ) {
        if ( pCvr->ppCovers[i] ) {
            printf( "PART[%d]=\n", i );
            Mvc_CoverPrint( pCvr->ppCovers[i] );
        }
        else {
            printf( "PART[%d]=NULL\n", i );
        }
    }
}


  
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


