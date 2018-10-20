/**CFile****************************************************************

  FileName    [vmxCreate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Functionality of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmxCreate.c,v 1.2 2004/01/06 21:02:58 alanmi Exp $]

***********************************************************************/

#include "vmxInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a binary Vmx with the natural ordering of bits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreateBinary( Vm_Manager_t * pManVm, Vmx_Manager_t * pManVmx, int nVarsIn, int nVarsOut )
{
    Vm_VarMap_t * pVm;
    pVm = Vm_VarMapCreateBinary( pManVm, nVarsIn, nVarsOut );
    return Vmx_VarMapLookup( pManVmx, pVm, -1, NULL );
}

/**Function*************************************************************

  Synopsis    [Creates a simple Vmx with the natural ordering of bits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreateSimple( Vm_Manager_t * pManVm, Vmx_Manager_t * pManVmx, int nVarsIn, int nVarsOut, int * pValues )
{
    Vm_VarMap_t * pVm;
    pVm = Vm_VarMapLookup( pManVm, nVarsIn, nVarsOut, pValues );
    return Vmx_VarMapLookup( pManVmx, pVm, -1, NULL );
}


/**Function*************************************************************

  Synopsis    [Creates the expanded variable map for Ntk_NodeCollapse().]

  Description [The first arg (pVmxN) is the old extended variable map of the
  node. The second arg (pVmNew) is the new variable map of the collapsed node.
  The third arg (pTransNInv) for each var in pVmNew gives the corresponding entry
  in the old var map (pVmxN->pVm).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreateExpanded( Vmx_VarMap_t * pVmxN, Vm_VarMap_t * pVmNew, int * pTransNInv )
{
	Vmx_VarMap_t * pVmxNew;
	Vm_VarMap_t * pVm;
	int * pBitsOrder;
	int iBinVarAfter, iBinVarNew, nBitsThisVar;
	int nVars, i, v; 

	// get the old MV var map
	pVm = pVmxN->pVm;
	// allocate temporary storage for the bin var support
	pBitsOrder = Vm_VarMapGetStoragePermute( pVm );
	// create the new binary var support from the new MV supp 
	// go through all the variables of the new MV supp 
	iBinVarAfter = pVmxN->nBits;
	iBinVarNew   = 0;
    nVars = pVmNew->nVarsIn + pVmNew->nVarsOut;
	for ( i = 0; i < nVars; i++ )
		if ( pTransNInv[i] >= 0 ) 
		{ // this var belongs to N
			nBitsThisVar = pVmxN->pBits[ pTransNInv[i] ];
			for ( v = 0; v < nBitsThisVar; v++ )
				pBitsOrder[iBinVarNew++] = pVmxN->pBitsOrder[ pVmxN->pBitsFirst[ pTransNInv[i] ] + v ];
		}
		else
		{ // this var belongs to the new map but is not in N
			nBitsThisVar = Extra_Base2Log( pVmNew->pValues[i] );
			for ( v = 0; v < nBitsThisVar; v++ )
				pBitsOrder[iBinVarNew++] = iBinVarAfter++;
		}
//	assert( iBinVarNew == iBinVarAfter );
    // disabled this very useful assert to be able to work with automata
    // which are not mapped into the topmost variables of the BDD manager

	// find or create the new extended variable map
    pVmxNew = Vmx_VarMapLookup( pVmxN->pMan, pVmNew, iBinVarNew, pBitsOrder );
    return pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Creates the reduced variable map for Ntk_NodeMakeMinimumBase().]

  Description [Given is the original extended variable map (pVmx), from which
  some MV variables have been removed. The remaining variables are marked 
  with 1's in the MV support, given as the second argument.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreateReduced( Vmx_VarMap_t * pVmx, int * pSuppMv )
{
	Vmx_VarMap_t * pVmxNew;
	Vm_VarMap_t * pVm, * pVmNew;
	int * pBitsOrder, * pSuppBin;
	int i, v, iBinVar, nVars;

	// create the new MV map
	pVm = pVmx->pVm;
	pVmNew = Vm_VarMapCreateReduced( pVm, pSuppMv );
	// allocate temporary storage for the bin support
	pSuppBin   = Vm_VarMapGetStorageArray1( pVm );
	pBitsOrder = Vm_VarMapGetStorageArray2( pVm );
	// create the binary var support from the MV supp 
	// i-th entry in bin support is 1 if the corresponding MV var is present
    nVars = pVm->nVarsIn + pVm->nVarsOut;
	for ( i = 0; i < nVars; i++ )
		for ( v = 0; v < pVmx->pBits[i]; v++ )
		{
			iBinVar = pVmx->pBitsOrder[ pVmx->pBitsFirst[i] + v ];
			assert( iBinVar >= 0 && iBinVar < pVmx->nBits );
			pSuppBin[ iBinVar ] = pSuppMv[i];
		}
	// compress the binary support 
	iBinVar = 0;
	for ( i = 0; i < pVmx->nBits; i++ )
		if ( pSuppBin[i] )
			pSuppBin[i] = iBinVar++;
	// create the new bit order
	iBinVar = 0;
	for ( i = 0; i < nVars; i++ )
		if ( pSuppMv[i] )
		{ // this MV variable belong to the resulting pVmx
			for ( v = 0; v < pVmx->pBits[i]; v++ )
				pBitsOrder[ iBinVar++ ] = pSuppBin[ pVmx->pBitsOrder[ pVmx->pBitsFirst[i] + v ] ];
		}
	// find or create the new extended variable map
    pVmxNew = Vmx_VarMapLookup( pVmx->pMan, pVmNew, iBinVar, pBitsOrder );
    return pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Creates the permuted variable map for Ntk_NodeSort().]

  Description [The array pPermuteInv contains, for each fanin in the new order,
  its place in the old fanin order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreatePermuted( Vmx_VarMap_t * pVmx, int * pPermuteInv )
{
	Vmx_VarMap_t * pVmxNew;
	Vm_VarMap_t * pVm, * pVmNew;
	int * pBitsOrder;
	int i, v, iVarsBin, nVars; 

	// get the old MV var map
	pVm = pVmx->pVm;
	pVmNew = Vm_VarMapCreatePermuted( pVm, pPermuteInv );
	// allocate temporary storage for the bin var support
	pBitsOrder = Vm_VarMapGetStorageArray1( pVm );
	// create the permuted binary support
	iVarsBin = 0;
    nVars = pVm->nVarsIn + pVm->nVarsOut;
	// go through the new MV variables
	for ( i = 0; i < nVars; i++ )
		for ( v = 0; v < pVmx->pBits[pPermuteInv[i]]; v++ )
			pBitsOrder[ iVarsBin++ ] = pVmx->pBitsOrder[ pVmx->pBitsFirst[pPermuteInv[i]] + v ];
	assert( iVarsBin == pVmx->nBits );
	// find or create the new extended variable map
    pVmxNew = Vmx_VarMapLookup( pVmx->pMan, pVmNew, iVarsBin, pBitsOrder );
    return pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Creates the ordered variable map for the flexibility manager.]

  Description [This procedure derives the new extended variable map, 
  which contains the same VM map pVmx->pVm but with a different bits order. 
  Array pVarOrder gives the ordering of variables in pVm, which determines
  the ordering of their bits.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreateOrdered( Vmx_VarMap_t * pVmx, int * pVarOrder )
{
	Vmx_VarMap_t * pVmxNew;
	int * pBitsOrder;
	int b, v, iBit, nVars; 
    // get the number of MV vars
	nVars = pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut;
	// allocate temporary storage for the bin var support
	pBitsOrder = Vm_VarMapGetStorageArray1( pVmx->pVm );
    // put the bits into the specified order
    iBit = 0;
	for ( v = 0; v < nVars; v++ )
        for ( b = 0; b < pVmx->pBits[pVarOrder[v]]; b++ )
            pBitsOrder[iBit++] = pVmx->pBitsOrder[ pVmx->pBitsFirst[pVarOrder[v]] + b ];
	// the same MV var map will work for the ordered extended var map
	// find or create the new extended variable map
    pVmxNew = Vmx_VarMapLookup( pVmx->pMan, pVmx->pVm, pVmx->nBits, pBitsOrder );
    return pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Creates the reordered variable map for Ntk_NodeReorder().]

  Description [The relation (pVmx->bRel) has been reordered and the new,
  compressed variable order is given in the array pOrder. This procedure
  derives the new extended variable map, which contains the same VM map
  pVmx->pVm but with a different bits order. Each entry in pOrder tells
  what is the place of the given bin variable in the original bits order
  of the original extended variable map. Additionally, variable nBinVars
  gives the number of variables in the binary support.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreateReordered( Vmx_VarMap_t * pVmx, int * pOrder, int nBinVars )
{
	Vmx_VarMap_t * pVmxNew;
	Vm_VarMap_t * pVm;
	int * pBitsOrder, * pOrderInv;
	int i, nBitsAfter; 

	// get the old MV var map
	pVm = pVmx->pVm;
	// the same MV var map will work for the reordered extended var map
	// allocate temporary storage for the bin var support
	pOrderInv  = Vm_VarMapGetStorageArray1( pVm );
	pBitsOrder = Vm_VarMapGetStorageArray2( pVm );
	// create the inverse of the binary support
	for ( i = 0; i < pVmx->nBits; i++ )
		pOrderInv[i] = -1;
	for ( i = 0; i < nBinVars; i++ )
		pOrderInv[pOrder[i]] = i;
	// assign the unused variables
	nBitsAfter = nBinVars;
	for ( i = 0; i < pVmx->nBits; i++ )
		if ( pOrderInv[i] == -1 )
			pOrderInv[i] = nBitsAfter++;
	assert( nBitsAfter == pVmx->nBits );
	// set the new bin order
	for ( i = 0; i < pVmx->nBits; i++ )
		pBitsOrder[i] = pOrderInv[ pVmx->pBitsOrder[i] ];
	// find or create the new extended variable map
    pVmxNew = Vmx_VarMapLookup( pVmx->pMan, pVm, pVmx->nBits, pBitsOrder );
    return pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Creates the power variable map for Mvr_RelationCreatePower().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreatePower( Vmx_VarMap_t * pVmx, int Degree )
{
	Vmx_VarMap_t * pVmxNew;
	Vm_VarMap_t * pVm, * pVmNew;
	int * pBitsOrder;
	int i, v, d, iBinVar, nVars;

	// create the new MV map
	pVm    = pVmx->pVm;
	pVmNew = Vm_VarMapCreatePower( pVm, Degree );
	pBitsOrder = Vm_VarMapGetStoragePermute( pVmNew );
	// create the new binary variable order
	iBinVar = 0;
    nVars = pVm->nVarsIn + pVm->nVarsOut;
	for ( i = 0; i < nVars; i++ )
		for ( v = 0; v < pVmx->pBits[i]; v++ )
			for ( d = 0; d < Degree; d++ )
				pBitsOrder[iBinVar++] = Degree * pVmx->pBitsOrder[ pVmx->pBitsFirst[i] + v ] + d;
	// find or create the new extended variable map
    pVmxNew = Vmx_VarMapLookup( pVmx->pMan, pVmNew, pVmx->nBits * Degree, pBitsOrder );
    return pVmxNew;
}

/**Function*************************************************************

  Synopsis    [Appends the new variables in pVmNew to pVmx.]

  Description [Takes the available Vmx possibly with some nontrivial
  ordering of bits. Assumes that pVmNew contains the same MV vars
  that pVmx->pVm plus some additional var appended at the end. Creates
  a new Vmx, which used pVmNew with the bit order taken from pVmx
  plus some appended bits for the new vars at the end of pVmNew.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_VarMap_t * Vmx_VarMapCreateAppended( Vmx_VarMap_t * pVmx, Vm_VarMap_t * pVmNew )
{
	Vmx_VarMap_t * pVmxNew;
	int * pBitsOrder;
    int nMvVarsOld, nMvVarsNew;
	int nBinVarsOld, nBinVarsNew;
    int nLargestBit;
    int i;

    nMvVarsOld  = pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut;
    nMvVarsNew  = pVmNew->nVarsIn + pVmNew->nVarsOut;
    assert( nMvVarsOld < nMvVarsNew );

    nLargestBit = Vmx_VarMapGetLargestBit( pVmx );
    nBinVarsOld = pVmx->nBits;
    nBinVarsNew = nBinVarsOld;
    for ( i = nMvVarsOld; i < nMvVarsNew; i++ )
        nBinVarsNew += Extra_Base2Log( pVmNew->pValues[i] );

    // create the bits order
    pBitsOrder = ALLOC( int, nBinVarsNew );
    memcpy( pBitsOrder, pVmx->pBitsOrder, sizeof(int) * nBinVarsOld );
    for ( i = nBinVarsOld; i < nBinVarsNew; i++ )
        pBitsOrder[i] = nLargestBit - nBinVarsOld + i;

    pVmxNew = Vmx_VarMapLookup( pVmx->pMan, pVmNew, nBinVarsNew, pBitsOrder );
    FREE( pBitsOrder );

    return pVmxNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


