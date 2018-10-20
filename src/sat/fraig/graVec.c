/**CFile****************************************************************

  FileName    [graVec.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Vector of FRAIG nodes.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: graVec.c,v 1.2 2005/07/08 01:01:34 alanmi Exp $]

***********************************************************************/

#include "graInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates a vector with the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_NodeVec_t * Gra_NodeVecAlloc( int nCap )
{
    Gra_NodeVec_t * p;
    p = ALLOC( Gra_NodeVec_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( Gra_Node_t *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeVecFree( Gra_NodeVec_t * p )
{
    FREE( p->pArray );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    [Duplicates the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_NodeVec_t * Gra_NodeVecDup( Gra_NodeVec_t * pVec )
{
    Gra_NodeVec_t * p;
    p = ALLOC( Gra_NodeVec_t, 1 );
    p->nSize  = pVec->nSize;
    p->nCap   = pVec->nCap;
    p->pArray = p->nCap? ALLOC( Gra_Node_t *, p->nCap ) : NULL;
    memcpy( p->pArray, pVec->pArray, sizeof(Gra_Node_t *) * pVec->nSize );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_Node_t ** Gra_NodeVecReadArray( Gra_NodeVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gra_NodeVecReadSize( Gra_NodeVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeVecGrow( Gra_NodeVec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = REALLOC( Gra_Node_t *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeVecShrink( Gra_NodeVec_t * p, int nSizeNew )
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
void Gra_NodeVecClear( Gra_NodeVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeVecPush( Gra_NodeVec_t * p, Gra_Node_t * Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Gra_NodeVecGrow( p, 16 );
        else
            Gra_NodeVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    [Add the element while ensuring uniqueness.]

  Description [Returns 1 if the element was found, and 0 if it was new. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gra_NodeVecPushUnique( Gra_NodeVec_t * p, Gra_Node_t * Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Gra_NodeVecPush( p, Entry );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Inserts a new node in the order by arrival times.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeVecPushOrder( Gra_NodeVec_t * p, Gra_Node_t * pNode )
{
    Gra_Node_t * pNode1, * pNode2;
    int i;
    Gra_NodeVecPush( p, pNode );
    // find the p of the node
    for ( i = p->nSize-1; i > 0; i-- )
    {
        pNode1 = p->pArray[i  ];
        pNode2 = p->pArray[i-1];
        if ( pNode1 >= pNode2 )
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
int Gra_NodeVecPushUniqueOrder( Gra_NodeVec_t * p, Gra_Node_t * pNode )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == pNode )
            return 1;
    Gra_NodeVecPushOrder( p, pNode );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_Node_t * Gra_NodeVecPop( Gra_NodeVec_t * p )
{
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeVecRemove( Gra_NodeVec_t * p, Gra_Node_t * Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            break;
    assert( i < p->nSize );
    for ( i++; i < p->nSize; i++ )
        p->pArray[i-1] = p->pArray[i];
    p->nSize--;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeVecWriteEntry( Gra_NodeVec_t * p, int i, Gra_Node_t * Entry )
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
Gra_Node_t * Gra_NodeVecReadEntry( Gra_NodeVec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

///////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

