/**CFile****************************************************************

  FileName    [fmInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fmInt.h,v 1.3 2003/05/27 23:15:33 alanmi Exp $]

***********************************************************************/

#ifndef __FM_INT_H__
#define __FM_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "util.h"
#include "array.h"
#include "extra.h"
#include "reo.h"
#include "shInt.h"

#include "vmx.h"
#include "mvr.h"
#include "mva.h"
#include "fm.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct FmsManagerStruct
{
    // the manager for global/local functions
    DdManager *     dd;         // this manager is used to create the global BDDs
    DdNode **       pbVars0;    // the primary variables
    DdNode **       pbVars1;    // the secondary variables
    int             nVars0;     // the number of allocated variables
    int             nVars1;     // the number of allocated variables
    // the global BDDs for the COs
    DdNode **       pbGlos;     // the BDDs for each CO value
    int             nGlos;      // the number of CO values
    // other managers
    Vm_Manager_t *  pManVm;     // the manager of MV var maps
    Vmx_Manager_t * pManVmx;    // the manager of extended var maps
    Mvr_Manager_t * pManMvr;    // the relation manager
    // the extended variable maps used
    Vmx_VarMap_t *  pVmxGlo;    // the map of the global space
    Vmx_VarMap_t *  pVmxGloS;   // the map of the global space with set outputs
    Vmx_VarMap_t *  pVmxGloL;   // the map of the global space for the leaves only
    Vmx_VarMap_t *  pVmxLoc;    // the map of the local space
    // this flag determines what methods are used
    bool            fBinary;    // set to 1 for purely binary deterministic networks  
    bool            fSetInputs; // set to 1 for ND networks to use set inputs  
    bool            fSetOutputs;// set to 1 for ND networks to use set outputs 
    bool            fVerbose;   // set to 1 to output details
};

struct FmwManagerStruct
{
    // the manager for global/local functions
    DdManager *     dd;         // this manager is used to create the global BDDs
    DdNode **       pbVars0;    // the primary variables
    DdNode **       pbVars1;    // the secondary variables
    int             nVarsAlloc; // the number of allocated variables
    // other managers
    Vm_Manager_t *  pManVm;     // the manager of MV var maps
    Vmx_Manager_t * pManVmx;    // the manager of extended var maps
    Mvr_Manager_t * pManMvr;    // the relation manager
    // the extended variable maps used
    Vmx_VarMap_t *  pVmxGlo;    // the map of the global space
    Vmx_VarMap_t *  pVmxGloS;   // the map of the global space with set outputs
    Vmx_VarMap_t *  pVmxLoc;    // the map of the local space
    // this flag determines what methods are used
    bool            fBinary;    // set to 1 for purely binary deterministic networks  
    bool            fSetInputs; // set to 1 for ND networks to use set inputs  
    bool            fSetOutputs;// set to 1 for ND networks to use set outputs 
    bool            fVerbose;   // set to 1 to output details
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== fmData.c =============================================================*/
extern DdNode *       Fm_DataGetNodeGlo( Sh_Node_t * pNode );
extern DdNode *       Fm_DataGetNodeGloZ( Sh_Node_t * pNode );
extern DdNode *       Fm_DataGetNodeGloS( Sh_Node_t * pNode );
extern void           Fm_DataSetNodeGlo( Sh_Node_t * pNode, DdNode * bGlo );
extern void           Fm_DataSetNodeGloZ( Sh_Node_t * pNode, DdNode * bGlo );
extern void           Fm_DataSetNodeGloS( Sh_Node_t * pNode, DdNode * bGlo );
extern void           Fm_DataSwapNodeGloAndGloZ( Sh_Node_t * pNode );
extern void           Fm_DataDerefNodeGlo( DdManager * dd, Sh_Node_t * pNode );
/*=== fmUtils.c =============================================================*/
extern Vmx_VarMap_t * Fm_UtilsCreateVarOrdering( Vmx_VarMap_t * pVmxGlo, Sh_Network_t * pNet );
extern void           Fm_UtilsAssignLeaves( DdManager * dd, DdNode * pbVars[], Sh_Network_t * pNet, Vmx_VarMap_t * pVmx, bool fOutputsOnly );
extern void           Fm_UtilsAssignLeavesSet( DdManager * dd, DdNode * pbVars[], Sh_Network_t * pNet, Vmx_VarMap_t * pVmx, bool fOutputsOnly );
extern void           Fm_UtilsDerefNetwork( DdManager * dd, Sh_Network_t * pNet );
extern void           Fm_UtilsResizeManager( DdManager * dd, Vmx_VarMap_t * pVmx );
/*=== fmImage.c =============================================================*/
extern DdNode *       Fm_ImageCompute( DdManager * dd, DdNode * bCareSet, DdNode ** pbFuncs, int nFuncs, DdNode ** pbVars, int nBddSize );
/*=== fmImageMv.c =============================================================*/
extern DdNode *       Fm_ImageMvCompute( DdManager * dd, DdNode * bCareSet, DdNode ** pbFuncs, int nFuncs, Vmx_VarMap_t * pVmxGlo, Vmx_VarMap_t * pVmxLoc, DdNode ** pbVars, int nBddSize );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif
