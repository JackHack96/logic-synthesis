/**CFile****************************************************************

  FileName    [dcmn.h]

  PackageName [New don't care manager.]

  Synopsis    [External declarations of the don't-care manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 13, 2003.]

  Revision    [$Id: fmn.h,v 1.2 2003/11/18 18:55:12 alanmi Exp $]

***********************************************************************/

#ifndef __FMN_H__
#define __FMN_H__

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define DCMN_MAX_FANIN_GROWTH             50    // the largest allowed increase in the MAX fanin count of the network
#define DCMN_BDD_DFS_LIMIT           5000000    // the largest tolerated global BDD size
#define DCMN_VERY_LAGRE_BDD_SIZE    10000000    // very large BDD size

////////////////////////////////////////////////////////////////////////
///                         TYPEDEFS                                 ///
////////////////////////////////////////////////////////////////////////

typedef enum dcmn_dir_type_enum dcmn_dir_type_t;
enum dcmn_dir_type_enum {
    DCMN_DIR_FROM_INPUTS, DCMN_DIR_FROM_OUTPUTS, DCMN_DIR_RANDOM, DCMN_DIR_UNASSIGNED
};

typedef enum dcmn_bdd_type_enum dcmn_bdd_type_t;
enum dcmn_bdd_type_enum {
    DCMN_BDD_CONSTRAIN, DCMN_BDD_RESTRICT, DCMN_BDD_SQUEEZE, DCMN_BDD_ISOP, DCMN_BDD_UNASSIGNED
};

typedef enum dcmn_type_enum dcmn_type_enum_t;
enum dcmn_type_enum {
    DCMN_TYPE_NONE, DCMN_TYPE_SS, DCMN_TYPE_NSC, DCMN_TYPE_NS
};

////////////////////////////////////////////////////////////////////////
///                      STRUCTURE DEFITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Dcmn_Man_t_   Dcmn_Man_t;     // the don't-care manager
typedef struct Dcmn_Par_t_   Dcmn_Par_t;     // the don't-care manager parameters

struct Dcmn_Par_t_ 
{
    int          TypeSpec;    // the flexibility type to compute (dcmn_type_enum_t)
    int          TypeFlex;    // the spec type to use (dcmn_type_enum_t)
	int          nBddSizeMax; // determines the limit after which the global BDD construction aborts
	int          fSweep;      // enable/disable sweeping at the end
	int          fUseExdc;    // set to 1 if the EXDC is to be used
	int          fDynEnable;  // enables dynmamic variable reordering
	int          fVerbose;    // to enable the output of puzzling messages
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      EXPORTED FUNCTIONS                          ///
////////////////////////////////////////////////////////////////////////

/*=== dcmnApi.c =======================================================*/
extern Dcmn_Man_t *   Dcmn_ManagerStart( Ntk_Network_t * pNet, Dcmn_Par_t * pPars );
extern void           Dcmn_ManagerStop( Dcmn_Man_t * p );
extern DdNode *       Dcmn_ManagerComputeGlobalDcNode( Dcmn_Man_t * p, Ntk_Node_t * pNode );
extern DdNode *       Dcmn_ManagerComputeGlobalDcNodeUseProduct( Dcmn_Man_t * p, Ntk_Node_t * pPivot, Ntk_Node_t * pNodeCo );
extern Mvr_Relation_t * Dcmn_ManagerComputeLocalDcNode( Dcmn_Man_t * p, Ntk_Node_t * pNode );
extern Mvr_Relation_t * Dcmn_ManagerComputeLocalDcNodeUseProduct( Dcmn_Man_t * p, Ntk_Node_t * pNode );
extern Mvr_Relation_t * Dcmn_ManagerComputeLocalDcNodeUseNsSpec( Dcmn_Man_t * p, Ntk_Node_t * pNode, DdNode * bSpecNs );

extern void           Dcmn_ManagerUpdateGlobalFunctions( Dcmn_Man_t * p, Ntk_Node_t * pNode );
extern void           Dcmn_ManagerSetParameters( Dcmn_Man_t * p, Dcmn_Par_t * pPars );
 
extern DdManager *    Dcmn_ManagerReadDdLoc( Dcmn_Man_t * p );
extern DdManager *    Dcmn_ManagerReadDdGlo( Dcmn_Man_t * p );
extern int            Dcmn_ManagerReadVerbose( Dcmn_Man_t * p );

/*=== dcmnGlobal.c    =======================================================*/
extern int            Dcmn_NetworkBuildGlo( Dcmn_Man_t * pMan, Ntk_Network_t * pNet, bool fExdc, int fUseNsc );
extern int            Dcmn_NodeBuildGlo( Dcmn_Man_t * pMan, Ntk_Node_t * pNode, int fUseNsc );
extern int            Dcmn_NodeBuildGloZ( Dcmn_Man_t * pMan, Ntk_Node_t * pNode, int fUseNsc );

/*=== dcmnFlex.c ==========================================================*/
extern DdNode *       Dcmn_FlexComputeGlobal( Dcmn_Man_t * p );
extern DdNode *       Dcmn_FlexComputeGlobalUseSpecNs( Dcmn_Man_t * pMan, DdNode * bSpecNs );
extern DdNode *       Dcmn_FlexComputeGlobalOne( Dcmn_Man_t * pMan, Ntk_Node_t * pNode );
extern Mvr_Relation_t * Dcmn_FlexComputeLocal( Dcmn_Man_t * pMan, DdNode * bGlobal, Ntk_Node_t ** ppFanins, int nFanins );
extern Mvr_Relation_t * Dcmn_FlexComputeLocalUseProduct( Dcmn_Man_t * pMan, Ntk_Node_t ** ppFanins, int nFanins, Ntk_Node_t * pNode );

/*=== dcmnSweep.c ==========================================================*/
extern int            Dcmn_NetworkSweep( Ntk_Network_t * pNet );
extern int            Dcmn_NetworkMergeEquivalentNets( Dcmn_Man_t * p );

/*=== dcmnUtils.c =======================================================*/
extern Vmx_VarMap_t * Dcmn_UtilsGetNetworkVmx( Ntk_Network_t * pNet );
extern DdNode *       Dcmn_FlexQuantifyFunction( Dcmn_Man_t * pMan, DdNode * bFunc ); 
extern void           Dcmn_FlexQuantifyFunctionArray( Dcmn_Man_t * pMan, DdNode ** pbFuncs, int nFuncs );
extern int            Dcmn_UtilsNumSharedBddNodes( Dcmn_Man_t * pMan );
extern DdNode *       Dcmn_UtilsNetworkDeriveRelation( Dcmn_Man_t * pMan, int fUseGloZ );
extern double         Dcmn_UtilsNetworkGetRelationRatio( Dcmn_Man_t * pMan, DdNode * bRel );
extern double         Dcmn_UtilsNetworkGetAverageNdRatio( Dcmn_Man_t * pMan );
extern DdNode *       Dcmn_UtilsNetworkImageNs( Dcmn_Man_t * pMan, DdNode * bCare, Ntk_Node_t * ppNodes[], int nNodes );
extern DdNode *       Dcmn_UtilsNetworkImageNsUseProduct( Dcmn_Man_t * pMan, Ntk_Node_t * ppNodes[], int nNodes, Ntk_Node_t * pNode );
extern void           Dcmn_UtilsCopyFuncs( DdNode ** pbDest, DdNode ** pbSource, int nFuncs );
extern void           Dcmn_UtilsRefFuncs( DdNode ** pbFuncs, int nFuncs );
extern void           Dcmn_UtilsDerefFuncs( DdManager * dd, DdNode ** pbFuncs, int nFuncs );
extern bool           Dcmn_UtilsCheckNDFuncs( DdManager * dd, DdNode ** pbFuncs, int nValues );
extern Vmx_VarMap_t * Dcmn_UtilsCreateGlobalVmx( Dcmn_Man_t * p, Ntk_Node_t * pNode );
extern Vmx_VarMap_t * Dcmn_UtilsCreateGlobalSetVmx( Dcmn_Man_t * p, Ntk_Node_t * pNode );
extern Vmx_VarMap_t * Dcmn_UtilsCreateLocalVmx( Dcmn_Man_t * p, Ntk_Node_t * pNode, Ntk_Node_t ** ppFanins, int nFanins );
extern DdNode *       Dcmn_UtilsTransferFromSetOutputs( Dcmn_Man_t * pMan, DdNode * bFlexGlo );

#endif
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
