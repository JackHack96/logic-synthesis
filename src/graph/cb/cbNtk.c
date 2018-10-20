/**CFile****************************************************************

  FileName    [cbNtk.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [interface between Ntk and Cb package]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cbNtk.c,v 1.2 2003/05/27 23:14:45 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "cb.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void        Cb_NetworkClusterClub( Ntk_Network_t *pNet, Cb_Graph_t *pG, Cb_Graph_t *pClub);
static Ntk_Node_t *Cb_NetworkClusterClub_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV,
                                              Ntk_Node_t *pNodeRoot);
static int         CbVertexAddOutput( Cb_Vertex_t *pV, Cb_Vertex_t *pOut );




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
Cb_NetworkClub( Cb_Option_t *pOpt, Ntk_Network_t *pNet )
{
    Cb_Graph_t  *pG;
    sarray_t    *listClubs;
    
    pG = Cb_GraphCreateFromNetwork( pNet );
    
    /* greedy clubbing algorithm */
    if ( pOpt->iMethod == 0 ) {
        
        listClubs = Cb_GraphClubGreedy( pOpt, pG );
    }
    else {
        
        listClubs = Cb_GraphClubDominator( pOpt, pG );
    }
    
    if ( pOpt->fVerb ) {
        
        printf( "\nList of valid clubs:\n" );
        Cb_ClubListPrint( listClubs );
    }
    
    /* cluster the network based on the clubbing */
    Cb_NetworkClusterFromClubs( pNet, pG, listClubs );
    
    /* free memory space */
    Cb_ClubListFree( listClubs );
    Cb_GraphFreeVertices( pG );
    Cb_GraphFree( pG );
    
    return;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_NetworkClusterFromClubs( Ntk_Network_t *pNet, Cb_Graph_t *pG,
                            sarray_t *listClubs )
{
    int i;
    Cb_Graph_t  *pClub;
    
    sarrayForEachItem( Cb_Graph_t *, listClubs, i, pClub ) {
        
        if ( pClub->nVertices > 1 ) {  /* real clubs */
            
            Cb_NetworkClusterClub( pNet, pG, pClub );
        }
            
    }
    assert( Ntk_NetworkCheck( pNet ) );
    Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );
    
    return;
}


/**Function*************************************************************
  Synopsis    []
  Description [store a reference of a node in the pV->pData1 field of its
  corresponding vertex; store the number of values in the pV->nWeight field.]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
Cb_Graph_t *
Cb_GraphCreateFromNetwork( Ntk_Network_t *pNet ) 
{
    int          nCi, nCo, i;
    Cb_Graph_t  *pG;
    Cb_Vertex_t *pV;
    Ntk_Node_t  *pNode;
    
    nCi = Ntk_NetworkReadCiNum( pNet );
    nCo = Ntk_NetworkReadCoNum( pNet );
    pG  = Cb_GraphAlloc( nCo, nCi );
    
    /* traverse from inputs to outputs */
    Ntk_NetworkDfs( pNet, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode ) {
        
        pV = Cb_VertexCreateFromNode( pNode );
        Cb_GraphAddVertexToTail( pG, pV );
        
        /* cross reference between Vertex <-> node */
        Ntk_NodeSetData( pNode, (char *)pV );
        pV->pData1 = pNode;
        
        /* add to terminals */
        if ( Ntk_NodeIsCi( pNode ) ) {
            
            Cb_GraphAddLeaf( pG, pV );
        }
        if ( Ntk_NodeIsCo( pNode ) ) {
            
            Cb_GraphAddRoot( pG, pV );
        }
    }
    /* sweep to complete double-linked edges */
    Cb_GraphForEachVertex( pG, pV ) {
        
        if ( pV->nIn > 0 ) {
            
            for ( i=0; i < pV->nIn; ++i ) {
                
                CbVertexAddOutput( pV->pIn[i], pV );
            }
        }
    }
    
    return pG;
}

/**Function*************************************************************
  Synopsis    []
  Description [assume fanins have corresponding Vertices created. allocate
  space for input edges and fill them with fanin vertices; allocate
  space for output edges, but do not fill them.
  1) Store the number of values of a node in the pV->nWeight field
  2) store the number of F.F. literals in the pV->pData2 field as cost.]
  
  SideEffects []
  SeeAlso     []
***********************************************************************/
Cb_Vertex_t *
Cb_VertexCreateFromNode( Ntk_Node_t  *pNode ) 
{
    int             nFanin, nFanout, iIndex;
    Ntk_Node_t     *pFanin;
    Ntk_Pin_t      *pPin;
    Cb_Vertex_t    *pV, *pVin;
    Cvr_Cover_t    *pCvr;
    
    nFanin  = Ntk_NodeReadFaninNum( pNode );
    nFanout = Ntk_NodeReadFanoutNum( pNode );
    
    pV = Cb_VertexAlloc( nFanin, nFanout );
    pV->nWeigh = Ntk_NodeReadValueNum( pNode );
    
    if ( !Ntk_NodeIsCi( pNode ) && !Ntk_NodeIsCo( pNode ) ) {
        pCvr = Ntk_NodeGetFuncCvr( pNode );
        pV->pData2 = (void *) Cvr_CoverReadLitFacNum( pCvr );
    }
    
    iIndex = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin ) {
        
        pVin = (Cb_Vertex_t *)Ntk_NodeReadData( pFanin );
        assert( pVin );
        pV->pIn[iIndex++] = pVin;
    }
    
    return pV;
}



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function*************************************************************
  Synopsis    []
  Description [collapse nodes in one club]
  SideEffects []
  SeeAlso     []
***********************************************************************/
void
Cb_NetworkClusterClub( Ntk_Network_t *pNet, Cb_Graph_t *pG, Cb_Graph_t *pClub )
{
    int          i;
    Ntk_Node_t  *pNodeRoot, *pNodeTemp;
    Cb_Vertex_t *pV;
    
    /* first merge all root nodes */
    if ( sarray_n( pClub->pRoots ) == 1 ) {
        
        pV = sarray_fetch( Cb_Vertex_t *, pClub->pRoots, 0 );
        pNodeRoot = (Ntk_Node_t *) pV->pData1;
    }
    else {
        
        /* retrieve the first root node */
        pV = sarray_fetch( Cb_Vertex_t *, pClub->pRoots, 0 );
        pNodeRoot = pV->pData1;
        pV->pData1 = NULL;
        assert( pNodeRoot );
        assert( !Ntk_NodeIsCi( pNodeRoot ) );
        assert( !Ntk_NodeIsCo( pNodeRoot ) );
        
        /* successively merge with other roots */
        for ( i = 1; i < sarray_n( pClub->pRoots ); ++i ) {
            
            pV = sarray_fetch( Cb_Vertex_t *, pClub->pRoots, i );
            pNodeTemp = (Ntk_Node_t *)pV->pData1;
            pV->pData1 = NULL;
            
            assert( pNodeTemp );
            assert( !Ntk_NodeIsCi( pNodeTemp ) );
            assert( !Ntk_NodeIsCo( pNodeTemp ) );
            
            if ( Ntk_NodeReadFaninIndex( pNodeRoot, pNodeTemp ) != -1 ) {
                Ntk_NetworkCollapseNodes( pNodeRoot, pNodeTemp );
            }
            else if ( Ntk_NodeReadFaninIndex( pNodeTemp, pNodeRoot ) != -1 ) {
                Ntk_NetworkCollapseNodes( pNodeTemp, pNodeRoot );
            }
            
            pNodeRoot = Ntk_NetworkMergeNodes( pNodeRoot, pNodeTemp );
            assert( pNodeRoot != NULL );
            /* intermediate merged nodes are removed from the network */
        }
    }
    
    /* mark all vertices in this club */
    pG->iTravs++;
    Cb_GraphForEachVertexClub( pClub, pV ) {
        pV->iTravs = pG->iTravs;
    }
    
    /* successively collapse all nodes in the club in DFS order */
    for ( i = 0; i < sarray_n( pClub->pRoots ); ++i ) {
        
        pV = sarray_fetch( Cb_Vertex_t *, pClub->pRoots, i );
        pNodeRoot = Cb_NetworkClusterClub_rec( pG, pV, pNodeRoot );
    }
    /* remove all markings */
    Cb_GraphForEachVertexClub( pClub, pV ) {
        Cb_VertexResetFlag1( pV );
    }
    
    return;
}

/**Function*************************************************************
  Synopsis    []
  Description [recursively collapse nodes of a club in DFS order]
  SideEffects []
  SeeAlso     []
***********************************************************************/
Ntk_Node_t *
Cb_NetworkClusterClub_rec( Cb_Graph_t *pG, Cb_Vertex_t *pV, Ntk_Node_t *pNodeRoot ) 
{
    int i;
    Ntk_Node_t  *pNodeMerged;
    
    if ( pV == NULL || pV->iTravs != pG->iTravs )
        return NULL;
    
    pNodeMerged = pV->pData1;
    if ( pNodeMerged ) {
        //printf("%s ", Ntk_NodeGetNamePrintable( pNodeMerged ) );
    }
    else {
        pNodeMerged = pNodeRoot;
    }
    
    /* if this node exist, then merge its fanins first */
    if ( !Cb_VertexTestFlag1( pV ) ) {
        
        /* if it's been collapsed already then reuse */
        Cb_VertexSetFlag1( pV );
        for ( i = 0; i < pV->nIn; ++i ) {
            
            if ( pV->pIn[i]->iTravs == pG->iTravs ) {
                
                /* fanin inside the club */
                pNodeMerged = Cb_NetworkClusterClub_rec( pG, pV->pIn[i], pNodeMerged );
            }
        }
    }
    
    /* merge this node with its root at last */
    if ( pNodeMerged != pNodeRoot ) {
        
        Ntk_NetworkCollapseNodes( pNodeRoot, pNodeMerged );
    }
    
    return pNodeRoot;
}


/**Function*************************************************************
  Synopsis    []
  Description [return 1 if successful]
  SideEffects []
  SeeAlso     []
***********************************************************************/
int
CbVertexAddOutput( Cb_Vertex_t *pV, Cb_Vertex_t *pOut )
{
    int i;
    for ( i=0; i < pV->nOut ; ++i ) {
        if ( pV->pOut[i] == NULL ) {
            pV->pOut[i] = pOut;
            return 1;
        }
    }
    return 0;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


