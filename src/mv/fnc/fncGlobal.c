/**CFile****************************************************************

  FileName    [fncGlobal.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Creating and accessing global functions of the node.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fncGlobal.c,v 1.1 2003/11/18 18:55:03 alanmi Exp $]

***********************************************************************/

#include "fncInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create the global function structure for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fnc_Global_t * Fnc_GlobalAlloc( int nValues )
{
    Fnc_Global_t * pG;
	pG = ALLOC( Fnc_Global_t, 1 );
	memset( pG, 0, sizeof(Fnc_Global_t) );
    pG->nValues = nValues;
    pG->pbGlo   = ALLOC( DdNode *, pG->nValues );
    pG->pbGloZ  = ALLOC( DdNode *, pG->nValues );
    pG->pbGloS  = ALLOC( DdNode *, pG->nValues );
	memset( pG->pbGlo,  0, sizeof(DdNode *) * pG->nValues );
	memset( pG->pbGloZ, 0, sizeof(DdNode *) * pG->nValues );
	memset( pG->pbGloS, 0, sizeof(DdNode *) * pG->nValues );
    return pG;
}

/**Function*************************************************************

  Synopsis    [Cleans the DC structure of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_GlobalClean( Fnc_Global_t * pG )
{
    int i;
    assert( pG );
    for ( i = 0; i < pG->nValues; i++ )
    {
        if ( pG->pbGlo[i]  )  Cudd_RecursiveDeref( pG->dd, pG->pbGlo[i] );
        if ( pG->pbGloZ[i] )  Cudd_RecursiveDeref( pG->dd, pG->pbGloZ[i] );
        if ( pG->pbGloS[i] )  Cudd_RecursiveDeref( pG->dd, pG->pbGloS[i] );
    }
	memset( pG->pbGlo,  0, sizeof(DdNode *) * pG->nValues );
	memset( pG->pbGloZ, 0, sizeof(DdNode *) * pG->nValues );
	memset( pG->pbGloS, 0, sizeof(DdNode *) * pG->nValues );
    pG->dd      = NULL;
    pG->nNumber = 0;
//    pG->nValues = 0; // the number of values is not cleaned
    pG->fNonDet = 0;
}

/**Function*************************************************************

  Synopsis    [Deletes the global function structure for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_GlobalFree( Fnc_Global_t * pG )
{
    if ( pG == NULL )
        return;
	Fnc_GlobalClean( pG );
    FREE( pG->pbGlo );
    FREE( pG->pbGloZ );
    FREE( pG->pbGloS );
    FREE( pG );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Fnc_GlobalReadDd( Fnc_Global_t * pG )
{
	return pG->dd;
}

void Fnc_GlobalWriteDd( Fnc_Global_t * pG, DdManager * dd )
{
	pG->dd = dd;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Fnc_GlobalReadGlo( Fnc_Global_t * pG )
{
	return pG->pbGlo;
}

void Fnc_GlobalWriteGlo( Fnc_Global_t * pG, DdNode ** pbGlo )
{
    int i;
    for ( i = 0; i < pG->nValues; i++ )
    {
        assert( pG->pbGlo[i] == NULL );
	    pG->pbGlo[i] = pbGlo[i];  Cudd_Ref( pG->pbGlo[i] );
    }
}

void Fnc_GlobalDerefGlo( Fnc_Global_t * pG )
{
    int i;
    for ( i = 0; i < pG->nValues; i++ )
        if ( pG->pbGlo[i] )  
        {
            Cudd_RecursiveDeref( pG->dd, pG->pbGlo[i] );
            pG->pbGlo[i] = NULL;
        }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Fnc_GlobalReadGloZ( Fnc_Global_t * pG )
{
	return pG->pbGloZ;
}

void Fnc_GlobalWriteGloZ( Fnc_Global_t * pG, DdNode ** pbGloZ )
{
    int i;
    for ( i = 0; i < pG->nValues; i++ )
    {
        assert( pG->pbGloZ[i] == NULL );
	    pG->pbGloZ[i] = pbGloZ[i];  Cudd_Ref( pG->pbGloZ[i] );
    }
}

void Fnc_GlobalDerefGloZ( Fnc_Global_t * pG )
{
    int i;
    for ( i = 0; i < pG->nValues; i++ )
        if ( pG->pbGloZ[i] )  
        {
            Cudd_RecursiveDeref( pG->dd, pG->pbGloZ[i] );
            pG->pbGloZ[i] = NULL;
        }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Fnc_GlobalReadGloS( Fnc_Global_t * pG )
{
	return pG->pbGloS;
}

void Fnc_GlobalWriteGloS( Fnc_Global_t * pG, DdNode ** pbGloS )
{
    int i;
    for ( i = 0; i < pG->nValues; i++ )
    {
        assert( pG->pbGloS[i] == NULL );
	    pG->pbGloS[i] = pbGloS[i];  Cudd_Ref( pG->pbGloS[i] );
    }
}

void Fnc_GlobalDerefGloS( Fnc_Global_t * pG )
{
    int i;
    for ( i = 0; i < pG->nValues; i++ )
        if ( pG->pbGloS[i] )  
        {
            Cudd_RecursiveDeref( pG->dd, pG->pbGloS[i] );
            pG->pbGloS[i] = NULL;
        }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Fnc_GlobalReadNonDet( Fnc_Global_t * pG )
{
	return pG->fNonDet;
}

void Fnc_GlobalWriteNonDet( Fnc_Global_t * pG, bool fNonDet )
{
	pG->fNonDet = fNonDet;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fnc_GlobalReadNumber( Fnc_Global_t * pG )
{
	return pG->nNumber;
}

void Fnc_GlobalWriteNumber( Fnc_Global_t * pG, int nNumber )
{
	pG->nNumber = nNumber;
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fnc_GlobalReadBinGlo( Fnc_Global_t * pG )
{
	return pG->pbGlo[1];
}

void Fnc_GlobalWriteBinGlo( Fnc_Global_t * pG, DdNode * bGlo )
{
    assert( pG->pbGlo[1] == NULL );
	pG->pbGlo[1] = bGlo;  Cudd_Ref( pG->pbGlo[1] );
}

void Fnc_GlobalDerefBinGlo( Fnc_Global_t * pG )
{
    if ( pG->pbGlo[1] )  
    {
        Cudd_RecursiveDeref( pG->dd, pG->pbGlo[1] );
        pG->pbGlo[1] = NULL;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fnc_GlobalReadBinGloZ( Fnc_Global_t * pG )
{
	return pG->pbGloZ[1];
}

void Fnc_GlobalWriteBinGloZ( Fnc_Global_t * pG, DdNode * bGloZ )
{
    assert( pG->pbGloZ[1] == NULL );
	pG->pbGloZ[1] = bGloZ;  Cudd_Ref( pG->pbGloZ[1] );
}

void Fnc_GlobalDerefBinGloZ( Fnc_Global_t * pG )
{
    if ( pG->pbGloS[1] )  
    {
        Cudd_RecursiveDeref( pG->dd, pG->pbGloS[1] );
        pG->pbGloS[1] = NULL;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fnc_GlobalReadBinGloS( Fnc_Global_t * pG )
{
	return pG->pbGloS[1];
}

void Fnc_GlobalWriteBinGloS( Fnc_Global_t * pG, DdNode * bGloS )
{
    assert( pG->pbGloS[1] == NULL );
	pG->pbGloS[1] = bGloS;  Cudd_Ref( pG->pbGloS[1] );
}

void Fnc_GlobalDerefBinGloS( Fnc_Global_t * pG )
{
    if ( pG->pbGloS[1] )  
    {
        Cudd_RecursiveDeref( pG->dd, pG->pbGloS[1] );
        pG->pbGloS[1] = NULL;
    }
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


