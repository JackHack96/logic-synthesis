/**CFile****************************************************************

  FileName    [satVec.c]

  PackageName [A C version of SAT solver MINISAT, originally developed 
  in C++ by Niklas Een and Niklas Sorensson, Chalmers University of 
  Technology, Sweden: http://www.cs.chalmers.se/~een/Satzoo.]

  Synopsis    [Integer vector borrowed from Extra.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: satVec.c,v 1.1 2005/07/08 01:01:30 alanmi Exp $]

***********************************************************************/

#include "satInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Sat_IntVec_t_
{
    int *  pArray;
    int    nSize;
    int    nCap;
};

static int Sat_IntVecSortCompare1( int * pp1, int * pp2 );
static int Sat_IntVecSortCompare2( int * pp1, int * pp2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sat_IntVec_t * Sat_IntVecAlloc( int nCap )
{
    Sat_IntVec_t * p;
    p = ALLOC( Sat_IntVec_t, 1 );
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
Sat_IntVec_t * Sat_IntVecAllocArray( int * pArray, int nSize )
{
    Sat_IntVec_t * p;
    p = ALLOC( Sat_IntVec_t, 1 );
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
Sat_IntVec_t * Sat_IntVecAllocArrayCopy( int * pArray, int nSize )
{
    Sat_IntVec_t * p;
    p = ALLOC( Sat_IntVec_t, 1 );
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
Sat_IntVec_t * Sat_IntVecDup( Sat_IntVec_t * pVec )
{
    Sat_IntVec_t * p;
    p = ALLOC( Sat_IntVec_t, 1 );
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
Sat_IntVec_t * Sat_IntVecDupArray( Sat_IntVec_t * pVec )
{
    Sat_IntVec_t * p;
    p = ALLOC( Sat_IntVec_t, 1 );
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
void Sat_IntVecFree( Sat_IntVec_t * p )
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
void Sat_IntVecFill( Sat_IntVec_t * p, int nSize, int Entry )
{
    int i;
    Sat_IntVecGrow( p, nSize );
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
int * Sat_IntVecReleaseArray( Sat_IntVec_t * p )
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
int * Sat_IntVecReadArray( Sat_IntVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_IntVecReadSize( Sat_IntVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_IntVecReadEntry( Sat_IntVec_t * p, int i )
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
void Sat_IntVecWriteEntry( Sat_IntVec_t * p, int i, int Entry )
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
int Sat_IntVecReadEntryLast( Sat_IntVec_t * p )
{
    return p->pArray[p->nSize-1];
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_IntVecGrow( Sat_IntVec_t * p, int nCapMin )
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
void Sat_IntVecShrink( Sat_IntVec_t * p, int nSizeNew )
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
void Sat_IntVecClear( Sat_IntVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sat_IntVecPush( Sat_IntVec_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Sat_IntVecGrow( p, 16 );
        else
            Sat_IntVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    [Returns the last entry and removes it from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_IntVecPop( Sat_IntVec_t * p )
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
void Sat_IntVecSort( Sat_IntVec_t * p, int fReverse )
{
    if ( fReverse ) 
        qsort( (void *)p->pArray, p->nSize, sizeof(int), 
                (int (*)(const void *, const void *)) Sat_IntVecSortCompare2 );
    else
        qsort( (void *)p->pArray, p->nSize, sizeof(int), 
                (int (*)(const void *, const void *)) Sat_IntVecSortCompare1 );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_IntVecSortCompare1( int * pp1, int * pp2 )
{
    // for some reason commenting out lines (as shown) led to crashing of the release version
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 ) //
        return 1;
    return 0; //
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sat_IntVecSortCompare2( int * pp1, int * pp2 )
{
    // for some reason commenting out lines (as shown) led to crashing of the release version
    if ( *pp1 > *pp2 )
        return -1;
    if ( *pp1 < *pp2 ) //
        return 1;
    return 0; //
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


