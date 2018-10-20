/**CFile****************************************************************

  FileName    [cbDfs.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [DFS traversal and levelization]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cbDfs.c,v 1.2 2003/05/27 23:14:44 alanmi Exp $]

***********************************************************************/

#include "cb.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void CbGraphDfsFromRoot_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV,
                                    Cb_Vertex_t ***pppTail, bool fPreTrav );
static void CbGraphDfsFromLeaf_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV,
                                    Cb_Vertex_t ***pppTail, bool fPreTrav );
static int CbGraphLevelizeFromRoot_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV, Cb_Vertex_t **ppLevels);
static int CbGraphLevelizeFromLeaf_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV, Cb_Vertex_t **ppLevels);
static int CbGraphGetLevelNum_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV );



////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************
  Synopsis    []
  Description [vertices are linked in the special list according to the
  DFS order. If (fPreTrav) is true, a vertex is added to the list before
  its children.]
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_GraphDfs( Cb_Graph_t *pG, bool fFromRoot, bool fPreTrav ) 
{
    int i;
    Cb_Vertex_t *pV, **ppTail;
    
    pG->iTravs++;
    ppTail = &pG->pVertexSpec;
    
    if ( fFromRoot ) {
        
        sarrayForEachItem( Cb_Vertex_t *, pG->pRoots, i, pV ) {
            CbGraphDfsFromRoot_rec( pG, pV, &ppTail, fPreTrav );
        }
    }
    else {
        
        sarrayForEachItem( Cb_Vertex_t *, pG->pLeaves, i, pV ) {
            CbGraphDfsFromLeaf_rec( pG, pV, &ppTail, fPreTrav );
        }
    }
    
    /* final tail of the special list */
    *ppTail = NULL;
    return;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
int
Cb_GraphLevelize( Cb_Graph_t *pG, Cb_Vertex_t **ppLevels, int fFromRoot )
{
    int i, iLevelMax, iLevel;
    Cb_Vertex_t *pV;
    
    pG->iTravs++;
    iLevelMax = -1;
    if ( fFromRoot ) {
        
        sarrayForEachItem( Cb_Vertex_t *, pG->pRoots, i, pV ) {
            iLevel = CbGraphLevelizeFromRoot_rec( pG, pV, ppLevels );
            if ( iLevelMax < iLevel )
                iLevelMax = iLevel;
        }
    }
    else {
        
        sarrayForEachItem( Cb_Vertex_t *, pG->pLeaves, i, pV ) {
            iLevel = CbGraphLevelizeFromLeaf_rec( pG, pV, ppLevels );
            if ( iLevelMax < iLevel )
                iLevelMax = iLevel;
        }
    }
    pG->nLevels = iLevelMax;
    return pG->nLevels;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
int
Cb_GraphGetLevelNum( Cb_Graph_t *pG ) 
{
    int i, iLevelMax, iLevel;
    Cb_Vertex_t *pV;
    
    pG->iTravs++;
    
    iLevelMax = -1;
    sarrayForEachItem( Cb_Vertex_t *, pG->pRoots, i, pV ) {
        iLevel = CbGraphGetLevelNum_rec( pG, pV );
        if ( iLevelMax < iLevel )
            iLevelMax = iLevel;
    }
    return iLevelMax;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************
  Synopsis    []
  Description [append the vertex to the tail of the linked list]
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
CbGraphDfsFromRoot_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV,
                        Cb_Vertex_t ***pppTail, bool fPreTrav )
{
    int i;
    
    /* node already visited */
    if ( pV->iTravs == pG->iTravs ) {
        return;
    }
    
    /* mark as visited */
    pV->iTravs = pG->iTravs;
    
    /* terminal cases */
    if ( pV->nIn == 0 ) {
        **pppTail = pV;
        *pppTail = &pV->pNextSpec;
        return;
    }
    
    /* add this vertex before its children */
    if ( fPreTrav ) {
        **pppTail = pV;
        *pppTail = &pV->pNextSpec;
    }
    
    for ( i=0; i<pV->nIn; ++i ) {
        CbGraphDfsFromRoot_rec( pG, pV->pIn[i], pppTail, fPreTrav );
    }
    
    if ( !fPreTrav ) {
        **pppTail = pV;
        *pppTail = &pV->pNextSpec;
    }
    
    return;
}

/**Function*************************************************************
  Synopsis    []
  Description [append the vertex to the tail of the linked list]
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
CbGraphDfsFromLeaf_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV,
                        Cb_Vertex_t ***pppTail, bool fPreTrav )
{
    int i;
    
    /* node already visited */
    if ( pV->iTravs == pG->iTravs ) {
        return;
    }
    
    /* mark as visited */
    pV->iTravs = pG->iTravs;
    
    /* terminal cases */
    if ( pV->nOut == 0 ) {
        **pppTail = pV;
        *pppTail = &pV->pNextSpec;
        return;
    }
    
    /* add this vertex before its children */
    if ( fPreTrav ) {
        **pppTail = pV;
        *pppTail = &pV->pNextSpec;
    }
    
    for ( i=0; i<pV->nOut; ++i ) {
        CbGraphDfsFromRoot_rec( pG, pV->pOut[i], pppTail, fPreTrav );
    }
    
    if ( !fPreTrav ) {
        **pppTail = pV;
        *pppTail = &pV->pNextSpec;
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
CbGraphLevelizeFromRoot_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV, Cb_Vertex_t **ppLevels )
{
    int i, iLevel, iLevelMax;
    
    /* node already visited */
    if ( pV->iTravs == pG->iTravs ) {
        return pV->iLevel;
    }
    
    /* mark as visited */
    pV->iTravs = pG->iTravs;
    
    if ( pV->nIn == 0 ) {
        pV->iLevel = 0;
    }
    else {
        iLevelMax  = -1;
        for ( i=0; i<pV->nIn; ++i ) {
            iLevel = CbGraphLevelizeFromRoot_rec( pG, pV->pIn[i], ppLevels );
            if ( iLevel > iLevelMax )
                iLevelMax = iLevel;
        }
        pV->iLevel = iLevelMax + 1;
    }
    
    /* insert into the array of this level */
    pV->pNextSpec = ppLevels[pV->iLevel];
    ppLevels[pV->iLevel] = pV;
    
    return pV->iLevel;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
int
CbGraphLevelizeFromLeaf_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV, Cb_Vertex_t **ppLevels )
{
    int i, iLevel, iLevelMax;
    
    /* node already visited */
    if ( pV->iTravs == pG->iTravs ) {
        return pV->iLevel;
    }
    
    /* mark as visited */
    pV->iTravs = pG->iTravs;
    
    if ( pV->nOut == 0 ) {
        pV->iLevel = 0;
    }
    else {
        iLevelMax  = -1;
        for ( i=0; i<pV->nOut; ++i ) {
            iLevel = CbGraphLevelizeFromLeaf_rec( pG, pV->pOut[i], ppLevels );
            if ( iLevel > iLevelMax )
                iLevelMax = iLevel;
        }
        pV->iLevel = iLevelMax + 1;
    }
    
    /* insert into the array of this level */
    pV->pNextSpec = ppLevels[pV->iLevel];
    ppLevels[pV->iLevel] = pV;
    
    return pV->iLevel;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
int 
CbGraphGetLevelNum_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV ) 
{
    int i, iLevel, iLevelMax;
    
    /* node already visited */
    if ( pV->iTravs == pG->iTravs ) {
        return pV->iLevel;
    }
    
    /* mark as visited */
    pV->iTravs = pG->iTravs;
    
    if ( pV->nIn == 0 ) {
        pV->iLevel = 0;
    }
    else {
        iLevelMax  = -1;
        for ( i=0; i<pV->nIn; ++i ) {
            iLevel = CbGraphGetLevelNum_rec( pG, pV->pIn[i] );
            if ( iLevel > iLevelMax )
                iLevelMax = iLevel;
        }
        pV->iLevel = iLevelMax + 1;
    }
    
    return pV->iLevel;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


