/**CFile****************************************************************

  FileName    [vmMan.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to manipulate the manager of variable maps.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmMan.c,v 1.11 2003/09/01 04:56:58 alanmi Exp $]

***********************************************************************/

#include "vmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the variable map manager.]

  Description [The manager of variable maps is necessary because it 
  provides the hash table, which stores variable maps that are
  currently used in the network. Some of these maps may be re-used.
  Each time a variable map is reused, its reference counter is 
  incremented. Currently, the variable maps are not deleted
  even when they become useless (ref counter = 0). This is done
  in hope that the useless var maps may be reused later in the same
  manager. All the var maps (useful and useless) are deleted when
  the manager is deleted. The variable map manager can also be used
  by several networks. In this case, its reference counter is 
  incremented. Only when the ref counter of the manager becomes zero,
  the manager is Deallocated by Vm_ManagerDeallocate.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_Manager_t * Vm_ManagerAlloc()
{
    Vm_Manager_t * pMan;
    // allocate the var map manager
    pMan = ALLOC( Vm_Manager_t, 1 );
    memset( pMan, 0, sizeof(Vm_Manager_t) );
    pMan->nTableSize = Cudd_Prime(1000);
    pMan->pTable = ALLOC( Vm_VarMap_t *, pMan->nTableSize );
    memset( pMan->pTable, 0, sizeof(Vm_VarMap_t *) * pMan->nTableSize );
    pMan->nRefs = 1;
	// allocate temporary storage
    pMan->nValuesMax = 1000;
    pMan->pSupport1 = ALLOC( int, 2 * pMan->nValuesMax );
    pMan->pSupport2 = ALLOC( int, 2 * pMan->nValuesMax );
    pMan->pPermute  = ALLOC( int, 2 * pMan->nValuesMax );
    pMan->pArray1   = ALLOC( int, 2 * pMan->nValuesMax );
    pMan->pArray2   = ALLOC( int, 2 * pMan->nValuesMax );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Deallocated the manager of variable maps.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vm_ManagerFree( Vm_Manager_t * pMan )
{
    Vm_VarMap_t * pMap, * pMap2;
    int i;

    // delete var maps
    for ( i = 0; i < pMan->nTableSize; i++ )
        for ( pMap = pMan->pTable[i], pMap2 = pMap? pMap->pNext: NULL; 
            pMap; pMap = pMap2, pMap2 = pMap? pMap->pNext: NULL )
                Vm_VarMapFree( pMap );

printf( "Unique maps = %d. Reused maps = %d.\n", pMan->nMaps, pMan->nMapsUsed );
    // deloc memory
    free( pMan->pSupport1 );
    free( pMan->pSupport2 );
    free( pMan->pPermute );
    free( pMan->pArray1 );
    free( pMan->pArray2 );
    free( pMan->pTable );
    free( pMan );
}


/**Function*************************************************************

  Synopsis    [References the existent DD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_Manager_t * Vm_ManagerReference( Vm_Manager_t * p )
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
void Vm_ManagerDereference( Vm_Manager_t * p )
{
    if ( p == NULL )
        return;
    p->nRefs--;
    if ( p->nRefs == 0 )
        Vm_ManagerFree( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


