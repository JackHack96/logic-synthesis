/**CFile****************************************************************

  FileName    [mvaFunc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [BDD array package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvaFunc.c,v 1.4 2003/05/27 23:15:08 alanmi Exp $]

***********************************************************************/


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "mv.h"


DdManager *Mva_FuncReadDd     ( Mva_Func_t *pMva )     { return pMva->dd;         }
int        Mva_FuncReadIsetNum( Mva_Func_t *pMva )     { return pMva->nIsets;     }
DdNode    *Mva_FuncReadIset   ( Mva_Func_t *pMva, int i ) { return pMva->pbFuncs[i]; }


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mva_Func_t *
Mva_FuncAlloc( DdManager *dd, int nIsets )
{
    Mva_Func_t *pMva;
    pMva = ALLOC( Mva_Func_t, 1 );
    pMva->nIsets  = nIsets;
    pMva->dd      = dd;
    pMva->pbFuncs = ALLOC( DdNode *, nIsets );
    memset( pMva->pbFuncs, 0, nIsets * sizeof(DdNode *) );
    return pMva;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mva_Func_t *
Mva_FuncAllocConstant( DdManager *dd, int nIsets, int iConst )
{
    Mva_Func_t *pMva;
    pMva = ALLOC( Mva_Func_t, 1 );
    pMva->nIsets  = nIsets;
    pMva->dd      = dd;
    pMva->pbFuncs = ALLOC( DdNode *, nIsets );
    memset( pMva->pbFuncs, 0, nIsets * sizeof(DdNode *) );
    pMva->pbFuncs[iConst] = Cudd_ReadOne( dd );
    Cudd_Ref ( pMva->pbFuncs[iConst] );
    return pMva;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mva_Func_t *
Mva_FuncCreate( DdManager *dd, int nIsets, DdNode **pbFuncs )
{
    int i;
    Mva_Func_t *pMva;
    if ( pbFuncs == NULL ) return NULL;
    pMva = ALLOC( Mva_Func_t, 1 );
    pMva->nIsets  = nIsets;
    pMva->dd      = dd;
    pMva->pbFuncs = ALLOC( DdNode *, nIsets );
    memcpy( pMva->pbFuncs, pbFuncs, nIsets * sizeof(DdNode *) );
    for ( i=0; i<nIsets; ++i ) {
        Cudd_Ref( pbFuncs[i] );
    }
    return pMva;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mva_Func_t *
Mva_FuncDup( Mva_Func_t *pMva )
{
    int i;
    Mva_Func_t *pMvaNew;
    
    if ( pMva == NULL ) return NULL;
    pMvaNew = ALLOC( Mva_Func_t, 1 );
    pMvaNew->nIsets  = pMva->nIsets;
    pMvaNew->dd      = pMva->dd;
    pMvaNew->pbFuncs = ALLOC( DdNode *, pMva->nIsets );
    memcpy( pMvaNew->pbFuncs, pMva->pbFuncs, pMva->nIsets * sizeof(DdNode *) );
    for ( i=0; i<pMva->nIsets; ++i ) {
        Cudd_Ref( pMva->pbFuncs[i] );
    }
    return pMvaNew;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Mva_FuncFree( Mva_Func_t *pMva )
{
    int i;
    if ( pMva == NULL ) return;
    if ( pMva->pbFuncs ) {
        for ( i=0; i<pMva->nIsets; ++i ) {
            if ( pMva->pbFuncs[i] )
                Cudd_RecursiveDeref( pMva->dd, pMva->pbFuncs[i] );
        }
        FREE( pMva->pbFuncs );
    }
    FREE( pMva );
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Mva_FuncReplaceIset( Mva_Func_t *pMva, int iIndex, DdNode *bFunc )
{
    if ( pMva == NULL ) return;
    if ( pMva->pbFuncs[iIndex] ) {
        Cudd_RecursiveDeref ( pMva->dd, pMva->pbFuncs[iIndex] );
    }
    pMva->pbFuncs[iIndex] = bFunc;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mva_Func_t *
Mva_FuncCofactor( Mva_Func_t *pMva, DdNode *bFunc )
{
    int          i;
    Mva_Func_t * pCof;
    DdNode     * bCof;
    pCof = Mva_FuncAlloc( pMva->dd, pMva->nIsets );
    
    for ( i=0; i<pMva->nIsets; ++i ) {
        if ( pMva->pbFuncs[i] ) {
            bCof = Cudd_bddConstrain( pMva->dd, pMva->pbFuncs[i], bFunc );
            Cudd_Ref( bCof );
            pCof->pbFuncs[i] = bCof;
        }
    }
    return pCof;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
Mva_FuncIsConstant( Mva_Func_t *pMva, int *pConst )
{
    int i, nTaut;
    if ( pMva == NULL) return 0;
    
    nTaut = 0;
    for ( i=0; i<pMva->nIsets; ++i) {
        
        if ( pMva->pbFuncs[i] == Cudd_ReadOne( pMva->dd )) {
            nTaut++; *pConst = i;
        }
        else if ( pMva->pbFuncs[i] == NULL) {
            continue;
        }
        else if ( pMva->pbFuncs[i] != Cudd_ReadLogicZero( pMva->dd )) {
            return 0;
        }
    }
    return (nTaut==1);
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

