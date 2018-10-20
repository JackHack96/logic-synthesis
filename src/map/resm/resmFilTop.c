/**CFile****************************************************************

  FileName    [resmFilTop.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmFilTop.c,v 1.1 2005/01/23 06:59:52 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Resm_TestTopological_rec( Resm_Node_t * pNode, int LevelMin );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks that the cut is not a topological cut of the root.]

  Description [Returns 1 if the cut is topological.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_TestTopological( Resm_Man_t * pMan, Resm_Node_t * pRoot, Resm_Node_t * pNodes[], int nNodes )
{
    int LevelMin, i, RetValue;

    // mark the nodes of the cut
    LevelMin = 100000;
    for ( i = 0; i < nNodes; i++ )
    {
        pNodes[i]->fUsed = 1;
//        if ( LevelMin > (int)pNodes[i]->Level )
//            LevelMin = (int)pNodes[i]->Level;
    }

    // start the search which terminates when a PI is encountered
    // or when a node that is less than LevelMin is encountered
    RetValue = Resm_TestTopological_rec( pRoot, LevelMin );

    // unmark the nodes of the cut
    for ( i = 0; i < nNodes; i++ )
        pNodes[i]->fUsed = 0;
    return RetValue;
}
/**Function*************************************************************

  Synopsis    [Checks that the cut is not a topological cut of the root.]

  Description [Returns 1 if the cut is topological.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_TestTopological_rec( Resm_Node_t * pNode, int LevelMin )
{
    int i;
    assert( !Resm_IsComplement(pNode) );
    // if the node is part of the cut, it looks like the cut is topological
    if ( pNode->fUsed )
        return 1;
    // if the node is a PI, the cut is NOT topological
    if ( Resm_NodeIsVar(pNode) )
        return 0;
    // if the level of the node is below the cut, the cut is NOT topological
//    if ( (int)pNode->Level < LevelMin )
//        return 0;
    for ( i = 0; i < (int)pNode->nLeaves; i++ )
        if ( !Resm_TestTopological_rec( Resm_Regular(pNode->ppLeaves[i]), LevelMin ) )
            return 0;
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


