/**CFile****************************************************************

  FileName    [resFil.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resFil.c,v 1.1 2005/01/23 06:59:47 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

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
Fpga_Cut_t * Res_CheckCut( Res_Man_t * p, Fpga_Node_t * pRoot, Fpga_Node_t * ppNodes[], int nNodes, int fFilterTop )
{
    Fpga_Cut_t * pCut;
    float Area1, Area2;
    int clk, i, v;

    p->nCutsTried++;

    /////////////////////////////////////////////////////////
    // make sure the nodes are not repeated
    for ( i = 0; i < nNodes; i++ )
        if ( pRoot == ppNodes[i] )
        {
            printf( "Res_CheckCut(): Duplicated nodes.\n" );
            assert( 0 );
        }
    for ( i = 0; i < nNodes; i++ )
    for ( v = i + 1; v < nNodes; v++ )
        if ( ppNodes[v] == ppNodes[i] )
        {
            printf( "Res_CheckCut(): Duplicated nodes.\n" );
            assert( 0 );
        }
    /////////////////////////////////////////////////////////


if ( fFilterTop )
{
clk = clock();
    if ( Res_TestTopological( p, pRoot, ppNodes, nNodes ) )
    {
        p->nCutsTop++;
p->timeFilTop += clock() - clk;
        return NULL;
    }
p->timeFilTop += clock() - clk;
}

clk = clock();
    if ( !Res_TestNodes( p, pRoot, ppNodes, nNodes ) )
    {
        p->nCutsSim++;
p->timeFilSim += clock() - clk;
        return NULL;
    }
p->timeFilSim += clock() - clk;

clk = clock();
    pCut = Res_CheckSat( p, pRoot, ppNodes, nNodes );
p->timeFilSat += clock() - clk;
    if ( pCut == NULL )
        return NULL;

    // compute the arrival times for the non-referenced cuts in the TFI
    pCut->tArrival = Fpga_TimeCutComputeArrival_rec( p->pMan, pCut );
    // compute the area
    Area1 = Fpga_CutDeref( p->pMan, NULL, pRoot->pCutBest, 0 );
    pCut->aFlow = Fpga_CutGetAreaDerefed( p->pMan, pCut );
    Area2 = Fpga_CutRef( p->pMan, NULL, pRoot->pCutBest, 0 );
    assert( Area1 == Area2 );

if ( p->fVerbose )
Fpga_CutPrint( p->pMan, pRoot, pCut );
    return pCut;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


