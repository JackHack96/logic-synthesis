/**CFile****************************************************************

  FileName    [cbClub.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [clubbing utilities]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cbClub.c,v 1.3 2003/05/27 23:14:44 alanmi Exp $]

***********************************************************************/

#include "cb.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static long     Cb_GraphGetRootWeight( Cb_Graph_t *pClub );
static long     Cb_GraphGetLeafWeight( Cb_Graph_t *pClub );
static bool     Cb_ClubCheckRootsRemain( Cb_Graph_t *pClub, Cb_Vertex_t *pV);


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_ClubAddVertexToTail( Cb_Graph_t *pClub, Cb_Vertex_t *pV )
{
    if ( pClub->pVertexHead == NULL ) {
        
        pClub->pVertexHead = pClub->pVertexTail = pV;
        pV->pNextClub = NULL;
    }
    else {
        
        pClub->pVertexTail->pNextClub = pV;
        pV->pNextClub = NULL;
        pClub->pVertexTail = pV;
    }
    
    pClub->nVertices++;
    
    return;
}

/**Function*************************************************************
  Synopsis    [remove vertex from club list]
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_ClubRemoveVertexFromList( Cb_Graph_t *pClub, Cb_Vertex_t *pV )
{
    Cb_Vertex_t *pPrev, *pSpec;
    
    /* check head */
    if ( pClub->pVertexHead == pV ) {
        
        pClub->pVertexHead = pV->pNextClub;
    }
    /* iterate through the list */
    else {
        
        pPrev = pClub->pVertexHead;
        pSpec = pPrev->pNextClub;
        while ( pSpec ) {
            if ( pSpec == pV ) {
                pPrev->pNextClub = pV->pNextClub;
                break;
            }
            pPrev = pSpec;
            pSpec = pSpec->pNextClub;
        }
    }
    pClub->nVertices--;
    return;
}


/**Function*************************************************************
  Synopsis    []
  Description [return true if a vertex can be included into a club according
  to the option specified.]
  SideEffects []
  SeeAlso     []
***********************************************************************/
bool
Cb_ClubCheckInclusion( Cb_Option_t *pOpt, Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial )
{
    int          nCost;
    Cb_Vertex_t *pV;
    
    /* special case */
    if ( pClub->nVertices == 0 )
        return TRUE;
    
    /* limit in the size of the root domain */
    if ( Cb_ClubCheckVertexRoot( pClub, pSpecial ) &&
         Cb_GraphGetRootWeight( pClub ) >= (1<<pOpt->nMaxOut) )
        return FALSE;
    
    /* limit in the size of the leaf domain */
    if ( Cb_ClubCheckVertexLeaf( pClub, pSpecial ) &&
         Cb_GraphGetLeafWeight( pClub ) >= (1<<pOpt->nMaxIn) )
        return FALSE;
    
    /* disallow merging of graph root nodes (PO) */
    if ( !Cb_ClubCheckInclusionGraphRoot( pClub, pSpecial ) )
       return FALSE;
    
    /* limit in the amount of logic/area */
    nCost = 0;
    Cb_GraphForEachVertexClub( pClub, pV )
    {
        nCost += (int) pV->pData2;
    }
    if ( nCost > pOpt->nCost )
        return FALSE;
    
    /* no cycle is allowed */
    if ( Cb_ClubCheckVertexCyclic( pClub, pSpecial ) )
        return FALSE;
    
    return TRUE;
}


/**Function*************************************************************
  Synopsis    [check if the inclusion will result in merging a PO]
  Description [return TRUE if the inclusion is OK.]
  SideEffects []
  SeeAlso     []
***********************************************************************/
bool
Cb_ClubCheckInclusionGraphRoot( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial ) 
{
    int i;
    Cb_Vertex_t * pV;
    
    /* if new node is a root, check PO presence of current roots */
    if ( Cb_ClubCheckVertexRoot( pClub, pSpecial ) ) {
        
        for ( i = 0; i < sarray_n(pClub->pRoots); ++i ) {
            
            pV = sarray_fetch( Cb_Vertex_t *, pClub->pRoots, i );
            if ( Cb_VertexIsGraphRootFanin( pV ) )  /* PO */
                return FALSE;
        }
    }
    
    /* if new node is a PO, check remainess of current roots */
    if ( Cb_VertexIsGraphRootFanin( pSpecial ) ) {
        
        if ( Cb_ClubCheckRootsRemain( pClub, pSpecial ) )
            return FALSE;
    }
    
    return TRUE;
}


/**Function*************************************************************
  Synopsis    []
  Description [insert a vertex into a club; update its roots and leaves;
  if fAdjustBound flag is TRUE, and also check if the roots(leaves) are
  real.]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_ClubInclude( Cb_Graph_t *pClub, Cb_Vertex_t *pV, bool fAdjustBound ) 
{
    Cb_ClubAddVertexToTail( pClub, pV );
    
    if ( Cb_ClubCheckVertexRoot( pClub, pV ) )
        Cb_GraphAddRoot( pClub, pV );
    
    if ( Cb_ClubCheckVertexLeaf( pClub, pV ) )
        Cb_GraphAddLeaf( pClub, pV );
    
    /* old roots/leaves may no longer be roots/leaves */
    if ( fAdjustBound )
        Cb_ClubImposeBoundary( pClub );
    
    //assert( !Cb_ClubCheckGraphRootMerged( pClub ) );
    
    return;
}


/**Function*************************************************************
  Synopsis    []
  Description [remove a vertex from a club; update its roots and leaves;
  if fAdjustBound flag is TRUE, and also check if other vertices migh
  be exposed as boundaries.]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_ClubExclude( Cb_Graph_t *pClub, Cb_Vertex_t *pV, bool fAdjustBound ) 
{
    int  i;
    Cb_Vertex_t *pSpec;
    
    /* remove the vertex from club list */
    Cb_ClubRemoveVertexFromList( pClub, pV );
    
    /* remove from the root list */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pRoots, i, pSpec ) {
        
        if ( pSpec == pV ) {
            sarray_insert( Cb_Vertex_t *, pClub->pRoots, i, NULL );
            sarray_compact( pClub->pRoots );
            break;
        }
    }
    
    /* remove from the leaf list */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pLeaves, i, pSpec ) {
        
        if ( pSpec == pV ) {
            sarray_insert( Cb_Vertex_t *, pClub->pLeaves, i, NULL );
            sarray_compact( pClub->pLeaves );
            break;
        }
    }
    
    /* removed roots/leaves may expose new roots/leaves */
    if ( fAdjustBound )
        Cb_ClubExposeBoundary( pClub );
    
    return;
}


/**Function*************************************************************
  Synopsis    [adjust the roots and leaves of a club]
  Description [assume all roots(leaves) are present in the roots(leaves)
  array, and possibly others. iterates through the arrays and remove
  the ones that do not qualify. ]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_ClubImposeBoundary( Cb_Graph_t *pClub ) 
{
    int i, k;
    Cb_Vertex_t  *pV;
    
    /* first mark all vertices inside */
    Cb_GraphForEachVertexClub( pClub, pV ) {
        
        Cb_VertexSetFlag1( pV );
    }
    
    /* remove fake roots */
    for ( i = 0; i < sarray_n( pClub->pRoots ); ++i ) {
        
        pV = sarray_fetch( Cb_Vertex_t *, pClub->pRoots, i );
        for ( k = 0; k < pV->nOut; ++k ) {
            
            if ( !Cb_VertexTestFlag1( pV->pOut[k] ) )
                break;
        }
        /* check if all fanouts are inside */
        if ( k == pV->nOut ) {
            
            sarray_insert( Cb_Vertex_t *, pClub->pRoots, i, NULL );
        }
    }
    sarray_compact( pClub->pRoots );
    
    /* remove fake leaves */
    for ( i = 0; i < sarray_n( pClub->pLeaves ); ++i ) {
        
        pV = sarray_fetch( Cb_Vertex_t *, pClub->pLeaves, i );
        for ( k = 0; k < pV->nIn; ++k ) {
            
            if ( !Cb_VertexTestFlag1( pV->pIn[k] ) )
                break;
        }
        /* check if all fanouts are inside */
        if ( k == pV->nIn ) {
            
            sarray_insert( Cb_Vertex_t *, pClub->pLeaves, i, NULL );
        }
    }
    sarray_compact( pClub->pLeaves );
    
    
    /* remove marks */
    Cb_GraphForEachVertexClub( pClub, pV ) {
        Cb_VertexResetFlag1( pV );
    }
    
    return;
}


/**Function*************************************************************
  Synopsis    [adjust the roots and leaves of a club]
  Description [assume all roots(leaves) are present in the roots(leaves)
  array and no more. iterate through all vertices, and if it happen to
  be a boundary node but not present in the root/leaf array, then expose
  it in the boundary.]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_ClubExposeBoundary( Cb_Graph_t *pClub ) 
{
    int i, k;
    Cb_Vertex_t  *pV;
    
    /* first mark all vertices inside */
    Cb_GraphForEachVertexClub( pClub, pV ) {
        Cb_VertexSetFlag1( pV );
    }
    
    /* mark roots */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pRoots, i, pV ) {
        Cb_VertexSetFlag2( pV );
    }
    /* add hiden roots */
    Cb_GraphForEachVertexClub( pClub, pV ) {
        
        /* this is already a root */
        if ( Cb_VertexTestFlag2( pV ) )
            continue;
        
        for ( k = 0; k < pV->nOut; ++k ) {
            if ( !Cb_VertexTestFlag1( pV->pOut[k] ) )
                break;
        }
        /* check if all fanouts are inside */
        if ( k < pV->nOut ) {
            sarray_insert_last_safe( Cb_Vertex_t *, pClub->pRoots, pV );
        }
    }
    /* remove root marks */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pRoots, i, pV ) {
        Cb_VertexResetFlag2( pV );
    }
    
    
    /* mark leaves */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pLeaves, i, pV ) {
        Cb_VertexSetFlag2( pV );
    }
    /* add hiden leaves */
    Cb_GraphForEachVertexClub( pClub, pV ) {
        
        /* this is already a leaf */
        if ( Cb_VertexTestFlag2( pV ) )
            continue;
        
        for ( k = 0; k < pV->nIn; ++k ) {
            if ( !Cb_VertexTestFlag1( pV->pIn[k] ) )
                break;
        }
        /* check if all fanins are inside */
        if ( k < pV->nIn ) {
            sarray_insert_last_safe( Cb_Vertex_t *, pClub->pLeaves, pV );
        }
    }
    /* remove leaf marks */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pLeaves, i, pV ) {
        Cb_VertexResetFlag2( pV );
    }
    
    
    /* remove marks */
    Cb_GraphForEachVertexClub( pClub, pV ) {
        Cb_VertexResetFlag1( pV );
    }
    
    return;
}


/**Function*************************************************************
  Synopsis    []
  
  Description [expand a club towards fanout direction; the newly included node
  should not have outside fanins whose level is larger than the current one.
  The expanded node (to be included) should not have a level that is greater
  than the current one by 2; otherwise a cycle is likely to result.]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_ClubExpandFanout( Cb_Option_t *pOpt, Cb_Graph_t *pG,
                     Cb_Graph_t *pClub, int iLevel )
{
    int i, k, iRoot, nInside;
    Cb_Vertex_t *pV, *pRoot, *pFanout;
    sarray_t    *listCand;
    
    if ( pClub->nVertices == 0 )
        return;
    
    /* first mark all vertices inside */
    Cb_GraphForEachVertexClub( pClub, pV ) {
        Cb_VertexSetFlag1( pV );
    }
    
    listCand = sarray_alloc( Cb_Vertex_t *, Cb_GraphReadRootsNum(pClub) );
    
    /* visit fanout's of roots (with marking) */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pRoots, iRoot, pRoot ) {
        
        /* do not expand graph roots */
        if ( Cb_VertexIsGraphRootFanin( pRoot ) )
            continue;
        
        for ( i=0; i < pRoot->nOut; ++i ) {
            
            /* either inside the club or have been considered */
            if ( Cb_VertexTestFlag1( pRoot->pOut[i] ) ||
                 Cb_VertexTestFlag2( pRoot->pOut[i] ))
                continue;
            
            /* do not consider special CO vertex */
            if ( pRoot->pOut[i]->iTravs == pG->iTravs ||
                 pRoot->pOut[i]->nOut == 0 )
                continue;
            
            pFanout = pRoot->pOut[i];
            Cb_VertexSetFlag2( pFanout );
            
            /* find benefitial ones */
            nInside = 0;
            for ( k=0; k < pFanout->nIn; ++k ) {
                
                /* accross too many levels, no good */
                if ( pFanout->pIn[k]->iLevel > iLevel ) {
                    nInside = -2;
                    break;
                }
                /* if ( pFanout->pIn[k]->iTravs == pG->iTravs ) */
                if ( Cb_VertexTestFlag1( pFanout->pIn[k] ) )
                    nInside++;
            }
            /* all fanins should fall inside (conservative) */
            if ( (pFanout->nIn - nInside) < 1 ) {
                
                /* insert into candidate list */
                sarray_insert_last_safe( Cb_Vertex_t *, listCand, pFanout );
            }
        }
    }
    
    /* remove all marks */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pRoots, iRoot, pRoot ) {
        for ( i=0; i < pRoot->nOut; ++i ) {
            Cb_VertexResetFlag2( pRoot->pOut[i] );
        }
    }
    Cb_GraphForEachVertexClub( pClub, pV ) {
        Cb_VertexResetFlag1( pV );
    }
    
    /* after marks have been removed, check inclusion of candidates
       (TODO) the order of the candidates is important; should alway try
       the best one first! (create a heap with its sharing amount)
     */
    sarrayForEachItem( Cb_Vertex_t *, listCand, i, pFanout ) {
        
        if ( Cb_ClubCheckInclusion( pOpt, pClub, pFanout ) ) {
            
            Cb_ClubInclude( pClub, pFanout, 1 );
            pFanout->iTravs = pG->iTravs;
        }
    }
    
    sarray_free( listCand );
}

/**Function*************************************************************
  Synopsis    []
  Description [return true if one of pSpecial's fanout is not in club]
  SideEffects [clear the flags of pSpecial's fanouts]
  SeeAlso     []
***********************************************************************/
bool
Cb_ClubCheckVertexRoot( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial )
{
    int          i, nFound;
    Cb_Vertex_t *pV;
    
    if ( pSpecial->nOut == 0 )
        return TRUE;
    
    /* marked the fanouts of pSpecial */
    for ( i=0; i < pSpecial->nOut; ++i ) {
        Cb_VertexSetFlag1( pSpecial->pOut[i] );
    }
    
    /* the number of found fanout's */
    nFound = 0;
    Cb_GraphForEachVertexClub( pClub, pV ) {
        if ( Cb_VertexTestFlag1( pV ) ) {
            nFound++;
        }
    }
    
    /* remove marking */
    for ( i=0; i < pSpecial->nOut; ++i ) {
        Cb_VertexResetFlag1( pSpecial->pOut[i] );
    }
    
    return ( nFound != pSpecial->nOut );
}

/**Function*************************************************************
  Synopsis    []
  Description [return true if one of pSpecial's fanin is not in club]
  SideEffects [clear the flags of pSpecial's fanins]
  SeeAlso     []
***********************************************************************/
bool
Cb_ClubCheckVertexLeaf( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial )
{
    int          i, nFound;
    Cb_Vertex_t *pV;
    
    if ( pSpecial->nIn == 0 )
        return TRUE;
        
    /* marked the fanouts of pSpecial */
    for ( i=0; i < pSpecial->nIn; ++i ) {
        Cb_VertexSetFlag1( pSpecial->pIn[i] );
    }
    
    /* the number of found fanin's */
    nFound = 0;
    Cb_GraphForEachVertexClub( pClub, pV ) {
        if ( Cb_VertexTestFlag1( pV ) ) {
            nFound++;
        }
    }
    
    /* remove marking */
    for ( i=0; i < pSpecial->nIn; ++i ) {
        Cb_VertexResetFlag1( pSpecial->pIn[i] );
    }
    
    return ( nFound != pSpecial->nIn );
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
bool
Cb_ClubCheckVertexCyclic( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial )
{
    /* can be skipped since we expand with limit on the level */
    return FALSE;
}

/**Function*************************************************************
  Synopsis    []
  Description [return 1 if club has multiple roots including a PO fanin.
  This will result the PO fanin be merged with other nodes, which is
  undesirable.]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
bool
Cb_ClubCheckGraphRootMerged( Cb_Graph_t *pClub ) 
{
    int i;
    Cb_Vertex_t * pV;
    
    /* single root club has no problem */
    if ( sarray_n( pClub->pRoots ) == 1 )
        return FALSE;
    
    /* check if roots contain a PO */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pRoots, i, pV ) {
        
        if ( Cb_VertexIsGraphRootFanin( pV ) ||
             Cb_VertexIsGraphRoot( pV )) {
            return TRUE;
        }
    }
    
    return FALSE;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_ClubListFree( sarray_t *listClubs )
{
    int i;
    Cb_Graph_t *pClub;
    sarrayForEachItem( Cb_Graph_t *, listClubs, i, pClub ) {
        Cb_GraphFree( pClub );
    }
    sarray_free( listClubs );
    return;
}

/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_ClubListPrint( sarray_t *listClubs )
{
    int i;
    Cb_Graph_t *pClub;
    Cb_Vertex_t *pV;
    
    sarrayForEachItem( Cb_Graph_t *, listClubs, i, pClub ) {
        
        printf( "club[%3d]: ", i );
        Cb_GraphForEachVertexClub( pClub, pV ) {
            printf( "%s(l:%d) ", Ntk_NodeGetNamePrintable( pV->pData1 ),
                    pV->iLevel);
        }
        printf( "\n" );
    }
    return;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
long
Cb_GraphGetRootWeight( Cb_Graph_t *pClub )
{
    int  i;
    long nWeight;
    Cb_Vertex_t *pV;
    
    nWeight = 1;
    sarrayForEachItem( Cb_Vertex_t *, pClub->pRoots, i, pV ) {
        
        if ( pV->nWeigh > 0 ) {
            
            nWeight *= pV->nWeigh;
        }
    }
    return nWeight;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
long
Cb_GraphGetLeafWeight( Cb_Graph_t *pClub )
{
    int  i;
    long nWeight;
    Cb_Vertex_t *pV;
    
    nWeight = 1;
    sarrayForEachItem( Cb_Vertex_t *, pClub->pLeaves, i, pV ) {
        
        if ( pV->nWeigh > 0 ) {
            
            nWeight *= pV->nWeigh;
        }
    }
    return nWeight;
}

/**Function*************************************************************
  Synopsis    []
  Description [check whether the inclusion of an external node will
  invalidate all the current roots.]
  SideEffects []
  SeeAlso     []
***********************************************************************/
bool
Cb_ClubCheckRootsRemain( Cb_Graph_t *pClub, Cb_Vertex_t *pSpecial ) 
{
    bool          fRemain;
    int           i, k;
    Cb_Vertex_t  *pV;
    
    /* first mark all root vertices inside */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pRoots, i, pV ) {
        
        Cb_VertexSetFlag1( pV );
    }
    
    /* check validity of current roots */
    fRemain = FALSE;
    for ( i = 0; i < sarray_n( pClub->pRoots ); ++i ) {
        
        pV = sarray_fetch( Cb_Vertex_t *, pClub->pRoots, i );
        for ( k = 0; k < pV->nOut; ++k ) {
            
            /* fanout to external territory */
            if ( pV->pOut[k] != pSpecial &&
                 !Cb_VertexTestFlag1( pV->pOut[k] ) ) {
                fRemain = TRUE;
                break;
            }
        }
        if ( fRemain )
            break;
    }
    
    /* remove marks */
    sarrayForEachItem( Cb_Vertex_t *, pClub->pRoots, i, pV ) {
        
        Cb_VertexResetFlag1( pV );
    }
    
    return fRemain;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


