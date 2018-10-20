/**CFile****************************************************************

  FileName    [vmxMan.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to manipulate the manager of variable maps.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vmxMan.c,v 1.11 2003/09/01 04:57:00 alanmi Exp $]

***********************************************************************/

#include "vmxInt.h"

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
  the manager is Deallocated by Vmx_ManagerDeallocate.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_Manager_t * Vmx_ManagerAlloc()
{
    Vmx_Manager_t * pMan;
    // allocate the var map manager
    pMan = ALLOC( Vmx_Manager_t, 1 );
    memset( pMan, 0, sizeof(Vmx_Manager_t) );
    pMan->nTableSize = Cudd_Prime(2000);
    pMan->pTable = ALLOC( Vmx_VarMap_t *, pMan->nTableSize );
    memset( pMan->pTable, 0, sizeof(Vmx_VarMap_t *) * pMan->nTableSize );
    pMan->nRefs = 1;
    // characteristic and value-encoding cubes
    pMan->nValuesMax = 5000;
    pMan->pbCodes  = ALLOC( DdNode *, pMan->nValuesMax + 1 );
    memset( pMan->pbCodes, 0, sizeof(int) * (pMan->nValuesMax + 1) );
    // the log-binary cube
    pMan->nBitsMax = 5000;
    pMan->pArray = ALLOC( int, pMan->nBitsMax );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Deallocated the manager of variable maps.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vmx_ManagerFree( Vmx_Manager_t * pMan )
{
    Vmx_VarMap_t * pMap, * pMap2;
    int i;

    // delete var maps
    for ( i = 0; i < pMan->nTableSize; i++ )
        for ( pMap = pMan->pTable[i], pMap2 = pMap? pMap->pNext: NULL; 
            pMap; pMap = pMap2, pMap2 = pMap? pMap->pNext: NULL )
                Vmx_VarMapFree( pMap );

    // deloc memory
    free( pMan->pbCodes );
    free( pMan->pArray );
    free( pMan->pTable );
    free( pMan );
}


/**Function*************************************************************

  Synopsis    [References the existent DD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vmx_Manager_t * Vmx_ManagerRef( Vmx_Manager_t * p )
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
void Vmx_ManagerDeref( Vmx_Manager_t * p )
{
    if ( p == NULL )
        return;
    p->nRefs--;
    if ( p->nRefs == 0 )
        Vmx_ManagerFree( p );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


