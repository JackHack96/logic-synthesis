/**CFile****************************************************************

  FileName    [mvr.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Functionality of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvrMan.c,v 1.4 2003/05/27 23:15:19 alanmi Exp $]

***********************************************************************/

#include "mvrInt.h"

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
Mvr_Manager_t * Mvr_ManagerAlloc()
{
    Mvr_Manager_t * pMan;
    // allocate the var map manager
    pMan = ALLOC( Mvr_Manager_t, 1 );
    memset( pMan, 0, sizeof(Mvr_Manager_t) );
    // start the local manager
    pMan->pDdLoc = Cudd_Init( 50, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_zddVarsFromBddVars( pMan->pDdLoc, 2 );
    // start other managers
    pMan->pPerm = Extra_PermutationManagerInit();
    pMan->pReo  = Extra_ReorderInit( pMan->pDdLoc->sizeZ, 10000 );
//	Extra_ReorderSetRemapping( pMan->pReo, 1 );
//	Extra_ReorderSetVerification( pMan->pReo, 1 );
    return pMan;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_ManagerFree( Mvr_Manager_t * pMan )
{
	// stop the managers
	Extra_PermutationManagerQuit( pMan->pPerm );
	Extra_ReorderQuit( pMan->pReo );
	Extra_StopManager( pMan->pDdLoc );
    free( pMan );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


