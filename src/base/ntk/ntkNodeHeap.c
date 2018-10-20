/**CFile****************************************************************

  FileName    [ntkNodeHeap.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The priority queue for nodes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkNodeHeap.c,v 1.10 2003/09/01 04:56:00 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the heap of nodes by weight assigned to pNode->pCopy
struct NtkNodeHeapStruct
{
    Ntk_Node_t **    pTree;
    int              nItems;
    int              nItemsAlloc;
    int              i;
};

#define NTK_NODE_HEAP_WEIGHT(pNode)           ((int)(pNode)->pCopy)
#define NTK_NODE_HEAP_NUMBER(pNode)           ((int)(pNode)->pData)
#define NTK_NODE_HEAP_NUMBER_SET(pNode,Num)   ((pNode)->pData = ((char *)Num))
#define NTK_NODE_HEAP_CURRENT(p, pNode)       ((p)->pTree[NTK_NODE_HEAP_NUMBER(pNode)])
#define NTK_NODE_HEAP_PARENT_EXISTS(p, pNode) (NTK_NODE_HEAP_NUMBER(pNode) > 1)
#define NTK_NODE_HEAP_CHILD1_EXISTS(p, pNode) ((NTK_NODE_HEAP_NUMBER(pNode) << 1) <= p->nItems)
#define NTK_NODE_HEAP_CHILD2_EXISTS(p, pNode) (((NTK_NODE_HEAP_NUMBER(pNode) << 1)+1) <= p->nItems)
#define NTK_NODE_HEAP_PARENT(p, pNode)        ((p)->pTree[NTK_NODE_HEAP_NUMBER(pNode) >> 1])
#define NTK_NODE_HEAP_CHILD1(p, pNode)        ((p)->pTree[NTK_NODE_HEAP_NUMBER(pNode) << 1])
#define NTK_NODE_HEAP_CHILD2(p, pNode)        ((p)->pTree[(NTK_NODE_HEAP_NUMBER(pNode) << 1)+1])
#define NTK_NODE_HEAP_ASSERT(p, pNode)        assert( NTK_NODE_HEAP_NUMBER(pNode) >= 1 && NTK_NODE_HEAP_NUMBER(pNode) <= p->nItemsAlloc )

static void Ntk_NodeHeapResize( Ntk_NodeHeap_t * p );                  
static void Ntk_NodeHeapSwap( Ntk_Node_t ** pNode1, Ntk_Node_t ** pNode2 );  
static void Ntk_NodeHeapMoveUp( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode );  
static void Ntk_NodeHeapMoveDn( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode );  

#define Ntk_NodeHeapForEachItem( Heap, Node )\
    for ( Heap->i = 1;\
          Heap->i <= Heap->nItems && (Node = Heap->pTree[Heap->i]);\
          Heap->i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_NodeHeap_t * Ntk_NodeHeapStart()
{
    Ntk_NodeHeap_t * p;
    p = ALLOC( Ntk_NodeHeap_t, 1 );
    memset( p, 0, sizeof(Ntk_NodeHeap_t) );
    p->nItems      = 0;
    p->nItemsAlloc = 10000;
    p->pTree       = ALLOC( Ntk_Node_t *, p->nItemsAlloc + 1 );
    p->pTree[0]    = NULL;
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeHeapResize( Ntk_NodeHeap_t * p )
{
    p->nItemsAlloc *= 2;
    p->pTree = REALLOC( Ntk_Node_t *, p->pTree, p->nItemsAlloc + 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeHeapStop( Ntk_NodeHeap_t * p )
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
void Ntk_NodeHeapPrint( FILE * pFile, Ntk_NodeHeap_t * p )
{
    Ntk_Node_t * pNode;
    int Counter = 1;
    int Degree  = 1;

    Ntk_NodeHeapCheck( p );
    fprintf( pFile, "The contents of the heap:\n" );
    fprintf( pFile, "Level %d:  ", Degree );
    Ntk_NodeHeapForEachItem( p, pNode )
    {
        assert( Counter == NTK_NODE_HEAP_NUMBER(p->pTree[Counter]) );
        fprintf( pFile, "%2d=%3d  ", Counter, NTK_NODE_HEAP_WEIGHT(p->pTree[Counter]) );
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
void Ntk_NodeHeapCheck( Ntk_NodeHeap_t * p )
{
    Ntk_Node_t * pNode;
    Ntk_NodeHeapForEachItem( p, pNode )
    {
        assert( NTK_NODE_HEAP_NUMBER(pNode) == p->i );
        Ntk_NodeHeapCheckOne( p, pNode );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeHeapCheckOne( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode )
{
    int Weight1, Weight2;
    if ( NTK_NODE_HEAP_CHILD1_EXISTS(p,pNode) )
    {
        Weight1 = NTK_NODE_HEAP_WEIGHT(pNode);
        Weight2 = NTK_NODE_HEAP_WEIGHT( NTK_NODE_HEAP_CHILD1(p,pNode) );
        assert( Weight1 >= Weight2 );
    }
    if ( NTK_NODE_HEAP_CHILD2_EXISTS(p,pNode) )
    {
        Weight1 = NTK_NODE_HEAP_WEIGHT(pNode);
        Weight2 = NTK_NODE_HEAP_WEIGHT( NTK_NODE_HEAP_CHILD2(p,pNode) );
        assert( Weight1 >= Weight2 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeHeapInsert( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode )
{
    if ( p->nItems == p->nItemsAlloc )
        Ntk_NodeHeapResize( p );
    // put the last entry to the last place and move up
    p->pTree[++p->nItems] = pNode;
    NTK_NODE_HEAP_NUMBER_SET( pNode, p->nItems );
    // move the last entry up if necessary
    Ntk_NodeHeapMoveUp( p, pNode );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeHeapUpdate( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode )
{
//printf( "Updating divisor %d.\n", pNode->Num );

    NTK_NODE_HEAP_ASSERT(p,pNode);
    if ( NTK_NODE_HEAP_PARENT_EXISTS(p,pNode) && 
         NTK_NODE_HEAP_WEIGHT(pNode) > NTK_NODE_HEAP_WEIGHT( NTK_NODE_HEAP_PARENT(p,pNode) ) )
        Ntk_NodeHeapMoveUp( p, pNode );
    else if ( NTK_NODE_HEAP_CHILD1_EXISTS(p,pNode) && 
        NTK_NODE_HEAP_WEIGHT(pNode) < NTK_NODE_HEAP_WEIGHT( NTK_NODE_HEAP_CHILD1(p,pNode) ) )
        Ntk_NodeHeapMoveDn( p, pNode );
    else if ( NTK_NODE_HEAP_CHILD2_EXISTS(p,pNode) && 
        NTK_NODE_HEAP_WEIGHT(pNode) < NTK_NODE_HEAP_WEIGHT( NTK_NODE_HEAP_CHILD2(p,pNode) ) )
        Ntk_NodeHeapMoveDn( p, pNode );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeHeapDelete( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode )
{
    int HNum;
    NTK_NODE_HEAP_ASSERT(p,pNode);
    HNum = NTK_NODE_HEAP_NUMBER(pNode);
    // put the last entry to the deleted place
    // decrement the number of entries
    p->pTree[HNum] = p->pTree[p->nItems--];
    NTK_NODE_HEAP_NUMBER_SET( p->pTree[HNum], HNum );
    // move the top entry down if necessary
    Ntk_NodeHeapUpdate( p, p->pTree[HNum] );
    NTK_NODE_HEAP_NUMBER_SET( pNode, 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeHeapReadMax( Ntk_NodeHeap_t * p )
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
Ntk_Node_t * Ntk_NodeHeapGetMax( Ntk_NodeHeap_t * p )
{
    Ntk_Node_t * pNode;
    if ( p->nItems == 0 )
        return NULL;
    // prepare the return value
    pNode = p->pTree[1];
    NTK_NODE_HEAP_NUMBER_SET( pNode, 0 );
    // put the last entry on top
    // decrement the number of entries
    p->pTree[1] = p->pTree[p->nItems--];
    NTK_NODE_HEAP_NUMBER_SET( p->pTree[1], 1 );
    // move the top entry down if necessary
    Ntk_NodeHeapMoveDn( p, p->pTree[1] );
    return pNode;    
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Ntk_NodeHeapReadMaxWeight( Ntk_NodeHeap_t * p )
{
    if ( p->nItems == 0 )
        return -1;
    else
        return NTK_NODE_HEAP_WEIGHT(p->pTree[1]);
}


/**Function*************************************************************

  Synopsis    [Returns the number of nodes having weight above the limit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Ntk_NodeHeapCountNodes( Ntk_NodeHeap_t * p, int WeightLimit )
{
    Ntk_Node_t * pNode;
    int Counter;
    Counter = 0;
    Ntk_NodeHeapForEachItem( p, pNode )
        if ( NTK_NODE_HEAP_WEIGHT(pNode) >= WeightLimit )
            Counter++;
    return Counter;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeHeapSwap( Ntk_Node_t ** pNode1, Ntk_Node_t ** pNode2 )
{
    Ntk_Node_t * pNode;
    int HNum1, HNum2;
    pNode   = *pNode1;
    *pNode1 = *pNode2;
    *pNode2 = pNode;
    HNum1   = NTK_NODE_HEAP_NUMBER( *pNode1 );
    HNum2   = NTK_NODE_HEAP_NUMBER( *pNode2 );
    NTK_NODE_HEAP_NUMBER_SET( *pNode1, HNum2 );
    NTK_NODE_HEAP_NUMBER_SET( *pNode2, HNum1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeHeapMoveUp( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode )
{
    Ntk_Node_t ** ppNode, ** ppPar;
    ppNode = &NTK_NODE_HEAP_CURRENT(p, pNode);
    while ( NTK_NODE_HEAP_PARENT_EXISTS(p,*ppNode) )
    {
        ppPar = &NTK_NODE_HEAP_PARENT(p,*ppNode);
        if ( NTK_NODE_HEAP_WEIGHT(*ppNode) > NTK_NODE_HEAP_WEIGHT(*ppPar) )
        {
            Ntk_NodeHeapSwap( ppNode, ppPar );
            ppNode = ppPar;
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
void Ntk_NodeHeapMoveDn( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode )
{
    Ntk_Node_t ** ppChild1, ** ppChild2, ** ppNode;
    ppNode = &NTK_NODE_HEAP_CURRENT(p, pNode);
    while ( NTK_NODE_HEAP_CHILD1_EXISTS(p,*ppNode) )
    { // if Child1 does not exist, Child2 also does not exists

        // get the children
        ppChild1 = &NTK_NODE_HEAP_CHILD1(p,*ppNode);
        if ( NTK_NODE_HEAP_CHILD2_EXISTS(p,*ppNode) )
        {
            ppChild2 = &NTK_NODE_HEAP_CHILD2(p,*ppNode);

            // consider two cases
            if ( NTK_NODE_HEAP_WEIGHT(*ppNode) >= NTK_NODE_HEAP_WEIGHT(*ppChild1) &&
                 NTK_NODE_HEAP_WEIGHT(*ppNode) >= NTK_NODE_HEAP_WEIGHT(*ppChild2) )
            { // Div is larger than both, skip
                break;
            }
            else
            { // Div is smaller than one of them, then swap it with larger 
                if ( NTK_NODE_HEAP_WEIGHT(*ppChild1) >= NTK_NODE_HEAP_WEIGHT(*ppChild2) )
                {
                    Ntk_NodeHeapSwap( ppNode, ppChild1 );
                    // update the pointer
                    ppNode = ppChild1;
                }
                else
                {
                    Ntk_NodeHeapSwap( ppNode, ppChild2 );
                    // update the pointer
                    ppNode = ppChild2;
                }
            }
        }
        else // Child2 does not exist
        {
            // consider two cases
            if ( NTK_NODE_HEAP_WEIGHT(*ppNode) >= NTK_NODE_HEAP_WEIGHT(*ppChild1) )
            { // Div is larger than Child1, skip
                break;
            }
            else
            { // Div is smaller than Child1, then swap them
                Ntk_NodeHeapSwap( ppNode, ppChild1 );
                // update the pointer
                ppNode = ppChild1;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


