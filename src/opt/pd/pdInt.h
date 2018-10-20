/**CFile****************************************************************

  FileName    [pdInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: pdInt.h,v 1.6 2003/05/28 04:08:11 alanmi Exp $]

***********************************************************************/

#ifndef _PDINT
#define _PDINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/
#include "mv.h"
#include "simp.h"
#include "pd.h"
#include "pdHash.h"

#define NTK_COST_LIT        1
#define NTK_COST_CUBE       0
#define INFINITY        (1 << 30)
#define RESUB_SIMP          1
#define RESUB_ALGEBRAIC     0



#endif /* _PDINT */
