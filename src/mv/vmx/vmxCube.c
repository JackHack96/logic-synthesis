/**CFile****************************************************************

  FileName    [vmxCube.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Functionality of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmxCube.c,v 1.2 2004/01/06 21:02:58 alanmi Exp $]

***********************************************************************/

#include "vmxInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the positive polarity cube, composed of binary vars representing the MV var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapCharCube( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar )
{
    DdNode * bCube, * bTemp, * bVar;
    int v;

    bCube = b1;  Cudd_Ref( bCube );
    for ( v = 0; v < pVmx->pBits[iVar]; v++ )
    {
        assert( pVmx->pBitsOrder[pVmx->pBitsFirst[iVar] + v] < dd->size );
        bVar = dd->vars[ pVmx->pBitsOrder[pVmx->pBitsFirst[iVar] + v] ];
        bCube = Cudd_bddAnd( dd, bTemp = bCube, bVar );  Cudd_Ref( bCube );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    Cudd_Deref( bCube );
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Returns the positive polarity cube, composed of binary vars representing the MV var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapCharCubeArray( DdManager * dd, Vmx_VarMap_t * pVmx, int pVars[], int nVar )
{
    DdNode * bCube, * bRes, * bTemp;
    int i;
    bRes = b1;    Cudd_Ref( bRes );
    for ( i = 0; i < nVar; i++ )
    {
        bCube = Vmx_VarMapCharCube( dd, pVmx, pVars[i] );     Cudd_Ref( bCube );
        bRes  = Cudd_bddAnd( dd, bTemp = bRes, bCube );       Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Returns the positive polarity cube, composed of binary vars representing the MV var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapCharCubeRange( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar, int nVars )
{
    DdNode * bCube, * bCube1, * bTemp;
    int i;
    assert( iVar >= 0 && iVar  <= pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut );
    assert( nVars>= 0 && nVars <= pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut );
    bCube = b1;   Cudd_Ref( bCube );
    for ( i = iVar; i < iVar + nVars; i++ )
    {
        bCube1 = Vmx_VarMapCharCube( dd, pVmx, i );         Cudd_Ref( bCube1 );
        bCube  = Cudd_bddAnd( dd, bTemp = bCube, bCube1 );  Cudd_Ref( bCube );
        Cudd_RecursiveDeref( dd, bTemp ); 
        Cudd_RecursiveDeref( dd, bCube1 ); 
    }
    Cudd_Deref( bCube );
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Returns the positive polarity cube, composed of binary vars representing the MV var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapCharCubeUsingVars( DdManager * dd, DdNode ** pbVars, Vmx_VarMap_t * pVmx, int iVar )
{
    DdNode * bCube, * bTemp, * bVar;
    int v;

    bCube = b1;  Cudd_Ref( bCube );
    for ( v = 0; v < pVmx->pBits[iVar]; v++ )
    {
        bVar = pbVars[ pVmx->pBitsOrder[pVmx->pBitsFirst[iVar] + v] ];
        bCube = Cudd_bddAnd( dd, bTemp = bCube, bVar );  Cudd_Ref( bCube );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    Cudd_Deref( bCube );
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Returns the positive polarity cube, composed of binary vars representing the MV var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapCharCubeInput( DdManager * dd, Vmx_VarMap_t * pVmx )
{
    DdNode * bCube, * bTemp, * bComp;
    int v;

    bCube = b1;  Cudd_Ref( bCube );
    for ( v = 0; v < pVmx->pVm->nVarsIn; v++ )
    {
        bComp = Vmx_VarMapCharCube( dd, pVmx, v );       Cudd_Ref( bComp );
        bCube = Cudd_bddAnd( dd, bTemp = bCube, bComp ); Cudd_Ref( bCube );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bComp );
    }

    Cudd_Deref( bCube );
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Returns the positive polarity cube, composed of binary vars representing the MV var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapCharCubeInputUsingVars( DdManager * dd, DdNode ** pbVars, Vmx_VarMap_t * pVmx )
{
    DdNode * bCube, * bTemp, * bComp;
    int v;

    bCube = b1;  Cudd_Ref( bCube );
    for ( v = 0; v < pVmx->pVm->nVarsIn; v++ )
    {
        bComp = Vmx_VarMapCharCubeUsingVars( dd, pbVars, pVmx, v );       Cudd_Ref( bComp );
        bCube = Cudd_bddAnd( dd, bTemp = bCube, bComp ); Cudd_Ref( bCube );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bComp );
    }

    Cudd_Deref( bCube );
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Returns the positive polarity cube, composed of binary vars representing the MV var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapCharCubeOutput( DdManager * dd, Vmx_VarMap_t * pVmx )
{
    DdNode * bCube, * bTemp, * bComp;
    int v;

    bCube = b1;  Cudd_Ref( bCube );
    for ( v = pVmx->pVm->nVarsIn; v < pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut; v++ )
    {
        bComp = Vmx_VarMapCharCube( dd, pVmx, v );       Cudd_Ref( bComp );
        bCube = Cudd_bddAnd( dd, bTemp = bCube, bComp ); Cudd_Ref( bCube );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bComp );
    }

    Cudd_Deref( bCube );
    return bCube;
}

/**Function*************************************************************

  Synopsis    [Creates the cube of the values of the given vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapCodeCube( DdManager * dd, Vmx_VarMap_t * pVmx, int pVars[], int pCodes[], int nVars )
{
    DdNode ** pbCodes;
    int * pValuesFirst;
    DdNode * bCube, * bValue, * bTemp;
    int i;

    // get the info
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm(pVmx) );

    // create the cube
    bCube = b1;   Cudd_Ref( bCube );
    for ( i = 0; i < nVars; i++ )
    {
        bValue = pbCodes[ pValuesFirst[pVars[i]] + pCodes[i] ];
        bCube = Cudd_bddAnd( dd, bTemp = bCube, bValue );   Cudd_Ref( bCube );
        Cudd_RecursiveDeref( dd, bTemp ); 
    }

    // get rid of codes
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );

    Cudd_Deref( bCube );
    return bCube;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


