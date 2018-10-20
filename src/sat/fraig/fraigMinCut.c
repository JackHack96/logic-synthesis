/**CFile****************************************************************

  FileName    [fraigMinSet.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Computing minimimal cutset of a cyclic circuit.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigMinCut.c,v 1.2 2005/07/08 01:01:32 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"
#include "graInt.h"

/* 
    Implementation is based on the paper: D. H. Lee, S. M. Reddy, "On determining 
    scan flip-flops in partial-scan designs", Proc. ICCAD '90, pp. 322-325.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// write and read the graph node corresponding to the FRAIG node
#define FRAIG_SET_GRA(pNode, pGra)     (pNode->pData0 = (Fraig_Node_t *)(pGra))
#define FRAIG_READ_GRA(pNode)          ((Gra_Node_t *)pNode->pData0) 

static Gra_Graph_t * Fraig_Man2Graph( Fraig_Man_t * pMan, int nLatches, Gra_Node_t ** ppGraHost );
static Gra_Graph_t * Fraig_Man2GraphLatch( Fraig_Man_t * pMan, int nLatches, Gra_Node_t ** ppGraHost );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Compute the minimal cutset of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManCutSet( Fraig_Man_t * pMan, int nLatches )
{
    Gra_Graph_t * p;
    Gra_Node_t * pGraHost;
    int clk;
    // create the graph
clk = clock();
    p = Fraig_Man2Graph( pMan, nLatches, &pGraHost );
//    p = Fraig_Man2GraphLatch( pMan, nLatches, &pGraHost );
PRT( "Setup", clock() - clk );
    // solve the problem
clk = clock();
    Gra_GraphMinCut( p, pGraHost );
PRT( "Solve", clock() - clk );
    // delete the graph
    Gra_GraphDelete( p );
}

/**Function*************************************************************

  Synopsis    [Prepare the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_Graph_t * Fraig_Man2Graph( Fraig_Man_t * pMan, int nLatches, Gra_Node_t ** ppGraHost )
{
    Gra_Graph_t * p;
    Gra_Node_t * pGraHost, * pGraNodeIn, * pGraNodeOut;
    Fraig_Node_t * pNodeIn, * pNodeOut, * pNode;
    int i, nSelfLoops;

    // create the graph
    p = Gra_GraphCreate();

    // create the graph nodes for each FRAIG node, except constant
    Gra_NodeCreate(p, 0);
    for ( i = 1; i < pMan->vNodes->nSize; i++ )
    {
        pNode = pMan->vNodes->pArray[i];
        FRAIG_SET_GRA( pNode, Gra_NodeCreate(p, i) );
    }

    // create the internal edges
    for ( i = 0; i < pMan->vNodes->nSize; i++ )
    {
        if ( !Fraig_NodeIsAnd(pMan->vNodes->pArray[i]) )
            continue;
        pNodeOut    = pMan->vNodes->pArray[i];
        pGraNodeOut = FRAIG_READ_GRA(pNodeOut);
        // add the first edge
        pNodeIn    = Fraig_Regular( pNodeOut->p1 );
        pGraNodeIn = FRAIG_READ_GRA(pNodeIn);
        Gra_EdgeCreate( pGraNodeIn, pGraNodeOut );
        // add the second edge
        pNodeIn    = Fraig_Regular( pNodeOut->p2 );
        pGraNodeIn = FRAIG_READ_GRA(pNodeIn);
        Gra_EdgeCreate( pGraNodeIn, pGraNodeOut );
    }

    // create the latch edges
    nSelfLoops = 0;
    for ( i = 0; i < nLatches; i++ )
    {
        pNodeIn  = pMan->vInputs->pArray[pMan->vInputs->nSize - nLatches + i];
        pNodeOut = Fraig_Regular( pMan->vOutputs->pArray[pMan->vOutputs->nSize - nLatches + i] );
        // skip self-loops
        if ( pNodeIn == pNodeOut )
        {
            nSelfLoops++;
            continue;
        }
        // skip constant nodes
        if ( pNodeOut == pMan->pConst1 )
            continue;
        // add the latch edge
        pGraNodeIn = FRAIG_READ_GRA(pNodeIn);
        pGraNodeOut = FRAIG_READ_GRA(pNodeOut);
        Gra_EdgeCreate( pGraNodeOut, pGraNodeIn );
    }
    printf( "Detected %d self-loops.\n", nSelfLoops );

    // add the host node
    pGraHost = Gra_NodeCreate( p, pMan->vNodes->nSize );
    // constant becomes input of the host node
    Gra_EdgeCreate( p->lNodes.pHead, pGraHost );
    // the true POs become inputs of the host node
    for ( i = 0; i < pMan->vOutputs->nSize - nLatches; i++ )
    {
        pNodeOut = Fraig_Regular( pMan->vOutputs->pArray[i] );
        if ( pNodeOut == pMan->pConst1 )
            continue;
        pGraNodeOut = FRAIG_READ_GRA(pNodeOut);
        Gra_EdgeCreate( pGraNodeOut, pGraHost );
    }
    // the true PIs become outputs of the host node
    for ( i = 0; i < pMan->vInputs->nSize - nLatches; i++ )
    {
        pNodeIn = pMan->vInputs->pArray[i];
        pGraNodeIn = FRAIG_READ_GRA(pNodeIn);
        Gra_EdgeCreate( pGraHost, pGraNodeIn );
    }

    *ppGraHost = pGraHost;
    return p;
}

#if 0

/**Function*************************************************************

  Synopsis    [Prepare the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gra_Graph_t * Fraig_Man2GraphLatch( Fraig_Man_t * pMan, int nLatches, Gra_Node_t ** ppGraHost )
{
    Gra_Graph_t * p;
    Gra_Node_t * pGraHost;
    Gra_Node_t ** pGraLatches;
    Fraig_Node_t * pNode;
    int i, k, iVarIn, iVarOut, nSelfLoops;

    // create the graph
    p = Gra_GraphCreate();

    // create the graph nodes for each latch
    pGraLatches = ALLOC( Gra_Node_t *, nLatches );
    for ( i = 0; i < nLatches; i++ )
        pGraLatches[i] = Gra_NodeCreate(p, i);

    // set the latch dependencies using the support info
    nSelfLoops = 0;
    for ( i = 0; i < nLatches; i++ ) // input latch
        for ( k = 0; k < nLatches; k++ ) // output latch
        {
            iVarIn  = pMan->vInputs->nSize - nLatches + i;
            iVarOut = pMan->vOutputs->nSize - nLatches + k;
            pNode   = Fraig_Regular( pMan->vOutputs->pArray[iVarOut] );
            if ( !Fraig_NodeHasVarStr( pNode, iVarIn ) )
                continue;
            // there is dependency
            if ( i == k )
            {
                nSelfLoops++;
                continue;
            }
            Gra_EdgeCreate( pGraLatches[i], pGraLatches[k] );
        }
    printf( "Detected %d self-loops.\n", nSelfLoops );

    // add the host node
    pGraHost = Gra_NodeCreate( p, pMan->vNodes->nSize );
    // the true POs become inputs of the host node
    for ( i = 0; i < nLatches; i++ )
        Gra_EdgeCreate( pGraLatches[i], pGraHost );
    *ppGraHost = pGraHost;

//Gra_GraphPrint( p );
    return p;
}

#endif

///////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


