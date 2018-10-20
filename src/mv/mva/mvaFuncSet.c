/**CFile****************************************************************

  FileName    [mvaFunc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [BDD array package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvaFuncSet.c,v 1.2 2003/05/27 23:15:08 alanmi Exp $]

***********************************************************************/


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "mv.h"


DdManager  *   Mva_FuncSetReadDd  ( Mva_FuncSet_t * p )        { return p->pFuncs[0]->dd; }
int            Mva_FuncSetReadSize( Mva_FuncSet_t * p )        { return p->nFuncs;        }
Mva_Func_t *   Mva_FuncSetReadIset( Mva_FuncSet_t * p, int i ) { return p->pFuncs[i];     }
Vmx_VarMap_t * Mva_FuncSetReadVmx ( Mva_FuncSet_t * p )        { return p->pVmx;          }


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mva_FuncSet_t *
Mva_FuncSetAlloc( Vmx_VarMap_t * pVmx, int nFuncs )
{
    Mva_FuncSet_t * p;
    p = ALLOC( Mva_FuncSet_t, 1 );
    p->nFuncs = nFuncs;
    p->pFuncs = ALLOC( Mva_Func_t *, nFuncs );
    memset( p->pFuncs, 0, nFuncs * sizeof(Mva_Func_t *) );
    p->pVmx = pVmx;
    return p;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mva_FuncSet_t *
Mva_FuncSetDup( Mva_FuncSet_t * p )
{
    Mva_FuncSet_t * pNew;
    int i;
    if ( p == NULL ) 
        return NULL;
    pNew = Mva_FuncSetAlloc( p->pVmx, p->nFuncs );
    for ( i = 0; i < p->nFuncs; i++ )
        pNew->pFuncs[i] = Mva_FuncDup( p->pFuncs[i] );
    return pNew;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Mva_FuncSetFree( Mva_FuncSet_t * p )
{
    int i;
    if ( p == NULL ) 
        return;
    for ( i = 0; i < p->nFuncs; i++ )
        Mva_FuncFree( p->pFuncs[i] );
    free( p->pFuncs );
    free( p );
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Mva_FuncSetAdd( Mva_FuncSet_t * p, int i, Mva_Func_t * pMva )
{
    assert( p->pFuncs[i] == NULL );
    p->pFuncs[i] = pMva;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
