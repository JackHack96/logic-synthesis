/**CFile****************************************************************

  FileName    [extraUtilIntVec.c]

  PackageName [extra]

  Synopsis    [Vector of integers.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - September 1, 2003.]

  Revision    [$Id: extraUtilIntVec.c,v 1.2 2004/05/12 04:30:10 alanmi Exp $]

***********************************************************************/

#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Extra_IntVec_t_
{
    int *  pArray;
    int    nSize;
    int    nCap;
};

static int Extra_IntVecSortCompare( int * pp1, int * pp2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_IntVec_t * Extra_IntVecAlloc( int nCap )
{
    Extra_IntVec_t * p;
    p = ALLOC( Extra_IntVec_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( int, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_IntVec_t * Extra_IntVecAllocArray( int * pArray, int nSize )
{
    Extra_IntVec_t * p;
    p = ALLOC( Extra_IntVec_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = pArray;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_IntVec_t * Extra_IntVecAllocArrayCopy( int * pArray, int nSize )
{
    Extra_IntVec_t * p;
    p = ALLOC( Extra_IntVec_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = ALLOC( int, nSize );
    memcpy( p->pArray, pArray, sizeof(int) * nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_IntVec_t * Extra_IntVecDup( Extra_IntVec_t * pVec )
{
    Extra_IntVec_t * p;
    p = ALLOC( Extra_IntVec_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = p->nCap? ALLOC( int, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(int) * pVec->nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Transfers the array into another vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_IntVec_t * Extra_IntVecDupArray( Extra_IntVec_t * pVec )
{
    Extra_IntVec_t * p;
    p = ALLOC( Extra_IntVec_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = pVec->pArray;
    pVec->nSize  = 0;
    pVec->nCap   = 0;
    pVec->pArray = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_IntVecFree( Extra_IntVec_t * p )
{
    FREE( p->pArray );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_IntVecFill( Extra_IntVec_t * p, int nSize, int Entry )
{
    int i;
    Extra_IntVecGrow( p, nSize );
    p->nSize = nSize;
    for ( i = 0; i < p->nSize; i++ )
        p->pArray[i] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Extra_IntVecReleaseArray( Extra_IntVec_t * p )
{
    int * pArray = p->pArray;
    p->nCap = 0;
    p->nSize = 0;
    p->pArray = NULL;
    return pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Extra_IntVecReadArray( Extra_IntVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_IntVecReadSize( Extra_IntVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_IntVecReadEntry( Extra_IntVec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_IntVecWriteEntry( Extra_IntVec_t * p, int i, int Entry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_IntVecReadEntryLast( Extra_IntVec_t * p )
{
    return p->pArray[p->nSize-1];
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_IntVecGrow( Extra_IntVec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = REALLOC( int, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_IntVecShrink( Extra_IntVec_t * p, int nSizeNew )
{
    assert( p->nSize >= nSizeNew );
    p->nSize = nSizeNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_IntVecClear( Extra_IntVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_IntVecPush( Extra_IntVec_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Extra_IntVecGrow( p, 16 );
        else
            Extra_IntVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    [Returns the last entry and removes it from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_IntVecPop( Extra_IntVec_t * p )
{
    assert( p->nSize > 0 );
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by their integer value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_IntVecSort( Extra_IntVec_t * p )
{
    qsort( (void *)p->pArray, p->nSize, sizeof(int), 
            (int (*)(const void *, const void *)) Extra_IntVecSortCompare );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_IntVecSortCompare( int * pp1, int * pp2 )
{
    // for some reason commenting out lines (as shown) led to crashing of the release version
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 ) //
        return 1;
    return 0; //
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


