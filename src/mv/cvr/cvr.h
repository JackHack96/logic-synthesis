/**CFile****************************************************************

  FileName    [cvr.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of Cvr.]

  Description [A Cvr_Cover_t is a data structure, similar to Tbl_Table_t, that
  represents a multi-valued output SOP. It is inherited from the cube
  (pset_family) structure used in ESPRESSO, but extends to multi-valued
  outputs.
 
  A Cvr_Cover_t is a static array of SOP (pset_family), eached called an i-set
  of the cover. The size of the array is the same as the number of output
  values. One of the output i-set is designated as the default valued and its
  cover is by definition the complement of all other i-sets. The Cvr structure
  has indexes to keep track of the number of output values and which value is
  the default. 

  A Cvr_Cover_t can optionally have a compressed representation. Given a 
  priority ordering of all output values, each i-set can be minimized using
  don't cares consisting of i-sets that preceeds it in the ordering. This has
  direct application in functional simulation and software synthesis.]

  Author      [MVSIS Group]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cvr.h,v 1.25 2003/05/27 23:14:56 alanmi Exp $]

***********************************************************************/

#ifndef __CVR_H__
#define __CVR_H__

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct CvrCoverStruct      Cvr_Cover_t;   
typedef Mvc_Cover_t *              Cvr_Iset_t;

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

/*=== cvrApi.c ========================================================*/
EXTERN Cvr_Cover_t *   Cvr_CoverCreate( Vm_VarMap_t * pVm, Mvc_Cover_t ** pIsets );
EXTERN Cvr_Cover_t *   Cvr_CoverDup( Cvr_Cover_t * p );
EXTERN void            Cvr_CoverFree( Cvr_Cover_t * p );
EXTERN Mvc_Cover_t **  Cvr_CoverReadIsets( Cvr_Cover_t * p );
EXTERN Mvc_Cover_t *   Cvr_CoverReadIsetByIndex( Cvr_Cover_t * p, int ind );
EXTERN int             Cvr_CoverReadDefault( Cvr_Cover_t * p );
EXTERN Vm_VarMap_t *   Cvr_CoverReadVm( Cvr_Cover_t * p );
EXTERN void            Cvr_CoverSetIsets( Cvr_Cover_t * p, Mvc_Cover_t ** pIsets );
EXTERN void            Cvr_CoverSetVm( Cvr_Cover_t * p, Vm_VarMap_t * pVm );

EXTERN int             Cvr_CoverReadCubeNum( Cvr_Cover_t * p );
EXTERN int             Cvr_CoverReadLitSopNum( Cvr_Cover_t * p );
EXTERN int             Cvr_CoverReadLitSopValueNum( Cvr_Cover_t * p );

/*=== cvrFunc.c ===================================================*/
EXTERN void            Cvr_CoverComputeDefault (Cvr_Cover_t *cvr);
EXTERN void            Cvr_CoverResetDefault (Cvr_Cover_t *cvr);
EXTERN void            Cvr_CoverSetDefault  (Cvr_Cover_t *cvr, int i);
EXTERN bool            Cvr_CoverIsConst (Cvr_Cover_t *cvr, unsigned *bPos);
EXTERN void            Cvr_CoverAdjust  (Cvr_Cover_t *pCnew, Vm_VarMap_t *pVmNew,
                                         int *lPos);
EXTERN Cvr_Cover_t *   Cvr_CoverCreateAdjusted(Cvr_Cover_t *pCvr,Vm_VarMap_t *pVmNew,
                                         int *pnPos);
EXTERN Mvc_Cover_t *   Cvr_IsetAdjust (Mvc_Cover_t *pIset, Vm_VarMap_t *pMnew,
                                       Vm_VarMap_t *pMold, int *pnPos);
EXTERN Cvr_Cover_t *   Cvr_CoverCreatePermuted( Cvr_Cover_t * pCvr, int * pPermuteInv );
EXTERN Cvr_Cover_t *   Cvr_CoverCreateAdjust  (Cvr_Cover_t *pCnew, Vm_VarMap_t *pVmNew,
                                               int *lPos);
EXTERN int             Cvr_CoverCountLiterals( Vm_VarMap_t * pVm, Mvc_Cover_t * pCover );
EXTERN int             Cvr_CoverCountLiteralValues( Vm_VarMap_t * pVm, Mvc_Cover_t * pCover );
EXTERN Cvr_Cover_t *   Cvr_CoverOr  ( Cvr_Cover_t *pCvr1, Cvr_Cover_t *pCvr2 );

EXTERN Cvr_Cover_t *   Cvr_CoverCreateCollapsed( Cvr_Cover_t * pCvrN, Cvr_Cover_t * pCvrF, 
                           Vm_VarMap_t * pVmNew, int * pTransMapNInv, int * pTransMapF );
EXTERN Cvr_Cover_t *   Cvr_CoverCreateExpanded( Cvr_Cover_t * pCvr, Vm_VarMap_t * pVmNew );
EXTERN bool            Cvr_CoverIsND( Mvc_Data_t * pData, Cvr_Cover_t * pCvr );
EXTERN int             Cvr_CoverCountLitsWithValue( Mvc_Data_t * pData, Cvr_Cover_t * pCvr, int iVar, int iValue );

/*=== cvrEspresso.c ===================================================*/
EXTERN void            Cvr_CoverEspressoRelation( Cvr_Cover_t * pCf, Mvc_Cover_t * pCd,
                                                  int method, bool fSparse,
                                                  bool fConserve, bool fPhase );
EXTERN void            Cvr_CoverEspresso( Cvr_Cover_t * pCvr, Mvc_Cover_t *pCd,int Method,
                                          bool fSparse, bool fConserve, bool fPhase );
EXTERN Mvc_Cover_t *   Cvr_IsetEspresso( Vm_VarMap_t * pVm, 
                                         Mvc_Cover_t *OnSet, Mvc_Cover_t *DcSet, int Method,
                                         bool fSparse, bool fConserve);
EXTERN Mvc_Cover_t *   Cvr_CoverComplement( Vm_VarMap_t *pVm, Mvc_Cover_t *pMvc );
EXTERN void            Cvr_CubeCleanUp( void );


/*=== cvrFactor.c ===================================================*/
EXTERN Ft_Tree_t *     Cvr_CoverFactor( Cvr_Cover_t * pCvr );
EXTERN int             Cvr_CoverReadLitFacNum( Cvr_Cover_t * p );
EXTERN int             Cvr_CoverReadLeafValueNum( Cvr_Cover_t * p );
EXTERN Ft_Tree_t *     Cvr_CoverReadTree( Cvr_Cover_t * p );
EXTERN void            Cvr_CoverFreeFactor( Cvr_Cover_t * p );


/*=== cvrUtils.c ===================================================*/
EXTERN int             Cvr_CoverSupport( Cvr_Cover_t *pCvr, int *pSuppMv );
EXTERN Cvr_Cover_t *   Cvr_CoverCreateMinimumBase( Cvr_Cover_t *pCvr, int *pSuppMv );
EXTERN Mvc_Cube_t *    Cvr_CoverCreateCube( Cvr_Cover_t *pCvr );
EXTERN void            Cvr_CoverDestroyCube( Cvr_Cover_t *pCvr, Mvc_Cube_t *pMvcCube );
EXTERN void            Cvr_CubeClean( Cvr_Cover_t *pCvr, Mvc_Cube_t *pCube );
EXTERN void            Cvr_CoverPrint( Cvr_Cover_t *pCvr );


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
#endif

