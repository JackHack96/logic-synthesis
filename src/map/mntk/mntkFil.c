/**CFile****************************************************************

  FileName    [mntkFil.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mntkFil.c,v 1.1 2005/02/28 05:34:29 alanmi Exp $]

***********************************************************************/

#include "mntkInt.h"

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
Mntk_Cut_t * Mntk_CheckCut( Mntk_Node_t * pRoot, Mntk_Node_t * ppNodes[], int nNodes, int fFilterTop )
{
    Mntk_Man_t * p = pRoot->p;
    Mntk_Cut_t * pCutFound;
    int clk;
//    int RetValue;

    p->nCutsTried++;
    assert( nNodes > 0 && nNodes <= 6 );

    Mntk_CheckNoDuplicates( pRoot, ppNodes, nNodes );

clk = clock();
//    RetValue = Mntk_NodeBddsCheckResub( pRoot, ppNodes, nNodes );
p->time1 += clock() - clk;

/*
if ( fFilterTop )
{
clk = clock();
    if ( Mntk_TestTopological( p, pRoot, ppNodes, nNodes ) )
    {
        p->nCutsTop++;
p->timeFilTop += clock() - clk;
        return NULL;
    }
p->timeFilTop += clock() - clk;
}
*/

clk = clock();
    if ( !Mntk_TestNodes( p, pRoot, ppNodes, nNodes ) )
    {
//        assert( RetValue == 0 );
        p->nCutsSim++;
p->timeFilSim += clock() - clk;
        return NULL;
    }
p->timeFilSim += clock() - clk;


clk = clock();
    pCutFound = Mntk_CheckSat( p, pRoot, ppNodes, nNodes );
p->timeFilSat += clock() - clk;
    if ( pCutFound == NULL )
    {
//        assert( RetValue == 0 );
        return NULL;
    }
//    assert( RetValue == 1 );

if ( p->fVerbose > 1 )
Mntk_NodePrint( pCutFound );
    return pCutFound;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


