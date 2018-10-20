/**CFile****************************************************************

  FileName    [mvcDd.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Coverting from the Mvc_Cover_t to BDDs and ZDDs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcDd.c,v 1.2 2004/01/06 21:02:58 alanmi Exp $]

***********************************************************************/

#include "mvc.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvc_CoverConvertToBdd( DdManager * dd, Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    DdNode * bRes, * bCube, * bTemp;
    bRes = b0; Cudd_Ref( bRes );
    Mvc_CoverForEachCube( pCover, pCube )
    {
        bCube = Mvc_CubeConvertToBdd( dd, pCube );     Cudd_Ref( bCube );
        bRes = Cudd_bddOr( dd, bTemp = bRes, bCube );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvc_CoverConvertToZdd( DdManager * dd, Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    DdNode * zRes, * zCube, * zTemp;
    zRes = z0; Cudd_Ref( zRes );
    Mvc_CoverForEachCube( pCover, pCube )
    {
        zCube = Mvc_CubeConvertToZdd( dd, pCube );        Cudd_Ref( zCube );
        zRes = Cudd_zddUnion( dd, zTemp = zRes, zCube );  Cudd_Ref( zRes );
        Cudd_RecursiveDerefZdd( dd, zTemp );
        Cudd_RecursiveDerefZdd( dd, zCube );
    }
    Cudd_Deref( zRes );
    return zRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvc_CoverConvertToZdd2( DdManager * dd, Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    DdNode * zRes, * zCube, * zTemp;
    zRes = z0; Cudd_Ref( zRes );
    Mvc_CoverForEachCube( pCover, pCube )
    {
        zCube = Mvc_CubeConvertToZdd2( dd, pCube );       Cudd_Ref( zCube );
        zRes = Cudd_zddUnion( dd, zTemp = zRes, zCube );  Cudd_Ref( zRes );
        Cudd_RecursiveDerefZdd( dd, zTemp );
        Cudd_RecursiveDerefZdd( dd, zCube );
    }
    Cudd_Deref( zRes );
    return zRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvc_CubeConvertToBdd( DdManager * dd, Mvc_Cube_t * pCube )
{
    DdNode * bRes, * bTemp;
    int nBits, i, v0, v1;
	// transform the combination from the array VarValues into a ZDD cube
    bRes = b1;  cuddRef(bRes);
    nBits = (pCube->iLast + 1) * sizeof(Mvc_CubeWord_t) * 8 - pCube->nUnused;
    for ( i = nBits/2 - 1; i >= 0; i-- ) 
	{ 
        v0 = Mvc_CubeBitValue( pCube, 2 * i     );
        v1 = Mvc_CubeBitValue( pCube, 2 * i + 1 );
        assert( !v0 || !v1 );
		if ( v0 ) 
		{
            bRes = Cudd_bddAnd( dd, bTemp = bRes, dd->vars[i] ); Cudd_Ref( bRes );
            Cudd_RecursiveDeref( dd, bTemp );
		}
		else if ( v1 ) 
		{
            bRes = Cudd_bddAnd( dd, bTemp = bRes, Cudd_Not(dd->vars[i]) ); Cudd_Ref( bRes );
            Cudd_RecursiveDeref( dd, bTemp );
		}
	}
	Cudd_Deref( bRes );
	return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvc_CubeConvertToZdd( DdManager * dd, Mvc_Cube_t * pCube )
{
    DdNode * zRes, * zTemp;
    int nBits, i;
	// transform the combination from the array VarValues into a ZDD cube
    zRes = z1;  cuddRef(zRes);
    nBits = (pCube->iLast + 1) * sizeof(Mvc_CubeWord_t) * 8 - pCube->nUnused;
    for ( i = nBits - 1; i >= 0; i-- ) 
	{ 
		if ( Mvc_CubeBitValue( pCube, i ) ) 
		{
			/* compose zRes with ZERO for the given ZDD variable */
			zRes = cuddZddGetNode( dd, i, zTemp = zRes, z0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes );
			cuddDeref( zTemp );
		}
	}
	cuddDeref( zRes );
	return zRes;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Mvc_CubeConvertToZdd2( DdManager * dd, Mvc_Cube_t * pCube )
{
    DdNode * zRes, * zTemp;
    int nBits, i, v0, v1;
	// transform the combination from the array VarValues into a ZDD cube
    zRes = z1;  cuddRef(zRes);
    nBits = (pCube->iLast + 1) * sizeof(Mvc_CubeWord_t) * 8 - pCube->nUnused;
    for ( i = nBits/2 - 1; i >= 0; i-- ) 
	{ 
        v0 = Mvc_CubeBitValue( pCube, 2 * i     );
        v1 = Mvc_CubeBitValue( pCube, 2 * i + 1 );
        assert( v0 || v1 );
		if ( !v1 ) 
		{
			zRes = cuddZddGetNode( dd, 2 * i + 1, zTemp = zRes, z0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes );
			cuddDeref( zTemp );
		}
		else if ( !v0 ) 
		{
			zRes = cuddZddGetNode( dd, 2 * i, zTemp = zRes, z0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes );
			cuddDeref( zTemp );
		}
	}
	Cudd_Deref( zRes );
	return zRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


