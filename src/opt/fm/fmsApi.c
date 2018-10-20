/**CFile****************************************************************

  FileName    [fmsApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Interface functions of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fmsApi.c,v 1.2 2003/05/27 23:15:34 alanmi Exp $]

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
bool Fms_ManagerIsBinary    ( Fms_Manager_t * pFm )    { return pFm->fBinary; }
bool Fms_ManagerIsSetInputs ( Fms_Manager_t * pFm )    { return pFm->fSetInputs; }
bool Fms_ManagerIsSetOutputs( Fms_Manager_t * pFm )    { return pFm->fSetOutputs; }
bool Fms_ManagerIsVerbose   ( Fms_Manager_t * pFm )    { return pFm->fVerbose; }

void Fms_ManagerSetBinary    ( Fms_Manager_t * pFm, bool fFlag )    { pFm->fBinary     = fFlag; }
void Fms_ManagerSetSetInputs ( Fms_Manager_t * pFm, bool fFlag )    { pFm->fSetInputs  = fFlag; }
void Fms_ManagerSetSetOutputs( Fms_Manager_t * pFm, bool fFlag )    { pFm->fSetOutputs = fFlag; }
void Fms_ManagerSetVerbose   ( Fms_Manager_t * pFm, bool fFlag )    { pFm->fVerbose    = fFlag; }


/**Function*************************************************************

  Synopsis    [Starts the flexibility manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fms_Manager_t * Fms_ManagerAlloc( Vm_Manager_t * pManVm, Vmx_Manager_t * pManVmx, Mvr_Manager_t * pManMvr )
{
    Fms_Manager_t * p;
    p = ALLOC( Fms_Manager_t, 1 );
    memset( p, 0, sizeof(Fms_Manager_t) );
    p->pManVm  = pManVm;
    p->pManVmx = pManVmx;
    p->pManMvr = pManMvr;
    // start the manager, in which the global MDDs will be stored
//    p->dd = Cudd_Init( 100, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Resizes the flexibility manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fms_ManagerResize( Fms_Manager_t * p, int nBinVars )
{
    int nVarsAdd, i;
    if ( p->nVars1 >= nBinVars )
        return;
    // create new variables
    nVarsAdd = nBinVars - p->nVars1;
    for ( i = 0; i < nVarsAdd; i++ )
        Cudd_bddNewVar( p->dd );
    // reallocate variable arrays
    if ( p->nVars1 < nBinVars )
    {
        FREE( p->pbVars1 );
        p->nVars1 = nBinVars;
        p->pbVars1 = ALLOC( DdNode *, p->nVars1 );
    }
    // reassign the basic variables
    assert( p->dd->size == p->nVars0 + p->nVars1 );
    for ( i = 0; i < p->nVars1; i++ )
        p->pbVars1[i] = p->dd->vars[p->nVars0 + i];
}

/**Function*************************************************************

  Synopsis    [Stops the flexibility manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fms_ManagerFree( Fms_Manager_t * p )
{
    if ( p->dd )
    Cudd_Quit( p->dd );
    free( p->pbGlos );
    free( p->pbVars0 );
    free( p->pbVars1 );
    free( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
