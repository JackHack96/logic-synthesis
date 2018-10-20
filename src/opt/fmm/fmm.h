/**CFile****************************************************************

  FileName    [fmm.h]

  PackageName [Multi-valued flexibility manager.]

  Synopsis    [External declarations of the MV flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmm.h,v 1.1 2003/11/18 18:55:10 alanmi Exp $]

***********************************************************************/

#ifndef __FMM_H__
#define __FMM_H__

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define FMM_MAX_FANIN_GROWTH             10    // the largest allowed increase in the MAX fanin count of the network
#define FMM_ADD_BITS_NUM                 32    // the number of additional bits used for Z vars of the cut network
#define FMM_BDD_DFS_LIMIT           5000000    // the largest tolerated global BDD size
#define FMM_VERY_LAGRE_BDD_SIZE    10000000    // very large BDD size

////////////////////////////////////////////////////////////////////////
///                         TYPEDEFS                                 ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      STRUCTURE DEFITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Fmm_Manager_t_   Fmm_Manager_t;  // the flexibility manager
typedef struct Fmm_Params_t_    Fmm_Params_t;   // the manager parameters

struct Fmm_Params_t_ 
{
	int          fUseExdc;    // set to 1 if the EXDC is to be used
	int          fDynEnable;  // enables dynmamic variable reordering
	int          fVerbose;    // to enable the output of puzzling messages
    int          nLevelTfi;   // the number of TFI logic levels
    int          nLevelTfo;   // the number of TFO logic levels
    int          fBinary;     // the binary network
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      EXPORTED FUNCTIONS                          ///
////////////////////////////////////////////////////////////////////////

/*=== fmmApi.c =========================================================*/
extern Fmm_Manager_t *  Fmm_ManagerStart( Ntk_Network_t * pNet, Fmm_Params_t * pPars );
extern void             Fmm_ManagerResize( Fmm_Manager_t * p, int nFaninMax );
extern void             Fmm_ManagerStop( Fmm_Manager_t * p );
extern DdNode *         Fmm_ManagerComputeGlobalDcNode( Fmm_Manager_t * p, Ntk_Node_t * pNode );
extern Mvr_Relation_t * Fmm_ManagerComputeLocalDcNode( Fmm_Manager_t * p, Ntk_Node_t * pNode );
extern void             Fmm_ManagerUpdateGlobalFunctions( Fmm_Manager_t * p, Ntk_Node_t * pNode );
extern void             Fmm_ManagerPrintTimeStats( Fmm_Manager_t * pMan );
extern void             Fmm_ManagerSetParameters( Fmm_Manager_t * p, Fmm_Params_t * pPars );
extern DdManager *      Fmm_ManagerReadDdGlo( Fmm_Manager_t * p );
extern int              Fmm_ManagerReadVerbose( Fmm_Manager_t * p );
extern void             Fmm_ManagerAddToTimeSopMin( Fmm_Manager_t * p, int Time );
extern void             Fmm_ManagerAddToTimeResub( Fmm_Manager_t * p, int Time );
/*=== fmmFlex.c ============================================================*/
extern DdNode *         Fmm_FlexComputeGlobal( Fmm_Manager_t * p );
extern Mvr_Relation_t * Fmm_FlexComputeLocal( Fmm_Manager_t * pMan, DdNode * bGlobal, Ntk_Node_t ** ppFanins, int nFanins );
/*=== fmmSweep.c ============================================================*/
extern int              Fmm_NetworkSweep( Ntk_Network_t * pNet );
extern int              Fmm_NetworkMergeEquivalentNets( Fmm_Manager_t * p );
/*=== fmmImage.c =======================================================*/
extern DdNode *         Fmm_ImageMvCompute( DdManager * dd, DdNode * bCare, DdNode ** pbFuncs, int nFuncs, 
                            Vmx_VarMap_t * pVmxGlo, Vmx_VarMap_t * pVmxLoc, DdNode ** pbVars, int nBddSize );
extern void             Fmm_ImageTimeoutSet( int Timeout );
extern void             Fmm_ImageTimeoutReset();

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
