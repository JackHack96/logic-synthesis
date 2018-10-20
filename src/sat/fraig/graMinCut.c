/**CFile****************************************************************

  FileName    [graMinCut.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Generic graph data struture.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: graMinCut.c,v 1.2 2005/07/08 01:01:34 alanmi Exp $]

***********************************************************************/

#include "graInt.h"

/* 
Implementation is based on the paper: D. H. Lee, S. M. Reddy, "On determining 
scan flip-flops in partial-scan designs", Proc. ICCAD '90, pp. 322-325.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define GRA_STACK_ADD( pNode )      ((pNode)->fMark1 = 1)
#define GRA_STACK_REM( pNode )      ((pNode)->fMark1 = 0)
#define GRA_STACK_TEST( pNode )     ((pNode)->fMark1)

static void         Gra_GraphRemoveNode( Gra_Graph_t * p, Gra_Node_t * pNode, Gra_NodeVec_t * vStack );
static void         Gra_ManCutSetReduce( Gra_Graph_t * p, Gra_NodeVec_t * vStack, Gra_NodeVec_t * vMinCut );
static Gra_Node_t * Gra_ManCutSetSelect( Gra_Graph_t * p, Gra_NodeVec_t * vStack );

static void         Gra_NodeStackPush( Gra_NodeVec_t * pStack, Gra_Node_t * pNode );
static Gra_Node_t * Gra_NodeStackPop( Gra_NodeVec_t * pStack );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the minimal cutset of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_GraphMinCut( Gra_Graph_t * p, Gra_Node_t * pHost )
{
    Gra_NodeVec_t * vStack;          // the stack of nodes to be processed
    Gra_NodeVec_t * vMinCut;         // the set of mincut nodes
    Gra_Node_t * pNode;

    // reduce the list iteratively
    vStack = Gra_NodeVecAlloc( 100 );
    vMinCut = Gra_NodeVecAlloc( 100 );
    // start by removing the host node
    pNode = pHost;
    while ( 1 )
    {
        Gra_GraphRemoveNode( p, pNode, vStack );
        Gra_ManCutSetReduce( p, vStack, vMinCut );
        if ( p->lNodes.nItems == 0 )
            break;
        pNode = Gra_ManCutSetSelect( p, vStack );
        Gra_NodeVecPush( vMinCut, (Gra_Node_t *)pNode->Num );
    }
    Gra_NodeVecFree( vStack );

/*
{
    int i;
    printf( "Outputs " );
    for ( i = 0; i < p->nOutputs; i++ )
        printf( "%d ", Gra_Regular(p->pOutputs[i])->Num );
    printf( "\n" );
    printf( "Cutset " );
    for ( i = 0; i < vMinCut->nSize; i++ )
        printf( "%d ", vMinCut->pArray[i]->Num );
    printf( "\n" );
 
}
*/
    printf( "CutSet size = %d.\n", vMinCut->nSize );
    Gra_NodeVecFree( vMinCut );
}

/**Function*************************************************************

  Synopsis    [Removes the node and puts related nodes on the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_GraphRemoveNode( Gra_Graph_t * p, Gra_Node_t * pNode, Gra_NodeVec_t * vStack )
{
    int i;
//printf( "Removing node %d.\n", pNode->Num );
    // put fanins and fanouts on the stack
    for ( i = 0; i < pNode->vEdgesI.nSize; i++ )
        if ( pNode->vEdgesI.pArray[i] != pNode )
            Gra_NodeStackPush( vStack, pNode->vEdgesI.pArray[i] );
    for ( i = 0; i < pNode->vEdgesO.nSize; i++ )
        if ( pNode->vEdgesO.pArray[i] != pNode )
            Gra_NodeStackPush( vStack, pNode->vEdgesO.pArray[i] );
    // delete the node from the graph
    Gra_NodeDelete( pNode );
}



/**Function*************************************************************

  Synopsis    [Reduces the graph by removing nodes.]

  Description [Removes the nodes that have in- or out-degree less than 2.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_ManCutSetReduce( Gra_Graph_t * p, Gra_NodeVec_t * vStack, Gra_NodeVec_t * vMinCut )
{
    Gra_Node_t * pNode;
    int i;
    // go through the entries in the stack
//    assert( vStack->nSize > 0 );
    while ( vStack->nSize )
    {
        // take the next entry from the stack
        pNode = Gra_NodeStackPop( vStack );
        // check if the node has a self-loop, if so, add it to the solution
        if ( Gra_NodeHasSelfLoop( pNode ) )
        {
            Gra_NodeVecPush( vMinCut, (Gra_Node_t *)pNode->Num );
            Gra_GraphRemoveNode( p, pNode, vStack );
            continue;
        }
        // if it has only one fanin or fanout, collapse it
        if ( pNode->vEdgesI.nSize < 2 ||  pNode->vEdgesO.nSize < 2 )
        {
            // put fanins and fanouts on the stack
            for ( i = 0; i < pNode->vEdgesI.nSize; i++ )
                Gra_NodeStackPush( vStack, pNode->vEdgesI.pArray[i] );
            for ( i = 0; i < pNode->vEdgesO.nSize; i++ )
                Gra_NodeStackPush( vStack, pNode->vEdgesO.pArray[i] );
            // collapse the node
            Gra_NodeCollapse( pNode );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Selects the node with the max sum of fanin/fanout degrees.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_Node_t * Gra_ManCutSetSelect( Gra_Graph_t * p, Gra_NodeVec_t * vStack )
{
    Gra_Node_t * pNodeMax, * pNode;
    int CounterMax, CounterCur;

    CounterMax = -1;
    Gra_NodeListForEachNode( &p->lNodes, pNode )
    {
        // get the sum of in- and out-degrees
        CounterCur = pNode->vEdgesI.nSize + pNode->vEdgesO.nSize;
        // compare
        if ( CounterMax < CounterCur )
        {
            CounterMax = CounterCur;
            pNodeMax = pNode;
        }
    }
    assert( CounterMax >= 0 );
    return pNodeMax;
}


/**Function*************************************************************

  Synopsis    [Put the node on the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeStackPush( Gra_NodeVec_t * pStack, Gra_Node_t * pNode )
{
    if ( GRA_STACK_TEST(pNode) )
        return;
    GRA_STACK_ADD( pNode );
    Gra_NodeVecPush( pStack, pNode );
}

/**Function*************************************************************

  Synopsis    [Remove the node from the stack.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_Node_t * Gra_NodeStackPop( Gra_NodeVec_t * pStack )
{
    Gra_Node_t * pNode;
    pNode = Gra_NodeVecPop( pStack );
    assert( GRA_STACK_TEST(pNode) );
    GRA_STACK_REM( pNode );
    return pNode;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


