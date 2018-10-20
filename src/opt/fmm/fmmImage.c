/**CFile****************************************************************

  FileName    [fmmImage.c]

  PackageName [Multi-valued flexibility manager.]

  Synopsis    [The MV image computation by output splitting.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmmImage.c,v 1.2 2004/05/12 04:30:14 alanmi Exp $]

***********************************************************************/

#include "extra.h"
#include "reo.h"
#include "array.h"
#include <time.h>

#include "vm.h"
#include "vmx.h"
#include "mvr.h"


////////////////////////////////////////////////////////////////////////
///                       DATA STRUCTURES                            ///
////////////////////////////////////////////////////////////////////////

// information about one partition of MV functions
typedef struct fmmImagePartInfoStruct    Fmm_ImagePartInfo_t;
struct fmmImagePartInfoStruct
{
    int       nValues;   // the number of values of this MV var
    int       BddSize;   // the number of nodes in the shared BDD of i-sets
	DdNode *  bSupp;     // the support of this partition (one supp var per MV var)
	DdNode ** pbFuncs;   // the partition functions of this MV var (nValues)
	DdNode ** pbCodes;   // the MV cubes for the image of this MV var (nValues)
};


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static DdNode *   fmmImageMvCompute( DdManager * dd, DdNode * bCare, DdNode ** pbFuncs, int nFuncs, 
                    Vmx_VarMap_t * pVmxGlo, Vmx_VarMap_t * pVmxLoc, DdNode ** pbVars, int nBddSize );
static DdNode *   fmmImageMv_rec( DdManager * dd, array_t * pParts, DdNode * bCubeIn, Vmx_VarMap_t * pVmxGlo, int nBddSize );
static DdNode *   fmmImageMvValue_rec( DdManager * dd, array_t * pParts, int iPart, DdNode * bCubeIn, Vmx_VarMap_t * pVmxGlo, int nBddSize );
static DdNode *   fmmImageMvSmallSize( DdManager * dd, array_t * pParts, DdNode * bCubeIn, int nBddSize );
static array_t *  fmmImageMvDisjointSupport( DdManager * dd, array_t * pParts );

static Fmm_ImagePartInfo_t * fmmImageMvPartAllocate( DdNode * bSupp, DdNode ** pbFuncs, DdNode ** pbCodes, int nValues );
static Fmm_ImagePartInfo_t * fmmImageMvPartCreate( DdManager * dd, DdNode * bCare, DdNode * pbFuncs[], DdNode * pbCodes[], int nValues, Vmx_VarMap_t * pVmxGlo, DdNode ** pbImagePart, bool fUseAnd );

static void       fmmImageMvPartFree( DdManager * dd, Fmm_ImagePartInfo_t * p );
static int        fmmImageMvPartCompareBddSize( Fmm_ImagePartInfo_t ** ppPart1, Fmm_ImagePartInfo_t ** ppPart2 );
static void       fmmImageMvPartDeref( DdManager * dd, DdNode ** pbFuncs, int nFuncs );
static void       fmmImageMvPartConstrain( DdManager * dd, DdNode * bCare, DdNode ** pbFuncs, int nValues, DdNode * pbConstrs[], bool fUseAnd );
static DdNode *   fmmImageMvPartSupport( DdManager * dd, DdNode ** pbFuncs, int nFuncs, Vmx_VarMap_t * pVmxGlo );
static DdNode *   fmmImageMvPartConvolve( DdManager * dd, DdNode ** pbFuncs, DdNode ** pbCodes, int nFuncs );
static DdNode *   fmmImageMvPartImage( DdManager * dd, DdNode * bCubeIn, Fmm_ImagePartInfo_t * p );
static void       fmmImageMvPartArrayFree( DdManager * dd, array_t * pParts );


// these are needed to implement timeout
static int s_Timeout   = 0;
static int s_TimeLimit = 0; 

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Computes the MV image of a set of function.]

  Description [Computes the image of the care set (dd,bCare) with the set 
  of global functions (pbFuncs,nFuncs). The variable maps (pVmxGlo,pVmxLoc)
  characterize the global/local spaces. Stops iterative splitting when 
  the shared BDD size of the global functions is less than nBddSize. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmm_ImageMvCompute( DdManager * dd, DdNode * bCare, DdNode ** pbFuncs, int nFuncs, 
    Vmx_VarMap_t * pVmxGlo, Vmx_VarMap_t * pVmxLoc, DdNode ** pbVars, int nBddSize )
{
    Vm_VarMap_t * pVm;
    DdNode ** pbCodesExt, * pbCodesLocal[32];
    DdNode * pbCofs[32], * pbImages[32];
    DdNode * bCubeOut, * bImage;
    int nVarsIn, nValuesOut, i;

    // set the timeout
	s_TimeLimit = (int)(s_Timeout /* in miliseconds */ * (float)(CLOCKS_PER_SEC) / 1000 ) + clock();

    pVm = Vmx_VarMapReadVm( pVmxGlo );
    nVarsIn = Vm_VarMapReadVarsInNum(pVm);
    nValuesOut = Vm_VarMapReadValuesOutNum(pVm);
    assert( Vm_VarMapReadVarsOutNum(pVm) == 1 );

    // create the local copy of the codes w.r.t. to the parameter (output) var
    pbCodesExt = Vmx_VarMapEncodeVar( dd, pVmxGlo, nVarsIn );
    for ( i = 0; i < nValuesOut; i++ )
    {
        pbCodesLocal[i] = pbCodesExt[i]; Cudd_Ref( pbCodesLocal[i] );
    }
    Vmx_VarMapEncodeDeref( dd, pVmxGlo, pbCodesExt );

    // derive the cofactors
    bCubeOut = Vmx_VarMapCharCubeOutput( dd, pVmxGlo );  Cudd_Ref( bCubeOut );
    for ( i = 0; i < nValuesOut; i++ )
    {
        pbCofs[i] = Cudd_bddAndAbstract( dd, bCare, pbCodesLocal[i], bCubeOut ); 
        Cudd_Ref( pbCofs[i] );
    }
    Cudd_RecursiveDeref( dd, bCubeOut ); 

	if ( s_Timeout && clock() > s_TimeLimit )
    {
        fmmImageMvPartDeref( dd, pbCodesLocal, nValuesOut );
        fmmImageMvPartDeref( dd, pbCofs, nValuesOut );
		return NULL;
    }

    // solve the image computation problem for the cofactors
    for ( i = 0; i < nValuesOut; i++ )
    {
        pbImages[i] = fmmImageMvCompute( dd, pbCofs[i], pbFuncs, nFuncs, pVmxGlo, pVmxLoc, pbVars, nBddSize );  
        if ( pbImages[i] == NULL )
        {
            fmmImageMvPartDeref( dd, pbImages, i - 1 );
            fmmImageMvPartDeref( dd, pbCofs, nValuesOut );
            fmmImageMvPartDeref( dd, pbCodesLocal, nValuesOut );
            return NULL;
        }
        Cudd_Ref( pbImages[i] );
    }
    bImage = fmmImageMvPartConvolve( dd, pbImages, pbCodesLocal, nValuesOut ); Cudd_Ref( bImage );
    fmmImageMvPartDeref( dd, pbCofs, nValuesOut );
    fmmImageMvPartDeref( dd, pbImages, nValuesOut );
    fmmImageMvPartDeref( dd, pbCodesLocal, nValuesOut );
    Cudd_Deref( bImage );
    return bImage;
}

/**Function*************************************************************

  Synopsis    [Computes the MV image of a set of function.]

  Description [Assumes that the care-set does not depend on the parameter variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmmImageMvCompute( DdManager * dd, DdNode * bCare, DdNode ** pbFuncs, int nFuncs, 
    Vmx_VarMap_t * pVmxGlo, Vmx_VarMap_t * pVmxLoc, DdNode ** pbVars, int nBddSize )
{
    DdNode ** pbCodes, * bCubeIn, * bTemp;
	DdNode * bImage, * bImagePart, * bImageProper;
    Vm_VarMap_t * pVm;
	Fmm_ImagePartInfo_t * pNew;
	array_t * pParts;
	int i, nVarsIn, nValuesIn, iValue;
    int * pValues;

    // get the parameters
    pVm       = Vmx_VarMapReadVm( pVmxLoc );
    nVarsIn   = Vm_VarMapReadVarsInNum( pVm );
    nValuesIn = Vm_VarMapReadValuesInNum( pVm );
    pValues   = Vm_VarMapReadValuesArray( pVm );
    // the number of functions should be equal to the number of input values in the local space
    assert( nFuncs == nValuesIn );

	// create the array of partitions
	pParts = array_alloc( Fmm_ImagePartInfo_t *, nVarsIn );

    // encode the local space
    pbCodes = Vmx_VarMapEncodeMapUsingVars( dd, pbVars, pVmxLoc );

	// constrain the functions
	bImage = b1;  Cudd_Ref( bImage );
    iValue = 0;
	for ( i = 0; i < nVarsIn; i++ )
	{
        // create the partition corresponding to the i-th MV variable
        pNew = fmmImageMvPartCreate( dd, bCare, pbFuncs + iValue, 
            pbCodes + iValue, pValues[i], pVmxGlo, &bImagePart, 1 );
        if ( pNew == NULL )
        { // the partition is trivial, the image has been computed instead
            // bImagePart comes referenced
            bImage = Cudd_bddAnd( dd, bTemp = bImage, bImagePart );  Cudd_Ref( bImage );
			Cudd_RecursiveDeref( dd, bTemp );
			Cudd_RecursiveDeref( dd, bImagePart );
        }
        else
        { // store away this partition
            assert( bImagePart == NULL );
			array_insert_last( Fmm_ImagePartInfo_t *, pParts, pNew );
        }
        // add to the values
        iValue += pValues[i];
	}
    assert( iValue == nValuesIn );

    // deref the codes of the local space
    Vmx_VarMapEncodeDeref( dd, pVmxLoc, pbCodes );

    if ( array_n(pParts) > 0 )
    {
        array_sort( pParts, fmmImageMvPartCompareBddSize );
        // get the cube of input vars
        bCubeIn = Vmx_VarMapCharCubeInput( dd, pVmxGlo );  Cudd_Ref( bCubeIn );
	    // call the recursive image computation
	    bImageProper = fmmImageMv_rec( dd, pParts, bCubeIn, pVmxGlo, nBddSize );  
	    if ( bImageProper == NULL )
	    {
		    Cudd_RecursiveDeref( dd, bImage );
	        Cudd_RecursiveDeref( dd, bCubeIn );
		    fmmImageMvPartArrayFree( dd, pParts );
		    return NULL;
	    }
	    Cudd_Ref( bImageProper );
	    Cudd_RecursiveDeref( dd, bCubeIn );

	    // add to the image
	    bImage = Cudd_bddAnd( dd, bTemp = bImage, bImageProper );  Cudd_Ref( bImage );
	    Cudd_RecursiveDeref( dd, bTemp );
	    Cudd_RecursiveDeref( dd, bImageProper );
    }
    fmmImageMvPartArrayFree( dd, pParts );
	Cudd_Deref( bImage );
	return bImage;
}

/**Function*************************************************************

  Synopsis    [Recursively computes the image for a set of partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmmImageMv_rec( DdManager * dd, array_t * pParts, DdNode * bCubeIn, Vmx_VarMap_t * pVmxGlo, int nBddSize )
{
	DdNode * bImage;
    DdNode * pbImages[32];
	array_t * pDisjs, * pArray;
	Fmm_ImagePartInfo_t * pStart;
	int i, k;

    assert( array_n(pParts) > 0 );

	// trivial case when there is only one partition
	if ( array_n(pParts) == 1 )
    {
        pStart = array_fetch( Fmm_ImagePartInfo_t *, pParts, 0 );
        return fmmImageMvPartImage( dd, bCubeIn, pStart );
    }

	// checking for disjoint supports
	pDisjs = fmmImageMvDisjointSupport( dd, pParts );
	if ( pDisjs != NULL ) // there are disjoint support groups
	{
		DdNode * bImageNew, * bTemp;
		bImage = b1; Cudd_Ref( bImage );
		for ( i = 0; i < array_n(pDisjs); i++ )
		{
			pArray = array_fetch( array_t *, pDisjs, i );
			bImageNew = fmmImageMv_rec( dd, pArray, bCubeIn, pVmxGlo, nBddSize );   
			if ( bImageNew == NULL )
			{
				Cudd_RecursiveDeref( dd, bImage );
				for ( k = i; k < array_n(pDisjs); k++ )
				{
					pArray = array_fetch( array_t *, pDisjs, k );
					array_free(pArray);
				}
				array_free( pDisjs );
				return NULL;
			}
			Cudd_Ref( bImageNew );

			bImage = Cudd_bddAnd( dd, bTemp = bImage, bImageNew );  Cudd_Ref( bImage );
			Cudd_RecursiveDeref( dd, bTemp );
			Cudd_RecursiveDeref( dd, bImageNew );
			array_free(pArray);
		}
		array_free( pDisjs );
		Cudd_Deref( bImage );
		return bImage;
	}

	// checking the BDD size
	if ( bImage = fmmImageMvSmallSize( dd, pParts, bCubeIn, nBddSize ) )
		return bImage;

	if ( s_Timeout && clock() > s_TimeLimit )
		return NULL;

	// the regular case - split around the first partition
	pStart  = array_fetch( Fmm_ImagePartInfo_t *, pParts, 0 );
    for ( i = 0; i < pStart->nValues; i++ )
    {
        pbImages[i] = fmmImageMvValue_rec( dd, pParts, i, bCubeIn, pVmxGlo, nBddSize );  
        if ( pbImages[i] == NULL )
        {
            fmmImageMvPartDeref( dd, pbImages, i - 1 );
            return NULL;
        }
        Cudd_Ref( pbImages[i] );
    }
    bImage = fmmImageMvPartConvolve( dd, pbImages, pStart->pbCodes, pStart->nValues );
    Cudd_Ref( bImage );
    fmmImageMvPartDeref( dd, pbImages, pStart->nValues );
    Cudd_Deref( bImage );
    return bImage;
}


/**Function*************************************************************

  Synopsis    [Recursively computes one cofactor of the image of a set of partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmmImageMvValue_rec( DdManager * dd, array_t * pParts, int iPart, DdNode * bCubeIn, Vmx_VarMap_t * pVmxGlo, int nBddSize )
{
	DdNode * bImageProper, * bImagePart;
	DdNode * bImage, * bTemp, * bCare;
	Fmm_ImagePartInfo_t * pCur, * pStart, * pNew;
	array_t * pPartsNew;
	int i;

    // get the current partition
    pStart  = array_fetch( Fmm_ImagePartInfo_t *, pParts, 0 );
    // get the care set
    bCare = pStart->pbFuncs[iPart];

	// create the array of partitions
	pPartsNew = array_alloc( Fmm_ImagePartInfo_t *, array_n(pParts)-1 );
	// constrain the functions
	bImage = b1;  Cudd_Ref( bImage );
	for ( i = 1; i < array_n(pParts); i++ )
	{
        // get the partition corresponding to the i-th MV var
        pCur = array_fetch( Fmm_ImagePartInfo_t *, pParts, i );
        // create the partition corresponding to the i-th MV variable
        pNew = fmmImageMvPartCreate( dd, bCare, pCur->pbFuncs, 
            pCur->pbCodes, pCur->nValues, pVmxGlo, &bImagePart, 1 );
        if ( pNew == NULL )
        { // the partition is trivial, the image has been computed instead
            // bImagePart comes referenced
            bImage = Cudd_bddAnd( dd, bTemp = bImage, bImagePart );  Cudd_Ref( bImage );
			Cudd_RecursiveDeref( dd, bTemp );
			Cudd_RecursiveDeref( dd, bImagePart );
        }
        else
        { // store away this partition
			array_insert_last( Fmm_ImagePartInfo_t *, pPartsNew, pNew );
        }
	}

    if ( array_n(pPartsNew) > 0 )
    {
        array_sort( pPartsNew, fmmImageMvPartCompareBddSize );
	    // call the recursive image computation
	    bImageProper = fmmImageMv_rec( dd, pPartsNew, bCubeIn, pVmxGlo, nBddSize );  
	    if ( bImageProper == NULL )
	    {
		    Cudd_RecursiveDeref( dd, bImage );
		    fmmImageMvPartArrayFree( dd, pPartsNew );
		    return NULL;
	    }
	    Cudd_Ref( bImageProper );

	    // add to the image
	    bImage = Cudd_bddAnd( dd, bTemp = bImage, bImageProper );  Cudd_Ref( bImage );
	    Cudd_RecursiveDeref( dd, bTemp );
	    Cudd_RecursiveDeref( dd, bImageProper );
    }
    fmmImageMvPartArrayFree( dd, pPartsNew );
	Cudd_Deref( bImage );
	return bImage;
}


/**Function*************************************************************

  Synopsis    [Splits the image using disjoint support of the functions.]

  Description [Returns NULL, if there is no way to split. Otherwise, returns
  the array of arrays holding the sets of disjoint partitions.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
array_t * fmmImageMvDisjointSupport( DdManager * dd, array_t * pParts )
{
	array_t * pArray = NULL, * pPartNew;
	char ** pMatrix, * pSet, * pAll;
	int i, k, c, nComps, nCompsTotal;
	Fmm_ImagePartInfo_t * p1, * p2;
	int fPrintPartitions = 0;
	
	assert( array_n(pParts) > 1 );

	// allocate the interaction matrix
	pMatrix = ALLOC( char *, array_n(pParts) );
	pMatrix[0] = ALLOC( char, array_n(pParts) * array_n(pParts) );
	for ( i = 1; i < array_n(pParts); i++ )
		pMatrix[i] = pMatrix[i-1] + sizeof(char) * array_n(pParts);

	if ( fPrintPartitions )
	{
		for ( i = 0; i < array_n(pParts); i++ )
		{
			p1 = array_fetch( Fmm_ImagePartInfo_t *, pParts, i );
//			PRB( dd, p1->bSupp );
		}
	}

	// fill in the matrix
	for ( i = 0; i < array_n(pParts); i++ )
	{
		pMatrix[i][i] = 0;
		for ( k = i + 1; k < array_n(pParts); k++ )
		{
			p1 = array_fetch( Fmm_ImagePartInfo_t *, pParts, i );
			p2 = array_fetch( Fmm_ImagePartInfo_t *, pParts, k );
			pMatrix[i][k] = Extra_bddSuppOverlapping( dd, p1->bSupp, p2->bSupp );
			pMatrix[k][i] = pMatrix[i][k];
		}
		if ( fPrintPartitions )
		{
			for ( k = 0; k < array_n(pParts); k++ )
				printf( "%d", pMatrix[i][k] );
			printf( "\n" );
		}
	}

	// allocate the place for the set
	pSet = ALLOC( char, array_n(pParts) );
	pAll = ALLOC( char, array_n(pParts) );
	memset( pAll, 0, sizeof(char) * array_n(pParts) );

	// isolate strongly connected components
	nCompsTotal = 0;
	while ( nCompsTotal < array_n(pParts) )
	{
		// find the next comp
		nComps = 0;
		for ( i = 0; i < array_n(pParts); i++ )
			if ( pAll[i] == 0 )
			{
				// add this comp
				pAll[i]        = 1;
				pSet[nComps++] = i;
				nCompsTotal++;
				break;
			}
		// add all the related comps
		for ( c = 0; c < nComps; c++ )
			for ( k = 0; k < array_n(pParts); k++ )
				if ( pAll[k] == 0 && pMatrix[pSet[c]][k] )
				{
					pAll[k]        = 1;
					pSet[nComps++] = k;
					nCompsTotal++;
				}
		// add this set of components to the result
		if ( nComps < array_n(pParts) )
		{
			if ( pArray == NULL )
				pArray = array_alloc( array_t *, 5 );
			// create the new set of partitions
			pPartNew = array_alloc( Fmm_ImagePartInfo_t *, 10 );
			for ( i = 0; i < nComps; i++ )
			{
				p1 = array_fetch( Fmm_ImagePartInfo_t *, pParts, pSet[i] );
				array_insert_last( Fmm_ImagePartInfo_t *, pPartNew, p1 );
			}
			// insert the new set
			array_insert_last( array_t *, pArray, pPartNew );
		}
	}
	assert( nCompsTotal == array_n(pParts) );
	free( pMatrix[0] );
	free( pMatrix );
	free( pSet );
	free( pAll );

	if ( fPrintPartitions && pArray )
	{
		Fmm_ImagePartInfo_t * p;
		array_t * pComp;
		int i, k;

		printf( "The total number of components = %d.\n", array_n(pParts) );
		for ( i = 0; i < array_n(pArray); i++ )
		{
			pComp = array_fetch( array_t *, pArray, i );
			printf( "The number of components = %d.\n", array_n(pComp) );
			for ( k = 0; k < array_n(pComp); k++ )
			{
				p = array_fetch( Fmm_ImagePartInfo_t *, pComp, k );
//				PRB( dd, p->bSupp );
			}
		}
		i = 0;
	}

	return pArray;
}


/**Function*************************************************************

  Synopsis    [Solves small problems using monolithic transition relation.]

  Description [Returns NULL if the problem is larger than the given size. 
  Otherwise, returns the solution.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmmImageMvSmallSize( DdManager * dd, array_t * pParts, DdNode * bCubeIn, int nBddSize )
{
	DdNode * bTransRel, * bComp, * bTemp, * bImage;
	Fmm_ImagePartInfo_t * pCur;
	int i, nNodes;

	assert( array_n(pParts) > 1 );

	nNodes = 0;
	for ( i = 0; i < array_n(pParts); i++ )
	{
		pCur    = array_fetch( Fmm_ImagePartInfo_t *, pParts, i );
		nNodes += pCur->BddSize;
		if ( nNodes >= nBddSize )
			return NULL;
	}

	// build the transition relation mapping X space into P space using bPFuncs
	bTransRel = b1;  Cudd_Ref( b1 );
	for ( i = 0; i < array_n(pParts); i++ )
	{
		if ( s_Timeout && clock() > s_TimeLimit )
		{
			Cudd_RecursiveDeref( dd, bTransRel );
			return NULL;
		}

		// get the partition
		pCur = array_fetch( Fmm_ImagePartInfo_t *, pParts, i );
		// get the component
//		bComp = Cudd_bddXnor( dd, pCur->bPart, pCur->bVar );	Cudd_Ref( bComp );
        bComp = fmmImageMvPartConvolve( dd, pCur->pbFuncs, pCur->pbCodes, pCur->nValues );  Cudd_Ref( bComp );
		// multiply this component with the transitive relation
		bTransRel = Cudd_bddAnd( dd, bTemp = bTransRel, bComp );  Cudd_Ref( bTransRel );
		Cudd_RecursiveDeref( dd, bTemp );
		Cudd_RecursiveDeref( dd, bComp );
	}

	bImage = Cudd_bddExistAbstract( dd, bTransRel, bCubeIn );    Cudd_Ref( bImage );
	Cudd_RecursiveDeref( dd, bTransRel );
	Cudd_Deref( bImage );
	return bImage;
}




	
/**Function*************************************************************

  Synopsis    [Allocates the data structure to store one partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fmm_ImagePartInfo_t * fmmImageMvPartAllocate( DdNode * bSupp, DdNode ** pbFuncs, DdNode ** pbCodes, int nValues )
{
    Fmm_ImagePartInfo_t * p;
    int i;
    p = ALLOC( Fmm_ImagePartInfo_t, 1 );
    p->nValues = nValues;
    p->pbFuncs = ALLOC( DdNode *, nValues );
    p->pbCodes = ALLOC( DdNode *, nValues );
    p->bSupp = bSupp;   Cudd_Ref( bSupp );
    for ( i = 0; i < nValues; i++ )
    {
        p->pbFuncs[i] = pbFuncs[i];   Cudd_Ref( pbFuncs[i] );
        p->pbCodes[i] = pbCodes[i];   Cudd_Ref( pbCodes[i] );
    }
    p->BddSize = Cudd_SharingSize( pbFuncs, nValues );
	return p;
}

/**Function*************************************************************

  Synopsis    [Creates the partition corresponding to one MV var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fmm_ImagePartInfo_t * fmmImageMvPartCreate( DdManager * dd, DdNode * bCare, DdNode * pbFuncs[], 
    DdNode * pbCodes[], int nValues, Vmx_VarMap_t * pVmxGlo, DdNode ** pbImagePart, bool fUseAnd )
{
    DdNode * pbConstrs[32];
    DdNode * bSupp;
    Fmm_ImagePartInfo_t * pPart;

    // constrain the functions corresponding to this var
//	bConstr = Cudd_bddConstrain( dd, pbFuncs[i], bCareSet ); Cudd_Ref( bConstr );
    fmmImageMvPartConstrain( dd, bCare, pbFuncs, nValues, pbConstrs, fUseAnd );
    // compute the support of the partition
    bSupp = fmmImageMvPartSupport( dd, pbConstrs, nValues, pVmxGlo ); Cudd_Ref( bSupp );
    // decide whether this is a trivial case
	if ( bSupp == b1 ) // this MV partition is a constant after constraining
	{
//		bImage = Cudd_bddAnd( dd, bTemp = bImage, Cudd_NotCond(pbVars[i], (bConstr == b0)) );  Cudd_Ref( bImage );
        *pbImagePart = fmmImageMvPartConvolve( dd, pbConstrs, pbCodes, nValues );  Cudd_Ref( *pbImagePart );
        pPart = NULL;
    }
	else
	{
		pPart = fmmImageMvPartAllocate( bSupp, pbConstrs, pbCodes, nValues );
        *pbImagePart = NULL;
	}
    // deref the support
    Cudd_RecursiveDeref( dd, bSupp );
    // dereference the computed constrain
    fmmImageMvPartDeref( dd, pbConstrs, nValues );
    return pPart;
}



/**Function*************************************************************

  Synopsis    [Frees the data structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void fmmImageMvPartFree( DdManager * dd, Fmm_ImagePartInfo_t * p )
{
    int i;
    for ( i = 0; i < p->nValues; i++ )
    {
	    Cudd_RecursiveDeref( dd, p->pbFuncs[i] );
	    Cudd_RecursiveDeref( dd, p->pbCodes[i] );
    }
	Cudd_RecursiveDeref( dd, p->bSupp );
	free( p->pbFuncs );
	free( p->pbCodes );
	free( p );
}

/**Function*************************************************************

  Synopsis    [Compares two nodes by the number of their fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int fmmImageMvPartCompareBddSize( Fmm_ImagePartInfo_t ** ppPart1, Fmm_ImagePartInfo_t ** ppPart2 )
{
    if ( (*ppPart1)->BddSize < (*ppPart2)->BddSize )
        return  1;
    if ( (*ppPart1)->BddSize > (*ppPart2)->BddSize )
        return -1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Dereferences a set of functions]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void fmmImageMvPartDeref( DdManager * dd, DdNode ** pbFuncs, int nFuncs )
{
    int i;
    for ( i = 0; i < nFuncs; i++ )
        Cudd_RecursiveDeref( dd, pbFuncs[i] );
}

/**Function*************************************************************

  Synopsis    [Constrains the functions of one partition with the care set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void fmmImageMvPartConstrain( DdManager * dd, DdNode * bCare,
        DdNode ** pbFuncs, int nFuncs, DdNode * pbConstrs[], bool fUseAnd )
{
    int i;
    for ( i = 0; i < nFuncs; i++ )
    {
//        if ( fUseAnd )
//            pbConstrs[i] = Cudd_bddAnd( dd, pbFuncs[i], bCare ); 
//        else
            pbConstrs[i] = Cudd_bddConstrain( dd, pbFuncs[i], bCare ); 
        Cudd_Ref( pbConstrs[i] );
    }
}

/**Function*************************************************************

  Synopsis    [Computes the MV support of the set of functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmmImageMvPartSupport( DdManager * dd, DdNode ** pbFuncs, int nFuncs, Vmx_VarMap_t * pVmxGlo )
{
    DdNode * bTemp, * bComp, * bSupp;
    int i;

    bSupp = b1;   Cudd_Ref( bSupp );
    for ( i = 0; i < nFuncs; i++ )
    {
        bComp = Mvr_RelationSupportCube( dd, pbFuncs[i], pVmxGlo );   Cudd_Ref( bComp );
        bSupp = Cudd_bddAnd( dd, bTemp = bSupp, bComp );              Cudd_Ref( bSupp );
        Cudd_RecursiveDeref( dd, bTemp );  
        Cudd_RecursiveDeref( dd, bComp );
    }
    Cudd_Deref( bSupp );
    return bSupp;
}

/**Function*************************************************************

  Synopsis    [Convolves the array of functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmmImageMvPartConvolve( DdManager * dd, DdNode ** pbFuncs, DdNode ** pbCodes, int nFuncs )
{
    DdNode * bRes, * bComp, * bTemp;
    int i;
    bRes = b0;    Cudd_Ref( bRes );
    for ( i = 0; i < nFuncs; i++ )
    {
        bComp = Cudd_bddAnd( dd, pbFuncs[i], pbCodes[i] ); Cudd_Ref( bComp );
        bRes  = Cudd_bddOr ( dd, bTemp = bRes,  bComp );   Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );  
        Cudd_RecursiveDeref( dd, bComp );
    }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Computes the image of one partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * fmmImageMvPartImage( DdManager * dd, DdNode * bCubeIn, Fmm_ImagePartInfo_t * p )
{
    DdNode * bRes, * bPart;
    bPart = fmmImageMvPartConvolve( dd, p->pbFuncs, p->pbCodes, p->nValues );  Cudd_Ref( bPart );
    bRes = Cudd_bddExistAbstract( dd, bPart, bCubeIn ); Cudd_Ref( bRes );
    Cudd_RecursiveDeref( dd, bPart );
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Frees the array of partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void fmmImageMvPartArrayFree( DdManager * dd, array_t * pParts )
{
	int i;
	Fmm_ImagePartInfo_t * pCur;
	// free the partitions
	for ( i = 0; i < array_n(pParts); i++ )
	{
		pCur = array_fetch( Fmm_ImagePartInfo_t *, pParts, i );
		fmmImageMvPartFree( dd, pCur );
	}
	array_free( pParts );
}


/**Function********************************************************************

  Synopsis    [Sets the timeout.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void Fmm_ImageTimeoutSet( int timeout )
{
	s_Timeout = timeout;
}


/**Function********************************************************************

  Synopsis    [Resets the timeout.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
void Fmm_ImageTimeoutReset()
{
	s_Timeout = 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


