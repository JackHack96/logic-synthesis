/**CFile****************************************************************

  FileName    [ntkNodeVec.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The vector of network nodes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkNodeVec.c,v 1.2 2004/05/12 04:30:09 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ntk_NodeVecSortCompare( Ntk_Node_t ** pp1, Ntk_Node_t ** pp2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_NodeVec_t * Ntk_NodeVecAlloc( int nCap )
{
    Ntk_NodeVec_t * p;
    p = ALLOC( Ntk_NodeVec_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( Ntk_Node_t *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the vector from an integer array of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_NodeVec_t * Ntk_NodeVecAllocArray( Ntk_Node_t ** pArray, int nSize )
{
    Ntk_NodeVec_t * p;
    p = ALLOC( Ntk_NodeVec_t, 1 );
    p->nSize  = nSize;
    p->nCap   = nSize;
    p->pArray = pArray;
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_NodeVec_t * Ntk_NodeVecDup( Ntk_NodeVec_t * pVec )
{
    Ntk_NodeVec_t * p;
    p = ALLOC( Ntk_NodeVec_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = p->nCap? ALLOC( Ntk_Node_t *, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(Ntk_Node_t *) * pVec->nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    [Transfers the array into another vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_NodeVec_t * Ntk_NodeVecDupArray( Ntk_NodeVec_t * pVec )
{
    Ntk_NodeVec_t * p;
    p = ALLOC( Ntk_NodeVec_t, 1 );
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
void Ntk_NodeVecFree( Ntk_NodeVec_t * p )
{
    if ( p == NULL )
        return;
    FREE( p->pArray );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Fills the vector with given number of entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeVecFill( Ntk_NodeVec_t * p, int nSize, Ntk_Node_t * pEntry )
{
    int i;
    Ntk_NodeVecGrow( p, nSize );
    p->nSize = nSize;
    for ( i = 0; i < p->nSize; i++ )
        p->pArray[i] = pEntry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t ** Ntk_NodeVecReleaseArray( Ntk_NodeVec_t * p )
{
    Ntk_Node_t ** pArray = p->pArray;
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
Ntk_Node_t ** Ntk_NodeVecReadArray( Ntk_NodeVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeVecReadSize( Ntk_NodeVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeVecReadEntry( Ntk_NodeVec_t * p, int i )
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
Ntk_Node_t * Ntk_NodeVecReadEntryLast( Ntk_NodeVec_t * p )
{
    return p->pArray[p->nSize-1];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeVecWriteEntry( Ntk_NodeVec_t * p, int i, Ntk_Node_t * pEntry )
{
    assert( i >= 0 && i < p->nSize );
    p->pArray[i] = pEntry;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeVecGrow( Ntk_NodeVec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = REALLOC( Ntk_Node_t *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeVecShrink( Ntk_NodeVec_t * p, int nSizeNew )
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
void Ntk_NodeVecClear( Ntk_NodeVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeVecPush( Ntk_NodeVec_t * p, Ntk_Node_t * pEntry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Ntk_NodeVecGrow( p, 16 );
        else
            Ntk_NodeVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = pEntry;
}

/**Function*************************************************************

  Synopsis    [Returns the last entry and removes it from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeVecPop( Ntk_NodeVec_t * p )
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
void Ntk_NodeVecSort( Ntk_NodeVec_t * p )
{
    qsort( (void *)p->pArray, p->nSize, sizeof(Ntk_Node_t *), 
            (int (*)(const void *, const void *)) Ntk_NodeVecSortCompare );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeVecSortCompare( Ntk_Node_t ** pp1, Ntk_Node_t ** pp2 )
{
    if ( (*pp1)->Id < (*pp2)->Id )
        return -1;
    if ( (*pp1)->Id > (*pp2)->Id )
        return 1;
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


