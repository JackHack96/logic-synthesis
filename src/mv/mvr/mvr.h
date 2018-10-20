/**CFile****************************************************************

  FileName    [mvr.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvr.h,v 1.26 2004/10/06 03:33:47 alanmi Exp $]

***********************************************************************/

#ifndef __MVR_H__
#define __MVR_H__

/* 
	The complete relation as a BEMDD is always available.
	The cofactors with respect to a variable are computed on demand.
    (i-sets are cofactors with respect to the output variable.) 
*/


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

typedef struct MvrRelationStruct   Mvr_Relation_t;   // the relation
typedef struct MvrManagerStruct    Mvr_Manager_t;    // the relation manager

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== mvr.c ==========================================================*/
/*=== mvrApi.c =======================================================*/
extern Mvr_Manager_t *  Mvr_RelationReadMan( Mvr_Relation_t * pMvr );
extern DdManager *      Mvr_RelationReadDd( Mvr_Relation_t * pMvr );
extern DdNode *         Mvr_RelationReadRel( Mvr_Relation_t * pMvr );
extern void             Mvr_RelationWriteRel( Mvr_Relation_t * pMvr, DdNode * bRel );
extern Vmx_VarMap_t *   Mvr_RelationReadVmx( Mvr_Relation_t * pMvr );
extern Vm_VarMap_t *    Mvr_RelationReadVm( Mvr_Relation_t * pMvr );
extern int              Mvr_RelationReadNodes( Mvr_Relation_t * pMvr );
extern int              Mvr_RelationGetNodes( Mvr_Relation_t * pMvr );
extern bool             Mvr_RelationReadMark( Mvr_Relation_t * pMvr );
extern void             Mvr_RelationCleanMark( Mvr_Relation_t * pMvr );
extern DdManager *      Mvr_ManagerReadDdLoc( Mvr_Manager_t * pMan );
extern reo_man *        Mvr_ManagerReadReo( Mvr_Manager_t * pMan );
extern Extra_PermMan_t* Mvr_ManagerReadPerm( Mvr_Manager_t * pMan );
/*=== mvrMan.c =======================================================*/
extern Mvr_Manager_t *  Mvr_ManagerAlloc();
extern void             Mvr_ManagerFree( Mvr_Manager_t * pMan );
/*=== mvrCofs.c =======================================================*/
extern DdNode *         Mvr_RelationGetIset( Mvr_Relation_t * pMvr, int iSet );
extern void             Mvr_RelationCofactorsDerive( Mvr_Relation_t * pMvr, DdNode ** pbCofs, int iVar, int nValues );
extern DdNode *         Mvr_RelationCofactorsDeriveRelation( Mvr_Relation_t * pMvr, DdNode ** pbCofs, int iVar, int nValues );
extern void             Mvr_RelationCofactorsDeref( Mvr_Relation_t * pMvr, DdNode ** pbCofs, int iVar, int nValues );
extern DdNode *         Mvr_RelationQuantifyExist( Mvr_Relation_t * pMvr, DdNode * bFunc, int iVar );
extern DdNode *         Mvr_RelationQuantifyUniv( Mvr_Relation_t * pMvr, DdNode * bFunc, int iVar );
/*=== mvrRel.c =======================================================*/
extern Mvr_Relation_t * Mvr_RelationAlloc();
extern Mvr_Relation_t * Mvr_RelationClone( Mvr_Relation_t * pMvr );
extern Mvr_Relation_t * Mvr_RelationDup( Mvr_Relation_t * pMvr );
extern void             Mvr_RelationFree( Mvr_Relation_t * pMvr );
extern void             Mvr_RelationInvalidateRel( Mvr_Relation_t * pMvr );
extern bool             Mvr_RelationIsND( Mvr_Relation_t * pMvr );
extern bool             Mvr_RelationIsWellDefined( Mvr_Relation_t * pMvr );
extern double           Mvr_RelationFlexRatio( Mvr_Relation_t * pMvr );
/*=== mvrUtils.c ====================================================*/
extern Mvr_Relation_t * Mvr_RelationCreate( Mvr_Manager_t * pMan, Vmx_VarMap_t * pVmx, DdNode * bRel );
extern Mvr_Relation_t * Mvr_RelationCreateConstant( Mvr_Manager_t * pMan, Vmx_Manager_t * pMapVmx, Vm_Manager_t * pMapVm, int nValues, unsigned Pol );
extern Mvr_Relation_t * Mvr_RelationCreateOneInputOneOutput( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int nValuesIn, int nValuesOut, unsigned Pols[] );
extern Mvr_Relation_t * Mvr_RelationCreateTwoInputBinary( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, unsigned TruthTable );
extern Mvr_Relation_t * Mvr_RelationCreateSimpleBinary( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int pCompls[], int nInputs, int fGateOr );
extern Mvr_Relation_t * Mvr_RelationCreateMuxBinary( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm );
extern unsigned         Mvr_RelationGetConstant( Mvr_Relation_t * pMvr );
extern Mvr_Relation_t * Mvr_RelationCreateDecoder( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int nValues1, int nValues2 );
extern Mvr_Relation_t * Mvr_RelationCreateDecoderGeneral( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int CodeWidth, int * pValueAssign, int nTotalValues, int nOutputValues );
extern void             Mvr_RelationCreateEncoded( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int CodeWidth, int * pValueAssign, int nTotalValues, int nOutputValues, Mvr_Relation_t ** ppMvrsRes, Mvr_Relation_t * pMvrInit );
extern Mvr_Relation_t * Mvr_RelationCreateCollector( Mvr_Manager_t * pMan, Vmx_Manager_t * pManVmx, Vm_Manager_t * pManVm, int nValues, int DefValue );
extern Mvr_Relation_t * Mvr_RelationCreateMinimumBase( Mvr_Relation_t * pMvr, int * pSuppMv );
extern Mvr_Relation_t * Mvr_RelationCreateCollapsed( Mvr_Relation_t * pMvrN, Mvr_Relation_t * pMvrF, Vm_VarMap_t * pVmNew, int * pTransMapNInv, int * pTransMapF );
extern Mvr_Relation_t * Mvr_RelationCreatePermuted( Mvr_Relation_t * pMvr, int * pPermuteInv );
extern Mvr_Relation_t * Mvr_RelationCreatePower( Mvr_Relation_t * pMvr, int Degree );
extern Mvr_Relation_t * Mvr_RelationCreateFromGlobal( DdManager * ddGlo, Mvr_Manager_t * pManMvr, Vmx_Manager_t * pManVmx, Vm_VarMap_t * pVm, DdNode ** pbMdds, int nValues );
extern DdNode *         Mvr_RelationRemap( Mvr_Relation_t * pMvr, DdNode * bFunc, Vmx_VarMap_t * pMapOld, Vmx_VarMap_t * pMapNew, int * pVarTrans );
extern void             Mvr_RelationMakeVarsEqual( Mvr_Relation_t * pMvr, int iVar1, int iVar2 );
extern bool             Mvr_RelationReorder( Mvr_Relation_t * pMvr );
extern int              Mvr_RelationSupportBinarySize( Mvr_Relation_t * pMvr );
extern bool             Mvr_RelationSupportBinaryIsRange( Mvr_Relation_t * pMvr );
extern bool             Mvr_RelationCheckSupport( Mvr_Relation_t * pMvr );
extern int              Mvr_RelationSupport( Mvr_Relation_t * pMvr, int * pSuppMv );
extern DdNode *         Mvr_RelationSupportCube( DdManager * dd, DdNode * bFuncs, Vmx_VarMap_t * pVmx );
extern DdNode *         Mvr_RelationSupportCubeUsingVars( DdManager * dd, DdNode * pbVars[], DdNode * bFuncs, Vmx_VarMap_t * pVmx );
extern void             Mvr_RelationAddDontCare( Mvr_Relation_t * pMvr, DdNode * bDc );
extern bool             Mvr_RelationDeterminize( Mvr_Relation_t * pMvr );
/*=== mvrPrint.c ====================================================*/
extern void             Mvr_RelationPrintKmap( FILE * pFile, Mvr_Relation_t * pMvr, char ** pVarNames );
/*=== mvrUcp.c ====================================================*/
extern int              Mvr_RelationMinimumValueSet( Mvr_Relation_t * pMvr, DdNode * bRel, int ** pValues );


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
