/**CFile****************************************************************

  FileName    [sh.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of structural hashing package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: sh.h,v 1.7 2004/05/12 04:30:10 alanmi Exp $]

***********************************************************************/

#ifndef __SH_H__
#define __SH_H__

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct ShManagerStruct      Sh_Manager_t;   
typedef struct ShNetworkStruct      Sh_Network_t;   
typedef struct ShNodeStruct         Sh_Node_t;   

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

#define Sh_IsComplement(p)    (((int)((long) (p) & 01)))
#define Sh_Regular(p)         ((Sh_Node_t *)((unsigned)(p) & ~01)) 
#define Sh_Not(p)             ((Sh_Node_t *)((long)(p) ^ 01)) 
#define Sh_NotCond(p,c)       ((Sh_Node_t *)((long)(p) ^ (c)))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== shApi.c ===========================================================*/
extern Sh_Node_t **   Sh_ManagerReadVars( Sh_Manager_t * p );
extern Sh_Node_t *    Sh_ManagerReadVar( Sh_Manager_t * p, int i );
extern int            Sh_ManagerReadVarsNum( Sh_Manager_t * p );
extern Sh_Node_t *    Sh_ManagerReadConst1( Sh_Manager_t * p );
extern int            Sh_ManagerReadNodes( Sh_Manager_t * p );

extern bool           Sh_ManagerGabrageCollectionEnable( Sh_Manager_t * p );
extern bool           Sh_ManagerGabrageCollectionDisable( Sh_Manager_t * p );
extern bool           Sh_ManagerTwoLevelEnable( Sh_Manager_t * p );
extern bool           Sh_ManagerTwoLevelDisable( Sh_Manager_t * p );

extern Sh_Manager_t * Sh_NetworkReadManager( Sh_Network_t * p );
extern int            Sh_NetworkReadInputNum( Sh_Network_t * p );
extern int            Sh_NetworkReadOutputNum( Sh_Network_t * p );
extern void           Sh_NetworkSetOutputNum( Sh_Network_t * p, int nOuts );
extern Sh_Node_t **   Sh_NetworkReadInputs( Sh_Network_t * p );
extern Sh_Node_t **   Sh_NetworkReadOutputs( Sh_Network_t * p );
extern Sh_Node_t *    Sh_NetworkReadOutput( Sh_Network_t * p, int i );
extern Sh_Node_t **   Sh_NetworkReadNodes( Sh_Network_t * p );
extern int            Sh_NetworkReadNodeNum( Sh_Network_t * p );
extern Vm_VarMap_t *  Sh_NetworkReadVmL( Sh_Network_t * p );
extern Vm_VarMap_t *  Sh_NetworkReadVmR( Sh_Network_t * p );
extern Vm_VarMap_t *  Sh_NetworkReadVmS( Sh_Network_t * p );
extern Vm_VarMap_t *  Sh_NetworkReadVmLC( Sh_Network_t * p );
extern Vm_VarMap_t *  Sh_NetworkReadVmRC( Sh_Network_t * p );
extern void           Sh_NetworkSetVmL( Sh_Network_t * p, Vm_VarMap_t * pVm );
extern void           Sh_NetworkSetVmR( Sh_Network_t * p, Vm_VarMap_t * pVm );
extern void           Sh_NetworkSetVmS( Sh_Network_t * p, Vm_VarMap_t * pVm );
extern void           Sh_NetworkSetVmLC( Sh_Network_t * p, Vm_VarMap_t * pVm );
extern void           Sh_NetworkSetVmRC( Sh_Network_t * p, Vm_VarMap_t * pVm );

extern Sh_Node_t **   Sh_NetworkAllocInputsCore( Sh_Network_t * p, int nInputsCore );
extern Sh_Node_t **   Sh_NetworkAllocOutputsCore( Sh_Network_t * p, int nOutputsCore );
extern Sh_Node_t **   Sh_NetworkAllocSpecials( Sh_Network_t * p, int nSpecials );

extern Sh_Node_t *    Sh_NodeReadOne( Sh_Node_t * p );
extern Sh_Node_t *    Sh_NodeReadTwo( Sh_Node_t * p );
extern Sh_Node_t *    Sh_NodeReadOrder( Sh_Node_t * p );
extern int            Sh_NodeReadIndex( Sh_Node_t * p );

extern unsigned       Sh_NodeReadData( Sh_Node_t * p );
extern void           Sh_NodeSetData( Sh_Node_t * p, unsigned uData );
extern unsigned       Sh_NodeReadDataCompl( Sh_Node_t * p );
extern void           Sh_NodeSetDataCompl( Sh_Node_t * p, unsigned uData );

extern Sh_Node_t *    Sh_NodeAnd ( Sh_Manager_t * p, Sh_Node_t * pNode1, Sh_Node_t * pNode2 );
extern Sh_Node_t *    Sh_NodeOr  ( Sh_Manager_t * p, Sh_Node_t * pNode1, Sh_Node_t * pNode2 );
extern Sh_Node_t *    Sh_NodeExor( Sh_Manager_t * p, Sh_Node_t * pNode1, Sh_Node_t * pNode2 );
extern Sh_Node_t *    Sh_NodeMux( Sh_Manager_t * p, Sh_Node_t * pNode, Sh_Node_t * pNodeT, Sh_Node_t * pNodeE );

extern bool           Sh_NodeIsAnd( Sh_Node_t * pNode );
extern bool           Sh_NodeIsVar( Sh_Node_t * pNode );
extern bool           Sh_NodeIsConst( Sh_Node_t * pNode );

extern void           Sh_Ref( Sh_Node_t * pNode );
extern void           Sh_Deref( Sh_Node_t * pNode );
extern void           Sh_RecursiveDeref( Sh_Manager_t * pMan, Sh_Node_t * pNode );
/*=== shCreate.c ===========================================================*/
extern Sh_Manager_t * Sh_ManagerCreate( int nVars, int nSize, bool fVerbose );
extern void           Sh_ManagerResize( Sh_Manager_t * pMan, int nVarsNew );
extern void           Sh_ManagerFree( Sh_Manager_t * p, bool fCheckZeroRefs );
extern Sh_Network_t * Sh_NetworkCreate( Sh_Manager_t * p, int nInputs, int nOutputs );
extern Sh_Network_t * Sh_NetworkCreateFromNode( Sh_Manager_t * p, Sh_Node_t * gNode );
extern void           Sh_NetworkFree( Sh_Network_t * p );
extern Sh_Network_t * Sh_NetworkDup( Sh_Network_t * p );
extern Sh_Network_t * Sh_NetworkAppend( Sh_Network_t * p1, Sh_Network_t * p2 );
extern Sh_Network_t * Sh_NetworkMiter( Sh_Network_t * p1, Sh_Network_t * p2 );
/*=== shHash.c ===========================================================*/
extern void           Sh_ManagerGarbageCollect( Sh_Manager_t * pMan );
extern int            Sh_ManagerCheckZeroRefs( Sh_Manager_t * pMan );
extern void           Sh_ManagerCleanData( Sh_Manager_t * pMan );
/*=== shUtils.c ===========================================================*/
extern void           Sh_NetworkSimulate( Sh_Network_t * p, unsigned uSignIn[], unsigned uSignOut[] );
extern void           Sh_NetworkBuildGlobalBdds( Sh_Network_t * p, DdManager * dd, DdNode * bFuncsIn[], DdNode * bFuncsOut[] );
/*=== shNetwork.c =========================================================*/
extern int            Sh_NodeCollectDfs( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t *** ppgNodes );
extern int            Sh_NetworkCollectInternal( Sh_Network_t * pNet );
extern int            Sh_NetworkDfs( Sh_Network_t * pNet );
extern int            Sh_NetworkDfsNode( Sh_Manager_t * pMan, Sh_Node_t * gNode );
extern int            Sh_NodeCountNodes( Sh_Manager_t * pMan, Sh_Node_t * gNode );
extern int            Sh_NetworkInterleaveNodes( Sh_Network_t * pNet );
extern int            Sh_NetworkGetNumLevels( Sh_Network_t * pNet );
extern int            Sh_NetworkCountLiterals( Sh_Manager_t * pMan, Sh_Node_t * gNode );
extern void           Sh_NetworkPrintAig( Sh_Manager_t * pMan, Sh_Node_t * gNode );
extern void           Sh_NetworkCleanData( Sh_Network_t * pNet );
extern int            Sh_NetworkSetNumbers( Sh_Network_t * pNet );
extern void           Sh_NodePrintArray( Sh_Node_t * pgNodes[], int nNodes );
extern void           Sh_NodePrint( Sh_Node_t * gNode );
extern void           Sh_NodePrintOne( Sh_Node_t * gNode );
extern void           Sh_NodePrintTrees( Sh_Node_t * pgNodes[], int nNodes );
/*=== shUtils.c =========================================================*/
extern void           Sh_NodeShow( Sh_Manager_t * pMan, Sh_Node_t * gNode, char * pFileName );
extern void           Sh_NodeShowArray( Sh_Manager_t * pMan, Sh_Node_t * pNodes[], int nNodes, char * pFileName );
extern void           Sh_NetworkShow( Sh_Network_t * pShNet, char * pFileName );
extern void           Sh_NetworkPrintDot( Sh_Network_t * pShNet, char * pFileName );
extern void           Sh_NodePrintFunction( Sh_Manager_t * pMan, Sh_Node_t * gNode );
extern DdNode *       Sh_NodeDeriveBdd( DdManager * dd, Sh_Manager_t * pMan, Sh_Node_t * pNode );
/*=== shOper.c =========================================================*/
extern Sh_Node_t *    Sh_NodeExistAbstract( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t ** pgVars, int nVars );
extern Sh_Node_t *    Sh_NodeVarReplace( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t ** pgVarsCs, Sh_Node_t ** pgVarsNs, int nVars );
extern Sh_Node_t *    Sh_NodeAndAbstract( Sh_Manager_t * pMan, Sh_Node_t * gRel, Sh_Node_t * gCare, Sh_Node_t ** pgVars, int nVars );
extern Sh_Node_t *    Sh_RelationSquare( Sh_Manager_t * pMan, Sh_Node_t * gRel, 
                        Sh_Node_t ** pgVarsCs, Sh_Node_t ** pgVarsNs, Sh_Node_t ** pgVarsIs, int nLatches );
/*=== shResyn.c =========================================================*/
extern Sh_Node_t *    Sh_NodeCollapseBdd( DdManager * dd, Sh_Manager_t * pMan, Sh_Node_t * gNode );
extern Sh_Node_t *    Sh_NodeCollapse( Sh_Manager_t * pMan, Sh_Node_t * gNode );
extern Sh_Node_t *    Sh_NodeResynthesize( Sh_Manager_t * pMan, Sh_Node_t * gNode );
extern void           Sh_NodeWriteBlif( Sh_Manager_t * pMan, Sh_Node_t * gNode, char * pFileName );
extern void           Sh_NetworkWriteBlif( Sh_Network_t * p, char * sNamesIn[], char * sNamesOut[], char * sNameFile );
extern Sh_Node_t *    Sh_NodeReadBlif( Sh_Manager_t * pMan, char * pFileName );
extern Sh_Network_t * Sh_NetworkReadBlif( Sh_Manager_t * pMan, char * sNameFile );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
