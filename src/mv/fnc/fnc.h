/**CFile****************************************************************

  FileName    [fnc.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Represetation of functionality in MVSIS.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fnc.h,v 1.18 2003/11/18 18:55:03 alanmi Exp $]

***********************************************************************/

#ifndef __FNC_H__
#define __FNC_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct FncManagerStruct    Fnc_Manager_t;    // the functionality manager
typedef struct FncFunctionStruct   Fnc_Function_t;   // the function structure 
typedef struct FncGlobalStruct     Fnc_Global_t;     // the global function structure 

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== fncApi.c ====================================================*/
extern Fnc_Function_t *  Fnc_FunctionAlloc();
extern void              Fnc_FunctionCopy( Fnc_Function_t * pNDest, Fnc_Function_t * pNSource );
extern void              Fnc_FunctionClean( Fnc_Function_t * pF );
extern void              Fnc_FunctionDelete( Fnc_Function_t * pF );
extern void              Fnc_FunctionDup( Fnc_Manager_t * pManOld, Fnc_Manager_t * pManNew, Fnc_Function_t * pFOld, Fnc_Function_t * pFNew );
/*=== fncMan.c ====================================================*/
extern Fnc_Manager_t *   Fnc_ManagerAllocate();
extern void              Fnc_ManagerDeallocate( Fnc_Manager_t * p );
extern Fnc_Manager_t *   Fnc_ManagerRef( Fnc_Manager_t * p );
extern void              Fnc_ManagerDeref( Fnc_Manager_t * p );
extern DdManager *       Fnc_ManagerReadDdLoc( Fnc_Manager_t * p );
extern Vm_Manager_t *    Fnc_ManagerReadManVm( Fnc_Manager_t * p );
extern Vmx_Manager_t *   Fnc_ManagerReadManVmx( Fnc_Manager_t * p );
extern Mvr_Manager_t *   Fnc_ManagerReadManMvr( Fnc_Manager_t * p );
extern Mvc_Manager_t *   Fnc_ManagerReadManMvc( Fnc_Manager_t * p );
/*=== fncNode.c ====================================================*/
// "Read" functions get the value of the corresponding pointer, even if it is NULL.
extern Vm_VarMap_t *     Fnc_FunctionReadVm( Fnc_Function_t * pF );
extern Cvr_Cover_t *     Fnc_FunctionReadCvr( Fnc_Function_t * pF );
extern Mvr_Relation_t *  Fnc_FunctionReadMvr( Fnc_Function_t * pF );
extern DdNode **         Fnc_FunctionReadGlo( Fnc_Function_t * pF );
// "Get" functions retrieve the object by computing it if it is currently NULL
extern Vm_VarMap_t *     Fnc_FunctionGetVm( Fnc_Function_t * pF );
extern Cvr_Cover_t *     Fnc_FunctionGetCvr( Fnc_Manager_t * pMan, Fnc_Function_t * pF );
extern Mvr_Relation_t *  Fnc_FunctionGetMvr( Fnc_Manager_t * pMan, Fnc_Function_t * pF );
extern DdNode **         Fnc_FunctionGetGlo( Fnc_Manager_t * pMan, Fnc_Function_t * pF );
// "Write" functions free the old pointer (if not NULL), set the new pointer, do not invalidate other pointers
extern void              Fnc_FunctionWriteVm( Fnc_Function_t * pF, Vm_VarMap_t * pVm );
extern void              Fnc_FunctionWriteCvr( Fnc_Function_t * pF, Cvr_Cover_t * pCvr );
extern void              Fnc_FunctionWriteMvr( Fnc_Function_t * pF, Mvr_Relation_t * pMvr );
extern void              Fnc_FunctionWriteGlo( Fnc_Function_t * pF, DdNode ** pGlo );
// "Set" functions free the old pointer (if not NULL), set the new pointer, invalidate other pointers
extern void              Fnc_FunctionSetVm( Fnc_Function_t * pF, Vm_VarMap_t * pVm ); 
extern void              Fnc_FunctionSetCvr( Fnc_Function_t * pF, Cvr_Cover_t * pCvr ); 
extern void              Fnc_FunctionSetMvr( Fnc_Function_t * pF, Mvr_Relation_t * pMvr ); 
extern void              Fnc_FunctionSetGlo( Fnc_Function_t * pF, DdNode ** pGlo );
// freeing
extern void              Fnc_FunctionFreeVm( Fnc_Function_t * pF );
extern void              Fnc_FunctionFreeCvr( Fnc_Function_t * pF );
extern void              Fnc_FunctionFreeMvr( Fnc_Function_t * pF );
extern void              Fnc_FunctionFreeGlo( Fnc_Function_t * pF );
/*=== fncGlobal.c ====================================================*/
// create/clean/delete
extern Fnc_Global_t *    Fnc_GlobalAlloc( int nValues );
extern void              Fnc_GlobalClean( Fnc_Global_t * pG );
extern void              Fnc_GlobalFree( Fnc_Global_t * pG );
// global BDD manager
extern DdManager *       Fnc_GlobalReadDd( Fnc_Global_t * pG );
extern void              Fnc_GlobalWriteDd( Fnc_Global_t * pG, DdManager * dd );
// global functions
extern DdNode **         Fnc_GlobalReadGlo( Fnc_Global_t * pG );
extern void              Fnc_GlobalWriteGlo( Fnc_Global_t * pG, DdNode ** pbGlo );
extern void              Fnc_GlobalDerefGlo( Fnc_Global_t * pG );
// modified global functions
extern DdNode **         Fnc_GlobalReadGloZ( Fnc_Global_t * pG );
extern void              Fnc_GlobalWriteGloZ( Fnc_Global_t * pG, DdNode ** pbGloZ );
extern void              Fnc_GlobalDerefGloZ( Fnc_Global_t * pG );
// symbolic global functions
extern DdNode **         Fnc_GlobalReadGloS( Fnc_Global_t * pG );
extern void              Fnc_GlobalWriteGloS( Fnc_Global_t * pG, DdNode ** pbGloS );
extern void              Fnc_GlobalDerefGloS( Fnc_Global_t * pG );
// other data members
extern bool              Fnc_GlobalReadNonDet( Fnc_Global_t * pG );
extern void              Fnc_GlobalWriteNonDet( Fnc_Global_t * pG, bool fNonDet );
extern int               Fnc_GlobalReadNumber( Fnc_Global_t * pG );
extern void              Fnc_GlobalWriteNumber( Fnc_Global_t * pG, int nNumber );
// global functions (binary)
extern DdNode *          Fnc_GlobalReadBinGlo( Fnc_Global_t * pG );
extern void              Fnc_GlobalWriteBinGlo( Fnc_Global_t * pG, DdNode * bGlo );
extern void              Fnc_GlobalDerefBinGlo( Fnc_Global_t * pG );
// modified global functions (binary)
extern DdNode *          Fnc_GlobalReadBinGloZ( Fnc_Global_t * pG );
extern void              Fnc_GlobalWriteBinGloZ( Fnc_Global_t * pG, DdNode * bGloZ );
extern void              Fnc_GlobalDerefBinGloZ( Fnc_Global_t * pG );
// symbolic global functions (binary)
extern DdNode *          Fnc_GlobalReadBinGloS( Fnc_Global_t * pG );
extern void              Fnc_GlobalWriteBinGloS( Fnc_Global_t * pG, DdNode * bGloS );
extern void              Fnc_GlobalDerefBinGloS( Fnc_Global_t * pG );
/*=== fncMvr.c ====================================================*/
extern Mvr_Relation_t *  Fnc_FunctionDeriveMvrFromCvr( Mvr_Manager_t * pManMvr, Vmx_Manager_t * pManVmx, Cvr_Cover_t * pCvr );
extern int               Fnc_FunctionDeriveMddFromCvr( DdManager * dd, Vm_VarMap_t * pVm, Cvr_Cover_t * pCvr, DdNode ** pbCodes, DdNode ** pbResults );
extern DdNode *          Fnc_FunctionDeriveMddFromSop( DdManager * dd, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, DdNode ** pInputMdds );
extern DdNode *          Fnc_FunctionDeriveMddFromZdd( DdManager * dd, Vm_VarMap_t * pVm, DdNode * zIsop, DdNode ** pInputMdds );
/*=== fncCvr.c ====================================================*/
extern Cvr_Cover_t *     Fnc_FunctionDeriveCvrFromMvr( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, int fUseDefault );
extern int               Fnc_FunctionDeriveZddFromMvr( DdManager * dd, DdNode * pbIsets[], DdNode * pzIsets[], int nIsets );
extern Mvc_Cover_t *     Fnc_FunctionDeriveSopFromMdd( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, DdNode * bMddOn, DdNode * bMddOnDc, int nVarsUsed );
extern Mvc_Cover_t *     Fnc_FunctionDeriveSopFromMddSpecial( Mvc_Manager_t * pManMvc, DdManager * dd, DdNode * bMddOn, DdNode * bMddOnDc, Vmx_VarMap_t * pVmx, int nVarsUsed );
extern Mvc_Cover_t *     Fnc_FunctionDeriveSopFromMddLimited( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, DdNode * bMddOn, DdNode * bMddOnDc, int nVarsUsed );
extern Mvc_Cover_t *     Fnc_FunctionDeriveSopFromMddEspresso( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, DdNode * bOn, DdNode * bOnDc, int nVarsUsed );
extern DdNode *          Fnc_FunctionDeriveZddFromMdd( Mvr_Relation_t * pMvr, DdNode * bMddOn, DdNode * bMddOnDc );
/*=== fncSopMin.c ====================================================*/
extern Cvr_Cover_t *     Fnc_FunctionMinimizeCvr( Mvc_Manager_t * pManMvc, Mvr_Relation_t * pMvr, bool fUseIsop );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

