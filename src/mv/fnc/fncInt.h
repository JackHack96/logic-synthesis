/**CFile****************************************************************

  FileName    [fncInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Represetation of functionality in MVSIS.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fncInt.h,v 1.14 2003/11/18 18:55:03 alanmi Exp $]

***********************************************************************/

#ifndef __FNC_INT_H__
#define __FNC_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "vmInt.h"
#include "vmxInt.h"
#include "mvrInt.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct FncManagerStruct
{
    Mvr_Manager_t *     pManMvr;    // the Mvr manager (contains local BDD, perm and reo managers)
    Vm_Manager_t *      pManVm;     // the variable map manager
    Vmx_Manager_t *     pManVmx;    // the extended variable map manager
    Mvc_Manager_t *     pManMvc;    // the MVC manager
    int                 nRefs;      // the reference counter of the manager
};

struct FncFunctionStruct
{
    Vm_VarMap_t *       pVm;        // the variable map
    Cvr_Cover_t *       pCvr;       // the MV SOP
    Mvr_Relation_t *    pMvr;       // the local MDD of MV relation
    DdNode **           pGlo;       // the global BDDs for each i-set
};

struct FncGlobalStruct
{
    DdManager *         dd;         // the BDD manager to represent the global functions
    DdNode **           pbGlo;      // the i-sets of the global BDDs
    DdNode **           pbGloZ;     // the i-sets of the parametrized global BDDs
    DdNode **           pbGloS;     // the i-sets of the parametrized global BDDs
    short               nNumber;    // the number of this node in the universal order
    char                nValues;    // the number of values
    char                fNonDet;    // marks the non-deterministic node
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== file.c ====================================================*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

