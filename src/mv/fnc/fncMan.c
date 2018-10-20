/**CFile****************************************************************

  FileName    [fncMan.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fncMan.c,v 1.17 2003/11/18 18:55:03 alanmi Exp $]

***********************************************************************/

#include "fncInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fnc_Manager_t * Fnc_ManagerAllocate()
{
    Fnc_Manager_t * pMan;

    // allocate the manager
    pMan = ALLOC( Fnc_Manager_t, 1 );
    memset( pMan, 0, sizeof(Fnc_Manager_t) );
    pMan->nRefs = 1;
    // start other managers
    pMan->pManMvr = Mvr_ManagerAlloc();
    pMan->pManVm  = Vm_ManagerAlloc();
    pMan->pManVmx = Vmx_ManagerAlloc();
    pMan->pManMvc = Mvc_ManagerStart();
    return pMan;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects [] 

  SeeAlso     []

***********************************************************************/
void Fnc_ManagerDeallocate( Fnc_Manager_t * p )
{ 
    Mvr_ManagerFree( p->pManMvr );
    Vmx_ManagerFree( p->pManVmx );
    Vm_ManagerFree(  p->pManVm );
    Mvc_ManagerFree( p->pManMvc );
    free( p );
}


/**Function*************************************************************

  Synopsis    [References the existent DD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fnc_Manager_t * Fnc_ManagerRef( Fnc_Manager_t * p )
{
    if ( p == NULL )
        return NULL;
    p->nRefs++;
    return p;
}

/**Function*************************************************************

  Synopsis    [Dereferences the existent DD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_ManagerDeref( Fnc_Manager_t * p )
{
    if ( p == NULL )
        return;
    p->nRefs--;
//    if ( p->nRefs == 0 )
//        Fnc_ManagerDeallocate( p );
}


/**Function*************************************************************

  Synopsis    [Basic APIs of the functionality manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Manager_t * Fnc_ManagerReadManMvr( Fnc_Manager_t * p ) { return p->pManMvr; }
DdManager *     Fnc_ManagerReadDdLoc( Fnc_Manager_t * p )  { return Mvr_ManagerReadDdLoc(p->pManMvr); }
Vm_Manager_t *  Fnc_ManagerReadManVm( Fnc_Manager_t * p )  { return p->pManVm;  }
Vmx_Manager_t * Fnc_ManagerReadManVmx( Fnc_Manager_t * p ) { return p->pManVmx; }
Mvc_Manager_t * Fnc_ManagerReadManMvc( Fnc_Manager_t * p ) { return p->pManMvc; }

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


