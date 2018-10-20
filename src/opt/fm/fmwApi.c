/**CFile****************************************************************

  FileName    [fmwApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Interface functions of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fmwApi.c,v 1.2 2003/05/27 23:15:36 alanmi Exp $]

***********************************************************************/

#include "fmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Trivial APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Fmw_ManagerIsBinary    ( Fmw_Manager_t * pFm )    { return pFm->fBinary; }
bool Fmw_ManagerIsSetInputs ( Fmw_Manager_t * pFm )    { return pFm->fSetInputs; }
bool Fmw_ManagerIsSetOutputs( Fmw_Manager_t * pFm )    { return pFm->fSetOutputs; }
bool Fmw_ManagerIsVerbose   ( Fmw_Manager_t * pFm )    { return pFm->fVerbose; }

void Fmw_ManagerSetBinary    ( Fmw_Manager_t * pFm, bool fFlag )    { pFm->fBinary     = fFlag; }
void Fmw_ManagerSetSetInputs ( Fmw_Manager_t * pFm, bool fFlag )    { pFm->fSetInputs  = fFlag; }
void Fmw_ManagerSetSetOutputs( Fmw_Manager_t * pFm, bool fFlag )    { pFm->fSetOutputs = fFlag; }
void Fmw_ManagerSetVerbose   ( Fmw_Manager_t * pFm, bool fFlag )    { pFm->fVerbose    = fFlag; }

/**Function*************************************************************

  Synopsis    [Starts the flexibility manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fmw_Manager_t * Fmw_ManagerAlloc( Vm_Manager_t * pManVm, Vmx_Manager_t * pManVmx, Mvr_Manager_t * pManMvr )
{
    Fmw_Manager_t * p;
    int i;

    p = ALLOC( Fmw_Manager_t, 1 );
    memset( p, 0, sizeof(Fmw_Manager_t) );
    p->pManVm  = pManVm;
    p->pManVmx = pManVmx;
    p->pManMvr = pManMvr;

    // start the manager, in which the global MDDs will be stored
    p->dd = Cudd_Init( 100, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
//    Cudd_AutodynEnable( p->dd, CUDD_REORDER_SYMM_SIFT );
    p->nVarsAlloc = 100;
    p->pbVars0 = ALLOC( DdNode *, p->nVarsAlloc );
    p->pbVars1 = ALLOC( DdNode *, p->nVarsAlloc );
    for ( i = 0; i < 50; i++ )
    {
        p->pbVars0[i] = p->dd->vars[i];
        p->pbVars1[i] = p->dd->vars[50 + i];
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the flexibility manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmw_ManagerFree( Fmw_Manager_t * p )
{
//printf( "Start freeing flexibility manager\n" );
    Extra_StopManager( p->dd );
//printf( "Stop freeing flexibility manager\n" );
    free( p->pbVars0 );
    free( p->pbVars1 );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Resizes the flexibility manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmw_ManagerResize( Fmw_Manager_t * p, int nBinVars )
{
    int nVarsAdd, i;
    if ( p->dd->size >= 2 * nBinVars )
        return;
    // create new variables
    nVarsAdd = 2 * nBinVars - p->dd->size;
    for ( i = 0; i < nVarsAdd; i++ )
        Cudd_bddNewVar( p->dd );
    // reallocate variable arrays
    if ( p->nVarsAlloc < nBinVars )
    {
        FREE( p->pbVars0 );
        FREE( p->pbVars1 );
        p->nVarsAlloc = 2 * nBinVars;
        p->pbVars0 = ALLOC( DdNode *, p->nVarsAlloc );
        p->pbVars1 = ALLOC( DdNode *, p->nVarsAlloc );
    }
    // reassign the basic variables
    assert( p->dd->size == 2 * nBinVars );
    for ( i = 0; i < nBinVars; i++ )
    {
        p->pbVars0[i] = p->dd->vars[i];
        p->pbVars1[i] = p->dd->vars[nBinVars + i];
    }
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
