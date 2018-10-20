/**CFile****************************************************************

  FileName    [simpMvf.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Utilities for DdNode arrays]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpMvf.c,v 1.9 2003/05/27 23:16:16 alanmi Exp $]

***********************************************************************/

#include "simpInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************
  Synopsis    [Compute the primary input support of a MVF MDD array.]
  Description [Assume the extended variable map is available; and assume the
  BDD's are already in the global CI's support space.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
st_table *
SimpMvaComputeSupport(
    Mva_Func_t  *pMva,
    Simp_Info_t *pInfo) 
{
    int iSeq, nVars;
    int           *pSup;

    Ntk_Node_t    *pStick;
    st_table      *stSup;
    
    /* access information */
    nVars = Cudd_ReadSize( pInfo->ddmg );
    stSup = st_init_table(st_ptrcmp, st_ptrhash);
    
    pSup  = Cudd_VectorSupportIndex(pInfo->ddmg, pMva->pbFuncs, pMva->nIsets);
    pStick = NULL;
    for ( iSeq = 0; iSeq < nVars; ++iSeq ) {
        
        if ( pSup[iSeq] && pStick != pInfo->ppNodes[iSeq] ) {
            
            pStick = pInfo->ppNodes[iSeq];
            st_insert(stSup, (char *)pStick, NIL(char));
        }
    }
    FREE(pSup);
    
    return stSup;
}

/**Function********************************************************************

  Synopsis    [Free an Mvf function array list.]
  Description [Free an Mvf function array list. The mvf_arr is an array of
  sarray_t, each containing Mvf function elements. Free the entire array; and
  for each sarray_t, free the Mvf function starting from `start' index.]

  SideEffects []
  SeeAlso     []

******************************************************************************/
void
SimpMvaArrayListFree (
    sarray_t **list,
    int       size,
    int       start)
{
    int i;
    sarray_t *mvf_arr;
    
    for (i=0; i<size; ++i) {
        mvf_arr = list[i];
        if (mvf_arr==NULL) continue;
        SimpMvaArrayFree( mvf_arr, start );
    }
    FREE(list);
    return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
SimpMvaArrayFree( sarray_t *lFuncs, int iStart ) 
{
    int i;

    Mva_Func_t *pMva;
    
    for ( i=iStart; i<sarray_n(lFuncs); ++i) {
        pMva = sarray_fetch(Mva_Func_t *, lFuncs, i);
        if (!pMva) continue;
        Mva_FuncFree( pMva );
    }
    sarray_free(lFuncs);
    return;
}

