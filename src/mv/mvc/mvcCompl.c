/**CFile****************************************************************

  FileName    [mvcCompl.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The complement of the MV cover.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcCompl.c,v 1.7 2003/09/01 04:56:50 alanmi Exp $]

***********************************************************************/

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
Mvc_Cover_t * Mvc_CoverComplement( Mvc_Data_t * p, Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pTau;
    Mvc_Cover_t * pRes;
    pTau = Mvc_CoverCreateTautology( pCover );
    pRes = Mvc_CoverSharp( p, pTau, pCover );
    Mvc_CoverFree( pTau );
    Mvc_CoverContain( pRes );
    return pRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


