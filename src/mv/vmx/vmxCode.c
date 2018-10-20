/**CFile****************************************************************

  FileName    [vmx.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The extended variable map (VMX) package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmxCode.c,v 1.12 2003/11/18 18:55:06 alanmi Exp $]

***********************************************************************/

#include "vmxInt.h"

/*
    Encoding and decoding procedures in this file assume the same kind
    of minimum-length (logarithmic) encoding with well-ballanced distribution
    of code minterms. For example, if 5-values are encoded using 2 bits,
    the following codes are used:
    value0 = 00-
    value1 = 01-
    value2 = 10-
    value3 = 110
    value4 = 111
             | |------------- least significant bit
             |--------------- most significant bit
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Vmx_VarMapEncodeMinterm( DdManager * dd, 
    Vmx_VarMap_t * pVmx, int iVar, DdNode ** pbCodes );
static void Vmx_VarMapEncode( DdManager * dd, DdNode * pbVars[], 
    Vmx_VarMap_t * pVmx, int iVar, DdNode ** pbCodes, bool fSimple );
static void Vmx_VarMapGetVars_rec( DdManager * dd, DdNode * bCube, 
    int pVars[], int pPols[], int * pnVars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the codes of the MV variables in pVmx->pVm.]

  Description [Uses the encoding scheme, in which all the code
  minterms are distributed among the values.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Vmx_VarMapEncodeMap( DdManager * dd, Vmx_VarMap_t * pVmx )
{
    DdNode ** pbCodes;
    int * pValuesFirst;
    int nVars, v, i;

    // extend the local manager if necessary
    if ( dd->size < pVmx->nBits )
    {
        for ( i = dd->size; i < pVmx->nBits + 10; i++ )
            Cudd_bddIthVar( dd, i );
        Cudd_zddVarsFromBddVars( dd, 2 );
    }

    // get the parameters
    pbCodes      = pVmx->pMan->pbCodes;
    pValuesFirst = pVmx->pVm->pValuesFirst;
    nVars        = pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut;

    // derive the encoding
    for ( v = 0; v < nVars; v++ )
        Vmx_VarMapEncode( dd, dd->vars, pVmx, v, pbCodes + pValuesFirst[v], 0 );
/*    
    // print out the variable value codes
	for ( i = 0; i < pVm->nVarsIn + pVm->nVarsOut; i++ )
	{
		printf( "VARIABLE #%d:\n", i );
		for ( v = 0; v < pVmx->pVm->pValues[i]; v++ )
		{
			printf( "Value %d:  ", v );
			PRB( dd, pbCodes[ pVmx->pVm->pValuesFirst[i] + v ] );
		}
	}
*/
    return pbCodes;
}

/**Function*************************************************************

  Synopsis    [Creates the codes of the MV variables in pVmx->pVm.]

  Description [Uses the encoding scheme with one code minterm per value.
  This leaves some unused code minterms.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Vmx_VarMapEncodeMapMinterm( DdManager * dd, Vmx_VarMap_t * pVmx )
{
    DdNode ** pbCodes;
    int * pValuesFirst;
    int nVars, v, i;

    // extend the local manager if necessary
    if ( dd->size < pVmx->nBits )
    {
        for ( i = dd->size; i < pVmx->nBits + 10; i++ )
            Cudd_bddIthVar( dd, i );
        Cudd_zddVarsFromBddVars( dd, 2 );
    }

    // get the parameters
    pbCodes      = pVmx->pMan->pbCodes;
    pValuesFirst = pVmx->pVm->pValuesFirst;
    nVars        = pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut;

    // derive the encoding
    for ( v = 0; v < nVars; v++ )
        Vmx_VarMapEncodeMinterm( dd, pVmx, v, pbCodes + pValuesFirst[v] );
    return pbCodes;
}

/**Function*************************************************************

  Synopsis    [Creates the encoding of one MV variable in pVmx->pVm.]

  Description [Uses the encoding scheme with one code minterm per value.
  This leaves some unused code minterms.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Vmx_VarMapEncodeVarMinterm( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar )
{
    DdNode ** pbCodes;
    int * pValuesFirst;
    pbCodes      = pVmx->pMan->pbCodes;
    pValuesFirst = pVmx->pVm->pValuesFirst;
    Vmx_VarMapEncodeMinterm( dd, pVmx, iVar, pbCodes + pValuesFirst[iVar] );
    return pbCodes + pValuesFirst[iVar];
}

/**Function*************************************************************

  Synopsis    [Creates the encoding of one MV variable in pVmx->pVm.]

  Description [Uses the encoding scheme with one code minterm per value.
  This leaves some unused code minterms.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapEncodeMinterm( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar, DdNode ** pbCodes )
{
    DdNode ** pbBinVars;
    int nValues, nBits, v;

    nValues   = pVmx->pVm->pValues[iVar];
    nBits     = pVmx->pBits[iVar];
    pbBinVars = Vmx_VarMapBinVarsVar( dd, pVmx, iVar );
    for ( v = 0; v < nValues; v++ )
    {
        pbCodes[v] = Extra_bddBitsToCube2( dd, v, nBits, pbBinVars );  
        Cudd_Ref( pbCodes[v] );
    }
    FREE( pbBinVars );
}

/**Function*************************************************************

  Synopsis    [Create the encoding of an VM using bits of VMX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Vmx_VarMapEncodeMapUsingVars( DdManager * dd, DdNode * pbVars[], Vmx_VarMap_t * pVmx )
{
    DdNode ** pbCodes;
    int * pValuesFirst;
    int nVars, v;

    // the manager should be extended
    assert( dd->size >= pVmx->nBits );

    // get the parameters
    pbCodes      = pVmx->pMan->pbCodes;
    pValuesFirst = pVmx->pVm->pValuesFirst;
    nVars        = pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut;

    // derive the encoding
    for ( v = 0; v < nVars; v++ )
        Vmx_VarMapEncode( dd, pbVars, pVmx, v, pbCodes + pValuesFirst[v], 0 );
    return pbCodes;
}

/**Function*************************************************************

  Synopsis    [Derives encoding for one MV variable.]

  Description [Derives the encoding cubes to represent the MV variable 
  using binary variables. The BDD manager is used to build the cubes.
  The VMX (pVmx) contains information about the binary variables
  which are used to encode the MV variables of the VM (pVm).
  Uses two adjacent minterms to encode one value whenever possible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Vmx_VarMapEncodeVar( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar )
{
    DdNode ** pbCodes;
    int * pValuesFirst;
    // derive the encoding
    pbCodes      = pVmx->pMan->pbCodes;
    pValuesFirst = pVmx->pVm->pValuesFirst;
    Vmx_VarMapEncode( dd, dd->vars, pVmx, iVar, pbCodes + pValuesFirst[iVar], 0 );
    return pbCodes + pValuesFirst[iVar];
}

/**Function*************************************************************

  Synopsis    [Derives encoding for one MV variable.]

  Description [Derives the encoding cubes to represent the MV variable 
  using binary variables. The BDD manager is used to build the cubes.
  The VMX (pVmx) contains information about the binary variables
  which are used to encode the MV variables of the VM (pVm).
  Uses two adjacent minterms to encode one value whenever possible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Vmx_VarMapEncodeVarUsingVars( DdManager * dd, DdNode * pbVars[], Vmx_VarMap_t * pVmx, int iVar )
{
    DdNode ** pbCodes;
    int * pValuesFirst;
    // derive the encoding
    pbCodes      = pVmx->pMan->pbCodes;
    pValuesFirst = pVmx->pVm->pValuesFirst;
    Vmx_VarMapEncode( dd, pbVars, pVmx, iVar, pbCodes + pValuesFirst[iVar], 0 );
    return pbCodes + pValuesFirst[iVar];
}

/**Function*************************************************************

  Synopsis    [Derives encoding for one MV variable.]

  Description [Derives the encoding cubes to represent the MV variable 
  using binary variables. The BDD manager is used to build the cubes.
  The VMX (pVmx) contains information about the binary variables
  which are used to encode the MV variables of the VM (pVm).
  Uses two adjacent minterms to encode one value whenever possible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapEncode( DdManager * dd, DdNode * pbVars[], Vmx_VarMap_t * pVmx, int iVar, DdNode ** pbCodes, bool fSimple )
{
    Vmx_Manager_t * pMan;
    DdNode * bVar, * bCode, * bTemp;
    int nValues, nBits, nBitsFirst;
	int nValCube, nMinterms, iMint;
    int b, i;

    // general parameters
    pMan       = pVmx->pMan;
    // encoding parameters
    nValues    = pVmx->pVm->pValues[iVar]; // the number of values to encode
    nBits      = pVmx->pBits[iVar];        // the number of bits to use
	nBitsFirst = pVmx->pBitsFirst[iVar];   // the first bit
    nMinterms  = (1 << nBits);             // the number of minterms for codes
    nValCube   = nMinterms - nValues;      // the number of spare minterms (also, the number of values to be endoded with two minterms)

    // encode the values
    iMint = 0;
    for ( i = 0; i < nValues; i++ )
    {
        bCode = b1;   Cudd_Ref( bCode );
        // do not include the first bit into the set
        // for the codes of values encoded with two minterms
        for ( b = ((fSimple || i >= nValCube)? 0 : 1); b < nBits; b++ )
        {
            bVar = pbVars[ pVmx->pBitsOrder[nBitsFirst + b] ];
            if ( (iMint & (1<<b)) == 0 )
                bVar = Cudd_Not( bVar );
            bCode = Cudd_bddAnd( dd, bTemp = bCode, bVar );  Cudd_Ref( bCode );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        // assign the code
        assert( pbCodes[i] == NULL );
        pbCodes[i] = bCode;
        // for the first codes, scroll through two minterms
//        if ( fSimple || i >= nValCube )
        if ( i >= nValCube )
            iMint += 1;
        else
            iMint += 2;
    }
}

/**Function*************************************************************

  Synopsis    [Derefs the cubes associated with the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapEncodeDeref( DdManager * dd, Vmx_VarMap_t * pVmx, DdNode ** pbCodes )
{
    int i;
    // iterate through the referenced values
    for ( i = 0; pbCodes[i]; i++ )
    {
        Cudd_RecursiveDeref( dd, pbCodes[i] );
        pbCodes[i] = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Decodes the log-encoded binary cube into the positional MV cube.]

  Description [This procedure decodes the MV cube var values found in 
  pCubeBin and writes the input MV variable value sets into pCubeMv. 
  The decoding provided by this function is compatible with the encoding 
  performed by Vmx_VarMaEncode().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapDecode( Vmx_VarMap_t * pVmx, int * pCubeBin, int * pCubeMv )
{
    Vm_VarMap_t * pVm = pVmx->pVm;
    unsigned MaskSign;   // signature of the bit-encoded value set
    unsigned MaskPolar;  // polarity of the bit-encoded value set
    unsigned Minterm1, Minterm2, Bit;
    int ValueSplit, ValueMax, Value;
    int i, b, v, nVars;

    nVars = pVmx->pVm->nVarsIn + pVmx->pVm->nVarsOut;
    for ( i = 0; i < nVars; i++ )
    {
        // get the mask
        MaskSign  = 0;
        MaskPolar = 0;
        for ( b = 0; b < pVmx->pBits[i]; b++ )
        {
            Value = pCubeBin[ pVmx->pBitsOrder[pVmx->pBitsFirst[i] + b] ];
//          Bit   = (1 << (pVmx->pBits[i]-1-b));
            Bit   = (1 << (b));

            if ( Value == 0 )
            {
                MaskSign |= Bit;
            }
            else if ( Value == 1 )
            {
                MaskSign  |= Bit;
                MaskPolar |= Bit;
            }
            else if ( Value != 2 )
            {
                assert( 0 );
            }
        }

        ValueSplit = ( 1 << pVmx->pBits[i] ) - pVm->pValues[i];

        // check what values overlap with the mask
        // these are those values that are encoded with two minterms each
        for ( v = 0; v < ValueSplit; v++ )
        {
            Minterm1 = (v<<1);
            Minterm2 = (v<<1) | 1;

            if ( ((MaskPolar ^ Minterm1) & MaskSign) == 0 || ((MaskPolar ^ Minterm2) & MaskSign) == 0 )
                pCubeMv[ pVm->pValuesFirst[i] + v ] = 1;
            else
                pCubeMv[ pVm->pValuesFirst[i] + v ] = 0;
        }

        // the remaining values are encoded with one minterm each
        ValueMax = (1 << pVmx->pBits[i]);
        for ( v = (v<<1); v < ValueMax; v++ )
            if ( ((MaskPolar ^ v) & MaskSign) == 0 )
                pCubeMv[ pVm->pValuesFirst[i] + v - ValueSplit ] = 1;
            else
                pCubeMv[ pVm->pValuesFirst[i] + v - ValueSplit ] = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Get the number of one code that is contained in the cube.]

  Description [Assumes that bCube is the set of codes w.r.t. the given
  MV var (iVar) of the variable map pVmx. Returns the number of one code 
  in the set. If the set contains several codes, only the code with the 
  smallest number is returned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vmx_VarMapDecodeCubeVar( DdManager * dd, DdNode * bCube, Vmx_VarMap_t * pVmx, int iVar )
{
    int nValues, nBits, nUnused;
    int * pBddVars, * pBddPols;
    int nBddVars, i;
    char * pBitsOrder;
    unsigned Code;

    assert( bCube != b0 );
    nBits   = pVmx->pBits[iVar];

    // get storage for variables and polarities
    pBddVars = Vm_VarMapGetStorageSupport1( pVmx->pVm );
    pBddPols = Vm_VarMapGetStorageSupport2( pVmx->pVm );

    // create the list of variables present in the cube
    nBddVars = 0;
    Vmx_VarMapGetVars_rec( dd, bCube, pBddVars, pBddPols, &nBddVars );
    assert( nBddVars <= nBits );

    // mark the bits that correspond to this particular variable
    pBitsOrder = (char *)Vm_VarMapGetStoragePermute( pVmx->pVm );
    memset( pBitsOrder, 255, sizeof(char) * dd->size );
    for ( i = 0; i < nBits; i++ )
        pBitsOrder[ pVmx->pBitsOrder[pVmx->pBitsFirst[iVar] + i] ] = i;

    // build the code and make sure the vars are in the range
    // assume that the vars that are not present have the zero polarity
    Code = 0;
    for ( i = 0; i < nBddVars; i++ )
    {
        assert( pBitsOrder[pBddVars[i]] != 255 );
        if ( pBddPols[i] ) 
            Code |= (1 << pBitsOrder[pBddVars[i]]);
    }

    // figure out, what code is represented by this minterm
    nValues = pVmx->pVm->pValues[iVar];
    nUnused = (1 << nBits) - nValues;
    if ( Code < (unsigned)2 * nUnused )
        return Code/2;
    return Code - nUnused;
}

/**Function*************************************************************

  Synopsis    [Decodes a cube that depends on a number of variables.]

  Description [Assumes that bCube is the product of codes of the given
  set of MV variables (pVars, nVars) with the given variable map pVmx. 
  Returns the array of codes of these variables (pCodes). If some vars
  can be represented by several codes, only the code with the 
  smallest number is returned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapDecodeCube( DdManager * dd, DdNode * bCube, Vmx_VarMap_t * pVmx, int * pVars, int * pCodes, int nVars )
{
    int nUnused, nBitsTotal;
    int * pBddVars, * pBddPols;
    int nBddVars, i, b, iBddVar;
    char * pBitsOrder;
    int * pVarsOrder;

    assert( bCube != b0 );
    // count the total number of bits present in all the variables
    nBitsTotal = 0;
    for ( i = 0; i < nVars; i++ )
        nBitsTotal += pVmx->pBits[ pVars[i] ];

    // get storage for variables and polarities
    pBddVars = Vm_VarMapGetStorageSupport1( pVmx->pVm );
    pBddPols = Vm_VarMapGetStorageSupport2( pVmx->pVm );

    // create the list of variables present in the cube
    nBddVars = 0;
    Vmx_VarMapGetVars_rec( dd, bCube, pBddVars, pBddPols, &nBddVars );
    assert( nBddVars <= nBitsTotal );

    // mark the bits that correspond to this particular variable
    pBitsOrder = (char *)Vm_VarMapGetStoragePermute( pVmx->pVm );
    pVarsOrder = (int *)Vm_VarMapGetStorageArray1( pVmx->pVm );
    memset( pBitsOrder, 255, sizeof(char) * dd->size );
    for ( i = 0; i < nVars; i++ )
        for ( b = 0; b < pVmx->pBits[ pVars[i] ]; b++ )
        {
            iBddVar = pVmx->pBitsOrder[pVmx->pBitsFirst[ pVars[i] ] + b];
            pVarsOrder[iBddVar] = i;
            pBitsOrder[iBddVar] = b;
        }

    // build the code and make sure the vars are in the range
    // assume that the vars that are not present have the zero polarity
    for ( i = 0; i < nVars; i++ )
        pCodes[i] = 0;
    for ( i = 0; i < nBddVars; i++ )
    {
        assert( pBitsOrder[pBddVars[i]] != 255 );
        if ( pBddPols[i] ) 
            pCodes[pVarsOrder[pBddVars[i]]] |= (1 << pBitsOrder[pBddVars[i]]);
    }

    // figure out, what code is represented by this minterm
    for ( i = 0; i < nVars; i++ )
    {
        nUnused = (1 << pVmx->pBits[ pVars[i] ]) - pVmx->pVm->pValues[ pVars[i] ];
        if ( pCodes[i] < 2 * nUnused )
            pCodes[i] /= 2;
        else
            pCodes[i] -= nUnused;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_VarMapGetVars_rec( DdManager * dd, DdNode * bCube, 
    int pVars[], int pPols[], int * pnVars )
{
    DdNode * bC0, * bC1, * bCubeR;

    assert( *pnVars < 32 );
    assert( bCube != b0 );
    if ( bCube == b1 )
        return;

	// cofactor the cube
    bCubeR = Cudd_Regular( bCube );
	if ( bCubeR != bCube ) // bFunc is complemented 
	{
		bC0 = Cudd_Not( cuddE(bCubeR) );
		bC1 = Cudd_Not( cuddT(bCubeR) );
	}
	else
	{
		bC0 = cuddE(bCubeR);
		bC1 = cuddT(bCubeR);
	}

    pVars[*pnVars] = bCubeR->index;
    if ( bC0 != b0 )
    {
        pPols[*pnVars] = 0;
        (*pnVars)++;
        Vmx_VarMapGetVars_rec( dd, bC0, pVars, pPols, pnVars );
    }
    else
    {
        pPols[*pnVars] = 1;
        (*pnVars)++;
        Vmx_VarMapGetVars_rec( dd, bC1, pVars, pPols, pnVars );
    }
}

/**Function*************************************************************

  Synopsis    [Returns the domain of one minterm per value for the given var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Vmx_VarMapCharDomain( DdManager * dd, Vmx_VarMap_t * pVmx, int iVar )
{
    DdNode * bDomain, * bTemp;
    DdNode * pbCodes[256];
    int nValues, v;

    nValues = pVmx->pVm->pValues[iVar];
    if ( nValues == 2 || nValues == 4 || nValues == 8 || nValues == 16 || nValues == 32 )
        return b1;

    // get the simple codes
    memset( pbCodes, 0, sizeof(DdNode *) * nValues );
    Vmx_VarMapEncode( dd, dd->vars, pVmx, iVar, pbCodes, 1 );
    // create the sum total of simple codes
    bDomain = b0;  Cudd_Ref( bDomain );
    for ( v = 0; v < nValues; v++ )
    {
        bDomain = Cudd_bddOr( dd, bTemp = bDomain, pbCodes[v] );  Cudd_Ref( bDomain );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, pbCodes[v] );
    }
    Cudd_Deref( bDomain );
    return bDomain;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


