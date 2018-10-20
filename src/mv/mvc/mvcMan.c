/**CFile****************************************************************

  FileName    [mvcMan.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures working with the MVC memory manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcMan.c,v 1.6 2003/11/18 18:55:05 alanmi Exp $]

***********************************************************************/

#include <string.h>
#include "mvc.h"

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
Mvc_Manager_t * Mvc_ManagerStart()
{
    Mvc_Manager_t * p;
    p = ALLOC( Mvc_Manager_t, 1 );
    memset( p, 0, sizeof(Mvc_Manager_t) );
    p->pMan1 = Extra_MmFixedStart( sizeof(Mvc_Cube_t),                              1000, 1000 );
    p->pMan2 = Extra_MmFixedStart( sizeof(Mvc_Cube_t) +     sizeof(Mvc_CubeWord_t), 1000, 1000 );
    p->pMan4 = Extra_MmFixedStart( sizeof(Mvc_Cube_t) + 3 * sizeof(Mvc_CubeWord_t), 1000, 1000 );
    p->pManC = Extra_MmFixedStart( sizeof(Mvc_Cover_t), 1000, 1000 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_ManagerFree( Mvc_Manager_t * p )
{
    Extra_MmFixedStop( p->pMan1, 0 );
    Extra_MmFixedStop( p->pMan2, 0 );
    Extra_MmFixedStop( p->pMan4, 0 );
    Extra_MmFixedStop( p->pManC, 0 );
    free( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


