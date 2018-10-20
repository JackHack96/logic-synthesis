/**CFile****************************************************************

  FileName    [resmVec.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmVec.c,v 1.1 2005/01/23 06:59:54 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Resm_NodeVecCompareArrivalsIncrease( Resm_Node_t ** pp1, Resm_Node_t ** pp2 );
static int Resm_NodeVecCompareArrivalsDecrease( Resm_Node_t ** pp1, Resm_Node_t ** pp2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_NodeVec_t * Resm_NodeVecAlloc( int nCap )
{
    Resm_NodeVec_t * p;
    p = ALLOC( Resm_NodeVec_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( Resm_Node_t *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_NodeVec_t * Resm_NodeVecDup( Resm_NodeVec_t * p )
{
    Resm_NodeVec_t * pNew;
    pNew = Resm_NodeVecAlloc( p->nSize );
    memcpy( pNew->pArray, p->pArray, sizeof(Resm_Node_t *) * p->nSize );
    pNew->nSize = p->nSize;
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeVecFree( Resm_NodeVec_t * p )
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
Resm_Node_t ** Resm_NodeVecReadArray( Resm_NodeVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeVecReadSize( Resm_NodeVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeVecGrow( Resm_NodeVec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = REALLOC( Resm_Node_t *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeVecShrink( Resm_NodeVec_t * p, int nSizeNew )
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
void Resm_NodeVecClear( Resm_NodeVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeVecPush( Resm_NodeVec_t * p, Resm_Node_t * pNode )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Resm_NodeVecGrow( p, 16 );
        else
            Resm_NodeVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = pNode;
}

/**Function*************************************************************

  Synopsis    [Add the element while ensuring uniqueness.]

  Description [Returns 1 if the element was found, and 0 if it was new. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeVecPushUnique( Resm_NodeVec_t * p, Resm_Node_t * pNode )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == pNode )
            return 1;
    Resm_NodeVecPush( p, pNode );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Node_t * Resm_NodeVecPop( Resm_NodeVec_t * p )
{
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeVecWriteEntry( Resm_NodeVec_t * p, int i, Resm_Node_t * Entry )
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
Resm_Node_t * Resm_NodeVecReadEntry( Resm_NodeVec_t * p, int i )
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
void Resm_NodeVecSortByArrival( Resm_NodeVec_t * p, int fIncreasing )
{
    if ( fIncreasing )
        qsort( (void *)p->pArray, p->nSize, sizeof(Resm_Node_t *), 
                (int (*)(const void *, const void *)) Resm_NodeVecCompareArrivalsIncrease );
    else
        qsort( (void *)p->pArray, p->nSize, sizeof(Resm_Node_t *), 
                (int (*)(const void *, const void *)) Resm_NodeVecCompareArrivalsDecrease );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeVecCompareArrivalsIncrease( Resm_Node_t ** pp1, Resm_Node_t ** pp2 )
{
    if ( Resm_Regular(*pp1)->tArrival.Worst < Resm_Regular(*pp2)->tArrival.Worst )
        return -1;
    if ( Resm_Regular(*pp1)->tArrival.Worst > Resm_Regular(*pp2)->tArrival.Worst )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_NodeVecCompareArrivalsDecrease( Resm_Node_t ** pp1, Resm_Node_t ** pp2 )
{
    if ( Resm_Regular(*pp1)->tArrival.Worst > Resm_Regular(*pp2)->tArrival.Worst )
        return -1;
    if ( Resm_Regular(*pp1)->tArrival.Worst < Resm_Regular(*pp2)->tArrival.Worst )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_NodeVecPushOrder( Resm_NodeVec_t * p, Resm_Node_t * pNode, int fIncreasing )
{
    Resm_Node_t * pNode1, * pNode2;
    int i;
    Resm_NodeVecPush( p, pNode );
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
int Resm_NodeVecPushUniqueOrder( Resm_NodeVec_t * p, Resm_Node_t * pNode, int fIncreasing )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == pNode )
            return 1;
    Resm_NodeVecPushOrder( p, pNode, fIncreasing );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

