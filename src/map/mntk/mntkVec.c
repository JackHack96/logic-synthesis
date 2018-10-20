/**CFile****************************************************************

  FileName    [mntkVec.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkVec.c,v 1.1 2005/02/28 05:34:32 alanmi Exp $]

***********************************************************************/

#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Mntk_NodeVecCompareArrivalsIncrease( Mntk_Node_t ** pp1, Mntk_Node_t ** pp2 );
static int Mntk_NodeVecCompareArrivalsDecrease( Mntk_Node_t ** pp1, Mntk_Node_t ** pp2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_NodeVec_t * Mntk_NodeVecAlloc( int nCap )
{
    Mntk_NodeVec_t * p;
    p = ALLOC( Mntk_NodeVec_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( Mntk_Node_t *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_NodeVec_t * Mntk_NodeVecDup( Mntk_NodeVec_t * p )
{
    Mntk_NodeVec_t * pNew;
    pNew = Mntk_NodeVecAlloc( p->nSize );
    memcpy( pNew->pArray, p->pArray, sizeof(Mntk_Node_t *) * p->nSize );
    pNew->nSize = p->nSize;
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeVecFree( Mntk_NodeVec_t * p )
{
    FREE( p->pArray );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t ** Mntk_NodeVecReadArray( Mntk_NodeVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_NodeVecReadSize( Mntk_NodeVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeVecGrow( Mntk_NodeVec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = REALLOC( Mntk_Node_t *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeVecShrink( Mntk_NodeVec_t * p, int nSizeNew )
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
void Mntk_NodeVecClear( Mntk_NodeVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeVecPush( Mntk_NodeVec_t * p, Mntk_Node_t * pNode )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Mntk_NodeVecGrow( p, 16 );
        else
            Mntk_NodeVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = pNode;
}

/**Function*************************************************************

  Synopsis    [Add the element while ensuring uniqueness.]

  Description [Returns 1 if the element was found, and 0 if it was new. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_NodeVecPushUnique( Mntk_NodeVec_t * p, Mntk_Node_t * pNode )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == pNode )
            return 1;
    Mntk_NodeVecPush( p, pNode );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Mntk_NodeVecPop( Mntk_NodeVec_t * p )
{
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeVecWriteEntry( Mntk_NodeVec_t * p, int i, Mntk_Node_t * Entry )
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
Mntk_Node_t * Mntk_NodeVecReadEntry( Mntk_NodeVec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

/**Function*************************************************************

  Synopsis    [Sorting the entries by in the decreasing order of arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeVecSortByArrival( Mntk_NodeVec_t * p, int fIncreasing )
{
    if ( fIncreasing )
        qsort( (void *)p->pArray, p->nSize, sizeof(Mntk_Node_t *), 
                (int (*)(const void *, const void *)) Mntk_NodeVecCompareArrivalsIncrease );
    else
        qsort( (void *)p->pArray, p->nSize, sizeof(Mntk_Node_t *), 
                (int (*)(const void *, const void *)) Mntk_NodeVecCompareArrivalsDecrease );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_NodeVecCompareArrivalsIncrease( Mntk_Node_t ** pp1, Mntk_Node_t ** pp2 )
{
    if ( Mntk_Regular(*pp1)->tArrival.Worst < Mntk_Regular(*pp2)->tArrival.Worst )
        return -1;
    if ( Mntk_Regular(*pp1)->tArrival.Worst > Mntk_Regular(*pp2)->tArrival.Worst )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_NodeVecCompareArrivalsDecrease( Mntk_Node_t ** pp1, Mntk_Node_t ** pp2 )
{
    if ( Mntk_Regular(*pp1)->tArrival.Worst > Mntk_Regular(*pp2)->tArrival.Worst )
        return -1;
    if ( Mntk_Regular(*pp1)->tArrival.Worst < Mntk_Regular(*pp2)->tArrival.Worst )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mntk_NodeVecPushOrder( Mntk_NodeVec_t * p, Mntk_Node_t * pNode, int fIncreasing )
{
    Mntk_Node_t * pNode1, * pNode2;
    int i;
    Mntk_NodeVecPush( p, pNode );
    // find the p of the node
    for ( i = p->nSize-1; i > 0; i-- )
    {
        pNode1 = p->pArray[i  ];
        pNode2 = p->pArray[i-1];
        if ( fIncreasing && pNode1->tArrival.Worst >= pNode2->tArrival.Worst ||
            !fIncreasing && pNode1->tArrival.Worst <= pNode2->tArrival.Worst )
            break;
        p->pArray[i  ] = pNode2;
        p->pArray[i-1] = pNode1;
    }
}

/**Function*************************************************************

  Synopsis    [Add the element while ensuring uniqueness in the order.]

  Description [Returns 1 if the element was found, and 0 if it was new. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mntk_NodeVecPushUniqueOrder( Mntk_NodeVec_t * p, Mntk_Node_t * pNode, int fIncreasing )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == pNode )
            return 1;
    Mntk_NodeVecPushOrder( p, pNode, fIncreasing );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

