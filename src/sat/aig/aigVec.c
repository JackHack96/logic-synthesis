/**CFile****************************************************************

  FileName    [aigVec.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [AND-INV graph package with built in sat_sweep.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: aigVec.c,v 1.1 2004/07/15 02:50:17 alanmi Exp $]

***********************************************************************/

#include "aigInt.h"

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
Aig_NodeVec_t * Aig_NodeVecAlloc( int nCap )
{
    Aig_NodeVec_t * p;
    p = ALLOC( Aig_NodeVec_t, 1 );
    if ( nCap > 0 && nCap < 16 )
        nCap = 16;
    p->nSize  = 0;
    p->nCap   = nCap;
    p->pArray = p->nCap? ALLOC( Aig_Node_t *, p->nCap ) : NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodeVecFree( Aig_NodeVec_t * p )
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
Aig_Node_t ** Aig_NodeVecReadArray( Aig_NodeVec_t * p )
{
    return p->pArray;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeVecReadSize( Aig_NodeVec_t * p )
{
    return p->nSize;
}

/**Function*************************************************************

  Synopsis    [Resizes the vector to the given capacity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodeVecGrow( Aig_NodeVec_t * p, int nCapMin )
{
    if ( p->nCap >= nCapMin )
        return;
    p->pArray = REALLOC( Aig_Node_t *, p->pArray, nCapMin ); 
    p->nCap   = nCapMin;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodeVecShrink( Aig_NodeVec_t * p, int nSizeNew )
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
void Aig_NodeVecClear( Aig_NodeVec_t * p )
{
    p->nSize = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodeVecPush( Aig_NodeVec_t * p, Aig_Node_t * Entry )
{
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Aig_NodeVecGrow( p, 16 );
        else
            Aig_NodeVecGrow( p, 2 * p->nCap );
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Node_t * Aig_NodeVecPop( Aig_NodeVec_t * p )
{
    return p->pArray[--p->nSize];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodeVecWriteEntry( Aig_NodeVec_t * p, int i, Aig_Node_t * Entry )
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
Aig_Node_t * Aig_NodeVecReadEntry( Aig_NodeVec_t * p, int i )
{
    assert( i >= 0 && i < p->nSize );
    return p->pArray[i];
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

