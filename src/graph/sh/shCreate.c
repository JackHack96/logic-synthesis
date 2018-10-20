/**CFile****************************************************************

  FileName    [shCreate.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Creates the main data structures (manager, network).]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shCreate.c,v 1.8 2004/05/12 04:30:11 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Sh_ManagerReadTable( Sh_Manager_t * p, char * pFileName );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create the AI-graph manager.]

  Description [Creates the AI-graph manager. By default, the package starts
  with the garbage collection enabled, and with two-level hashing disabled. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Manager_t * Sh_ManagerCreate( int nVars, int nSize, bool fVerbose )
{
    Sh_Manager_t * p;

    p = ALLOC( Sh_Manager_t, 1 );
    memset( p, 0, sizeof(Sh_Manager_t) );
    // set the parameters
    p->fTwoLevelHash = 0;
    p->fEnableGC     = 1;
    p->nRefErrors    = 0;
    p->nTravIds      = 1;
    p->fVerbose      = fVerbose;
    // allocate the memory manager
#ifndef USE_SYSTEM_MEMORY_MANAGEMENT
    p->pMem = Extra_MmFixedStart( sizeof(Sh_Node_t), 5000, 1000 ); 
#endif
    // allocate the table 
    p->pTable = Sh_TableAlloc( nSize ); 
    // create the costant node
    p->pConst1 = ALLOC( Sh_Node_t, 1 );
    memset( p->pConst1, 0, sizeof(Sh_Node_t) );
    p->pConst1->index = SH_CONST_INDEX;
    // create elementary variables
    Sh_ManagerResize( p, nVars );
    // create the canonicity table
//    p->pCanTable = Sh_SignPrecompute();
    // read in the 4-subgraphs
//    Sh_ManagerReadTable( p, "table4.txt" );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_ManagerFree( Sh_Manager_t * p, bool fCheckZeroRefs )
{
    if ( !p )
        return;
    Sh_TableFree( p, fCheckZeroRefs );
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    {
        int i;
        for ( i = 0; i < p->nVars; i++ )
        {
            assert( p->pVars[i]->nRefs == 0 );
            FREE( p->pVars[i] );
        }
    }
#else
    Extra_MmFixedStop( p->pMem, 0 ); 
#endif
    FREE( p->pConst1 );
    FREE( p->pVars );
    FREE( p->pCanTable );
    if ( p->tSubgraphs ) st_free_table( p->tSubgraphs );
    Extra_MmFixedStop( p->pMemCut, 0 );
    FREE( p->pBuffer );
    FREE( p );
}
 
/**Function*************************************************************

  Synopsis    [Resizes the manager to allow for more variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_ManagerResize( Sh_Manager_t * pMan, int nVarsNew )
{
    int i;
    if ( pMan->nVars >= nVarsNew )
        return;
    // allocate, or reallocate storage for vars
    if ( pMan->nVars == 0 )
        pMan->pVars = ALLOC( Sh_Node_t *, nVarsNew );
    else
        pMan->pVars = REALLOC( Sh_Node_t *, pMan->pVars, nVarsNew );
    // create the vars
    for ( i = pMan->nVars; i < nVarsNew; i++ )
    {
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
        pMan->pVars[i] = ALLOC( Sh_Node_t, 1 );
#else
        pMan->pVars[i] = (Sh_Node_t *)Extra_MmFixedEntryFetch( pMan->pMem );
#endif
        memset( pMan->pVars[i], 0, sizeof(Sh_Node_t) );
        pMan->pVars[i]->index = i;
    }
    // set the new number of variables
	pMan->nVars = nVarsNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Sh_NetworkCreate( Sh_Manager_t * pMan, int nInputs, int nOutputs )
{
    Sh_Network_t * p;
    p = ALLOC( Sh_Network_t, 1 );
    memset( p, 0, sizeof(Sh_Network_t) );
    // allocate arrays
    p->pMan      = pMan;
    p->nInputs   = nInputs;
    p->nOutputs  = nOutputs;
    p->ppOutputs = ALLOC( Sh_Node_t *, p->nOutputs );
    memset( p->ppOutputs, 0, sizeof(Sh_Node_t *) * p->nOutputs );
    // resize the manager if needed
    Sh_ManagerResize( pMan, nInputs );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Sh_NetworkCreateFromNode( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    Sh_Network_t * p;
    p = Sh_NetworkCreate( pMan, pMan->nVars, 1 );
    p->ppOutputs[0] = gNode;  shRef( gNode );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkFree( Sh_Network_t * p )
{
    int i;
    if ( !p )
        return;
    // deref all sorts of referenced nodes
    for ( i = 0; i < p->nOutputs; i++ )
        Sh_RecursiveDeref( p->pMan, p->ppOutputs[i] );
    for ( i = 0; i < p->nOutputsCore; i++ )
        Sh_RecursiveDeref( p->pMan, p->ppOutputsCore[i] );
    for ( i = 0; i < p->nInputsCore; i++ )
        Sh_RecursiveDeref( p->pMan, p->ppInputsCore[i] );
    for ( i = 0; i < p->nSpecials; i++ )
        Sh_RecursiveDeref( p->pMan, p->ppSpecials[i] );
    FREE( p->ppOutputs );
    FREE( p->ppOutputsCore );
    FREE( p->ppInputsCore );
    FREE( p->ppSpecials );
    FREE( p->ppNodes );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Sh_NetworkDup( Sh_Network_t * p )
{
    Sh_Network_t * pNew;
    int i;
    if ( !p )
        return NULL;

    pNew = ALLOC( Sh_Network_t, 1 );
    memset( pNew, 0, sizeof(Sh_Network_t) );

    pNew->pMan          = p->pMan;
    pNew->nInputs       = p->nInputs;

    pNew->nOutputs      = p->nOutputs;
    pNew->ppOutputs     = ALLOC( Sh_Node_t *, pNew->nOutputs );
    memcpy( pNew->ppOutputs, p->ppOutputs, sizeof(Sh_Node_t *) * pNew->nOutputs );
    for ( i = 0; i < pNew->nOutputs; i++ )
        shRef( pNew->ppOutputs[i] );

    pNew->nInputsCore   = p->nInputsCore;
    pNew->ppInputsCore  = ALLOC( Sh_Node_t *, pNew->nInputsCore );
    memcpy( pNew->ppInputsCore, p->ppInputsCore, sizeof(Sh_Node_t *) * pNew->nInputsCore );
    for ( i = 0; i < pNew->nInputsCore; i++ )
        shRef( pNew->ppInputsCore[i] );

    pNew->nOutputsCore  = p->nOutputsCore;
    pNew->ppOutputsCore = ALLOC( Sh_Node_t *, pNew->nOutputsCore );
    memcpy( pNew->ppOutputsCore, p->ppOutputsCore, sizeof(Sh_Node_t *) * pNew->nOutputsCore );
    for ( i = 0; i < pNew->nOutputsCore; i++ )
        shRef( pNew->ppOutputsCore[i] );

    pNew->nSpecials     = p->nSpecials;
    pNew->ppSpecials    = ALLOC( Sh_Node_t *, pNew->nSpecials );
    memcpy( pNew->ppSpecials, p->ppSpecials, sizeof(Sh_Node_t *) * pNew->nSpecials );
    for ( i = 0; i < pNew->nSpecials; i++ )
        shRef( pNew->ppSpecials[i] );

    // MV variable maps
    pNew->pVmL   = p->pVmL;
    pNew->pVmR   = p->pVmR;
    pNew->pVmS   = p->pVmS;
    pNew->pVmLC  = p->pVmLC;
    pNew->pVmRC  = p->pVmRC;
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Sh_NetworkAppend( Sh_Network_t * p1, Sh_Network_t * p2 )
{
    Sh_Network_t * pNew;
    // duplicate the first network
    pNew = Sh_NetworkDup( p1 );
    // add the outputs of the second network
    pNew->ppOutputs = REALLOC( Sh_Node_t *, pNew->ppOutputs, p1->nOutputs + p2->nOutputs );
    memcpy( pNew->ppOutputs + p1->nOutputs, p2->ppOutputs, sizeof(Sh_Node_t *) * p2->nOutputs );
    // set the number of outputs
    pNew->nOutputs = p1->nOutputs + p2->nOutputs;
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Sh_NetworkMiter( Sh_Network_t * p1, Sh_Network_t * p2 )
{
    Sh_Network_t * pNew;
    Sh_Node_t * pExor, * pOrTotal, * pTemp;
    int i;

    // duplicate the first network
    pNew = Sh_NetworkDup( p1 );

    // create the Miter for the outputs
    pOrTotal = Sh_Not( p1->pMan->pConst1 );   shRef( pOrTotal );
    for ( i = 1; i < p1->nOutputs; i += 2 )
    {
        // get the EXOR of the outputs
        pExor = Sh_NodeExor( p1->pMan, p1->ppOutputs[i], p2->ppOutputs[i] ); shRef(pExor);
        // add this EXOR to the total OR
        pOrTotal = Sh_NodeOr( p1->pMan, pTemp = pOrTotal, pExor );           shRef(pOrTotal);
        Sh_RecursiveDeref( p1->pMan, pExor );
        Sh_RecursiveDeref( p1->pMan, pTemp );
    }

    // update the outputs
    pNew->nOutputs = 1;
    pNew->ppOutputs[0] = pOrTotal; // takes ref
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


