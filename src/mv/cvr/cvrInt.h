/**CFile****************************************************************

  FileName    [cvrInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of Cvr.]

  Author      [MVSIS Group]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cvrInt.h,v 1.11 2003/05/27 23:14:59 alanmi Exp $]

***********************************************************************/

#ifndef __CVR_INT_H__
#define __CVR_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

// it may be a good idea to make Cvr dependent only on 
// those packages that are actually used in Cvr 
// alternatively, "mv.h" can be included instead of these:
#include "util.h"
#include "string.h"
#include "vm.h"   
#include "mvc.h"   
#include "ft.h"   
#include "cvr.h"   

// espresso includes are no included in "mv.h"
//#include "espresso.h"
//#include "minimize.h"

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Struct*********************************************************************

  Synopsis  [ This is basic Cvr_Cover_t data structure ]

  Description   [Basic multi-valued output cover structure. When first 
  initialized, the cover structure does not allocate the compressed covers.
  A compression algorithm, when called on demand, will go through the set of 
  output values, assign a priority for each value according to its i-set using
  some heuristic, and derive the compressed cover (stored in cmp). The
  compression scheme is not supported right now.

  If the cube is a constant, the cubeStruct and compressed tables are NULL.
  There are special routines for handling these constant covers, including
  non-deterministic constant covers.]

  SideEffects   [ ]

******************************************************************************/
struct CvrCoverStruct {
    Mvc_Cover_t **  ppCovers;    /* array of cube covers  */
    Vm_VarMap_t *   pVm;         /* the variable map */
    Ft_Tree_t *     pTree;       /* the factored tree */
//    unsigned       *lPorder;   /* priority order for all values */
//    pset_family    *pCompr;    /* array of compressed cube covers */
};

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

extern bool Cvr_CoverEspressoSpecial( Cvr_Cover_t *pCvr );
extern void Cvr_CoverEspressoSetup( Vm_VarMap_t *pVm);
extern void Cvr_CoverEspressoSetdown( Vm_VarMap_t *pVm);
extern bool Cvr_CoverIsTooLarge( Cvr_Cover_t *pCf );
extern bool Cvr_IsetIsTooLarge( Vm_VarMap_t *pVm, Mvc_Cover_t *pOnset );


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
