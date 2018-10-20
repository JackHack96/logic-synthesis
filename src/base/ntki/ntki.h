/**CFile****************************************************************

  FileName    [ntki.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Declarations of the network/node interface package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntki.h,v 1.29 2005/05/18 04:14:37 alanmi Exp $]

***********************************************************************/
 
#ifndef __NTKI_H__
#define __NTKI_H__


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "mv.h"
#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== ntkiBalance.c =====================================================*/
extern int               Ntk_NetworkAttach( Ntk_Network_t * pNet );
/*=== ntkiBalance.c =====================================================*/
extern Ntk_Network_t *   Ntk_NetworkBalance( Ntk_Network_t * pNet, double dAndGateDelay );
/*=== ntkiCollapse.c =====================================================*/
extern Ntk_Network_t *   Ntk_NetworkCollapse( Ntk_Network_t * pNet, bool fVerbose );
extern Ntk_Network_t *   Ntk_NetworkCollapseNew( Ntk_Network_t * pNet, bool fVerbose );
/*=== ntkiDecomp.c =========================================================*/
extern void              Ntk_NetworkDecomp( Ntk_Network_t * pNet );
/*=== ntkiDsd.c =========================================================*/
extern Ntk_Network_t *   Ntk_NetworkDsd( Ntk_Network_t * pNet, bool fUseNand, bool fVerbose );
extern void              Ntk_NetworkPrintDsd( Ntk_Network_t * pNet, int Output, bool fShort );
/*=== ntkiDefault.c =========================================================*/
extern void              Ntk_NetworkResetDefault( Ntk_Network_t * pNet );
extern void              Ntk_NetworkForceDefault( Ntk_Network_t * pNet );
extern void              Ntk_NetworkComputeDefault( Ntk_Network_t * pNet );
extern void              Ntk_NodeForceDefault( Ntk_Node_t * pNode );
extern void              Ntk_NodeResetDefault( Ntk_Node_t * pNode, int AcceptType );
extern bool              Ntk_NodeTestAcceptCvr( Cvr_Cover_t * pCvr, Cvr_Cover_t * pCvrMin, int AcceptType, bool fVerbose );
extern int               Ntk_NetworkGetAcceptType( Ntk_Network_t * pNet );
extern Mvc_Cover_t *     Ntk_NodeComputeDefault( Ntk_Node_t * pNode );
extern Ft_Node_t *       Ntk_NodeFactorDefault( Ntk_Node_t * pNode );
extern void              Ntk_NetworkFfResetDefault( Ntk_Network_t * pNet, bool fVerbose );
extern void              Ntk_NodeFfResetDefault( Ntk_Node_t * pNode, int AcceptType, bool fVerbose );
/*=== ntkiEliminate.c =====================================================*/
extern bool              Ntk_NetworkEliminate( Ntk_Network_t * pNet, int nNodesElim, int nCubeLimit, int nFaninLimit, int WeightLimit, bool fUseCostBdd, bool fVerbosity );
extern int               Ntk_NetworkGetNodeValueSop( Ntk_Node_t * pNode );
extern int               Ntk_EliminateGetNodeWeightBdd( Ntk_Node_t * pNode );
extern int               Ntk_EliminateGetNodeWeightSop( Ntk_Node_t * pNode );
extern int               Ntk_EliminateSetCollapsingWeightBdd( Ntk_Node_t * pNode, Ntk_Pin_t * pPin, Ntk_Node_t * pFanin );
extern int               Ntk_EliminateSetCollapsingWeightSop( Ntk_Node_t * pNode, Ntk_Pin_t * pPin, Ntk_Node_t * pFanin );
/*=== ntkiEncode.c =====================================================*/
extern void              Ntk_NetworkEncodeNode( Ntk_Node_t * pNode, int nValues1, int nValues2 );
/*=== ntkiFraig.c =====================================================*/
extern Fraig_Man_t *     Ntk_NetworkFraig( Ntk_Network_t * pNet, int fFuncRed, int fBalance, int fDoSparse, int fChoicing, int fVerbose );
extern void              Ntk_NetworkFraigInt( Fraig_Man_t * pMan, Ntk_Network_t * pNet, int fCopyOutputs );
extern void              Ntk_NetworkFraigDeref( Fraig_Man_t * pMan, Ntk_Network_t * pNet );
/*=== ntkiFraigSweep.c =====================================================*/
extern void              Net_NetworkEquiv( Ntk_Network_t * pNet, int fFuncRed, int fFeedBack, int fDoSparse, int fVerbose );
/*=== ntkiFrames.c =====================================================*/
extern Ntk_Network_t *   Ntk_NetworkDeriveTimeFrames( Ntk_Network_t * pNet, int nFrames );
extern void              Ntk_NetworkReorderCiCo( Ntk_Network_t * pNet );
extern void              Ntk_NetworkMakeCombinational( Ntk_Network_t * pNet );
/*=== ntkiFxu.c =========================================================*/
extern bool              Ntk_NetworkFastExtract( Ntk_Network_t * pNet, Fxu_Data_t * p );
extern void              Ntk_NetworkFxFreeInfo( Fxu_Data_t * p );
/*=== ntkiLxu.c =========================================================*/
extern bool              Ntk_NetworkLayoutFastExtract( Ntk_Network_t * pNet, Lxu_Data_t * p );
extern void              Ntk_NetworkLxFreeInfo( Lxu_Data_t * p );
/*=== ntkiGlobal.c ====================================================*/
extern void              Ntk_NetworkGlobalMdd                ( DdManager * dd, Ntk_Network_t * pNet, Vm_VarMap_t ** ppVm, char *** ppCiNames, DdNode **** ppMdds, DdNode **** ppMddsExdc );
extern DdNode ***        Ntk_NetworkGlobalMddCompute         ( DdManager * dd, Ntk_Network_t * pNet, char ** psLeavesByName, int nLeaves, char ** psRootsByName, int nRoots, Vmx_VarMap_t * pVmx );
extern DdNode ***        Ntk_NetworkGlobalMddEmpty           ( DdManager * dd, int * pValues, int nOuts );
extern DdNode ***        Ntk_NetworkGlobalMddDup             ( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds );
extern DdNode ***        Ntk_NetworkGlobalMddOr              ( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2  );
extern DdNode ***        Ntk_NetworkGlobalMddConvertExdc     ( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMddsExdc );
extern void              Ntk_NetworkGlobalMddMinimize        ( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2 );
extern void              Ntk_NetworkGlobalMddDeref           ( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds );
extern bool              Ntk_NetworkGlobalMddCheckEquality   ( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2 );
extern bool              Ntk_NetworkGlobalMddCheckContainment( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2 );
extern bool              Ntk_NetworkGlobalMddCheckND         ( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds );
extern DdNode *          Ntk_NetworkGlobalMddErrorTrace      ( DdManager * dd, int * pValues, int nOuts, DdNode *** ppMdds1, DdNode *** ppMdds2, int * Out, int * Value );
extern void              Ntk_NetworkGlobalMddPrintErrorTrace ( FILE * pFile, DdManager * dd, Vmx_VarMap_t * pVmx, DdNode * bTrace, int Out, char * pNameOut, int nValue, char * psLeavesByName[] );
extern Vmx_VarMap_t *    Ntk_NetworkGlobalGetVmx             ( Ntk_Network_t * pNet, char ** aLeavesByName );
extern Vmx_VarMap_t *    Ntk_NetworkGlobalGetVmxArray        ( Ntk_Node_t ** ppNodes, int nNodes );
extern char **           Ntk_NetworkGlobalGetOutputs         ( Ntk_Network_t * pNet );
extern int *             Ntk_NetworkGlobalGetOutputValues    ( Ntk_Network_t * pNet );
extern char **           Ntk_NetworkGlobalGetOutputsAndValues( Ntk_Network_t * pNet, int ** ppValues );
extern DdNode **         Ntk_NodeGlobalMdd         ( DdManager * dd, Ntk_Node_t * pNode );
extern DdNode *          Ntk_NodeGlobalMddCollapse( DdManager * ddGlo, DdNode * bFuncLoc, Ntk_Node_t * pNode );
/*=== ntkiMap.c ====================================================*/
extern int               Ntk_NetworkIsMapped( Ntk_Network_t * pNet );
extern void              Ntk_NetworkMapStats( Ntk_Network_t * pNetNew );
/*=== ntkiMerge.c ====================================================*/
extern void              Ntk_NetworkMerge( Ntk_Network_t * pNet, int nNodes );
extern Ntk_Node_t *      Ntk_NetworkMergeNodes( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 );
extern Ntk_Node_t *      Ntk_NodeMerge( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 );
/*=== ntkiOrder.c ====================================================*/
extern st_table *        Ntk_NetworkOrder( Ntk_Network_t * pNet, bool fUseDfs );
extern st_table *        Ntk_NetworkOrderByName( Ntk_Network_t * pNet, bool fUseDfs );
extern char **           Ntk_NetworkOrderArrayByName( Ntk_Network_t * pNet, bool fUseDfs );
extern void              Ntk_NetworkReorder( Ntk_Network_t * pNet, int fVerbose );
extern bool              Ntk_NodeReorderMvr( Ntk_Node_t * pNode );
/*=== ntkiPower.c ====================================================*/
extern void              Ntk_NetworkComputePower( Ntk_Network_t * pNet, int nLayers );
extern void              Ntk_NetworkNonDeterminize( Ntk_Network_t * pNet, double Prob );
/*=== ntkiPrint.c ====================================================*/
extern void              Ntk_NetworkPrintStats( FILE * pFile, Ntk_Network_t * pNet, bool fComputeCvr, bool fComputeMvr, bool fShort );
extern void		         Ntk_NodePrintStats( FILE * fp, Ntk_Node_t * node, bool cvr, bool mvr );
extern void              Ntk_NetworkPrintIo( FILE * pFile, Ntk_Network_t * pNet, int nNodes );
extern void              Ntk_NetworkPrintLatch( FILE * pFile, Ntk_Network_t * pNet, int nNodes );
extern void              Ntk_NodePrintCvr( FILE * fp, Ntk_Node_t * pNode, bool fPrintDefault, bool fPrintPositional );
extern void              Ntk_NodePrintMvr( FILE * pFile, Ntk_Node_t * pNode );
extern void              Ntk_NodePrintFlex( FILE * pFile, Ntk_Node_t * pNode, Mvr_Relation_t * pFlex );
extern void              Ntk_NodePrintMvrFlex( FILE * pFile, Ntk_Node_t * pNode, DdNode * bFlex );
extern void              Ntk_NetworkPrintValue( FILE * pFile, Ntk_Network_t * pNet, int nNodes );
extern void              Ntk_NetworkPrintRange( FILE * pFile, Ntk_Network_t * pNet, int nNodes );
extern void              Ntk_NodePrintValue( FILE * pFile, Ntk_Node_t * pNode );
extern void              Ntk_NodePrintRange( FILE * pFile, Ntk_Node_t * pNode );
extern void              Ntk_NodePrintFactor( FILE * pFile, Ntk_Node_t * pNode );
extern void              Ntk_NetworkPrintLevel( FILE * pFile, Ntk_Network_t * pNet, int nNodes, int fFromOutputs );
extern void              Ntk_NetworkPrintReconv( FILE * pFile, Ntk_Network_t * pNet, int nNodes, bool fFanout, bool fVerbose );
extern void              Ntk_NetworkPrintNd( FILE * pFile, Ntk_Network_t * pNet, int fVerbose );
extern void              Ntk_NodePrintCvrLiterals( FILE * pFile, Vm_VarMap_t * pVm, Ntk_Node_t * pFanins[], Mvc_Cover_t * Cover, int nVars, int * pPos );
extern void              Ntk_NodePrintCvrPositional( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, int nVars );
extern int               Ntk_NodePrintOutputName( FILE * pFile, char * pNameOut, int nValuesOut, int iSet );
/*=== ntkiResub.c ============================================================*/
extern void              Ntk_NetworkResub( Ntk_Network_t * pNet, int nCandsMax, int nFaninMax, bool fVerbose );
extern bool              Ntk_NetworkResubNode( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr, int AcceptType, int nCandsMax, bool fVerbose );
extern Ntk_Node_t *      Ntk_NetworkGetResubNode( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr, int nCandsMax );
extern Ntk_Node_t **     Ntk_NetworkOrderNodesByLevel( Ntk_Network_t * pNet, bool fFromOutputs );
extern Ntk_Node_t **     Ntk_NetworkOrderCoNodesBySize( Ntk_Network_t * pNet );
extern Ntk_Node_t **     Ntk_NetworkCollectInternal( Ntk_Network_t * pNet );
/*=== ntkiSat.c ======================================================*/
extern void              Ntk_NetworkWriteSat( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, char * pFileName ); 
extern void              Ntk_NetworkWriteClauses( Ntk_Network_t * pNet, char * pFileName ); 
/*=== ntkiShow.c ======================================================*/
extern void              Ntk_NetworkShow( Ntk_Network_t * pNet, Ntk_Node_t ** ppNodes, int nNodes, bool fTfiOnly );
extern void              Ntk_NetworkPrintGml( Ntk_Network_t * pNet, Ntk_Node_t * pRoot, bool fTfiOnly, char * pFileName );
/*=== ntkiStrash.c =========================================================*/
extern Ntk_Network_t *   Ntk_NetworkStrash( Ntk_Network_t * pNet );
/*=== ntkiSweep.c ================-----====================================*/
extern bool              Ntk_NetworkSweep( Ntk_Network_t * pNet, bool fSweepConsts, bool fSweepBuffers, bool fSweepIsets, bool fVerbose );
extern bool              Ntk_NetworkSweepNode( Ntk_Node_t * pNode, bool fSweepConsts, bool fSweepBuffers, bool fVerbose );
extern bool              Ntk_NetworkSweepIsets( Ntk_Node_t * pNode, bool fVerbose );
extern void              Ntk_NetworkSweepTransformNode( Ntk_Node_t * pNode, Ntk_Node_t * pTrans1, Ntk_Node_t * pTrans2 );
extern void              Ntk_NetworkSweepTransformFanouts( Ntk_Node_t * pNode, Ntk_Node_t * pTrans );
/*=== ntkiVerify.c =====================================================*/
extern void              Ntk_NetworkVerify( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose );
/*=== ntkiVerifyStr.c =====================================================*/
extern void              Ntk_NetworkVerifyStr( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose );
/*=== ntkiUnreach.c =====================================================*/
extern DdNode *          Ntk_NetworkComputeUnreach( Ntk_Network_t * pNet, int fVerbose );
extern DdNode **         Ntk_NetworkDerivePartitions( Ntk_Network_t * pNet, int fLatchOnly, int fMapIsLatchOnly );
extern DdNode *          Ntk_NetworkComputeInitState( Ntk_Network_t * pNet, int fLatchOnly );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

