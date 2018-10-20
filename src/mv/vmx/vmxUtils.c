/**CFile****************************************************************

  FileName    [vmxUtils.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various utilities to create extended variable maps.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmxUtils.c,v 1.13 2003/12/07 04:49:11 alanmi Exp $]

***********************************************************************/

#include "vmxInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Decrements the ref counter of the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapPrint( Vmx_VarMap_t * pVmx )
{
    Vm_VarMap_t * pVm = pVmx->pVm;
    int i, b;
    printf( "Extended variable map (hash value = %d)\n", 
        Vmx_VarMapHash( pVm, pVmx->nBits, pVmx->pBitsOrder ) ); 
    printf( "Statistics: Inputs = %d. Outputs = %d. Values = %d. Bits = %d.\n", 
        pVm->nVarsIn, pVm->nVarsOut, pVm->nValuesIn + pVm->nValuesOut, pVmx->nBits );
    printf( "Input variables:\n" ); 
    for ( i = 0; i < pVm->nVarsIn; i++ )
    {
        printf( "%2d:  Values = %d. Bits = %d. Map = {", 
            i, pVm->pValues[i], pVmx->pBits[i] );
        for ( b = 0; b < pVmx->pBits[i]; b++ )
            printf( " %d", pVmx->pBitsOrder[ pVmx->pBitsFirst[i] + b ] );
        printf( " }\n" );
    }
    printf( "Output variables:\n" ); 
    for ( i = pVm->nVarsIn; i < pVm->nVarsIn + pVm->nVarsOut; i++ )
    {
        printf( "%2d:  Values = %d. Bits = %d. Map = {", 
            i, pVm->pValues[i], pVmx->pBits[i] );
        for ( b = 0; b < pVmx->pBits[i]; b++ )
            printf( " %d", pVmx->pBitsOrder[ pVmx->pBitsFirst[i] + b ] );
        printf( " }\n" );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Returns the array of binary vars for the given MV var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Vmx_VarMapBinVarsVar( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar )
{
    return Vmx_VarMapBinVarsRange( dd, pVmx, iVar, 1 );
}

/**Function*************************************************************

  Synopsis    [Returns the array of binary vars for the given MV vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Vmx_VarMapBinVarsRange( DdManager * dd, Vmx_VarMap_t * pVmx, int iFirst, int nVars )
{
    DdNode ** pbVars;
    int i, b, iBit;
    pbVars = ALLOC( DdNode *, dd->size );
    iBit = 0;
    for ( i = iFirst; i < iFirst + nVars; i++ )
        for ( b = 0; b < pVmx->pBits[i]; b++ )
            pbVars[iBit++] = dd->vars[ pVmx->pBitsOrder[ pVmx->pBitsFirst[i] + b ] ];
    return pbVars;
}

/**Function*************************************************************

  Synopsis    [Returns the array of binary vars for the given MV vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Vmx_VarMapBinVarsCouple( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar1, int iVar2 )
{
    DdNode ** pbVars;
    int b, iBit;
    pbVars = ALLOC( DdNode *, dd->size );
    iBit = 0;
    for ( b = 0; b < pVmx->pBits[iVar1]; b++ )
        pbVars[iBit++] = dd->vars[ pVmx->pBitsOrder[ pVmx->pBitsFirst[iVar1] + b ] ];
    for ( b = 0; b < pVmx->pBits[iVar2]; b++ )
        pbVars[iBit++] = dd->vars[ pVmx->pBitsOrder[ pVmx->pBitsFirst[iVar2] + b ] ];
    return pbVars;
}

/**Function*************************************************************

  Synopsis    [Returns the array of binary vars for the given MV vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vmx_VarMapBitsNumRange( Vmx_VarMap_t * pVmx, int iFirst, int nVars )
{
    int i, nBits;
    nBits = 0;
    for ( i = iFirst; i < iFirst + nVars; i++ )
        nBits += pVmx->pBits[i];
    return nBits;
}

/**Function*************************************************************

  Synopsis    [Returns the largest bit used in the map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vmx_VarMapGetLargestBit( Vmx_VarMap_t * pVmx )
{
    int nVars, iBitMax;
    int i, b;

    iBitMax = -1;
    nVars = pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut;
	for ( i = 0; i < nVars; i++ )
		for ( b = 0; b < pVmx->pBits[i]; b++ )
        {
            if ( iBitMax < pVmx->pBitsOrder[ pVmx->pBitsFirst[i] + b ] )
                iBitMax = pVmx->pBitsOrder[ pVmx->pBitsFirst[i] + b ];
        }
    return iBitMax + 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


