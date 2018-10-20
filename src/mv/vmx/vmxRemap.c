/**CFile****************************************************************

  FileName    [vmxRemap.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Remaps the functions using information recorded in the 
  variable map.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmxRemap.c,v 1.2 2004/01/06 21:02:59 alanmi Exp $]

***********************************************************************/

#include "vmxInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Remaps BEMDD from the old map to the new map.]

  Description [Takes a BEMDD of any function that depends on the old
  variable map (pMapOld) (this function can be an i-set or a relation itself). 
  Assuming that this BEMDD is expressed using the old variable map (pMapOld), 
  this procedure remaps it to the new variable map (pMapNew). Additional 
  information for the remapping is given in the array pVarTrans. For each 
  MV variable in the old var map, this array contains the corresponding 
  MV variable in the new var map.  Array pVarTrans may contain entry -1,
  if this variable is not used in the function to be remapped. Obviously, 
  the number of values in the corresponding pairs of MV variables (one
  from the old map, another from the new map) should be the same.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapRemapFunc( DdManager * dd, DdNode * bFunc,
    Vmx_VarMap_t * pVmxOld, Vmx_VarMap_t * pVmxNew, int * pVarTrans )
{
    DdNode * bRes;
    Vm_VarMap_t * pVmOld, * pVmNew;
    int nVmOld, v, b, index;
    int * pPermute, i;

    // get the parameters
    pVmOld = pVmxOld->pVm;
    pVmNew = pVmxNew->pVm;
    nVmOld = pVmOld->nVarsIn + pVmOld->nVarsOut;

    // extend the local manager if necessary
    if ( dd->size < pVmxNew->nBits )
    {
        for ( i = dd->size; i < pVmxNew->nBits + 10; i++ )
            Cudd_bddIthVar( dd, i );
        Cudd_zddVarsFromBddVars( dd, 2 );
    }

    // set up the permutation array
    pPermute = ALLOC( int, dd->size );
    for ( v = 0; v < nVmOld; v++ )
        if ( pVarTrans[v] != -1 )
        {
            assert( pVmOld->pValues[v] == pVmNew->pValues[pVarTrans[v]] );
            for ( b = 0; b < pVmxOld->pBits[v]; b++ )
            {
                index = pVmxOld->pBitsOrder[ pVmxOld->pBitsFirst[v] + b ];
                assert( index >= 0 && index < pVmxOld->nBits );
                pPermute[index] = pVmxNew->pBitsOrder[ pVmxNew->pBitsFirst[pVarTrans[v]] + b ];
            }
        }
    // remap the function
    bRes = Cudd_bddPermute( dd, bFunc, pPermute ); // returns non-referenced
//    bRes = Extra_Permute( pMvr->pMan->pPerm, dd, bFunc, pPermute );  
    FREE( pPermute );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Remaps two MV variables in the given MDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapRemapVar( DdManager * dd, DdNode * bFunc, 
     Vmx_VarMap_t * pVmx, int iVar1, int iVar2 )
{
    return Vmx_VarMapRemapRange( dd, bFunc, pVmx, iVar1, 1, iVar2, 1 );
}

/**Function*************************************************************

  Synopsis    [Remaps two MV variables in the given MDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapRemapRange( DdManager * dd, DdNode * bFunc, 
     Vmx_VarMap_t * pVmx, int iVar1, int nVars1, int iVar2, int nVars2 )
{
    DdNode ** pbVars1, ** pbVars2;
    DdNode * bRes;
    int i, nBits1, nBits2;
    // count the number of variables in both ranges
    nBits1 = 0;
    for ( i = iVar1; i < iVar1 + nVars1; i++ )
        nBits1 += pVmx->pBits[i];
    nBits2 = 0;
    for ( i = iVar2; i < iVar2 + nVars2; i++ )
        nBits2 += pVmx->pBits[i];
    assert( nBits1 == nBits2 );
    pbVars1 = Vmx_VarMapBinVarsRange( dd, pVmx, iVar1, nVars1 );
    pbVars2 = Vmx_VarMapBinVarsRange( dd, pVmx, iVar2, nVars2 );
    bRes = Cudd_bddSwapVariables( dd, bFunc, pbVars1, pbVars2, nBits1 );
    FREE( pbVars1 );
    FREE( pbVars2 );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Remaps two MV variables in the given MDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapRemapCouple( DdManager * dd, DdNode * bFunc, 
     Vmx_VarMap_t * pVmx, int iVarA1, int iVarA2, int iVarB1, int iVarB2 )
{
    DdNode ** pbVars1, ** pbVars2;
    DdNode * bRes;
    int nBits1, nBits2;
    // count the number of variables in both ranges
    nBits1  = pVmx->pBits[iVarA1];
    nBits1 += pVmx->pBits[iVarA2];
    nBits2  = pVmx->pBits[iVarB1];
    nBits2 += pVmx->pBits[iVarB2];
    assert( nBits1 == nBits2 );
    pbVars1 = Vmx_VarMapBinVarsCouple( dd, pVmx, iVarA1, iVarA2 );
    pbVars2 = Vmx_VarMapBinVarsCouple( dd, pVmx, iVarB1, iVarB2 );
    bRes = Cudd_bddSwapVariables( dd, bFunc, pbVars1, pbVars2, nBits1 );
    FREE( pbVars1 );
    FREE( pbVars2 );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Remaps two MV variables in the given MDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapRemapVarSetup( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar1, int iVar2 )
{
    Vmx_VarMapRemapRangeSetup( dd, pVmx, iVar1, 1, iVar2, 1 );
}

/**Function*************************************************************

  Synopsis    [Sets up the CUDD BDD manager to remap these vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapRemapRangeSetup( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar1, int nVars1, int iVar2, int nVars2 )
{
    DdNode ** pbVars1, ** pbVars2;
    int i, nBits1, nBits2;
    // count the number of variables in both ranges
    nBits1 = 0;
    for ( i = iVar1; i < iVar1 + nVars1; i++ )
        nBits1 += pVmx->pBits[i];
    nBits2 = 0;
    for ( i = iVar2; i < iVar2 + nVars2; i++ )
        nBits2 += pVmx->pBits[i];
    assert( nBits1 == nBits2 );
    pbVars1 = Vmx_VarMapBinVarsRange( dd, pVmx, iVar1, nVars1 );
    pbVars2 = Vmx_VarMapBinVarsRange( dd, pVmx, iVar2, nVars2 );
    Cudd_SetVarMap( dd, pbVars1, pbVars2, nBits1 );
    FREE( pbVars1 );
    FREE( pbVars2 );
}

/**Function*************************************************************

  Synopsis    [Sets up the permutation variable map [CS <=> NS].]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapRemapSetup( DdManager * dd, Vmx_VarMap_t * pVmx, int * pVarsCs, int * pVarsNs, int nVars )
{
    DdNode ** pbVarsX, ** pbVarsY;
    int * pBits, * pBitsFirst, * pBitsOrder;
    int nBddVars, i, b;;

    // set the variable mapping for Cudd_bddVarMap()
    pbVarsX = ALLOC( DdNode *, dd->size );
    pbVarsY = ALLOC( DdNode *, dd->size );
    pBits = Vmx_VarMapReadBits( pVmx );
    pBitsFirst = Vmx_VarMapReadBitsFirst( pVmx );
    pBitsOrder = Vmx_VarMapReadBitsOrder( pVmx );
    nBddVars = 0;
    for ( i = 0; i < nVars; i++ )
    {
        assert( pBits[pVarsCs[i]] == pBits[pVarsNs[i]] );
        for ( b = 0; b < pBits[pVarsCs[i]]; b++ )
        {
            pbVarsX[nBddVars] = dd->vars[ pBitsOrder[pBitsFirst[pVarsCs[i]] + b] ];
            pbVarsY[nBddVars] = dd->vars[ pBitsOrder[pBitsFirst[pVarsNs[i]] + b] ];
            nBddVars++;
        }
    }
    Cudd_SetVarMap( dd, pbVarsX, pbVarsY, nBddVars );
    FREE( pbVarsX );
    FREE( pbVarsY );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


