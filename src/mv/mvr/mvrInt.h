/**CFile****************************************************************

  FileName    [mvrInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvrInt.h,v 1.15 2003/11/18 18:55:05 alanmi Exp $]

***********************************************************************/

#ifndef __MVR_INT_H__
#define __MVR_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "vmInt.h"
#include "vmxInt.h"
#include "mvtypes.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

enum { MVR_UNKNOWN, MVR_DETERM, MVR_NONDETERM }; // the constants used to label the relation

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct MvrManagerStruct
{
    DdManager *         pDdLoc;       // the BDD manager to represent local functions
    reo_man *           pReo;         // the reording manager
    Extra_PermMan_t *   pPerm;        // the permutation manager
};

struct MvrRelationStruct
{
    Mvr_Manager_t *     pMan;         // the relation manager 
    DdNode *            bRel;         // the relation
    Vmx_VarMap_t *      pVmx;         // the extended variable map of this relation
    unsigned            nBddNodes:29; // the number of the MV vars
    unsigned            fMark    :1;  // the flag used to mark the nodes with non-well-defined minterms added
    unsigned            NonDet   :2;  // the flag used to mark the ND relations
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

#define  BDD_NODES_UNKNOWN  0xfffffff


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== mvr.c =============================---=======================*/
/*=== mvrMan.c ====================================================*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
