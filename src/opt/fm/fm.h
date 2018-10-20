/**CFile****************************************************************

  FileName    [fm.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fm.h,v 1.3 2003/05/27 23:15:32 alanmi Exp $]

***********************************************************************/

#ifndef __FM_H__
#define __FM_H__

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct FmwManagerStruct      Fmw_Manager_t;   
typedef struct FmsManagerStruct      Fms_Manager_t;   

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

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

/*=== fmsApi.c ===========================================================*/
extern bool             Fms_ManagerIsBinary    ( Fms_Manager_t * pFm );
extern bool             Fms_ManagerIsSetInputs ( Fms_Manager_t * pFm );
extern bool             Fms_ManagerIsSetOutputs( Fms_Manager_t * pFm );
extern bool             Fms_ManagerIsVerbose   ( Fms_Manager_t * pFm );
extern void             Fms_ManagerSetBinary    ( Fms_Manager_t * pFm, bool fFlag );
extern void             Fms_ManagerSetSetInputs ( Fms_Manager_t * pFm, bool fFlag );
extern void             Fms_ManagerSetSetOutputs( Fms_Manager_t * pFm, bool fFlag );
extern void             Fms_ManagerSetVerbose   ( Fms_Manager_t * pFm, bool fFlag );
extern Fms_Manager_t *  Fms_ManagerAlloc( Vm_Manager_t *  pManVm, Vmx_Manager_t * pManVmx, Mvr_Manager_t * pManMvr );
extern void             Fms_ManagerFree( Fms_Manager_t * pMan );
extern void             Fms_ManagerResize( Fms_Manager_t * pMan, int nBinVars );
/*=== fmsFlex.c ===========================================================*/
extern bool             Fms_FlexibilityPrepare( Fms_Manager_t * pMan, Sh_Network_t * pShNet );
extern Mvr_Relation_t * Fms_FlexibilityCompute( Fms_Manager_t * pMan, Sh_Network_t * pShNet, Mvr_Relation_t * pMvrLoc );


/*=== fmwApi.c ===========================================================*/
extern bool             Fmw_ManagerIsBinary    ( Fmw_Manager_t * pFm );
extern bool             Fmw_ManagerIsSetInputs ( Fmw_Manager_t * pFm );
extern bool             Fmw_ManagerIsSetOutputs( Fmw_Manager_t * pFm );
extern bool             Fmw_ManagerIsVerbose   ( Fmw_Manager_t * pFm );
extern void             Fmw_ManagerSetBinary    ( Fmw_Manager_t * pFm, bool fFlag );
extern void             Fmw_ManagerSetSetInputs ( Fmw_Manager_t * pFm, bool fFlag );
extern void             Fmw_ManagerSetSetOutputs( Fmw_Manager_t * pFm, bool fFlag );
extern void             Fmw_ManagerSetVerbose   ( Fmw_Manager_t * pFm, bool fFlag );
extern Fmw_Manager_t *  Fmw_ManagerAlloc( Vm_Manager_t *  pManVm, Vmx_Manager_t * pManVmx, Mvr_Manager_t * pManMvr );
extern void             Fmw_ManagerFree( Fmw_Manager_t * pMan );
extern void             Fmw_ManagerResize( Fmw_Manager_t * pMan, int nBinVars );
/*=== fmwGlobal.c ===========================================================*/
extern Mva_FuncSet_t *  Fm_GlobalCompute( DdManager * dd, Fmw_Manager_t * pMan, Sh_Network_t * pNet );
extern void             Fm_GlobalComputeTwo( DdManager * dd, Fmw_Manager_t * pMan, Sh_Network_t * pNet1, Sh_Network_t * pNet2,
                             Mva_FuncSet_t ** ppRes1, Mva_FuncSet_t ** ppRes2 );
/*=== fmwFlex.c ===========================================================*/
extern Mvr_Relation_t * Fm_FlexibilityCompute( Fmw_Manager_t * pMan, Sh_Network_t * pShNet, Mvr_Relation_t * pMvrLoc );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
