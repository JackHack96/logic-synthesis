/**CFile****************************************************************

  FileName    [cbDom.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [compute dominators of a netlist]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cbDom.c,v 1.2 2003/05/27 23:14:45 alanmi Exp $]

***********************************************************************/

#include "cb.h"

/*
  Reference: T. Lengauer and R. E. Tarjan, A fast algorithm for finding
  dominators in a flowgraph, ACM Trans. on Prog. Lang. and Sys.,
  Vol 1, No. 1, July 1979, p121-141.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void CbDomCompress( int * pAncest, int * pSemi, int * pLabl, int iV );
static int  CbDomEval    ( int * pAncest, int * pSemi, int * pLabl, int iV );
static void CbDomDfs    ( Cb_Graph_t *pG, Cb_Vertex_t **ppDfs, int *pParent );
static void CbDomDfs_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV, int *iSeq,
                          Cb_Vertex_t **ppDfs, int *pParent );
static void CbDomInsertBucket( Cb_Vertex_t ** ppBuck, int iIndex, Cb_Vertex_t * pV );
static void CgGraphClubDominator_rec( Cb_Graph_t *pG, sarray_t *listClub,
                                      Cb_Graph_t *pClub, Cb_Vertex_t *pV );
static void Cb_GraphPrintDominators( Cb_Graph_t *pG );



////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************
  Synopsis    []
  Description [find dominators of the graph and eliminate all nodes that
  are not dominators. Note that CI/CO vertices are NOT included in clubs.]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
sarray_t *
Cb_GraphClubDominator( Cb_Option_t *pOpt, Cb_Graph_t *pG )
{
    int            i; 
    Cb_Vertex_t   *pV;
    Cb_Graph_t    *pClub;
    sarray_t      *listClubs;
    
    listClubs = sarray_alloc( Cb_Vertex_t *, pG->nVertices );
    
    /* first compute dominators */
    Cb_GraphDominators( pG );
    
    if ( pOpt->fVerb ) {
        Cb_GraphPrintDominators( pG ); 
    }
    
    /* start a club for each node not dominated by others */
    pG->iTravs++;
    for ( i = 0; i < sarray_n ( pG->pRoots ); i++ ) {
        
        pV = sarray_fetch( Cb_Vertex_t *, pG->pRoots, i );
        CgGraphClubDominator_rec( pG, listClubs, NULL, pV );
    }
    
    /* adjust clubs to recompute their leaves */
    for ( i = 0; i < sarray_n ( listClubs ); ++i ) {
        
        pClub = sarray_fetch( Cb_Graph_t *, listClubs, i );
        Cb_ClubExposeBoundary( pClub );
    }
    
    return listClubs;
}


/**Function*************************************************************
  Synopsis    [compute dominators of a graph]
  Description [the pV->pDomin field will be filled with the immediate
  dominator of vertex pV.]
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_GraphDominators( Cb_Graph_t *pG ) 
{
    int            i, k, nV, iU, iW, iV;
    int           *pAncest, *pParent, *pSemi, *pDomi, *pLabl;
    Cb_Vertex_t  **ppDfs, **ppBuck, *pV, *pV1;
    
    /* initialize the arrays */
    nV = pG->nVertices + 1;
    ppDfs   = ALLOC( Cb_Vertex_t *, nV );
    ppBuck  = ALLOC( Cb_Vertex_t *, nV );
    pSemi   = ALLOC( int, nV);
    pDomi   = ALLOC( int, nV);
    pLabl   = ALLOC( int, nV);
    pParent = ALLOC( int, nV);
    pAncest = ALLOC( int, nV);
    memset( ppDfs,   0, nV * sizeof( Cb_Vertex_t * ) );
    memset( ppBuck,  0, nV * sizeof( Cb_Vertex_t * ) );
    memset( pSemi,   0, nV * sizeof( int ) );
    memset( pDomi,   0, nV * sizeof( int ) );
    memset( pLabl,   0, nV * sizeof( int ) );
    memset( pParent, 0, nV * sizeof( int ) );
    memset( pAncest, -1, nV * sizeof( int ) );  /* initially empty ancestors */
    
    /* DFS traversal and array initialization (step 1) */
    CbDomDfs( pG, ppDfs, pParent );
    
    /* array initialization */
    for ( i = nV-1; i >= 0; --i ) {
        pSemi[i] = i;     /* initial sdom(v) = v */
        pLabl[i] = i;     /* initial label(v) = v */
    }
    
    /* compute semi-dominators (step 2 & 3 ) */
    for ( i = nV-1; i > 0; --i ) {
        
        pV = ppDfs[i];
        iW = pV->iLevel;
        /* evaluate each predecessor in the graph */
        for ( k = 0; k < pV->nOut; ++k ) {
            
            iV = pV->pOut[k]->iLevel;  /* predecessor */
            iU = CbDomEval( pAncest, pSemi, pLabl, iV );      /* EVAL()      */
            if ( pSemi[iU] < pSemi[iW] ) {
                pSemi[iW] = pSemi[iU];
            }
        }
        /* roots of the graph actually fanout to the rupper root */
        if ( pV->nOut == 0 ) {
            
            /* supper root */
            iU = CbDomEval( pAncest, pSemi, pLabl, 0 );
            if ( pSemi[iU] < pSemi[iW] ) {
                pSemi[iW] = pSemi[iU];
            }
        }
        
        CbDomInsertBucket( ppBuck, pSemi[iW], pV );
        
        /* LINK( pParent[iW], iW ) */
        pAncest[iW] = pParent[iW];
        
        /* remove all elements in the bucket */
        pV1 = ppBuck[pParent[iW]];
        ppBuck[pParent[iW]] = NULL;
        while ( pV1 ) {
            
            iV = pV1->iLevel;
            iU = CbDomEval( pAncest, pSemi, pLabl, iV );
            pDomi[iV] = ( pSemi[iU] < pSemi[iV] ) ? iU : pParent[iW];
            pV1 = pV1->pNextClub;
        }
    }
    
    /* step 4 */
    for ( i = 1; i < nV; ++i ) {
        
        if ( pDomi[i] != pSemi[i] ) {
            
            pDomi[i] = pDomi[pDomi[i]];
        }
        
        /* put result inside vertices */
        ppDfs[i]->pDomin = ppDfs[pDomi[i]];
    }
    
    
    FREE( pSemi );
    FREE( pDomi );
    FREE( pLabl );
    FREE( pParent );
    FREE( pAncest );
    FREE( ppBuck );
    FREE( ppDfs );
    
    return;
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
CbDomDfs( Cb_Graph_t *pG, Cb_Vertex_t **ppDfs, int *pParent )
{
    int i, iSeq;
    Cb_Vertex_t *pV;
    
    /* index 0 is the invisible upper-root */
    iSeq = 0;
    pG->iTravs++;
    
    sarrayForEachItem( Cb_Vertex_t *, pG->pRoots, i, pV ) {
        CbDomDfs_rec( pG, pV, &iSeq, ppDfs, pParent );
    }
    
    return;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
CbDomDfs_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV, int *iSeq, Cb_Vertex_t **ppDfs,
              int *pParent )
{
    int i;
    
    *iSeq += 1;
    pV->iTravs   = pG->iTravs;
    pV->iLevel   = *iSeq;     /* use iLevel to store DFS id */
    ppDfs[*iSeq] = pV;        /* store vertices in an array */
    
    for ( i=0; i<pV->nIn; ++i ) {
        
        if ( pV->pIn[i]->iTravs != pG->iTravs ) {
            
            pParent[*iSeq+1] = pV->iLevel;   /* remember DFS parent */
            CbDomDfs_rec( pG, pV->pIn[i], iSeq, ppDfs, pParent  );
        }
    }
    return;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
int
CbDomEval( int * pAncest, int * pSemi, int * pLabl, int iV )
{
    if ( pAncest[iV] < 0 )
        return iV;
    CbDomCompress( pAncest, pSemi, pLabl, iV );
    return pLabl[iV];
}


/**Function*************************************************************
  Synopsis    []
  Description [assume pAncest[iV] >= 0]
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
CbDomCompress( int * pAncest, int * pSemi, int * pLabl, int iV )
{
    if ( pAncest[pAncest[iV]] >= 0 ) {
        
        CbDomCompress( pAncest, pSemi, pLabl, pAncest[iV] );
        if ( pSemi[pLabl[pAncest[iV]]] < pSemi[pLabl[iV]] ) {
            
            pLabl[iV] = pLabl[pAncest[iV]];
        }
        pAncest[iV] = pAncest[pAncest[iV]];
    }
    return;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
CbDomInsertBucket( Cb_Vertex_t ** ppBuck, int iIndex, Cb_Vertex_t * pV )
{
    if ( ppBuck[iIndex] ) {
        pV->pNextClub = ppBuck[iIndex];
    }
    else {
        pV->pNextClub = NULL;
    }
    ppBuck[iIndex] = pV;
    return;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
CgGraphClubDominator_rec( Cb_Graph_t *pG, sarray_t *listClub,
                          Cb_Graph_t *pClub,
                          Cb_Vertex_t *pV )
{
    int          i;
    Cb_Graph_t  *pClubNew;
    
    if ( pV->iTravs == pG->iTravs )
        return;
    pV->iTravs = pG->iTravs;
    
    /* do not include CI's */
    if ( pV->nIn == 0 )
        return;
    
    
    /* start a new club for dominators */
    if ( pV->pDomin == NULL || pClub == NULL ) {
        
        /* ignore CO's */
        if ( pV->nOut > 0 ) {
            
            pClubNew = Cb_GraphAlloc( 1, 0 );  /* 1 root, n leaves */
            Cb_ClubInclude( pClubNew, pV, 1 );
        }
        else {
            
            pClubNew = NULL;
        }
        /* recursively club its children */
        for ( i = 0; i < pV->nIn; ++i ) {
            
            CgGraphClubDominator_rec( pG, listClub, pClubNew, pV->pIn[i] );
        }
        
        /* insert the club into the final list */
        if ( pClubNew ) 
            sarray_insert_last_safe( Cb_Club_t *, listClub, pClubNew );
    }
    /* continue including vertices for the current club */
    else {
        
        /* add the vertex to club without updating boundary
           (TODO) because we know a non-dominator cannot fanout to other nodes (true?)
         */
        Cb_ClubAddVertexToTail( pClub, pV );
        for ( i = 0; i < pV->nIn; ++i ) {
            
            CgGraphClubDominator_rec( pG, listClub, pClub, pV->pIn[i] );
        }
    }
    
    return;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_GraphPrintDominators( Cb_Graph_t *pG ) 
{
    Cb_Vertex_t *pV;
    Ntk_Node_t  *pNode, *pNodeDom;
    
    Cb_GraphForEachVertex( pG, pV ) {
        
        pNode = (Ntk_Node_t *) pV->pData1;
        if ( pV->pDomin ) {
            pNodeDom = (Ntk_Node_t *) pV->pDomin->pData1;
            printf( "%20s <==DOM== %s\n",
                    Ntk_NodeGetNamePrintable( pNode ),
                    Ntk_NodeGetNamePrintable( pNodeDom ) );
        }
        else {
            printf( "%20s <==DOM== ROOT\n",
                    Ntk_NodeGetNamePrintable( pNode ) );
        }
    }
    
    return;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


