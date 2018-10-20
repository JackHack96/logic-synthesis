/**CFile****************************************************************

  FileName    [vm.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Declarations of the variable map (VM) package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vm.h,v 1.14 2003/09/01 04:56:57 alanmi Exp $]

***********************************************************************/

#ifndef __VM_H__
#define __VM_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "mvtypes.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

typedef struct VmManagerStruct    Vm_Manager_t;    // the MV relation manager
typedef struct VmVarMapStruct     Vm_VarMap_t;     // the mapping of variables

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== Vm.c =======================================================*/
/*=== VmApi.c ====================================================*/
extern Vm_Manager_t *   Vm_VarMapReadMan( Vm_VarMap_t * p );
extern int              Vm_VarMapReadVarsInNum( Vm_VarMap_t * p );
extern int              Vm_VarMapReadVarsOutNum( Vm_VarMap_t * p );
extern int              Vm_VarMapReadVarsNum( Vm_VarMap_t * p );
extern int              Vm_VarMapReadValuesInNum( Vm_VarMap_t * p );
extern int              Vm_VarMapReadValuesOutNum( Vm_VarMap_t * p );
extern int              Vm_VarMapReadValuesNum( Vm_VarMap_t * p );
extern int              Vm_VarMapReadValues( Vm_VarMap_t * p, int iVar );
extern int              Vm_VarMapReadValuesFirst( Vm_VarMap_t * p, int iVar );
extern int              Vm_VarMapReadValuesOutput( Vm_VarMap_t * p );
extern int *            Vm_VarMapReadValuesArray( Vm_VarMap_t * p );
extern int *            Vm_VarMapReadValuesFirstArray( Vm_VarMap_t * p );
extern int *            Vm_VarMapGetStorageSupport1( Vm_VarMap_t * p );
extern int *            Vm_VarMapGetStorageSupport2( Vm_VarMap_t * p );
extern int *            Vm_VarMapGetStoragePermute( Vm_VarMap_t * p );
extern int *            Vm_VarMapGetStorageArray1( Vm_VarMap_t * p );
extern int *            Vm_VarMapGetStorageArray2( Vm_VarMap_t * p );
extern bool             Vm_VarMapIsBinary( Vm_VarMap_t * p );
extern bool             Vm_VarMapIsBinaryInput( Vm_VarMap_t * p );
extern int              Vm_VarMapGetBitsNum( Vm_VarMap_t * pVm );
/*=== VmMan.c ====================================================*/
extern Vm_Manager_t *   Vm_ManagerAlloc();
extern void             Vm_ManagerFree( Vm_Manager_t * p );
extern Vm_Manager_t *   Vm_ManagerRef( Vm_Manager_t * p );
extern void             Vm_ManagerDeref( Vm_Manager_t * p );
/*=== VmMap.c ====================================================*/
extern Vm_VarMap_t *    Vm_VarMapLookup( Vm_Manager_t * p, int nVarsIn, int nVarsOut, int * pValues );
extern Vm_VarMap_t *    Vm_VarMapRef( Vm_VarMap_t * pVm );
extern Vm_VarMap_t *    Vm_VarMapDeref( Vm_VarMap_t * pVm );
/*=== VmUtils.c ====================================================*/
extern Vm_VarMap_t *    Vm_VarMapCreateBinary( Vm_Manager_t * p, int nVarsIn, int nVarsOut );
extern Vm_VarMap_t *    Vm_VarMapCreateOneDiff( Vm_VarMap_t * pVm, int iVar, int nVarValuesNew );
extern Vm_VarMap_t *    Vm_VarMapCreateOnePlus( Vm_VarMap_t * pVm, int nVarValuesNew );
extern Vm_VarMap_t *    Vm_VarMapCreateOneMinus( Vm_VarMap_t * pVm );
extern Vm_VarMap_t *    Vm_VarMapCreateExpanded( Vm_VarMap_t * pVmBase, Vm_VarMap_t * pVmExt, int * pVarsUsed );
extern Vm_VarMap_t *    Vm_VarMapCreateReduced( Vm_VarMap_t * pVm, int * pVarsUsed );
extern Vm_VarMap_t *    Vm_VarMapCreatePermuted( Vm_VarMap_t * pVm, int * pPermuteInv );
extern Vm_VarMap_t *    Vm_VarMapCreatePower( Vm_VarMap_t * pVm, int Degree );
extern Vm_VarMap_t *    Vm_VarMapCreateExtended( Vm_VarMap_t * pVm, int nValues );
extern Vm_VarMap_t *    Vm_VarMapCreateAdded( Vm_VarMap_t * pVm1, Vm_VarMap_t * pVm2 );
extern Vm_VarMap_t *    Vm_VarMapCreateInputOutput( Vm_VarMap_t * pVmIn, Vm_VarMap_t * pVmOut );
extern Vm_VarMap_t *    Vm_VarMapCreateInputOutputSet( Vm_VarMap_t * pVmIn, Vm_VarMap_t * pVmOut, bool fUseSetIns, bool fUseSetOuts );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
