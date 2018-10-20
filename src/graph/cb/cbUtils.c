/**CFile****************************************************************

  FileName    [cbUtils.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [utilities of the Cb package]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cbUtils.c,v 1.2 2003/05/27 23:14:46 alanmi Exp $]

***********************************************************************/

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
Cb_GraphPrintDot( FILE *pFile, Cb_Graph_t *pG ) 
{
    int           i;
    Cb_Vertex_t  * pV;
    Ntk_Node_t   * pN1, * pN2;
    
    /* generic header */
    fprintf( pFile, "# DOT generated from MVSIS Club package\n" );
    fprintf( pFile, "digraph \"Graph\" {\n  rotate=90;\n" );
    fprintf( pFile, "  margin=0.5;\n  label=\"Graph\";\n" );
    fprintf( pFile, "  size=\"10,7.5\";\n  ratio=\"fill\";\n" );
    
    /* roots and leaves are at the same level */
    fprintf( pFile, "  { rank=same; ");
    sarrayForEachItem ( Cb_Vertex_t *, pG->pLeaves, i, pV ) {
        
        if ( (pN1 = (Ntk_Node_t *) pV->pData1 ) ) {
            fprintf( pFile, "\"%s\"; ", Ntk_NodeGetNamePrintable(pN1) );
        } else {
            fprintf( pFile, "\"%d\"; ", pV->Id );
        }
    }
    fprintf( pFile, "}\n");
    fprintf( pFile, "  { rank=same; ");
    sarrayForEachItem ( Cb_Vertex_t *, pG->pRoots, i, pV ) {
        
        if ( (pN1 = (Ntk_Node_t *) pV->pData1 ) ) {
            if ( Ntk_NodeIsCo( pN1 ) )  //special treatment for CO
                fprintf( pFile, "\"O%s\"; ", Ntk_NodeGetNamePrintable(pN1) );
            else
                fprintf( pFile, "\"%s\"; ", Ntk_NodeGetNamePrintable(pN1) );
        } else {
            fprintf( pFile, "\"%d\"; ", pV->Id );
        }
    }
    fprintf( pFile, "}\n");
    
    /* print all edges */
    Cb_GraphForEachVertex( pG, pV ) {
        
        /* if node exist, then print out its current name */
        if ( pV->pData1 ) {
            
            pN1 = (Ntk_Node_t *) (pV->pData1);
            for ( i=0; i<pV->nOut; ++i ) {
                
                fprintf( pFile, "  \"%s\" ", Ntk_NodeGetNamePrintable( pN1 ) );
                /* PO names are prefixed with "O" */
                pN2 = (Ntk_Node_t *) (pV->pOut[i]->pData1);
                if ( Ntk_NodeIsCo( pN2 ) )
                    fprintf( pFile, "-> \"O%s\";\n",
                             Ntk_NodeGetNamePrintable( pN2 ) );
                else 
                    fprintf( pFile, "-> \"%s\";\n",
                             Ntk_NodeGetNamePrintable( pN2 ) );
            }
        }
        /* otherwise, print out its I.D. */
        else {
            
            for ( i=0; i<pV->nOut; ++i ) {
                fprintf( pFile, "  \"%d\" -> \"%d\";\n",
                         pV->Id, pV->pOut[i]->Id );
            }
        }
    }
    
    fprintf( pFile, "}\n" );
    
    return;
}


/**Function*************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
***********************************************************************/
Cb_Vertex_t *
Cb_ListSpecialRemoveByIndex( Cb_Vertex_t **ppHead, int iSpecial )
{
    int          iIndex;
    Cb_Vertex_t *pHead, *pPrev, *pV;
    
    pHead = *ppHead;
    
    if ( iSpecial == 0 ) {
        *ppHead = pHead->pNextSpec;
        return pHead;
    }
    iIndex = 1;
    for ( pV = pHead->pNextSpec, pPrev = pHead;
          pV;
          pV = pV->pNextSpec ) {
        
        if ( iIndex == iSpecial ) {
            pPrev->pNextSpec = pV->pNextSpec;
            break;
        }
        pPrev = pV;
        iIndex++;
    }
    
    return pV;
}




/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


