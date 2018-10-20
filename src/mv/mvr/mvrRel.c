/**CFile****************************************************************

  FileName    [mvrRel.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Functionality of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvrRel.c,v 1.17 2003/05/27 23:15:20 alanmi Exp $]

***********************************************************************/

#include "mvrInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the data structure for the MV relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationAlloc()
{
    Mvr_Relation_t * pMvr;
    pMvr = ALLOC( Mvr_Relation_t, 1 );
    memset( pMvr, 0, sizeof(Mvr_Relation_t) );
    pMvr->nBddNodes = BDD_NODES_UNKNOWN;
    return pMvr;
}

/**Function*************************************************************

  Synopsis    [Creates the relation with the same var map and function as given.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationClone( Mvr_Relation_t * pMvr )
{
    Mvr_Relation_t * pMvrCopy;
    pMvrCopy = Mvr_RelationAlloc();
    pMvrCopy->pMan  = pMvr->pMan;
    pMvrCopy->pVmx  = pMvr->pVmx;
    return pMvrCopy;
}

/**Function*************************************************************

  Synopsis    [Creates the relation with the same var map and function as given.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Mvr_RelationDup( Mvr_Relation_t * pMvr )
{
    Mvr_Relation_t * pMvrCopy;
    pMvrCopy = Mvr_RelationAlloc();
    pMvrCopy->pMan = pMvr->pMan;
    pMvrCopy->pVmx = pMvr->pVmx;
    if ( pMvr->bRel )
    {
        pMvrCopy->bRel = pMvr->bRel;   Cudd_Ref( pMvrCopy->bRel );
        pMvrCopy->nBddNodes = pMvr->nBddNodes;
    }
    return pMvrCopy;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_RelationFree( Mvr_Relation_t * pMvr )
{
    Mvr_RelationInvalidateRel( pMvr );
    free( pMvr );
}

/**Function*************************************************************

  Synopsis    [Invalidates the relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_RelationInvalidateRel( Mvr_Relation_t * pMvr )
{
    if ( pMvr->bRel == NULL )
        return;
    Cudd_RecursiveDeref( pMvr->pMan->pDdLoc, pMvr->bRel );
    pMvr->bRel = NULL;
    pMvr->nBddNodes = BDD_NODES_UNKNOWN;
    pMvr->NonDet = MVR_UNKNOWN;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvr_RelationIsND( Mvr_Relation_t * pMvr )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
	Vm_VarMap_t * pVm;
    DdNode ** pbIsets;
    int nOutputValues, i, k;

    // if the ND status of this relation is known, return it
    if ( pMvr->NonDet != MVR_UNKNOWN )
        return (int)(pMvr->NonDet == MVR_NONDETERM);

	// get the MV var map of this relation
	pVm = pMvr->pVmx->pVm;
	// get the number of output values
	nOutputValues = Vm_VarMapReadValuesOutput( pVm );

    // allocate room for cofactors
    pbIsets = ALLOC( DdNode *, nOutputValues );
	// set the i-sets w.r.t the last variable
    Mvr_RelationCofactorsDerive( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );

    // check the i-sets for overlapping
    pMvr->NonDet = MVR_DETERM;
    for ( i = 0; i < nOutputValues; i++ )
        for ( k = i+1; k < nOutputValues; k++ )
		    if ( !Cudd_bddLeq( dd, pbIsets[i], Cudd_Not(pbIsets[k]) ) ) // they intersect
            {
                pMvr->NonDet = MVR_NONDETERM;
                break;
            }

    Mvr_RelationCofactorsDeref( pMvr, pbIsets, pVm->nVarsIn, nOutputValues );
    FREE( pbIsets );

    return (int)(pMvr->NonDet == MVR_NONDETERM);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mvr_RelationIsWellDefined( Mvr_Relation_t * pMvr )
{
    DdManager * dd = pMvr->pMan->pDdLoc;
    DdNode * bCubeOut, * bDomain;
    bool RetValue;
    // get the input char cube
    bCubeOut = Vmx_VarMapCharCubeOutput( dd, pMvr->pVmx );       Cudd_Ref( bCubeOut );
    // quantify the relation
    bDomain = Cudd_bddExistAbstract( dd, pMvr->bRel, bCubeOut ); Cudd_Ref( bDomain );
    // check if the domain includes the whole space
    RetValue = (bool)( bDomain == b1 );
    // derefence
    Cudd_RecursiveDeref( dd, bCubeOut );
    Cudd_RecursiveDeref( dd, bDomain );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Computes the flexibility ratio as shown in ICCAD 2002 paper.]

  Description [The paper defines the flexibility ratio as follows
  P = 100% * (T - M) / M / (V - 1), where T is the sum total of minterms of
  the output values for all the input minterms of the relation, M is the
  number of input minterms. V is the number of output values. This ratio
  is equal to 0% when the relation is completely specified and to 100%
  when it is a ND relation equal to all values in all minterms.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Mvr_RelationFlexRatio( Mvr_Relation_t * pMvr )
{
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    DdNode * pbCofs[32];
    int * pBitsFirst;
    int nVarsIn, nValuesOut, nBinVarsIn, v;
    double Res, T, M;

    // get the parameters
    pVm         = Mvr_RelationReadVm( pMvr );
    pVmx        = Mvr_RelationReadVmx( pMvr );
    nVarsIn     = Vm_VarMapReadVarsInNum( pVm );
    nValuesOut  = Vm_VarMapReadValuesOutNum( pVm );
    pBitsFirst  = Vmx_VarMapReadBitsFirst( pVmx );
    nBinVarsIn  = pBitsFirst[nVarsIn];

    // cofactor the relation w.r.t. the output variable
    Mvr_RelationCofactorsDerive( pMvr, pbCofs, nVarsIn, nValuesOut );
    // get the total number of minterms in the relation
    T = 0.0;
    for ( v = 0; v < nValuesOut; v++ )
        T += Cudd_CountMinterm( pMvr->pMan->pDdLoc, pbCofs[v], nBinVarsIn );
    Mvr_RelationCofactorsDeref( pMvr, pbCofs, nVarsIn, nValuesOut );

    // get the size of the input space
    M = Extra_Power2( nBinVarsIn );

    Res = 100 * (T - M) / M / (nValuesOut - 1 );
    return Res;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


