/**CFile****************************************************************

  FileName    [ntk.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Declarations of the network/node package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntk.h,v 1.58 2005/05/18 04:14:36 alanmi Exp $]

***********************************************************************/
 
#ifndef __NTK_H__
#define __NTK_H__


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// node types: no type, primary input, primary output, internal
typedef enum { MV_NODE_NONE, MV_NODE_CI, MV_NODE_CO, MV_NODE_INT }  Ntk_NodeType_t;
// node subtype for combinational inputs/outputs: 
// node is a PI/PO of the network; node is only an input/output a latch
typedef enum { MV_BELONGS_TO_NET, MV_BELONGS_TO_LATCH }  Ntk_NodeSubtype_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct NtkPinStruct              Ntk_Pin_t;        // fanin/fanout pin (16 bytes)
typedef struct NtkNodeStruct             Ntk_Node_t;       // the node data structure
typedef struct NtkLatchStruct            Ntk_Latch_t;      // the latch data structure
typedef struct NtkNetworkStruct          Ntk_Network_t;    // the network data structure
typedef struct NtkNodeHeapStruct         Ntk_NodeHeap_t;   // the heap of nodes
typedef struct NtkNodeVecStruct          Ntk_NodeVec_t;    // the vector of nodes
typedef struct Ntk_NodeBindingStruct_t_  Ntk_NodeBinding_t;// binding of a mapped node to a gate 

struct NtkNodeVecStruct
{
    Ntk_Node_t ** pArray;
    int           nSize;
    int           nCap;
};
#define Ntk_NodeVecForEach( Vec, i, Node )\
    for ( i = 0; i < Vec->nSize && ( ((unsigned)(Node = Vec->pArray[i])) >= 0 ); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== ntk.c ==========================================================*/
extern void              Ntk_Init( Mv_Frame_t * pMvsis );
extern void              Ntk_End( Mv_Frame_t * pMvsis );
/*=== ntkApi.c =======================================================*/
// "ntkApi.c" contains only simple lookup/insert type of APIs
// more complex APIs are located in other files detailed below
// data-access pin APIs
extern Ntk_Pin_t *       Ntk_PinReadNext( Ntk_Pin_t * pPin );
extern Ntk_Pin_t *       Ntk_PinReadPrev( Ntk_Pin_t * pPin );
extern Ntk_Pin_t *       Ntk_PinReadLink( Ntk_Pin_t * pPin );
extern Ntk_Node_t *      Ntk_PinReadNode( Ntk_Pin_t * pPin );
// data-access latch APIs
extern Ntk_Latch_t *     Ntk_LatchReadNext( Ntk_Latch_t * pLatch );
extern Ntk_Latch_t *     Ntk_LatchReadPrev( Ntk_Latch_t * pLatch );
extern int               Ntk_LatchReadReset( Ntk_Latch_t * pLatch );
extern Ntk_Node_t *      Ntk_LatchReadInput( Ntk_Latch_t * pLatch );
extern Ntk_Node_t *      Ntk_LatchReadOutput( Ntk_Latch_t * pLatch );
extern Ntk_Node_t *      Ntk_LatchReadNode( Ntk_Latch_t * pLatch );
extern Ntk_Network_t *   Ntk_LatchReadNet( Ntk_Latch_t * pLatch );
extern char *            Ntk_LatchReadData( Ntk_Latch_t * pLatch );
// data-access node APIs
extern Ntk_Node_t *      Ntk_NodeReadNext( Ntk_Node_t * pNode );
extern Ntk_Node_t *      Ntk_NodeReadPrev( Ntk_Node_t * pNode );
extern Ntk_Node_t *      Ntk_NodeReadOrder( Ntk_Node_t * pNode );
extern int               Ntk_NodeReadId( Ntk_Node_t * pNode );
extern char *            Ntk_NodeReadName( Ntk_Node_t * pNode );
extern Ntk_NodeType_t    Ntk_NodeReadType( Ntk_Node_t * pNode );
extern Ntk_NodeSubtype_t Ntk_NodeReadSubtype( Ntk_Node_t * pNode );
extern int               Ntk_NodeReadLevel( Ntk_Node_t * pNode );
extern int               Ntk_NodeReadValueNum( Ntk_Node_t * pNode );
extern Ntk_Pin_t *       Ntk_NodeReadFaninPinHead( Ntk_Node_t * pNode );
extern Ntk_Pin_t *       Ntk_NodeReadFaninPinTail( Ntk_Node_t * pNode );
extern int               Ntk_NodeReadFaninNum( Ntk_Node_t * pNode );
extern Ntk_Pin_t *       Ntk_NodeReadFanoutPinHead( Ntk_Node_t * pNode );
extern Ntk_Pin_t *       Ntk_NodeReadFanoutPinTail( Ntk_Node_t * pNode );
extern int               Ntk_NodeReadFanoutNum( Ntk_Node_t * pNode );
extern char *            Ntk_NodeReadData( Ntk_Node_t * pNode );
extern Ntk_NodeBinding_t*Ntk_NodeReadMap( Ntk_Node_t * pNode );
extern Ntk_Network_t *   Ntk_NodeReadNetwork( Ntk_Node_t * pNode );
extern Fnc_Manager_t *   Ntk_NodeReadMan( Ntk_Node_t * pNode );
extern Fnc_Function_t *  Ntk_NodeReadFunc( Ntk_Node_t * pNode );
extern Fnc_Global_t *    Ntk_NodeReadFuncGlobal( Ntk_Node_t * pNode );
extern double            Ntk_NodeReadArrTimeRise( Ntk_Node_t * pNode );
extern double            Ntk_NodeReadArrTimeFall( Ntk_Node_t * pNode );
extern FILE *            Ntk_NodeReadMvsisErr( Ntk_Node_t * pNode );
extern FILE *            Ntk_NodeReadMvsisOut( Ntk_Node_t * pNode );
// node functionality APIs
extern Vm_VarMap_t *     Ntk_NodeReadFuncVm( Ntk_Node_t * pNode );
extern Cvr_Cover_t *     Ntk_NodeReadFuncCvr( Ntk_Node_t * pNode );
extern Mvr_Relation_t *  Ntk_NodeReadFuncMvr( Ntk_Node_t * pNode );
extern DdNode **         Ntk_NodeReadFuncGlo( Ntk_Node_t * pNode );
extern Vm_VarMap_t *     Ntk_NodeGetFuncVm( Ntk_Node_t * pNode );
extern Cvr_Cover_t *     Ntk_NodeGetFuncCvr( Ntk_Node_t * pNode );
extern Mvr_Relation_t *  Ntk_NodeGetFuncMvr( Ntk_Node_t * pNode );
extern DdNode **         Ntk_NodeGetFuncGlo( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncVm( Ntk_Node_t * pNode, Vm_VarMap_t * pVm );
extern void              Ntk_NodeWriteFuncCvr( Ntk_Node_t * pNode, Cvr_Cover_t * pCvr );
extern void              Ntk_NodeWriteFuncMvr( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr );
extern void              Ntk_NodeWriteFuncGlo( Ntk_Node_t * pNode, DdNode ** pGlo );
extern void              Ntk_NodeSetFuncVm( Ntk_Node_t * pNode, Vm_VarMap_t * pVm );
extern void              Ntk_NodeSetFuncCvr( Ntk_Node_t * pNode, Cvr_Cover_t * pCvr );
extern void              Ntk_NodeSetFuncMvr( Ntk_Node_t * pNode, Mvr_Relation_t * pMvr );
extern void              Ntk_NodeSetFuncGlo( Ntk_Node_t * pNode, DdNode ** pGlo );
extern void              Ntk_NodeSetArrTimeRise( Ntk_Node_t * pNode, double dTime );
extern void              Ntk_NodeSetArrTimeFall( Ntk_Node_t * pNode, double dTime );
extern void              Ntk_NodeFreeFuncVm( Ntk_Node_t * pNode );
extern void              Ntk_NodeFreeFuncCvr( Ntk_Node_t * pNode );
extern void              Ntk_NodeFreeFuncMvr( Ntk_Node_t * pNode );
extern void              Ntk_NodeFreeFuncGlo( Ntk_Node_t * pNode );
extern int               Ntk_NodeReadDefault( Ntk_Node_t * pNode );
extern Fnc_Manager_t *   Ntk_NodeReadMan   ( Ntk_Node_t * pNode );
extern Mvr_Manager_t *   Ntk_NodeReadManMvr( Ntk_Node_t * pNode );
extern Vm_Manager_t *    Ntk_NodeReadManVm ( Ntk_Node_t * pNode );
extern Vmx_Manager_t *   Ntk_NodeReadManVmx( Ntk_Node_t * pNode );
extern Mvc_Manager_t *   Ntk_NodeReadManMvc( Ntk_Node_t * pNode );

// node global functionality APIs
// global BDD manager
extern DdManager *       Ntk_NodeReadFuncDd( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncDd( Ntk_Node_t * pNode, DdManager * dd );
// global functions
extern DdNode **         Ntk_NodeReadFuncGlob( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncGlob( Ntk_Node_t * pNode, DdNode ** pbGlo );
extern void              Ntk_NodeDerefFuncGlob( Ntk_Node_t * pNode );
// modified global functions
extern DdNode **         Ntk_NodeReadFuncGlobZ( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncGlobZ( Ntk_Node_t * pNode, DdNode ** pbGloZ );
extern void              Ntk_NodeDerefFuncGlobZ( Ntk_Node_t * pNode );
// symbolic global functions
extern DdNode **         Ntk_NodeReadFuncGlobS( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncGlobS( Ntk_Node_t * pNode, DdNode ** pbGloS );
extern void              Ntk_NodeDerefFuncGlobS( Ntk_Node_t * pNode );
// other data members
extern bool              Ntk_NodeReadFuncNonDet( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncNonDet( Ntk_Node_t * pNode, bool fNonDet );
extern int               Ntk_NodeReadFuncNumber( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncNumber( Ntk_Node_t * pNode, int nNumber );

// node global functionality APIs (binary)
// global functions
extern DdNode *          Ntk_NodeReadFuncBinGlo( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncBinGlo( Ntk_Node_t * pNode, DdNode * bGlo );
extern void              Ntk_NodeDerefFuncBinGlo( Ntk_Node_t * pNode );
// modified global functions
extern DdNode *          Ntk_NodeReadFuncBinGloZ( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncBinGloZ( Ntk_Node_t * pNode, DdNode * bGloZ );
extern void              Ntk_NodeDerefFuncBinGloZ( Ntk_Node_t * pNode );
// symbolic global functions
extern DdNode *          Ntk_NodeReadFuncBinGloS( Ntk_Node_t * pNode );
extern void              Ntk_NodeWriteFuncBinGloS( Ntk_Node_t * pNode, DdNode * bGloS );
extern void              Ntk_NodeDerefFuncBinGloS( Ntk_Node_t * pNode );

// data-access network APIs
extern char *            Ntk_NetworkReadName( Ntk_Network_t * pNet );
extern char *            Ntk_NetworkReadSpec( Ntk_Network_t * pNet );
extern Mv_Frame_t *      Ntk_NetworkReadMvsis( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkReadCiHead( Ntk_Network_t * pNet ); // pi + latch outputs
extern Ntk_Node_t *      Ntk_NetworkReadCiTail( Ntk_Network_t * pNet );
extern int               Ntk_NetworkReadCiNum( Ntk_Network_t * pNet );
extern int               Ntk_NetworkReadCiTrueNum( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkReadCoHead( Ntk_Network_t * pNet ); // po + latch inputs
extern Ntk_Node_t *      Ntk_NetworkReadCoTail( Ntk_Network_t * pNet );
extern int               Ntk_NetworkReadCoNum( Ntk_Network_t * pNet );
extern int               Ntk_NetworkReadCoTrueNum( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkReadNodeHead( Ntk_Network_t * pNet ); // only intenal
extern Ntk_Node_t *      Ntk_NetworkReadNodeTail( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkReadNodeHead2( Ntk_Network_t * pNet ); // only intenal
extern Ntk_Node_t *      Ntk_NetworkReadNodeTail2( Ntk_Network_t * pNet );
extern int               Ntk_NetworkReadNodeIntNum( Ntk_Network_t * pNet );
extern Ntk_Latch_t *     Ntk_NetworkReadLatchHead( Ntk_Network_t * pNet ); // latches
extern Ntk_Latch_t *     Ntk_NetworkReadLatchTail( Ntk_Network_t * pNet );
extern int               Ntk_NetworkReadLatchNum( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkReadOrder( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkReadOrderByLevel( Ntk_Network_t * pNet, int Level );
extern int               Ntk_NetworkReadNodeTotalNum( Ntk_Network_t * pNet ); // sum total of all nodes (w/o latches)
extern int               Ntk_NetworkReadNodeWritableNum( Ntk_Network_t * pNet );
extern FILE *            Ntk_NetworkReadMvsisErr( Ntk_Network_t * pNet );
extern FILE *            Ntk_NetworkReadMvsisOut( Ntk_Network_t * pNet );
extern Ntk_Network_t *   Ntk_NetworkReadBackup( Ntk_Network_t * pNet );
extern int               Ntk_NetworkReadStep( Ntk_Network_t * pNet );
// default timing APIs
extern double            Ntk_NetworkReadDefaultArrTimeRise ( Ntk_Network_t * pNet );
extern double            Ntk_NetworkReadDefaultArrTimeFall ( Ntk_Network_t * pNet );
// network functionality APIs
extern Fnc_Manager_t *   Ntk_NetworkReadMan( Ntk_Network_t * pNet );
extern Vm_Manager_t *    Ntk_NetworkReadManVm( Ntk_Network_t * pNet );
extern Vmx_Manager_t *   Ntk_NetworkReadManVmx( Ntk_Network_t * pNet );
extern Mvc_Manager_t *   Ntk_NetworkReadManMvc( Ntk_Network_t * pNet );
extern Mvr_Manager_t *   Ntk_NetworkReadManMvr( Ntk_Network_t * pNet );
extern DdManager *       Ntk_NetworkReadManDdLoc( Ntk_Network_t * pNet );
extern DdManager *       Ntk_NetworkReadManDdGlo( Ntk_Network_t * pNet );
extern DdManager *       Ntk_NetworkReadDdGlo( Ntk_Network_t * pNet );
extern Vmx_VarMap_t *    Ntk_NetworkReadVmxGlo( Ntk_Network_t * pNet );
extern Ntk_Network_t *   Ntk_NetworkReadNetExdc( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkReadFirstCo( Ntk_Network_t * pNet );
// data-entry node APIs
extern void              Ntk_NodeSetName( Ntk_Node_t * pNode, char * pName );
extern void              Ntk_NodeSetSubtype( Ntk_Node_t * pNode, int Subtype );
extern void              Ntk_NodeSetValueNum( Ntk_Node_t * pNode, int nValues );
extern void              Ntk_NodeSetData( Ntk_Node_t * pNode, char * pData );
extern void              Ntk_NodeSetMap( Ntk_Node_t * pNode, Ntk_NodeBinding_t * pBinding );
extern void              Ntk_NodeSetOrder( Ntk_Node_t * pNode, Ntk_Node_t * pNext );
extern void              Ntk_NodeSetLevel( Ntk_Node_t * pNode, int Level );
extern void              Ntk_NodeSetFuncCvr( Ntk_Node_t * pNode, Cvr_Cover_t * pCvr );
extern void              Ntk_NodeSetNetwork( Ntk_Node_t * pNode, Ntk_Network_t * pNet );
// data-entry network APIs
extern void              Ntk_NetworkSetNetExdc( Ntk_Network_t * pNet, Ntk_Network_t * pNetExdc );
extern void              Ntk_NetworkSetName( Ntk_Network_t * pNet, char * pName );
extern void              Ntk_NetworkSetSpec( Ntk_Network_t * pNet, char * pName );
extern void              Ntk_NetworkSetMvsis( Ntk_Network_t * pNet, Mv_Frame_t * pMvsis );
extern void              Ntk_NetworkSetBackup( Ntk_Network_t * pNet, Ntk_Network_t * pNetBackup );
extern void              Ntk_NetworkSetStep( Ntk_Network_t * pNet, int iStep );
extern void              Ntk_NetworkSetDdGlo( Ntk_Network_t * pNet, DdManager * ddGlo );
extern void              Ntk_NetworkSetVmxGlo( Ntk_Network_t * pNet, Vmx_VarMap_t * pVmxGlo );
extern void              Ntk_NetworkWriteDdGlo( Ntk_Network_t * pNet, DdManager * ddGlo );
// default timing APIs
extern void              Ntk_NetworkSetDefaultArrTimeRise ( Ntk_Network_t * pNet, double dTime );
extern void              Ntk_NetworkSetDefaultArrTimeFall ( Ntk_Network_t * pNet, double dTime );
// data-test node APIs
extern bool              Ntk_NodeIsCi( Ntk_Node_t * pNode );
extern bool              Ntk_NodeIsCo( Ntk_Node_t * pNode );
extern bool              Ntk_NodeIsCoDriver( Ntk_Node_t * pNode );
extern bool              Ntk_NodeIsCoFanin( Ntk_Node_t * pNode );
extern Ntk_Node_t *      Ntk_NodeHasCoName( Ntk_Node_t * pNode );
extern bool              Ntk_NodeIsInternal( Ntk_Node_t * pNode );
extern bool              Ntk_NodeIsConstant( Ntk_Node_t * pNode );
extern int               Ntk_NodeReadConstant( Ntk_Node_t * pNode );
extern bool              Ntk_NodeBelongsToLatch( Ntk_Node_t * pNode );
// data-test network APIs
/*=== ntkCheck.c ====================================================*/
extern int               Ntk_NetworkCheck( Ntk_Network_t * pNet );
extern int               Ntk_NetworkCheckNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern int               Ntk_NetworkCheckLatch( Ntk_Network_t * pNet, Ntk_Latch_t * pLatch );
/*=== ntkDfs.c =======================================================*/
extern void              Ntk_NetworkDfs( Ntk_Network_t * pNet, int fFromOutputs );
extern void              Ntk_NetworkDfsLatches( Ntk_Network_t * pNet, bool fFromOutputs );
extern void              Ntk_NetworkDfsNodes( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[], int nNodes, int fFromOutputs );
extern void              Ntk_NetworkDfsFromNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern void              Ntk_NetworkDfsByLevel( Ntk_Network_t * pNet, bool fFromOutputs );
extern int               Ntk_NetworkLevelize( Ntk_Network_t * pNet, int fFromOutputs );
extern int               Ntk_NetworkLevelizeNodes( Ntk_Node_t * ppNodes[], int nNodes, int fFromOutputs );
extern void              Ntk_NetworkCountVisits( Ntk_Network_t * pNet, bool fLatchOnly );
extern int               Ntk_NetworkGetNumLevels( Ntk_Network_t * pNet );
extern bool              Ntk_NetworkIsAcyclic( Ntk_Network_t * pNet );
extern bool              Ntk_NetworkIsAcyclic1( Ntk_Network_t * pNet );
extern bool              Ntk_NetworkIsAcyclic2( Ntk_Network_t * pNet );
extern int               Ntk_NetworkComputeNodeSupport( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes );
extern int               Ntk_NetworkInterleave( Ntk_Network_t * pNet );
extern int               Ntk_NetworkInterleaveNodes( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes );
extern int               Ntk_NetworkComputeNodeTfi( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes, int Depth, bool fIncludeCis, bool fExistPath );
extern int               Ntk_NetworkComputeNodeTfo( Ntk_Network_t * pNet, Ntk_Node_t * pNodes[], int nNodes, int Depth, bool fIncludeCos, bool fExistPath ); 
extern bool              Ntk_NodeHasCoInTfo( Ntk_Node_t * pNode );
/*=== ntkFanio.c ===================================================*/
// node fanin/fanout APIs
extern Ntk_Node_t *      Ntk_NodeReadFaninNode( Ntk_Node_t * pNode, int i );
extern Ntk_Node_t *      Ntk_NodeReadFanoutNode( Ntk_Node_t * pNode, int i );
extern int               Ntk_NodeReadFaninIndex( Ntk_Node_t * pNode, Ntk_Node_t * pFanin );
extern int               Ntk_NodeReadFanoutIndex( Ntk_Node_t * pNode, Ntk_Node_t * pFanout );
extern int               Ntk_NodeReadFanins( Ntk_Node_t * pNode, Ntk_Node_t * pFanins[] );
extern int               Ntk_NodeReadFanouts( Ntk_Node_t * pNode, Ntk_Node_t * pFanouts[] );
extern void              Ntk_NodeReadFanioValues( Ntk_Node_t * pNode, int * pValues );
extern void              Ntk_NodeReduceFanins( Ntk_Node_t * pNode, int * pSupport );
extern Ntk_Pin_t *       Ntk_NodeAddFanin( Ntk_Node_t * pNode, Ntk_Node_t * pFanin );
extern Ntk_Pin_t *       Ntk_NodeAddFanout( Ntk_Node_t * pNode, Ntk_Node_t * pFanout );
extern void              Ntk_NodeAddFaninFanout( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern void              Ntk_NodeDeleteFaninFanout( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern void              Ntk_NodeTransferFanout( Ntk_Node_t * pNodeFrom, Ntk_Node_t * pNodeTo );
extern void              Ntk_NodePatchFanin( Ntk_Node_t * pNode, Ntk_Node_t * pFaninOld, Ntk_Node_t * pFaninNew );
extern Vm_VarMap_t *     Ntk_NodeAssignVm( Ntk_Node_t * pNode );
extern bool              Ntk_NodeSupportContain( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 );
/*=== ntkLatch.c ====================================================*/
extern Ntk_Latch_t *     Ntk_LatchCreate( Ntk_Network_t * pNet, Ntk_Node_t * pNode, int ResValue, char * pNameInput, char * pNameOutput );
extern Ntk_Latch_t *     Ntk_LatchDup( Ntk_Network_t * pNet, Ntk_Latch_t * pLatch );
extern void              Ntk_LatchDelete( Ntk_Latch_t * pLatch );
extern void              Ntk_LatchAdjustInput( Ntk_Network_t * pNet, Ntk_Latch_t * pLatch );
extern void              Ntk_NetworkAddLatch( Ntk_Network_t * pNet, Ntk_Latch_t * pLatch );
extern void              Ntk_NetworkDeleteLatch( Ntk_Latch_t * pLatch );
extern void              Ntk_NetworkEncodeLatches( Ntk_Network_t * pNet );
/*=== ntkMdd.c ====================================================*/
extern Ntk_Node_t *      Ntk_NodeCreateFromMdd( Ntk_Network_t * pNet, Ntk_Node_t * pFanins[], int nFanins, DdManager * ddDc, DdNode * bOnSet );
extern Ntk_Network_t *   Ntk_NetworkCreateFromMdd( Ntk_Node_t * pNode );
/*=== ntkNames.c ====================================================*/
extern char *            Ntk_NodeGetNamePrintable( Ntk_Node_t * pNode );
extern char *            Ntk_NodeGetNameLong( Ntk_Node_t * pNode );
extern char *            Ntk_NodeGetNameShort( Ntk_Node_t * pNode );
extern char *            Ntk_NodeGetNameUniqueLong( Ntk_Node_t * pNode );
extern char *            Ntk_NodeGetNameUniqueShort( Ntk_Node_t * pNode );
extern char *            Ntk_NodeAssignName( Ntk_Node_t * pNode, char * pName );
extern char *            Ntk_NodeRemoveName( Ntk_Node_t * pNode );
extern int               Ntk_NodeCompareByNameAndId( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 );
extern int               Ntk_NodeCompareByNameAndIdOrder( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 );
extern int               Ntk_NodeCompareByValueAndId( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 );
extern int               Ntk_NodeCompareByLevel( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 );
extern bool              Ntk_NetworkIsModeShort( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkFindOrAddNodeByName( Ntk_Network_t * pNet, char * pName, Ntk_NodeType_t Type );
extern Ntk_Node_t *      Ntk_NetworkFindOrAddNodeByName2( Ntk_Network_t * pNet, char * pName, Ntk_NodeType_t Type );
extern Ntk_Node_t *      Ntk_NetworkFindNodeByName( Ntk_Network_t * pNet, char * pName );
extern Ntk_Node_t *      Ntk_NetworkFindNodeByNameWithMode( Ntk_Network_t * pNet, char * pName );
extern Ntk_Node_t *      Ntk_NetworkFindNodeByNameLong( Ntk_Network_t * pNet, char * pName );
extern Ntk_Node_t *      Ntk_NetworkFindNodeByNameShort( Ntk_Network_t * pNet, char * pName );
extern Ntk_Node_t *      Ntk_NetworkFindNodeById( Ntk_Network_t * pNet, int Id );
extern char *            Ntk_NetworkRegisterNewName( Ntk_Network_t * pNet, char * pName );
extern void              Ntk_NetworkRenameNodes( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkCollectNodeByName( Ntk_Network_t * pNet, char * pName, int fCollectCis );
extern void              Ntk_NetworkCollectIoNames( Ntk_Network_t * pNet, char *** ppsLeaves, char *** ppsRoots );
/*=== ntkNode.c =====================================================*/
extern Ntk_Node_t *      Ntk_NodeCreate( Ntk_Network_t * pNet, char * pName, Ntk_NodeType_t Type, int nValues );
extern Ntk_Node_t *      Ntk_NodeCreateConstant( Ntk_Network_t * pNet, int nValues, unsigned Pol );
extern Ntk_Node_t *      Ntk_NodeCreateOneInputNode( Ntk_Network_t * pNet, Ntk_Node_t * pFanin, int nValuesIn, int nValuesOut, unsigned Pols[] );
extern Ntk_Node_t *      Ntk_NodeCreateOneInputBinary( Ntk_Network_t * pNet, Ntk_Node_t * pFanin, bool fInv );
extern Ntk_Node_t *      Ntk_NodeCreateTwoInputBinary( Ntk_Network_t * pNet, Ntk_Node_t * ppFanins[], unsigned TruthTable );
extern Ntk_Node_t *      Ntk_NodeCreateSimpleBinary( Ntk_Network_t * pNet, Ntk_Node_t * ppFanins[], int pCompls[], int nFanins, int fGateOr );
extern Ntk_Node_t *      Ntk_NodeCreateMuxBinary( Ntk_Network_t * pNet, Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, Ntk_Node_t * pNode3 );
extern Ntk_Node_t *      Ntk_NodeCreateDecoder( Ntk_Network_t * pNet, Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 );
extern Ntk_Node_t *      Ntk_NodeCreateDecoderGeneral( Ntk_Node_t ** ppNodes, int nNodes, int * pValueAssign, int nTotalValues, int nOutputValues);
extern void              Ntk_NodeCreateEncoded( Ntk_Node_t * pNode, Ntk_Node_t ** ppNodes, int nNodes, int * pValueAssign, int nTotalValues, int nOutputValues);
extern void              Ntk_NodeCreateEncoders( Ntk_Network_t * pNet, Ntk_Node_t * pNode, int nValues1, int nValues2, Ntk_Node_t ** ppNode1, Ntk_Node_t ** ppNode2 );
extern Ntk_Node_t *      Ntk_NodeCreateCollector( Ntk_Network_t * pNet, Ntk_Node_t * ppFanins[], int nValues, int DefValue );
extern Ntk_Node_t *      Ntk_NodeCreateSelector( Ntk_Network_t * pNet, Ntk_Node_t * pNode, int iSet );
extern Ntk_Node_t *      Ntk_NodeCreateIset( Ntk_Network_t * pNet, Ntk_Node_t * pNode, int iSet );
extern Ntk_Node_t *      Ntk_NodeCreateFromNetwork( Ntk_Network_t * pNet, char ** psCiNames );
extern Ntk_Node_t *      Ntk_NodeDup( Ntk_Network_t * pNetNew, Ntk_Node_t * pNode );
extern Ntk_Node_t *      Ntk_NodeClone( Ntk_Network_t * pNetNew, Ntk_Node_t * pNode );
extern void              Ntk_NodeReplace( Ntk_Node_t * pNode, Ntk_Node_t * pNodeNew );
extern void              Ntk_NodeDelete( Ntk_Node_t * pNode );
extern void              Ntk_NodeRecycle( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern bool              Ntk_NodeIsTooLarge( Ntk_Node_t * pNode );
extern bool              Ntk_NodeIsBinaryBuffer( Ntk_Node_t * pNode );
extern bool              Ntk_NodesCanBeMerged( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2 );
extern bool              Ntk_NodeDeterminize( Ntk_Node_t * pNode );
extern Ntk_NodeBinding_t * Ntk_NodeBindingCreate( char * pGate, Ntk_Node_t * pNode, float Arrival );
extern char *            Ntk_NodeBindingReadGate( Ntk_NodeBinding_t * pBinding );
extern float             Ntk_NodeBindingReadArrival( Ntk_NodeBinding_t * pBinding );
extern Ntk_Node_t **     Ntk_NodeBindingReadFanInArray( Ntk_NodeBinding_t * pBinding );
extern void              Ntk_NodeFreeBinding( Ntk_Node_t * pNode );
extern void              Ntk_NetworkFreeBinding( Ntk_Network_t * pNet );
extern int               Ntk_NetworkCheckAllBound( Ntk_Network_t * pNet, char * pInfo );
extern int               Ntk_NodeGetArrivalTimeDiscrete( Ntk_Node_t * pNode, double dAndGateDelay );
/*=== ntkNodeHeap.c =======================================================*/
extern Ntk_NodeHeap_t *  Ntk_NodeHeapStart();
extern void              Ntk_NodeHeapStop( Ntk_NodeHeap_t * p );
extern void              Ntk_NodeHeapPrint( FILE * pFile, Ntk_NodeHeap_t * p );
extern void              Ntk_NodeHeapCheck( Ntk_NodeHeap_t * p );
extern void              Ntk_NodeHeapCheckOne( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode );
extern void              Ntk_NodeHeapInsert( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode );  
extern void              Ntk_NodeHeapUpdate( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode );  
extern void              Ntk_NodeHeapDelete( Ntk_NodeHeap_t * p, Ntk_Node_t * pNode );  
extern int               Ntk_NodeHeapReadMaxWeight( Ntk_NodeHeap_t * p );
extern int               Ntk_NodeHeapCountNodes( Ntk_NodeHeap_t * p, int WeightLimit );  
extern Ntk_Node_t *      Ntk_NodeHeapReadMax( Ntk_NodeHeap_t * p );  
extern Ntk_Node_t *      Ntk_NodeHeapGetMax( Ntk_NodeHeap_t * p );  
/*=== ntkNodeVec.c =======================================================*/
extern Ntk_NodeVec_t *   Ntk_NodeVecAlloc( int nCap );
extern Ntk_NodeVec_t *   Ntk_NodeVecAllocArray( Ntk_Node_t ** pArray, int nSize );
extern Ntk_NodeVec_t *   Ntk_NodeVecDup( Ntk_NodeVec_t * pVec );
extern Ntk_NodeVec_t *   Ntk_NodeVecDupArray( Ntk_NodeVec_t * pVec );
extern void              Ntk_NodeVecFree( Ntk_NodeVec_t * p );
extern void              Ntk_NodeVecFill( Ntk_NodeVec_t * p, int nSize, Ntk_Node_t * pEntry );
extern Ntk_Node_t **     Ntk_NodeVecReleaseArray( Ntk_NodeVec_t * p );
extern Ntk_Node_t **     Ntk_NodeVecReadArray( Ntk_NodeVec_t * p );
extern int               Ntk_NodeVecReadSize( Ntk_NodeVec_t * p );
extern Ntk_Node_t *      Ntk_NodeVecReadEntry( Ntk_NodeVec_t * p, int i );
extern Ntk_Node_t *      Ntk_NodeVecReadEntryLast( Ntk_NodeVec_t * p );
extern void              Ntk_NodeVecWriteEntry( Ntk_NodeVec_t * p, int i, Ntk_Node_t * pEntry );
extern void              Ntk_NodeVecGrow( Ntk_NodeVec_t * p, int nCapMin );
extern void              Ntk_NodeVecShrink( Ntk_NodeVec_t * p, int nSizeNew );
extern void              Ntk_NodeVecClear( Ntk_NodeVec_t * p );
extern void              Ntk_NodeVecPush( Ntk_NodeVec_t * p, Ntk_Node_t * pEntry );
extern Ntk_Node_t *      Ntk_NodeVecPop( Ntk_NodeVec_t * p );
extern void              Ntk_NodeVecSort( Ntk_NodeVec_t * p );
/*=== ntkSort.c ====================================================*/
extern int               Ntk_NetworkCompactNodeIds( Ntk_Network_t * pNet );
extern int               Ntk_NetworkOrderFanins( Ntk_Network_t * pNet );
extern int               Ntk_NodeOrderFanins( Ntk_Node_t * pNode );
extern int *             Ntk_NodeRelinkFanins( Ntk_Node_t * pNode );
extern void              Ntk_NetworkReassignIds( Ntk_Network_t * pNet );
extern void              Ntk_NodeCompactFanins( Ntk_Node_t * pNode, Ntk_Node_t * pFaninDup );
/*=== ntkSubnetwork.c ====================================================*/
extern Ntk_Network_t *   Ntk_SubnetworkExtract( Ntk_Network_t * pNet );
extern void              Ntk_SubnetworkInsert( Ntk_Network_t * pNet, Ntk_Network_t * pNetSub );
extern void              Ntk_SubnetworkWriteIntoFile( Ntk_Network_t * pNet, char * FileName, bool fBinary );
/*=== ntkUtils.c ====================================================*/
extern Ntk_Network_t *   Ntk_NetworkAlloc( Mv_Frame_t * pMvsis );
extern Ntk_Network_t *   Ntk_NetworkDup( Ntk_Network_t * pNet, Fnc_Manager_t * pMan );
extern Ntk_Network_t *   Ntk_NetworkAppend( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2 );
extern void              Ntk_NetworkDelete( Ntk_Network_t * pNet );
extern Ntk_Network_t *   Ntk_NetworkCreateFromNode( Mv_Frame_t * pMvsis, Ntk_Node_t * pNode );
extern void              Ntk_NetworkAddNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode, bool fConnect ); 
extern void              Ntk_NetworkAddNode2( Ntk_Network_t * pNet, Ntk_Node_t * pNode, bool fConnect ); 
extern Ntk_Node_t *      Ntk_NetworkAddNodeCo( Ntk_Network_t * pNet, Ntk_Node_t * pNode, bool fConnect );
extern void              Ntk_NetworkTransformNodeIntToCo( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern void              Ntk_NetworkTransformCiToCo( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern void              Ntk_NetworkTransformNodeIntToCi( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern void              Ntk_NetworkDeleteNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode, bool fDisconnect, bool fDeleteNode ); 
extern bool              Ntk_NetworkIsBinary( Ntk_Network_t * pNet );
extern bool              Ntk_NetworkIsND( Ntk_Network_t * pNet );
extern Ntk_Node_t *      Ntk_NetworkReadCiNode( Ntk_Network_t * pNet, int i );
extern Ntk_Node_t *      Ntk_NetworkReadCoNode( Ntk_Network_t * pNet, int i );
extern int               Ntk_NetworkReadCiIndex( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern int               Ntk_NetworkReadCoIndex( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern int               Ntk_NetworkChangeNodeName( Ntk_Node_t * pNode, char * pName );
extern void              Ntk_NetworkTransformCi( Ntk_Node_t * pNodeCi, Ntk_Node_t * pFanin );
extern char **           Ntk_NetworkCollectIONames( Ntk_Network_t * pNet, int fCollectPis );
/*=== ntkTravId.c =========================================================*/
extern int               Ntk_NetworkReadTravId( Ntk_Network_t * pNet );
extern void              Ntk_NetworkIncrementTravId( Ntk_Network_t * pNet );
extern int               Ntk_NodeReadTravId( Ntk_Node_t * pNode );
extern void              Ntk_NodeSetTravId( Ntk_Node_t * pNode, int TravId );
extern void              Ntk_NodeSetTravIdCurrent( Ntk_Node_t * pNode );
extern bool              Ntk_NodeIsTravIdCurrent( Ntk_Node_t * pNode );
extern bool              Ntk_NodeIsTravIdPrevious( Ntk_Node_t * pNode );
extern void              Ntk_NetworkStartSpecial( Ntk_Network_t * pNet );
extern void              Ntk_NetworkStopSpecial( Ntk_Network_t * pNet );
extern void              Ntk_NetworkAddToSpecial( Ntk_Node_t * pNode );
extern void              Ntk_NetworkMoveSpecial( Ntk_Node_t * pNode );
extern void              Ntk_NetworkResetSpecial( Ntk_Network_t * pNet );
extern int               Ntk_NetworkCountSpecial( Ntk_Network_t * pNet );
extern void              Ntk_NetworkCreateSpecialFromArray( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[], int nNodes );
extern int               Ntk_NetworkCreateArrayFromSpecial( Ntk_Network_t * pNet, Ntk_Node_t * ppNodes[] );
extern st_table *        Ntk_NetworkCreateTableFromSpecial( Ntk_Network_t * pNet );
extern void              Ntk_NetworkPrintSpecial( Ntk_Network_t * pNet );
extern void              Ntk_NetworkPrintArray( Ntk_Node_t ** ppNodes, int nNodes );

/*=== ntkGlobal.c ====================================================*/
// global functions of the network
extern void              Ntk_NetworkGlobalComputeCo( Ntk_Network_t * pNet, bool fDynEnable, bool fLatchOnly, bool fVerbose );
extern DdNode **         Ntk_NetworkGlobalComputeCoArray( Ntk_Network_t * pNet, bool fBinary, bool fDynEnable, bool fLatchOnly, bool fVerbose );
extern void              Ntk_NetworkGlobalCompute( Ntk_Network_t * pNet, bool fDropInternal, bool fDynEnable, bool fLatchOnly, bool fVerbose );
extern int               Ntk_NetworkGlobalComputeOne( Ntk_Network_t * pNet, bool fExdc, bool fDropInternal, bool fDynEnable, bool fLatchOnly, bool fVerbose );
extern void              Ntk_NetworkGlobalSetCis( Ntk_Network_t * pNetOld, Ntk_Network_t * pNetNew );
extern void              Ntk_NetworkGlobalDeref( Ntk_Network_t * pNet );
extern void              Ntk_NetworkGlobalClean( Ntk_Network_t * pNet );
extern void              Ntk_NetworkGlobalCleanCo( Ntk_Network_t * pNet );
extern DdNode *          Ntk_NetworkGlobalRelation( Ntk_Network_t * pNet, int fSeqOnly, int fReorder );
extern Vmx_VarMap_t *    Ntk_NetworkGlobalCreateVmxCi( Ntk_Network_t * pNet );
extern Vmx_VarMap_t *    Ntk_NetworkGlobalCreateVmxCo( Ntk_Network_t * pNet, bool fLatchOnly );
extern void              Ntk_NodeGlobalClean( Ntk_Node_t * pNode );
extern int               Ntk_NodeGlobalCompute( DdManager * dd, Ntk_Node_t * pNode, int fDropInternal );
extern int               Ntk_NodeGlobalComputeZ( Ntk_Node_t * pNode, int TimeLimit );
extern int               Ntk_NodeGlobalComputeBinary( DdManager * dd, Ntk_Node_t * pNode, int fDropInternal );
extern int               Ntk_NodeGlobalComputeZBinary( Ntk_Node_t * pNode );
// global functions of the node
extern void              Ntk_NodeGlobalCopyFuncs( DdNode ** pbDest, DdNode ** pbSource, int nFuncs );
extern void              Ntk_NodeGlobalAddToFuncs( DdManager * dd, DdNode ** pbFuncs, DdNode * bExdc, int nFuncs );
extern void              Ntk_NodeGlobalRefFuncs( DdNode ** pbFuncs, int nFuncs );
extern void              Ntk_NodeGlobalDerefFuncs( DdManager * dd, DdNode ** pbFuncs, int nFuncs );
extern DdNode *          Ntk_NodeGlobalConvolveFuncs( DdManager * dd, DdNode * pbFuncs1[], DdNode * pbFuncs2[], int nFuncs );
extern DdNode *          Ntk_NodeGlobalImplyFuncs( DdManager * dd, DdNode * pbFuncs1[], DdNode * pbFuncs2[], int nFuncs );
extern bool              Ntk_NodeGlobalCheckNDFuncs( DdManager * dd, DdNode ** pbFuncs, int nValues );
extern int               Ntk_NodeGlobalNumSharedBddNodes( DdManager * dd, Ntk_Network_t * pNet );
/*=== ntkMinBase.c ====================================================*/
extern bool              Ntk_NodeMakeMinimumBase( Ntk_Node_t * pNode );
/*=== ntkNodeCol.c ====================================================*/
extern bool              Ntk_NetworkCollapseNodes( Ntk_Node_t * pNode, Ntk_Node_t * pFanin );
extern void              Ntk_NetworkCollapseNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode );
extern Ntk_Node_t *      Ntk_NodeCollapse( Ntk_Node_t * pNode, Ntk_Node_t * pFanin );
extern Ntk_Node_t *      Ntk_NodeMakeCommonBase( Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, int * pTransMapN1, int * pTransMapN2 );

////////////////////////////////////////////////////////////////////////
///                        ITERATORS                                 ///
////////////////////////////////////////////////////////////////////////
/*
    The safe iterators allow the user to delete the object
    that is currently being iterated through.
    Note the following important difference of the iterator
    through the nodes (Ntk_NetworkForEachNode) compared to SIS/VIS.
    The present iterator iterated only through the internal nodes,
    and not through all nodes (including PIs and POs) as before!
    This change led to certain simplification of the code. 
*/

// iterator through the primary inputs
#define Ntk_NetworkForEachCi( Net, Node )                          \
    for ( Node = Ntk_NetworkReadCiHead(Net);                       \
          Node;                                                    \
          Node = Ntk_NodeReadNext(Node) )
#define Ntk_NetworkForEachCiSafe( Net, Node, Node2 )               \
    for ( Node = Ntk_NetworkReadCiHead(Net),                       \
          Node2 = (Node? Ntk_NodeReadNext(Node): NULL);            \
          Node;                                                    \
          Node = Node2,                                            \
          Node2 = (Node? Ntk_NodeReadNext(Node): NULL) )

// iterator through the primary outputs
#define Ntk_NetworkForEachCo( Net, Node )                          \
    for ( Node = Ntk_NetworkReadCoHead(Net);                       \
          Node;                                                    \
          Node = Ntk_NodeReadNext(Node) )
#define Ntk_NetworkForEachCoSafe( Net, Node, Node2 )               \
    for ( Node = Ntk_NetworkReadCoHead(Net),                       \
          Node2 = (Node? Ntk_NodeReadNext(Node): NULL);            \
          Node;                                                    \
          Node = Node2,                                            \
          Node2 = (Node? Ntk_NodeReadNext(Node): NULL) )

// iterator through the primary output drivers
#define Ntk_NetworkForEachCoDriver( Net, NodeCo, Driver )          \
    for ( NodeCo = Ntk_NetworkReadCoHead(Net),                     \
          Driver = NodeCo? Ntk_NodeReadFaninNode(NodeCo,0): NULL;  \
          NodeCo;                                                  \
          NodeCo = Ntk_NodeReadNext(NodeCo),                       \
          Driver = NodeCo? Ntk_NodeReadFaninNode(NodeCo,0): NULL )

// iterator through the internal nodes
#define Ntk_NetworkForEachNode( Net, Node )                        \
    for ( Node = Ntk_NetworkReadNodeHead(Net);                     \
          Node;                                                    \
          Node = Ntk_NodeReadNext(Node) )
#define Ntk_NetworkForEachNode2( Net, Node )                        \
    for ( Node = Ntk_NetworkReadNodeHead2(Net);                     \
          Node;                                                    \
          Node = Ntk_NodeReadNext(Node) )
#define Ntk_NetworkForEachNodeSafe( Net, Node, Node2 )             \
    for ( Node = Ntk_NetworkReadNodeHead(Net),                     \
          Node2 = (Node? Ntk_NodeReadNext(Node): NULL);            \
          Node;                                                    \
          Node = Node2,                                            \
          Node2 = (Node? Ntk_NodeReadNext(Node): NULL) )

// specialized iterator through the ordered nodes
#define Ntk_NetworkForEachNodeSpecial( Net, Node )                 \
    for ( Node = Ntk_NetworkReadOrder(Net);                        \
          Node;                                                    \
          Node = Ntk_NodeReadOrder(Node) )
#define Ntk_NetworkForEachNodeSpecialStart( Start, Node )          \
    for ( Node = Start;                                            \
          Node;                                                    \
          Node = Ntk_NodeReadOrder(Node) )
#define Ntk_NetworkForEachNodeSpecialStartSafe( Start, Node, Node2)\
    for ( Node = Start,                        \
          Node2 = (Node? Ntk_NodeReadOrder(Node): NULL);           \
          Node;                                                    \
          Node = Node2,                                            \
          Node2 = (Node? Ntk_NodeReadOrder(Node): NULL) )
#define Ntk_NetworkForEachNodeSpecialSafe( Net, Node, Node2 )      \
    for ( Node = Ntk_NetworkReadOrder(Net),                        \
          Node2 = (Node? Ntk_NodeReadOrder(Node): NULL);           \
          Node;                                                    \
          Node = Node2,                                            \
          Node2 = (Node? Ntk_NodeReadOrder(Node): NULL) )


// specialized iterator through the nodes in the levelized structure
#define Ntk_NetworkForEachNodeSpecialByLevel( Net, Level, Node )   \
    for ( Node = Ntk_NetworkReadOrderByLevel(Net, Level);          \
          Node;                                                    \
          Node = Ntk_NodeReadOrder(Node) )
#define Ntk_NetworkForEachNodeSpecialByLevelSafe( Net, Level, Node, Node2 )\
    for ( Node = Ntk_NetworkReadOrderByLevel(Net, Level),          \
          Node2 = (Node? Ntk_NodeReadOrder(Node): NULL);           \
          Node;                                                    \
          Node = Node2,                                            \
          Node2 = (Node? Ntk_NodeReadOrder(Node): NULL) )



// iterator through latches
#define Ntk_NetworkForEachLatch( Net, Latch )                      \
    for ( Latch = Ntk_NetworkReadLatchHead(Net);                   \
          Latch;                                                   \
          Latch = Ntk_LatchReadNext(Latch) )
#define Ntk_NetworkForEachLatchSafe( Net, Latch, Latch2 )          \
    for ( Latch = Ntk_NetworkReadLatchHead(Net),                   \
          Latch2 = (Latch? Ntk_LatchReadNext(Latch): NULL);        \
          Latch;                                                   \
          Latch = Latch2,                                          \
          Latch2 = (Latch? Ntk_LatchReadNext(Latch): NULL) )


// iterator through the fanins of the node
#define Ntk_NodeForEachFanin( Node, Pin, Fanin )                   \
    for ( Pin = Ntk_NodeReadFaninPinHead(Node);                    \
          Pin && (((unsigned)(Fanin = Ntk_PinReadNode(Pin)))>=0);  \
          Pin = Ntk_PinReadNext(Pin) )
#define Ntk_NodeForEachFaninSafe( Node, Pin, Pin2, Fanin )         \
    for ( Pin = Ntk_NodeReadFaninPinHead(Node),                    \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL);                \
          Pin && (((unsigned)(Fanin = Ntk_PinReadNode(Pin)))>=0);  \
          Pin = Pin2,                                              \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL) )

// iterator through the fanouts of the node
#define Ntk_NodeForEachFanout( Node, Pin, Fanout )                 \
    for ( Pin = Ntk_NodeReadFanoutPinHead(Node);                   \
          Pin && (((unsigned)(Fanout = Ntk_PinReadNode(Pin)))>=0); \
          Pin = Ntk_PinReadNext(Pin) )
#define Ntk_NodeForEachFanoutSafe( Node, Pin, Pin2, Fanout )       \
    for ( Pin = Ntk_NodeReadFanoutPinHead(Node),                   \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL);                \
          Pin && (((unsigned)(Fanout = Ntk_PinReadNode(Pin)))>=0); \
          Pin = Pin2,                                              \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL) )


// iterator through the fanins of the node with fanin index
#define Ntk_NodeForEachFaninWithIndex( Node, Pin, Fanin, i )       \
    for ( Pin = Ntk_NodeReadFaninPinHead(Node), i = 0;             \
          Pin && (((unsigned)(Fanin = Ntk_PinReadNode(Pin)))>=0);  \
          Pin = Ntk_PinReadNext(Pin), i++ )
#define Ntk_NodeForEachFaninWithIndexSafe( Node, Pin, Pin2, Fanin, i )\
    for ( Pin = Ntk_NodeReadFaninPinHead(Node),                    \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL), i = 0;         \
          Pin && (((unsigned)(Fanin = Ntk_PinReadNode(Pin)))>=0);  \
          Pin = Pin2,                                              \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL), i++ )

// iterator through the fanouts of the node with fanout index
#define Ntk_NodeForEachFanoutWithIndex( Node, Pin, Fanout, i )     \
    for ( Pin = Ntk_NodeReadFanoutPinHead(Node), i = 0;            \
          Pin && (((unsigned)(Fanout = Ntk_PinReadNode(Pin)))>=0); \
          Pin = Ntk_PinReadNext(Pin), i++ )
#define Ntk_NodeForEachFanoutWithIndexSafe( Node, Pin, Pin2, Fanout, i )\
    for ( Pin = Ntk_NodeReadFanoutPinHead(Node),                   \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL), i = 0;         \
          Pin && (((unsigned)(Fanout = Ntk_PinReadNode(Pin)))>=0); \
          Pin = Pin2,                                              \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL), i++ )


// iterator through the fanouts of the node with fanout's fanin index
#define Ntk_NodeForEachFanoutWithFaninIndex( Node, Pin, Fanout, FaninIndex )\
    for ( Pin = Ntk_NodeReadFanoutPinHead(Node);                   \
          Pin && (((unsigned)(Fanout = Ntk_PinReadNode(Pin)))>=0) &&\
          (((unsigned)(FaninIndex = Ntk_NodeReadFaninIndex(Fanout,Node)))>=0);\
          Pin = Ntk_PinReadNext(Pin), i++ )
#define Ntk_NodeForEachFanoutWithFaninIndexSafe( Node, Pin, Pin2, Fanout, FaninIndex )\
    for ( Pin = Ntk_NodeReadFanoutPinHead(Node),                   \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL), i = 0;         \
          Pin && (((unsigned)(Fanout = Ntk_PinReadNode(Pin)))>=0) &&\
          (((unsigned)(FaninIndex = Ntk_NodeReadFaninIndex(Fanout,Node)))>=0);\
          Pin = Pin2,                                              \
          Pin2 = (Pin? Ntk_PinReadNext(Pin): NULL), i++ )

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

