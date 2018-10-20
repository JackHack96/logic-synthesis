/**CFile****************************************************************

  FileName    [cbApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [API's of the club package]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cbApi.c,v 1.2 2003/05/27 23:14:43 alanmi Exp $]

***********************************************************************/

#include "cb.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
#define VERTEX_DEGREE_MINIMUM      3
#define VERTEX_DEGREE_MAXIMUM     20
#define GRAPH_VERTICES_MINIMUM    20
#define GRAPH_VERTICES_MAXIMUM  1000


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

int Cb_GraphReadVertexNum( Cb_Graph_t *pG )
{
    return pG->nVertices;
}

int Cb_GraphReadRootsNum( Cb_Graph_t *pG ) 
{
    return pG->pRoots->num;
}

sarray_t * Cb_GraphReadRoots( Cb_Graph_t *pG ) 
{
    return pG->pRoots;
}

int Cb_GraphReadLeavesNum( Cb_Graph_t *pG ) 
{
    return pG->pLeaves->num;
}

sarray_t * Cb_GraphReadLeaves( Cb_Graph_t *pG ) 
{
    return pG->pLeaves;
}

void Cb_GraphAddRoot( Cb_Graph_t *pG, Cb_Vertex_t *pV ) 
{
    sarray_insert_last_safe( Cb_Vertex_t *, pG->pRoots, pV );
}

void Cb_GraphAddLeaf( Cb_Graph_t *pG, Cb_Vertex_t *pV )
{
    sarray_insert_last_safe( Cb_Vertex_t *, pG->pLeaves, pV );
}

Cb_Vertex_t * Cb_VertexReadInput( Cb_Vertex_t *pV, int i ) 
{
    return pV->pIn[i];
}

Cb_Vertex_t * Cb_VertexReadOutput( Cb_Vertex_t *pV, int i ) 
{
    return pV->pOut[i];
}

void Cb_VertexSetInput( Cb_Vertex_t *pV, int i, Cb_Vertex_t *pInput ) 
{
    pV->pIn[i] = pInput;
}

void Cb_VertexSetOutput( Cb_Vertex_t *pV, int i, Cb_Vertex_t *pOutput ) 
{
    pV->pOut[i] = pOutput;
}

bool Cb_VertexIsGraphRoot( Cb_Vertex_t *pV ) 
{
    return ( pV->nOut == 0 );
}

bool Cb_VertexIsGraphRootDriver( Cb_Vertex_t *pV )
{
    return ( pV->nOut == 1 && pV->pOut[0]->nOut == 0 );
}

bool Cb_VertexIsGraphRootFanin( Cb_Vertex_t *pV )
{
    int i;
    for ( i = 0; i < pV->nOut; ++i ) {
        if ( pV->pOut[i]->nOut == 0 )
            return TRUE;
    }
    return FALSE;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
Cb_Vertex_t *
Cb_VertexAlloc( int nIn, int nOut ) 
{
    Cb_Vertex_t * pV;
    pV = ALLOC( Cb_Vertex_t, 1 );
    memset( pV, 0, sizeof( Cb_Vertex_t ) );
    pV->nIn  = nIn;
    pV->nOut = nOut;
    pV->pIn  = ALLOC( Cb_Vertex_t *, nIn );
    pV->pOut = ALLOC( Cb_Vertex_t *, nOut );
    memset( pV->pIn,  0, nIn  * sizeof( Cb_Vertex_t * ) );
    memset( pV->pOut, 0, nOut * sizeof( Cb_Vertex_t * ) );
    return pV;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_VertexFree( Cb_Vertex_t *pV ) 
{
    FREE( pV->pIn );
    FREE( pV->pOut );
    FREE( pV );
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
Cb_Graph_t *
Cb_GraphAlloc( int nRoots, int nLeaves ) 
{
    Cb_Graph_t * pG;
    pG = ALLOC( Cb_Graph_t, 1 );
    memset( pG, 0, sizeof( Cb_Graph_t ) );
    pG->pRoots  = sarray_alloc( Cb_Vertex_t *, MAX(GRAPH_VERTICES_MINIMUM, nRoots));
    pG->pLeaves = sarray_alloc( Cb_Vertex_t *, MAX(GRAPH_VERTICES_MINIMUM, nLeaves));
    return pG;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_GraphFree( Cb_Graph_t *pG ) 
{
    sarray_free( pG->pRoots );
    sarray_free( pG->pLeaves );
    FREE( pG );
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_GraphFreeVertices( Cb_Graph_t *pG ) 
{
    Cb_Vertex_t  *pV, *pV2;
    Cb_GraphForEachVertexSafe( pG, pV, pV2 ) {
        Cb_VertexFree( pV );
    }
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_GraphAddVertexToTail( Cb_Graph_t *pG, Cb_Vertex_t *pV ) 
{

    
    if ( pG->pVertexHead == NULL ) {
        
        pG->pVertexHead = pG->pVertexTail = pV;
        pV->pNext = NULL;
    }
    else {
        
        pG->pVertexTail->pNext = pV;
        pV->pNext = NULL;
        pG->pVertexTail = pV;
    }
    
    pV->Id = (unsigned) pG->nVertices++;
    return;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


