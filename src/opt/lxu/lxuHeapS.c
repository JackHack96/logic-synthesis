/**CFile****************************************************************

  FileName    [lxuHeapS.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [The priority queue for variables.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuHeapS.c,v 1.2 2004/01/30 00:18:48 satrajit Exp $]

***********************************************************************/

#include "lxuInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define LXU_HEAP_SINGLE_WEIGHT(pSingle)           ((pSingle)->Weight)
#define LXU_HEAP_SINGLE_CURRENT(p, pSingle)       ((p)->pTree[(pSingle)->HNum])
#define LXU_HEAP_SINGLE_PARENT_EXISTS(p, pSingle) ((pSingle)->HNum > 1)
#define LXU_HEAP_SINGLE_CHILD1_EXISTS(p, pSingle) (((pSingle)->HNum << 1) <= p->nItems)
#define LXU_HEAP_SINGLE_CHILD2_EXISTS(p, pSingle) ((((pSingle)->HNum << 1)+1) <= p->nItems)
#define LXU_HEAP_SINGLE_PARENT(p, pSingle)        ((p)->pTree[(pSingle)->HNum >> 1])
#define LXU_HEAP_SINGLE_CHILD1(p, pSingle)        ((p)->pTree[(pSingle)->HNum << 1])
#define LXU_HEAP_SINGLE_CHILD2(p, pSingle)        ((p)->pTree[((pSingle)->HNum << 1)+1])
#define LXU_HEAP_SINGLE_ASSERT(p, pSingle)        assert( (pSingle)->HNum >= 1 && (pSingle)->HNum <= p->nItemsAlloc )

static void Lxu_HeapSingleResize( Lxu_HeapSingle * p );                  
static void Lxu_HeapSingleSwap( Lxu_Single ** pSingle1, Lxu_Single ** pSingle2 );  
static void Lxu_HeapSingleMoveUp( Lxu_HeapSingle * p, Lxu_Single * pSingle );  
static void Lxu_HeapSingleMoveDn( Lxu_HeapSingle * p, Lxu_Single * pSingle );  

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_HeapSingle * Lxu_HeapSingleStart()
{
	Lxu_HeapSingle * p;
	p = ALLOC( Lxu_HeapSingle, 1 );
	memset( p, 0, sizeof(Lxu_HeapSingle) );
	p->nItems      = 0;
	p->nItemsAlloc = 2000;
	p->pTree       = ALLOC( Lxu_Single *, p->nItemsAlloc + 10 );
	p->pTree[0]    = NULL;
	return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleResize( Lxu_HeapSingle * p )
{
	p->nItemsAlloc *= 2;
	p->pTree = REALLOC( Lxu_Single *, p->pTree, p->nItemsAlloc + 10 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleStop( Lxu_HeapSingle * p )
{
    int i;
    i = 0;
	free( p->pTree );
    i = 1;
	free( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSinglePrint( FILE * pFile, Lxu_HeapSingle * p )
{
	Lxu_Single * pSingle;
	int Counter = 1;
	int Degree  = 1;

	Lxu_HeapSingleCheck( p );
	fprintf( pFile, "The contents of the heap:\n" );
	fprintf( pFile, "Level %d:  ", Degree );
	Lxu_HeapSingleForEachItem( p, pSingle )
	{
		assert( Counter == p->pTree[Counter]->HNum );
		fprintf( pFile, "%2d=%3g  ", Counter, LXU_HEAP_SINGLE_WEIGHT(p->pTree[Counter]) );
		if ( ++Counter == (1 << Degree) )
		{
			fprintf( pFile, "\n" );
			Degree++;
			fprintf( pFile, "Level %d:  ", Degree );
		}
	}
	fprintf( pFile, "\n" );
	fprintf( pFile, "End of the heap printout.\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleCheck( Lxu_HeapSingle * p )
{
	Lxu_Single * pSingle;
	Lxu_HeapSingleForEachItem( p, pSingle )
	{
		assert( pSingle->HNum == p->i );
        Lxu_HeapSingleCheckOne( p, pSingle );
	}
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleCheckOne( Lxu_HeapSingle * p, Lxu_Single * pSingle )
{
    weightType Weight1, Weight2;
	if ( LXU_HEAP_SINGLE_CHILD1_EXISTS(p,pSingle) )
	{
        Weight1 = LXU_HEAP_SINGLE_WEIGHT(pSingle);
        Weight2 = LXU_HEAP_SINGLE_WEIGHT( LXU_HEAP_SINGLE_CHILD1(p,pSingle) );
        assert( Weight1 >= Weight2 );
	}
	if ( LXU_HEAP_SINGLE_CHILD2_EXISTS(p,pSingle) )
	{
        Weight1 = LXU_HEAP_SINGLE_WEIGHT(pSingle);
        Weight2 = LXU_HEAP_SINGLE_WEIGHT( LXU_HEAP_SINGLE_CHILD2(p,pSingle) );
        assert( Weight1 >= Weight2 );
	}
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleInsert( Lxu_HeapSingle * p, Lxu_Single * pSingle )
{
	if ( p->nItems == p->nItemsAlloc )
		Lxu_HeapSingleResize( p );
	// put the last entry to the last place and move up
	p->pTree[++p->nItems] = pSingle;
	pSingle->HNum = p->nItems;
	// move the last entry up if necessary
	Lxu_HeapSingleMoveUp( p, pSingle );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleUpdate( Lxu_HeapSingle * p, Lxu_Single * pSingle )
{
	LXU_HEAP_SINGLE_ASSERT(p,pSingle);
	if ( LXU_HEAP_SINGLE_PARENT_EXISTS(p,pSingle) && 
         LXU_HEAP_SINGLE_WEIGHT(pSingle) > LXU_HEAP_SINGLE_WEIGHT( LXU_HEAP_SINGLE_PARENT(p,pSingle) ) )
		Lxu_HeapSingleMoveUp( p, pSingle );
	else if ( LXU_HEAP_SINGLE_CHILD1_EXISTS(p,pSingle) && 
        LXU_HEAP_SINGLE_WEIGHT(pSingle) < LXU_HEAP_SINGLE_WEIGHT( LXU_HEAP_SINGLE_CHILD1(p,pSingle) ) )
		Lxu_HeapSingleMoveDn( p, pSingle );
	else if ( LXU_HEAP_SINGLE_CHILD2_EXISTS(p,pSingle) && 
        LXU_HEAP_SINGLE_WEIGHT(pSingle) < LXU_HEAP_SINGLE_WEIGHT( LXU_HEAP_SINGLE_CHILD2(p,pSingle) ) )
		Lxu_HeapSingleMoveDn( p, pSingle );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleDelete( Lxu_HeapSingle * p, Lxu_Single * pSingle )
{
    int Place = pSingle->HNum;
	LXU_HEAP_SINGLE_ASSERT(p,pSingle);
	// put the last entry to the deleted place
	// decrement the number of entries
	p->pTree[Place] = p->pTree[p->nItems--];
	p->pTree[Place]->HNum = Place;
	// move the top entry down if necessary
	Lxu_HeapSingleUpdate( p, p->pTree[Place] );
    pSingle->HNum = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Single * Lxu_HeapSingleReadMax( Lxu_HeapSingle * p )
{
	if ( p->nItems == 0 )
		return NULL;
	return p->pTree[1];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Single * Lxu_HeapSingleGetMax( Lxu_HeapSingle * p )
{
	Lxu_Single * pSingle;
	if ( p->nItems == 0 )
		return NULL;
	// prepare the return value
	pSingle = p->pTree[1];
	pSingle->HNum = 0;
	// put the last entry on top
	// decrement the number of entries
	p->pTree[1] = p->pTree[p->nItems--];
	p->pTree[1]->HNum = 1;
	// move the top entry down if necessary
	Lxu_HeapSingleMoveDn( p, p->pTree[1] );
	return pSingle;	 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
weightType  Lxu_HeapSingleReadMaxWeight( Lxu_HeapSingle * p )
{
	if ( p->nItems == 0 )
		return -1;
	return LXU_HEAP_SINGLE_WEIGHT(p->pTree[1]);
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleSwap( Lxu_Single ** pSingle1, Lxu_Single ** pSingle2 )
{
	Lxu_Single * pSingle;
	int Temp;
	pSingle   = *pSingle1;
	*pSingle1 = *pSingle2;
	*pSingle2 = pSingle;
	Temp          = (*pSingle1)->HNum;
	(*pSingle1)->HNum = (*pSingle2)->HNum;
	(*pSingle2)->HNum = Temp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleMoveUp( Lxu_HeapSingle * p, Lxu_Single * pSingle )
{
	Lxu_Single ** ppSingle, ** ppPar;
	ppSingle = &LXU_HEAP_SINGLE_CURRENT(p, pSingle);
	while ( LXU_HEAP_SINGLE_PARENT_EXISTS(p,*ppSingle) )
	{
		ppPar = &LXU_HEAP_SINGLE_PARENT(p,*ppSingle);
		if ( LXU_HEAP_SINGLE_WEIGHT(*ppSingle) > LXU_HEAP_SINGLE_WEIGHT(*ppPar) )
		{
			Lxu_HeapSingleSwap( ppSingle, ppPar );
			ppSingle = ppPar;
		}
		else
			break;
	}
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapSingleMoveDn( Lxu_HeapSingle * p, Lxu_Single * pSingle )
{
	Lxu_Single ** ppChild1, ** ppChild2, ** ppSingle;
	ppSingle = &LXU_HEAP_SINGLE_CURRENT(p, pSingle);
	while ( LXU_HEAP_SINGLE_CHILD1_EXISTS(p,*ppSingle) )
	{ // if Child1 does not exist, Child2 also does not exists

		// get the children
		ppChild1 = &LXU_HEAP_SINGLE_CHILD1(p,*ppSingle);
        if ( LXU_HEAP_SINGLE_CHILD2_EXISTS(p,*ppSingle) )
        {
            ppChild2 = &LXU_HEAP_SINGLE_CHILD2(p,*ppSingle);

            // consider two cases
            if ( LXU_HEAP_SINGLE_WEIGHT(*ppSingle) >= LXU_HEAP_SINGLE_WEIGHT(*ppChild1) &&
                 LXU_HEAP_SINGLE_WEIGHT(*ppSingle) >= LXU_HEAP_SINGLE_WEIGHT(*ppChild2) )
            { // Var is larger than both, skip
                break;
            }
            else
            { // Var is smaller than one of them, then swap it with larger 
                if ( LXU_HEAP_SINGLE_WEIGHT(*ppChild1) >= LXU_HEAP_SINGLE_WEIGHT(*ppChild2) )
                {
			        Lxu_HeapSingleSwap( ppSingle, ppChild1 );
		            // update the pointer
		            ppSingle = ppChild1;
                }
                else
                {
			        Lxu_HeapSingleSwap( ppSingle, ppChild2 );
		            // update the pointer
		            ppSingle = ppChild2;
                }
            }
        }
        else // Child2 does not exist
        {
            // consider two cases
            if ( LXU_HEAP_SINGLE_WEIGHT(*ppSingle) >= LXU_HEAP_SINGLE_WEIGHT(*ppChild1) )
            { // Var is larger than Child1, skip
                break;
            }
            else
            { // Var is smaller than Child1, then swap them
			    Lxu_HeapSingleSwap( ppSingle, ppChild1 );
		        // update the pointer
		        ppSingle = ppChild1;
            }
        }
	}
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
