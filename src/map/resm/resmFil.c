/**CFile****************************************************************

  FileName    [resmFil.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmFil.c,v 1.2 2005/02/28 05:34:33 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

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
Resm_Cut_t * Resm_CheckCut( Resm_Node_t * pRoot, Resm_Node_t * ppNodes[], int nNodes, int fFilterTop )
{
    Resm_Man_t * p = pRoot->p;
    Resm_Cut_t * pCutFound;
    int clk;
//    int RetValue;

    p->nCutsTried++;
    assert( nNodes > 0 && nNodes <= 6 );

    Resm_CheckNoDuplicates( pRoot, ppNodes, nNodes );

clk = clock();
//    RetValue = Resm_NodeBddsCheckResub( pRoot, ppNodes, nNodes );
p->time1 += clock() - clk;

/*
if ( fFilterTop )
{
clk = clock();
    if ( Resm_TestTopological( p, pRoot, ppNodes, nNodes ) )
    {
        p->nCutsTop++;
p->timeFilTop += clock() - clk;
        return NULL;
    }
p->timeFilTop += clock() - clk;
}
*/

clk = clock();
    if ( !Resm_TestNodes( p, pRoot, ppNodes, nNodes ) )
    {
//        assert( RetValue == 0 );
        p->nCutsSim++;
p->timeFilSim += clock() - clk;
        return NULL;
    }
p->timeFilSim += clock() - clk;


clk = clock();
    pCutFound = Resm_CheckSat( p, pRoot, ppNodes, nNodes );
p->timeFilSat += clock() - clk;
    if ( pCutFound == NULL )
    {
//        assert( RetValue == 0 );
        return NULL;
    }
//    assert( RetValue == 1 );

if ( p->fVerbose > 1 )
Resm_NodePrint( pCutFound );
    return pCutFound;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


