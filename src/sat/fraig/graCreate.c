/**CFile****************************************************************

  FileName    [graCreate.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Generic graph data struture.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: graCreate.c,v 1.2 2005/07/08 01:01:34 alanmi Exp $]

***********************************************************************/

#include "graInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define GRA_EDGE_HASH(s1,s2)   ((((unsigned)s1) << 16) | (((unsigned)s2) & 0xffff))
#define GRA_EDGE_UNHASH1(s)     (((unsigned)s1) >> 16)
#define GRA_EDGE_UNHASH2(s)     (((unsigned)s1) & 0xffff)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_Graph_t * Gra_GraphCreate()
{
    Gra_Graph_t * p;
    p = ALLOC( Gra_Graph_t, 1 );
    memset( p, 0, sizeof(Gra_Graph_t) );
    p->tEdges = st_init_table(st_numcmp,st_numhash);
    return p;
}

/**Function*************************************************************

  Synopsis    [Delete the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_GraphDelete( Gra_Graph_t * p )
{
    Gra_Node_t * pNode, * pNode2;
    Gra_NodeListForEachNodeSafe( &p->lNodes, pNode, pNode2 )
        Gra_NodeDelete( pNode );
    st_free_table( p->tEdges );
    free( p );
}



/**Function*************************************************************

  Synopsis    [Create the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_Node_t * Gra_NodeCreate( Gra_Graph_t * p, int Num )
{
    Gra_Node_t * pNode;
    pNode = ALLOC( Gra_Node_t, 1 );
    memset( pNode, 0, sizeof(Gra_Node_t) );
    pNode->pGraph = p;
    pNode->Num = Num;
    Gra_NodeListAddLast( &pNode->pGraph->lNodes, pNode );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Delete the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeDelete( Gra_Node_t * pNode )
{
    Gra_Node_t * pNodeI, * pNodeO;
    int i;
    // remove the incoming edges
    for ( i = 0; i < pNode->vEdgesI.nSize; i++ )
    {
        pNodeI = pNode->vEdgesI.pArray[i];
        Gra_EdgeDelete( pNodeI, pNode );
        Gra_NodeVecRemove( &pNodeI->vEdgesO, pNode );
    }
    // remove the outgoing edges
    for ( i = 0; i < pNode->vEdgesO.nSize; i++ )
    {
        pNodeO = pNode->vEdgesO.pArray[i];
        Gra_EdgeDelete( pNode, pNodeO );
        Gra_NodeVecRemove( &pNodeO->vEdgesI, pNode );
    }
    Gra_NodeListDelete( &pNode->pGraph->lNodes, pNode );
    free( pNode );
}



/**Function*************************************************************

  Synopsis    [Create the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gra_EdgeCreate( Gra_Node_t * pNodeI, Gra_Node_t * pNodeO )
{
    Gra_Graph_t * p = pNodeI->pGraph; 
    unsigned Edge;
    Edge = GRA_EDGE_HASH( pNodeI->Num, pNodeO->Num );
    if ( st_is_member( p->tEdges, (char *)Edge ) )
        return 1;
    st_insert( p->tEdges, (char *)Edge, NULL );
    Gra_NodeVecPush( &pNodeI->vEdgesO, pNodeO );
    Gra_NodeVecPush( &pNodeO->vEdgesI, pNodeI );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Delete the edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_EdgeDelete( Gra_Node_t * pNodeI, Gra_Node_t * pNodeO )
{
    Gra_Graph_t * p = pNodeI->pGraph; 
    unsigned Edge;
    Edge = GRA_EDGE_HASH( pNodeI->Num, pNodeO->Num );
    assert( st_is_member( p->tEdges, (char *)Edge ) );
    st_delete( p->tEdges, (char **)&Edge, NULL );
}


/**Function*************************************************************

  Synopsis    [Collapses nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_NodeCollapse( Gra_Node_t * pNode )
{
    Gra_Graph_t * p = pNode->pGraph; 
    int i, k;
//printf( "Collapsing node %d.\n", pNode->Num );

    // for each of the fanouts, add the fanin edges
    for ( i = 0; i < pNode->vEdgesI.nSize; i++ )
        for ( k = 0; k < pNode->vEdgesO.nSize; k++ )
            Gra_EdgeCreate( pNode->vEdgesI.pArray[i], pNode->vEdgesO.pArray[k] );
    Gra_NodeDelete( pNode );
}

/**Function*************************************************************

  Synopsis    [Check if the node has a self-loop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gra_NodeHasSelfLoop( Gra_Node_t * pNode )
{
    int i;
    for ( i = 0; i < pNode->vEdgesI.nSize; i++ )
        if ( pNode->vEdgesI.pArray[i] == pNode )
            return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Prints the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gra_GraphPrint( Gra_Graph_t * p )
{
    Gra_Node_t * pNode;
    int i;
    printf( "Graph has %d nodes:\n", p->lNodes.nItems );
    Gra_NodeListForEachNode( &p->lNodes, pNode )
    {
        printf( "Node %2d <-- ", pNode->Num );
        for ( i = 0; i < pNode->vEdgesI.nSize; i++ )
            printf( "%d ", pNode->vEdgesI.pArray[i]->Num );
        printf( "\n" );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


