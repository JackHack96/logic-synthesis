/**CFile****************************************************************

  FileName    [lxuHeapD.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [The priority queue for double cube divisors.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuHeapD.c,v 1.6 2004/03/10 03:23:21 satrajit Exp $]

***********************************************************************/

#include "lxuInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define LXU_HEAP_DOUBLE_WEIGHT(pDiv)           ((pDiv)->Weight)
#define LXU_HEAP_DOUBLE_CURRENT(p, pDiv)       ((p)->pTree[(pDiv)->HNum])
#define LXU_HEAP_DOUBLE_PARENT_EXISTS(p, pDiv) ((pDiv)->HNum > 1)
#define LXU_HEAP_DOUBLE_CHILD1_EXISTS(p, pDiv) (((pDiv)->HNum << 1) <= p->nItems)
#define LXU_HEAP_DOUBLE_CHILD2_EXISTS(p, pDiv) ((((pDiv)->HNum << 1)+1) <= p->nItems)
#define LXU_HEAP_DOUBLE_PARENT(p, pDiv)        ((p)->pTree[(pDiv)->HNum >> 1])
#define LXU_HEAP_DOUBLE_CHILD1(p, pDiv)        ((p)->pTree[(pDiv)->HNum << 1])
#define LXU_HEAP_DOUBLE_CHILD2(p, pDiv)        ((p)->pTree[((pDiv)->HNum << 1)+1])
#define LXU_HEAP_DOUBLE_ASSERT(p, pDiv)        assert( (pDiv)->HNum >= 1 && (pDiv)->HNum <= p->nItemsAlloc )

static void Lxu_HeapDoubleResize( Lxu_HeapDouble * p );                  
static void Lxu_HeapDoubleSwap( Lxu_Double ** pDiv1, Lxu_Double ** pDiv2 );  
static void Lxu_HeapDoubleMoveUp( Lxu_HeapDouble * p, Lxu_Double * pDiv );  
static void Lxu_HeapDoubleMoveDn( Lxu_HeapDouble * p, Lxu_Double * pDiv );  

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_HeapDouble * Lxu_HeapDoubleStart()
{
	Lxu_HeapDouble * p;
	p = ALLOC( Lxu_HeapDouble, 1 );
	memset( p, 0, sizeof(Lxu_HeapDouble) );
	p->nItems      = 0;
	p->nItemsAlloc = 10000;
	p->pTree       = ALLOC( Lxu_Double *, p->nItemsAlloc + 1 );
	p->pTree[0]    = NULL;
	return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapDoubleResize( Lxu_HeapDouble * p )
{
	p->nItemsAlloc *= 2;
	p->pTree = REALLOC( Lxu_Double *, p->pTree, p->nItemsAlloc + 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapDoubleStop( Lxu_HeapDouble * p )
{
	free( p->pTree );
	free( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapDoublePrint( FILE * pFile, Lxu_HeapDouble * p )
{
	Lxu_Double * pDiv;
	int Counter = 1;
	int Degree  = 1;

	//Lxu_HeapDoubleCheck( p );
	fprintf( pFile, "The contents of the heap:\n" );
	fprintf( pFile, "Level %d:  ", Degree );
	Lxu_HeapDoubleForEachItem( p, pDiv )
	{
		assert( Counter == p->pTree[Counter]->HNum );
		fprintf( pFile, "%2d=%3g  ", Counter, LXU_HEAP_DOUBLE_WEIGHT(p->pTree[Counter]) );
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
void Lxu_HeapDoubleCheck( Lxu_HeapDouble * p )
{
	Lxu_Double * pDiv;
	Lxu_HeapDoubleForEachItem( p, pDiv )
	{
		assert( pDiv->HNum == p->i );
        Lxu_HeapDoubleCheckOne( p, pDiv );
	}
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapDoubleCheckOne( Lxu_HeapDouble * p, Lxu_Double * pDiv )
{
    weightType Weight1, Weight2;
	if ( LXU_HEAP_DOUBLE_CHILD1_EXISTS(p,pDiv) )
	{
        Weight1 = LXU_HEAP_DOUBLE_WEIGHT(pDiv);
        Weight2 = LXU_HEAP_DOUBLE_WEIGHT( LXU_HEAP_DOUBLE_CHILD1(p,pDiv) );
		// TODO: fix this
		if (!(Weight1 >= Weight2)) {
			fprintf(stderr, "DEBUG: Child1: Weight1 = %g, Weight2 = %g\n", Weight1, Weight2);
			fflush(stderr);
		}
        assert( Weight1 >= Weight2 );
	}
	if ( LXU_HEAP_DOUBLE_CHILD2_EXISTS(p,pDiv) )
	{
        Weight1 = LXU_HEAP_DOUBLE_WEIGHT(pDiv);
        Weight2 = LXU_HEAP_DOUBLE_WEIGHT( LXU_HEAP_DOUBLE_CHILD2(p,pDiv) );
		// TODO: fix this
		if (!(Weight1 >= Weight2)) {
			fprintf(stderr, "DEBUG: Child2: Weight1 = %g, Weight2 = %g\n", Weight1, Weight2);
			fflush(stderr);
		}
        assert( Weight1 >= Weight2 );
	}
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapDoubleInsert( Lxu_HeapDouble * p, Lxu_Double * pDiv )
{
	if ( p->nItems == p->nItemsAlloc )
		Lxu_HeapDoubleResize( p );
	// put the last entry to the last place and move up
	p->pTree[++p->nItems] = pDiv;
	pDiv->HNum = p->nItems;
	// move the last entry up if necessary
	Lxu_HeapDoubleMoveUp( p, pDiv );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapDoubleUpdate( Lxu_HeapDouble * p, Lxu_Double * pDiv )
{
//printf( "Updating divisor %d.\n", pDiv->Num );

	//fprintf (stderr, "%d\n", (pDiv)->HNum);
	//fflush (stderr);
	LXU_HEAP_DOUBLE_ASSERT(p,pDiv);
	if ( LXU_HEAP_DOUBLE_PARENT_EXISTS(p,pDiv) && 
         LXU_HEAP_DOUBLE_WEIGHT(pDiv) > LXU_HEAP_DOUBLE_WEIGHT( LXU_HEAP_DOUBLE_PARENT(p,pDiv) ) )
		Lxu_HeapDoubleMoveUp( p, pDiv );
	else if ( LXU_HEAP_DOUBLE_CHILD1_EXISTS(p,pDiv) && 
        LXU_HEAP_DOUBLE_WEIGHT(pDiv) < LXU_HEAP_DOUBLE_WEIGHT( LXU_HEAP_DOUBLE_CHILD1(p,pDiv) ) )
		Lxu_HeapDoubleMoveDn( p, pDiv );
	else if ( LXU_HEAP_DOUBLE_CHILD2_EXISTS(p,pDiv) && 
        LXU_HEAP_DOUBLE_WEIGHT(pDiv) < LXU_HEAP_DOUBLE_WEIGHT( LXU_HEAP_DOUBLE_CHILD2(p,pDiv) ) )
		Lxu_HeapDoubleMoveDn( p, pDiv );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapDoubleDelete( Lxu_HeapDouble * p, Lxu_Double * pDiv )
{
	LXU_HEAP_DOUBLE_ASSERT(p,pDiv);
	// put the last entry to the deleted place
	// decrement the number of entries
	p->pTree[pDiv->HNum] = p->pTree[p->nItems--];
	p->pTree[pDiv->HNum]->HNum = pDiv->HNum;
	// move the top entry down if necessary
	Lxu_HeapDoubleUpdate( p, p->pTree[pDiv->HNum] );
    pDiv->HNum = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lxu_Double * Lxu_HeapDoubleReadMax( Lxu_HeapDouble * p )
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
Lxu_Double * Lxu_HeapDoubleGetMax( Lxu_HeapDouble * p )
{
	Lxu_Double * pDiv;
	if ( p->nItems == 0 )
		return NULL;
	// prepare the return value
	pDiv = p->pTree[1];
	pDiv->HNum = 0;
	// put the last entry on top
	// decrement the number of entries
	p->pTree[1] = p->pTree[p->nItems--];
	p->pTree[1]->HNum = 1;
	// move the top entry down if necessary
	Lxu_HeapDoubleMoveDn( p, p->pTree[1] );
	return pDiv;	 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
weightType  Lxu_HeapDoubleReadMaxWeight( Lxu_HeapDouble * p )
{
	if ( p->nItems == 0 )
		return -1;
	else
		return LXU_HEAP_DOUBLE_WEIGHT(p->pTree[1]);
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapDoubleSwap( Lxu_Double ** pDiv1, Lxu_Double ** pDiv2 )
{
	Lxu_Double * pDiv;
	int Temp;
	pDiv   = *pDiv1;
	*pDiv1 = *pDiv2;
	*pDiv2 = pDiv;
	Temp          = (*pDiv1)->HNum;
	(*pDiv1)->HNum = (*pDiv2)->HNum;
	(*pDiv2)->HNum = Temp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_HeapDoubleMoveUp( Lxu_HeapDouble * p, Lxu_Double * pDiv )
{
	Lxu_Double ** ppDiv, ** ppPar;
	ppDiv = &LXU_HEAP_DOUBLE_CURRENT(p, pDiv);
	while ( LXU_HEAP_DOUBLE_PARENT_EXISTS(p,*ppDiv) )
	{
		ppPar = &LXU_HEAP_DOUBLE_PARENT(p,*ppDiv);
		if ( LXU_HEAP_DOUBLE_WEIGHT(*ppDiv) > LXU_HEAP_DOUBLE_WEIGHT(*ppPar) )
		{
			Lxu_HeapDoubleSwap( ppDiv, ppPar );
			ppDiv = ppPar;
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
void Lxu_HeapDoubleMoveDn( Lxu_HeapDouble * p, Lxu_Double * pDiv )
{
	Lxu_Double ** ppChild1, ** ppChild2, ** ppDiv;
	ppDiv = &LXU_HEAP_DOUBLE_CURRENT(p, pDiv);
	while ( LXU_HEAP_DOUBLE_CHILD1_EXISTS(p,*ppDiv) )
	{ // if Child1 does not exist, Child2 also does not exists

		// get the children
		ppChild1 = &LXU_HEAP_DOUBLE_CHILD1(p,*ppDiv);
        if ( LXU_HEAP_DOUBLE_CHILD2_EXISTS(p,*ppDiv) )
        {
            ppChild2 = &LXU_HEAP_DOUBLE_CHILD2(p,*ppDiv);

            // consider two cases
            if ( LXU_HEAP_DOUBLE_WEIGHT(*ppDiv) >= LXU_HEAP_DOUBLE_WEIGHT(*ppChild1) &&
                 LXU_HEAP_DOUBLE_WEIGHT(*ppDiv) >= LXU_HEAP_DOUBLE_WEIGHT(*ppChild2) )
            { // Div is larger than both, skip
                break;
            }
            else
            { // Div is smaller than one of them, then swap it with larger 
                if ( LXU_HEAP_DOUBLE_WEIGHT(*ppChild1) >= LXU_HEAP_DOUBLE_WEIGHT(*ppChild2) )
                {
			        Lxu_HeapDoubleSwap( ppDiv, ppChild1 );
		            // update the pointer
		            ppDiv = ppChild1;
                }
                else
                {
			        Lxu_HeapDoubleSwap( ppDiv, ppChild2 );
		            // update the pointer
		            ppDiv = ppChild2;
                }
            }
        }
        else // Child2 does not exist
        {
            // consider two cases
            if ( LXU_HEAP_DOUBLE_WEIGHT(*ppDiv) >= LXU_HEAP_DOUBLE_WEIGHT(*ppChild1) )
            { // Div is larger than Child1, skip
                break;
            }
            else
            { // Div is smaller than Child1, then swap them
			    Lxu_HeapDoubleSwap( ppDiv, ppChild1 );
		        // update the pointer
		        ppDiv = ppChild1;
            }
        }
	}
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


