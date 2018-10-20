/**CFile****************************************************************

  FileName    [vmUtils.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various utilities to create variable maps.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmUtils.c,v 1.11 2003/09/01 04:56:58 alanmi Exp $]

***********************************************************************/

#include "vmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the var map that is different in one variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateBinary( Vm_Manager_t * p, int nVarsIn, int nVarsOut )
{
    Vm_VarMap_t * pVmNew;
    int * pValues, nVarsIO, i;
    nVarsIO = nVarsIn + nVarsOut;
    pValues = ALLOC( int, nVarsIO );
    for ( i = 0; i < nVarsIO; i++ )
        pValues[i] = 2;
    pVmNew = Vm_VarMapLookup( p, nVarsIn, nVarsOut, pValues );
    FREE( pValues );
    return pVmNew;
}

/**Function*************************************************************

  Synopsis    [Creates the var map that is different in one variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateOneDiff( Vm_VarMap_t * pVm, int iVar, int nVarValuesNew )
{
    Vm_VarMap_t * pVmNew;
    int nVarValuesOld;

    nVarValuesOld = pVm->pValues[iVar];
    pVm->pValues[iVar] = nVarValuesNew;
    pVmNew = Vm_VarMapLookup( pVm->pMan, pVm->nVarsIn, pVm->nVarsOut, pVm->pValues );
    pVm->pValues[iVar] = nVarValuesOld;
    
    return pVmNew;
}

/**Function*************************************************************

  Synopsis    [Creates the var map with additional output variables.]

  Description [All the original variable are considered input variables
  of the new map.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateOnePlus( Vm_VarMap_t * pVm, int nVarValuesNew )
{
    Vm_VarMap_t * pVmNew;
    int * pValues;
    int nVarsIO;

    nVarsIO = pVm->nVarsIn + pVm->nVarsOut;
    pValues = ALLOC( int, nVarsIO + 1 );
    memcpy( pValues, pVm->pValues, sizeof(int) * nVarsIO );
    pValues[nVarsIO] = nVarValuesNew;
    pVmNew = Vm_VarMapLookup( pVm->pMan, nVarsIO, 1, pValues );
    FREE( pValues );
    return pVmNew;
}

/**Function*************************************************************

  Synopsis    [Creates the var map without the output var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateOneMinus( Vm_VarMap_t * pVm )
{
    return Vm_VarMapLookup( pVm->pMan, pVm->nVarsIn, 0, pVm->pValues );
}

/**Function*************************************************************

  Synopsis    [Creates the expanded variable map.]

  Description [Expands the given variable map (pVmBase) by appending 
  some variables from pVmExt. The array pVarsUsed contains 1 for those
  variables in pVmExt that should be appended to all the variable in
  pVmBase, and 0 for all other variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateExpanded( Vm_VarMap_t * pVmBase, Vm_VarMap_t * pVmExt, int * pVarsUsed )
{
	Vm_VarMap_t * pVmRes;
	int * pVarValues;
	int nVars, i;

	// allocate the array of values
	pVarValues = Vm_VarMapGetStorageArray1( pVmBase );
	// copy the input var values from pVmBase
	nVars = 0;
	for ( i = 0; i < pVmBase->nVarsIn; i++ )
		pVarValues[nVars++] = pVmBase->pValues[i];
	// copy other variables from pVmExt
	for ( i = 0; i < pVmExt->nVarsIn; i++ )
		if ( pVarsUsed[i] )
			pVarValues[nVars++] = pVmExt->pValues[i];
	// copy the output variable of pVmBase
	assert( pVmBase->nVarsOut == 1 );
	pVarValues[nVars++] = pVmBase->pValues[ pVmBase->nVarsIn ];

	// create the new var map
	pVmRes = Vm_VarMapLookup( pVmBase->pMan, nVars - 1, 1, pVarValues );
	return pVmRes;
}


/**Function*************************************************************

  Synopsis    [Creates the reduced variable map.]

  Description [Reduces the given variable map by preserving only the
  variables that have 1 in pVarsUsed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateReduced( Vm_VarMap_t * pVm, int * pVarsUsed )
{
	Vm_VarMap_t * pVmRes;
	int * pVarValues;
	int nVars, nVarsVm, i;

	// allocate the array of values
	pVarValues = Vm_VarMapGetStorageArray1( pVm );
	// copy the input var values from pVm
	nVars = 0;
    nVarsVm = pVm->nVarsIn + pVm->nVarsOut;
	for ( i = 0; i < nVarsVm; i++ )
		if ( pVarsUsed[i] )
			pVarValues[nVars++] = pVm->pValues[i];

	// create the new var map
	pVmRes = Vm_VarMapLookup( pVm->pMan, nVars - 1, 1, pVarValues );
	return pVmRes;
}



/**Function*************************************************************

  Synopsis    [Permutes the given variable map.]

  Description [Creates the permuted variable map. The permutation is given
  in the array pPermuteInv. This array, for each new variable in the permuted
  map, gives the index of this variable in the original variable order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreatePermuted( Vm_VarMap_t * pVm, int * pPermuteInv )
{
	Vm_VarMap_t * pVmRes;
	int * pVarValues;
	int nVarsVm, i, k;

	// allocate the array of values
//	pVarValues = Vm_VarMapGetStorageArray1( pVm );
	// make sure that the permutation is valid
    nVarsVm = pVm->nVarsIn + pVm->nVarsOut;
    pVarValues = ALLOC( int, nVarsVm );
    for ( i = 0; i < nVarsVm; i++ )
    {
        assert( pPermuteInv[i] >= 0 && pPermuteInv[i] < nVarsVm );
        for ( k = i + 1; k < nVarsVm; k++ )
            assert( pPermuteInv[i] != pPermuteInv[k] );
    }

	// set the values
    for ( i = 0; i < nVarsVm; i++ )
		pVarValues[i] = pVm->pValues[ pPermuteInv[i] ];

    pVmRes = Vm_VarMapLookup( pVm->pMan, pVm->nVarsIn, pVm->nVarsOut, pVarValues );
    FREE( pVarValues );
    return pVmRes;
}

/**Function*************************************************************

  Synopsis    [Computes the power of the var map.]

  Description [In the power var map, the number of variables is the same,
  but the number of values, is the power (of the given degree) of the original
  number of values.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreatePower( Vm_VarMap_t * pVm, int Degree )
{
    Vm_VarMap_t * pVmRes;
    int * pVarValues;
    int i, k, nVarsVm;

	pVarValues = Vm_VarMapGetStorageArray1( pVm );
    nVarsVm = pVm->nVarsIn + pVm->nVarsOut;
    for ( i = 0; i < nVarsVm; i++ )
	{
		pVarValues[i] = 1;
		for ( k = 0; k < Degree; k++ )
			pVarValues[i] *= pVm->pValues[i];
	}
    pVmRes = Vm_VarMapLookup( pVm->pMan, pVm->nVarsIn, pVm->nVarsOut, pVarValues );
    return pVmRes;
}

/**Function*************************************************************

  Synopsis    [Creates the variable map with the additional output variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateExtended( Vm_VarMap_t * pVm, int nValues )
{
    Vm_VarMap_t * pVmNew;
    int * pValues, nVars;

    nVars = pVm->nVarsIn + pVm->nVarsOut;
    pValues = ALLOC( int, nVars + 1 );
    memcpy( pValues, pVm->pValues, sizeof(int) * nVars );
    pValues[nVars] = nValues;

    // derive the new extended variable map
    pVmNew = Vm_VarMapLookup( pVm->pMan, nVars, 1, pValues );
    FREE( pValues );
    return pVmNew;
}

/**Function*************************************************************

  Synopsis    [Creates the variable map with the doubled input/output space.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateAdded( Vm_VarMap_t * pVm1, Vm_VarMap_t * pVm2 )
{
    Vm_VarMap_t * pRes;
    int * pValues;
    int i, iVar;

    pValues = ALLOC( int, pVm1->nVarsIn + pVm2->nVarsIn + pVm1->nVarsOut + pVm2->nVarsOut );

    iVar = 0;
    for ( i = 0; i < pVm1->nVarsIn; i++ )
        pValues[iVar++] = pVm1->pValues[i];
    for ( i = 0; i < pVm2->nVarsIn; i++ )
        pValues[iVar++] = pVm2->pValues[i];
    for ( i = 0; i < pVm1->nVarsOut; i++ )
        pValues[iVar++] = pVm1->pValues[pVm1->nVarsIn+i];
    for ( i = 0; i < pVm2->nVarsOut; i++ )
        pValues[iVar++] = pVm2->pValues[pVm2->nVarsIn+i];

    pRes = Vm_VarMapLookup( pVm1->pMan, pVm1->nVarsIn + pVm2->nVarsIn, 
        pVm1->nVarsOut + pVm2->nVarsOut, pValues );
    FREE( pValues );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Creates VM with the input/output spaces given by the two VMs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateInputOutput( Vm_VarMap_t * pVmIn, Vm_VarMap_t * pVmOut )
{
    Vm_VarMap_t * pRes;
    int * pValues;
    int i, iVar;

    assert( pVmIn->nVarsOut == 0 );
    assert( pVmOut->nVarsOut == 0 );

    pValues = ALLOC( int, pVmIn->nVarsIn + pVmOut->nVarsIn );

    iVar = 0;
    for ( i = 0; i < pVmIn->nVarsIn; i++ )
        pValues[iVar++] = pVmIn->pValues[i];
    for ( i = 0; i < pVmOut->nVarsIn; i++ )
        pValues[iVar++] = pVmOut->pValues[i];

    pRes = Vm_VarMapLookup( pVmIn->pMan, pVmIn->nVarsIn, pVmOut->nVarsIn, pValues );
    FREE( pValues );
    return pRes;
}


/**Function*************************************************************

  Synopsis    [Creates VM with the input/output spaces given by the two VM.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Vm_VarMapCreateInputOutputSet( Vm_VarMap_t * pVmIn, Vm_VarMap_t * pVmOut, bool fUseSetIns, bool fUseSetOuts )
{
    Vm_VarMap_t * pRes;
    int * pValues;
    int nVarsInNew, nVarsOutNew;
    int i, iVar;

    assert( pVmIn->nVarsOut == 0 );
    assert( pVmOut->nVarsOut == 0 );

    pValues = ALLOC( int, pVmIn->nValuesIn + pVmOut->nValuesIn );

    iVar = 0;
    if ( !fUseSetIns )
    {
        for ( i = 0; i < pVmIn->nVarsIn; i++ )
            pValues[iVar++] = pVmIn->pValues[i];
        nVarsInNew = pVmIn->nVarsIn;
    }
    else
    {
        for ( i = 0; i < pVmIn->nValuesIn; i++ )
            pValues[iVar++] = 2;
        nVarsInNew = pVmIn->nValuesIn;
    }

    if ( !fUseSetOuts )
    {
        for ( i = 0; i < pVmOut->nVarsIn; i++ )
            pValues[iVar++] = pVmOut->pValues[i];
        nVarsOutNew = pVmOut->nVarsIn;
    }
    else
    {
        for ( i = 0; i < pVmOut->nValuesIn; i++ )
            pValues[iVar++] = 2;
        nVarsOutNew = pVmOut->nValuesIn;
    }
    assert( iVar == nVarsInNew + nVarsOutNew );

    pRes = Vm_VarMapLookup( pVmIn->pMan, nVarsInNew, nVarsOutNew, pValues );
    FREE( pValues );
    return pRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


