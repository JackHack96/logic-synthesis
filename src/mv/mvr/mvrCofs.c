/**CFile****************************************************************

  FileName    [mvrCofs.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Functionality of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvrCofs.c,v 1.11 2003/11/18 18:55:05 alanmi Exp $]

***********************************************************************/

#include "mvrInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the 0-set of the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvr_RelationGetIset( Mvr_Relation_t * pMvr, int iSet )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode ** pbCodes;
    DdNode * bCube, * bRes;
    int iOutVar;

    iOutVar = pMvr->pVmx->pVm->nVarsIn;

    assert( pMvr->bRel );
    assert( pMvr->pVmx->pVm->nVarsOut == 1 );
    assert( iSet >= 0 );
    assert( iSet < pMvr->pVmx->pVm->pValues[iOutVar] );

    // derive the encoding of the output values
    pbCodes = Vmx_VarMapEncodeVar( dd, pMvr->pVmx, iOutVar );
    bCube   = Vmx_VarMapCharCube( dd, pMvr->pVmx, iOutVar );  Cudd_Ref( bCube );
    // cofactor the relation
    bRes = Cudd_bddAndAbstract( dd, pMvr->bRel, pbCodes[iSet], bCube );  Cudd_Ref( bRes );
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCodes );
    Cudd_RecursiveDeref( dd, bCube );
    Cudd_Deref( bRes );
    return bRes;
}


/**Function*************************************************************

  Synopsis    [Derives the cofactors from the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_RelationCofactorsDerive( Mvr_Relation_t * pMvr, DdNode ** pbCofs, int iVar, int nValues )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode ** pbCodes;
    DdNode * bCube;
    int v;

    assert( nValues == pMvr->pVmx->pVm->pValues[iVar] );
    assert( pMvr->bRel );

    // derive the encoding of the output values
    pbCodes = Vmx_VarMapEncodeVar( dd, pMvr->pVmx, iVar );
    bCube   = Vmx_VarMapCharCube( dd, pMvr->pVmx, iVar );  Cudd_Ref( bCube );
    // cofactor the relation
    for ( v = 0; v < nValues; v++ )
    {
        pbCofs[v] = Cudd_bddAndAbstract( dd, pMvr->bRel, pbCodes[v], bCube );
        Cudd_Ref( pbCofs[v] );
    }
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCodes );
    Cudd_RecursiveDeref( dd, bCube );
    // pbCofs remain referenced
}


/**Function*************************************************************

  Synopsis    [Derives the relation from the cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvr_RelationCofactorsDeriveRelation( Mvr_Relation_t * pMvr, DdNode ** pbCofs, int iVar, int nValues )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode ** pbCodes;
    DdNode * bProd, * bTemp;
    int v;

    assert( nValues == pMvr->pVmx->pVm->pValues[iVar] );
    Mvr_RelationInvalidateRel( pMvr );

    // derive the encoding of the values of the variable
    pbCodes = Vmx_VarMapEncodeVar( dd, pMvr->pVmx, iVar );
    // cofactor the relation
    pMvr->bRel = b0;   Cudd_Ref( pMvr->bRel );
    for ( v = 0; v < nValues; v++ )
    {
        bProd = Cudd_bddAnd( dd, pbCofs[v], pbCodes[v] ); Cudd_Ref( bProd );
        pMvr->bRel = Cudd_bddOr( dd, bTemp = pMvr->bRel, bProd ); Cudd_Ref( pMvr->bRel );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bProd );
    }
    Vmx_VarMapEncodeDeref( dd, pMvr->pVmx, pbCodes );
    pMvr->nBddNodes = BDD_NODES_UNKNOWN;
    // relation remains referenced
	return pMvr->bRel;
}



/**Function*************************************************************

  Synopsis    [Dereferences the cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_RelationCofactorsDeref( Mvr_Relation_t * pMvr, DdNode ** pbCofs, int iVar, int nValues )
{
    int v;
    assert( iVar == -1 || nValues == pMvr->pVmx->pVm->pValues[iVar] );
    // cofactor the relation
    for ( v = 0; v < nValues; v++ )
        Cudd_RecursiveDeref( pMvr->pMan->pDdLoc, pbCofs[v] );
}


/**Function*************************************************************

  Synopsis    [Performs the quantification.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvr_RelationQuantifyExist( Mvr_Relation_t * pMvr, DdNode * bFunc, int iVar )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode * bCube, * bRes;
    bCube = Vmx_VarMapCharCube( dd, pMvr->pVmx, iVar );  Cudd_Ref( bCube );
    bRes = Cudd_bddExistAbstract( dd, bFunc, bCube );    Cudd_Ref( bRes );
    Cudd_RecursiveDeref( dd, bCube );
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Performs the quantification.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvr_RelationQuantifyUniv( Mvr_Relation_t * pMvr, DdNode * bFunc, int iVar )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode * bCube, * bRes;
    bCube = Vmx_VarMapCharCube( dd, pMvr->pVmx, iVar );  Cudd_Ref( bCube );
    bRes = Cudd_bddUnivAbstract( dd, bFunc, bCube );    Cudd_Ref( bRes );
    Cudd_RecursiveDeref( dd, bCube );
    Cudd_Deref( bRes );
    return bRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


