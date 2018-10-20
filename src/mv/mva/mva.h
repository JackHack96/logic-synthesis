/**CFile****************************************************************

  FileName    [mva.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of the package to manipulate BDD arrays.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mva.h,v 1.4 2003/05/27 23:15:07 alanmi Exp $]

***********************************************************************/

#ifndef __MVA_H__
#define __MVA_H__

/* 
    This is used to represent i-sets with an array of BDD's
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

typedef struct MvaFuncStruct      Mva_Func_t;
typedef struct MvaFuncSetStruct   Mva_FuncSet_t;

// the array of BDDs (i-sets of a node)
struct MvaFuncStruct {
    DdManager *dd;
    DdNode   **pbFuncs;
    int        nIsets;
};

// the array of arrays of BDDs (global functions of a network)
struct MvaFuncSetStruct {
    Vmx_VarMap_t * pVmx;    // the variable map to describe encoding/ordering of leaves
    Mva_Func_t **  pFuncs;  // the BDDs for each i-set of each output
    int            nFuncs;  // the number of outputs
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

#define Mva_FuncForEachIset( mva, i, bFunc )                   \
    for ( (i)=0;                                                   \
          (i) < (mva)->nIsets && ((bFunc)=(mva)->pbFuncs[(i)],1);  \
          (i)++)


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== mvrFunc.c =======================================================*/
extern Mva_Func_t * Mva_FuncAlloc( DdManager *dd, int nIsets );
extern Mva_Func_t * Mva_FuncAllocConstant( DdManager *dd, int nIsets, int iConst );
extern Mva_Func_t * Mva_FuncCreate( DdManager *dd, int nIsets, DdNode **pbFuncs );
extern Mva_Func_t * Mva_FuncDup( Mva_Func_t *pMva );
extern void         Mva_FuncFree( Mva_Func_t *pMva );
extern DdManager  * Mva_FuncReadDd(  Mva_Func_t *pMva );
extern int          Mva_FuncReadIsetNum( Mva_Func_t *pMva );
extern DdNode     * Mva_FuncReadIset( Mva_Func_t *pMva, int iIndex );
extern void         Mva_FuncReplaceIset( Mva_Func_t *pMva, int iIndex, DdNode *bFunc );

extern bool         Mva_FuncIsConstant( Mva_Func_t *pMva, int *pConst );
extern Mva_Func_t * Mva_FuncCofactor( Mva_Func_t *pMva, DdNode *bFunc );

/*=== mvrFuncSet.c =======================================================*/
extern DdManager  *    Mva_FuncSetReadDd  ( Mva_FuncSet_t * p );
extern int             Mva_FuncSetReadSize( Mva_FuncSet_t * p );
extern Mva_Func_t *    Mva_FuncSetReadIset( Mva_FuncSet_t * p, int i );
extern Vmx_VarMap_t *  Mva_FuncSetReadVmx ( Mva_FuncSet_t * p );

extern Mva_FuncSet_t * Mva_FuncSetAlloc( Vmx_VarMap_t * pVmx, int nFuncs );
extern Mva_FuncSet_t * Mva_FuncSetDup( Mva_FuncSet_t * p );
extern void            Mva_FuncSetFree( Mva_FuncSet_t * p );
extern void            Mva_FuncSetAdd( Mva_FuncSet_t * p, int i, Mva_Func_t * pMva );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
