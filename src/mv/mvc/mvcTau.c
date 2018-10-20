/**CFile****************************************************************

  FileName    [mvcTau.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Impelementation of MV tautology checking.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcTau.c,v 1.7 2003/09/01 04:56:55 alanmi Exp $]

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
bool Mvc_CoverTautology( Mvc_Data_t * p, Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pCompl;
    bool RetValue;
    pCompl = Mvc_CoverComplement( p, pCover );
    RetValue = (int)(Mvc_CoverReadCubeNum(pCompl) == 0);
    Mvc_CoverFree( pCompl );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


