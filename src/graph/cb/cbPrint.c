/**CFile****************************************************************

  FileName    [cbPrint.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [print utilities]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cbPrint.c,v 1.2 2003/05/27 23:14:46 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "cb.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


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
Cb_NetworkPrintDominators( FILE *pOut, Ntk_Network_t *pNet, int nNodes ) 
{
    Cb_Graph_t  * pG;
    Cb_Vertex_t * pV;
    Ntk_Node_t  * pNode, * pNodeDomi ;
    int LengthMax, LenghtCur;
    int nColumns, iNode;
    
    /* compute dominators for all nodes */
    pG = Cb_GraphCreateFromNetwork( pNet );
    Cb_GraphDominators( pG );
    
    if ( nNodes == 1 )
    {
        pNode = Ntk_NetworkReadOrder( pNet );
        pV = (Cb_Vertex_t *)Ntk_NodeReadData( pNode );
        pNodeDomi = (Ntk_Node_t *)((pV->pDomin)?(pV->pDomin->pData1):NULL);
        
        fprintf( pOut, "%s : %s\n",
                 Ntk_NodeGetNamePrintable(pNode),
                 (pNodeDomi) ?
                 Ntk_NodeGetNamePrintable(pNodeDomi) : "ROOT");
        
        Cb_GraphFreeVertices( pG );
        Cb_GraphFree( pG );
        return;
    }
    
    // get the longest name
    LengthMax = 4;  /* ROOT */
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        LenghtCur = strlen( Ntk_NodeGetNamePrintable(pNode) );
        if ( LengthMax < LenghtCur )
            LengthMax = LenghtCur;
    }
    
    // decide how many columns to print
    nColumns = 80 / (LengthMax + LengthMax + 4);
    
    // print the node names
    iNode = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        pV = (Cb_Vertex_t *) Ntk_NodeReadData( pNode );
        pNodeDomi = (Ntk_Node_t *)((pV->pDomin)?(pV->pDomin->pData1):NULL);
        
        fprintf( pOut, "%*s : %-*s", LengthMax,
                 Ntk_NodeGetNamePrintable(pNode), LengthMax,
                 (pNodeDomi) ?
                 Ntk_NodeGetNamePrintable(pNodeDomi) : "ROOT");
        
        if ( ++iNode % nColumns == 0 )
            fprintf( pOut, "\n" ); 
        else
            fprintf( pOut, "  " ); 
    }
    if ( iNode % nColumns )
        fprintf( pOut, "\n" ); 
    
    /* deallocate memory */
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
Cb_NetworkPrintDot( FILE *pOut, Ntk_Network_t *pNet )
{
    Cb_Graph_t *pG;
    
    /* derive graph */
    pG = Cb_GraphCreateFromNetwork( pNet );
    
    /* print dot file */
    Cb_GraphPrintDot( pOut, pG );
    
    /* deallocate memory */
    Cb_GraphFreeVertices( pG );
    Cb_GraphFree( pG );
    return;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


