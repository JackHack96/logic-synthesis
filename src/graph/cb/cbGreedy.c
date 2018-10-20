/**CFile****************************************************************

  FileName    [cbGreedy.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [greedy clubbing algorithm]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cbGreedy.c,v 1.2 2003/05/27 23:14:45 alanmi Exp $]

***********************************************************************/

#include "cb.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Cb_Vertex_t * CbClubSelectVertexFromLevel( Cb_Graph_t *pG, Cb_Graph_t *pClub,Cb_Vertex_t **ppHead );
static int           CbClubComputeSharedFaninFanout( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial );
static bool          Cb_ClubExpandFanoutTest( Cb_Option_t *pOpt, Cb_Graph_t *pG,
                                              Cb_Graph_t *pClub, int iLevel, Cb_Vertex_t *pV );



////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************
  Synopsis    [greedy method of network clubbing]
  
  Description [return an array of non-overlapping sub-graphs.
  Note that CI/CO vertices are NOT included in clubs.
  (ref: Baleani, Gennari, Jiang, Patel, et al, CODES'01)]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
sarray_t *
Cb_GraphClubGreedy( Cb_Option_t *pOpt, Cb_Graph_t *pG ) 
{
    int            nLevels, iLevel;
    Cb_Vertex_t   *pV;
    Cb_Vertex_t ** ppLevels;
    Cb_Graph_t    *pClubCurr;
    sarray_t      *listClubs;
    
    /* first levelize the graph */
    nLevels  = Cb_GraphGetLevelNum( pG );
    ppLevels = ALLOC( Cb_Vertex_t *, nLevels + 2 );
    memset( ppLevels, 0, (nLevels+2) * sizeof(Cb_Vertex_t *) );
    
    Cb_GraphLevelize( pG, ppLevels, 1 );
    
    /* large enough for worst case */
    listClubs = sarray_alloc( Cb_Graph_t *, pG->nVertices );
    pClubCurr = Cb_GraphAlloc( 0, 0 );
    
    /* do not include CI/CO nodes into clubs
       (CI: level==0; CO: nOut==0).
     */
    pG->iTravs++;
    for ( iLevel=1; iLevel < nLevels; ++iLevel ) {
        
        do {
            pV = CbClubSelectVertexFromLevel( pG, pClubCurr, &ppLevels[iLevel] );
            if ( pV == NULL )
                break;       /* exhausted the current level */
            
            if ( !Cb_ClubCheckInclusion( pOpt, pClubCurr, pV ) ) {
                
                /* not ok. try expand fanout; may remove #roots */
                if ( Cb_ClubExpandFanoutTest( pOpt, pG, pClubCurr, iLevel, pV ) ) {
                    
                    pV->iTravs = pG->iTravs;
                    continue;
                }
                else {
                    
                    /* have to start a new club */
                    sarray_insert_last( Cb_Graph_t *, listClubs, pClubCurr );
                    pClubCurr = Cb_GraphAlloc( 0, 0 );
                }
            }
            
            /* absorb the vertex and mark as so */
            Cb_ClubInclude( pClubCurr, pV, 1 );
            pV->iTravs = pG->iTravs;
            
            /* try to expand to new teritory */
            Cb_ClubExpandFanout( pOpt, pG, pClubCurr, iLevel );
        }
        while ( pV );
    }
    
    if ( pClubCurr->nVertices == 0 ) {
        Cb_GraphFree( pClubCurr );
    }
    else {
        sarray_insert_last( Cb_Graph_t *, listClubs, pClubCurr );
    }
    
    FREE( ppLevels );
    
    return listClubs;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************
  Synopsis    []
  Description [return TRUE if the inclusion of vertex pV would result a
  successful fanout expansion, whereby reduce (or maintain) the root
  number.]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
bool
Cb_ClubExpandFanoutTest( Cb_Option_t *pOpt, Cb_Graph_t *pG,
                         Cb_Graph_t *pClub, int iLevel, Cb_Vertex_t *pV )
{
    int  nVertices;
    bool fSuccess;
    
    /* if involving POs then quit */
    if ( !Cb_ClubCheckInclusionGraphRoot( pClub, pV ) )
        return FALSE;
    
    /* temprarily include the vertex */
    Cb_ClubInclude( pClub, pV, 1 );
    nVertices = pClub->nVertices;
    
    /* check if we gain by fanout expansion */
    Cb_ClubExpandFanout( pOpt, pG, pClub, iLevel );
    
    fSuccess = ( nVertices < pClub->nVertices);
    
    /* if no gain, or accidentally merged PO's then undo */
    if ( !fSuccess || Cb_ClubCheckGraphRootMerged( pClub ) ) {
        
        Cb_ClubExclude( pClub, pV, 1 );
    }
    
    return fSuccess;
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function*************************************************************
  Synopsis    [pick a vertex that migh benefit from being collapsed]
  
  Description [a graph is a set of vertices contected with directed edges; a
  club is a subset of the vertices that can be collapsed into a singl vertex
  without creating cycles. return a vertex from the linked list "ppHead",
  which has the maximal fanout (and fanin) sharing with vertices in the
  "pClub". ]
  
  SideEffects [removed the selected vertex from the linked list]
  SeeAlso     []
***********************************************************************/
Cb_Vertex_t *
CbClubSelectVertexFromLevel( Cb_Graph_t *pG, Cb_Graph_t *pClub, Cb_Vertex_t **ppHead )
{
    int          nShared, nSharedBest, iIndex, iIndexBest;
    Cb_Vertex_t *pV, *pPrev, *pHead;
    
    pHead = *ppHead;
    
    /* list is empty */
    if ( pHead == NULL )
        return NULL;
    
    /* club is empty; take the first non-visited and non-CO vertex */
    if ( pClub->nVertices == 0 ) {
        
        if ( pHead->iTravs != pG->iTravs && pHead->nOut > 0 ) {
            *ppHead = pHead->pNextSpec;
            return pHead;
        }
        
        pPrev = pHead; pV = pHead->pNext;
        while ( pV && ( pV->iTravs == pG->iTravs || pV->nOut == 0 ) )
        {
            pPrev = pV; pV = pV->pNext;
        }
        if ( pV == NULL ) return pV;  // exhausted
        
        pPrev->pNext = pV->pNext;
        return pV;
    }
    
    iIndex  = 0;
    iIndexBest = -1;
    nSharedBest = -1;
    for ( pV = pHead; pV; pV = pV->pNextSpec ) {
        
        /* visited or CO vertices */
        if ( pV->iTravs == pG->iTravs || pV->nOut == 0 ) {
            iIndex++;
            continue;
        }
        
        /* evaluate logic sharing between pClub and pV */
        nShared = CbClubComputeSharedFaninFanout( pClub, pV );
        if ( nShared > nSharedBest ) {
            iIndexBest = iIndex;
            nSharedBest = nShared;
        }
        iIndex++;
    }
    
    if ( iIndexBest < 0 )
        return NULL;
    
    /* take the vertex with maximum fanout sharing */
    pV = Cb_ListSpecialRemoveByIndex( ppHead, iIndexBest );
    //assert( pV->nOut > 0 );
    
    return pV;
}


/**Function*************************************************************
  Synopsis    [compute fanin/fanout sharing between a club and a vertex]
  
  Description [total number of shared fanin/fanouts between all vertices
  in a club and the special vertex; may overlap.]
  
  SideEffects [clear the flags of pSpecial's fanouts]
  SeeAlso     []
***********************************************************************/
int
CbClubComputeSharedFaninFanout( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial )
{
    int          i, nShared;
    Cb_Vertex_t *pV;
    
    /* marked the fanin/fanouts of pSpecial */
    for ( i = 0; i < pSpecial->nOut; ++i ) {
        Cb_VertexSetFlag1( pSpecial->pOut[i] );
    }
    for ( i = 0; i < pSpecial->nIn; ++i ) {
        Cb_VertexSetFlag1( pSpecial->pIn[i] );
    }
    
    /* traverse vertices in the club and there fanin/fanouts */
    nShared = 0;
    Cb_GraphForEachVertexClub( pClub, pV ) {
        
        /* check the node; prevent double counting */
        if ( Cb_VertexTestFlag1( pV ) && !Cb_VertexTestFlag2( pV ) ) {
            nShared++;
            Cb_VertexSetFlag2( pV );
        }
        
        /* check the fanouts */
        for ( i=0; i < pV->nOut; ++i ) {
            if ( Cb_VertexTestFlag1( pV->pOut[i] ) &&
                 !Cb_VertexTestFlag2( pV->pOut[i] ) ) {
                nShared++;
                Cb_VertexSetFlag2( pV->pOut[i] );
            }
        }
        
        /* check the fanins */
        for ( i=0; i < pV->nIn; ++i ) {
            if ( Cb_VertexTestFlag1( pV->pIn[i] ) &&
                 !Cb_VertexTestFlag2( pV->pIn[i] ) ) {
                nShared++;
                Cb_VertexSetFlag2( pV->pIn[i] );
            }
        }
    }
    
    /* remove marking */
    for ( i=0; i < (pSpecial->nOut); ++i ) {
        Cb_VertexResetFlag1( pSpecial->pOut[i] );
        Cb_VertexResetFlag2( pSpecial->pOut[i] );
    }
    for ( i=0; i < (pSpecial->nIn); ++i ) {
        Cb_VertexResetFlag1( pSpecial->pIn[i] );
        Cb_VertexResetFlag2( pSpecial->pIn[i] );
    }
    
    return nShared;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


