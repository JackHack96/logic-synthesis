/**CFile****************************************************************

  FileName    [fnc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fncApi.c,v 1.6 2003/11/18 18:55:03 alanmi Exp $]

***********************************************************************/

#include "fncInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocating the function structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fnc_Function_t * Fnc_FunctionAlloc()
{
    Fnc_Function_t * p;
    p = ALLOC( Fnc_Function_t, 1 );
    memset( p, 0, sizeof(Fnc_Function_t) );
    return p;
}

/**Function*************************************************************

  Synopsis    [Copying the function structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_FunctionCopy( Fnc_Function_t * pNDest, Fnc_Function_t * pNSource )
{
    *pNDest = *pNSource;
}

/**Function*************************************************************

  Synopsis    [Duplicates the functionality.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_FunctionDup( Fnc_Manager_t * pManOld, Fnc_Manager_t * pManNew, 
    Fnc_Function_t * pFOld, Fnc_Function_t * pFNew )
{
    Vm_VarMap_t * pVm;
    Cvr_Cover_t * pCvr;
    Mvr_Relation_t * pMvr;

    // consider two cases:
    // the functionality manager is the same
    // the functionality manager is different
    if ( pManOld == pManNew )
    {
        // duplicate the variable map
        pVm = Fnc_FunctionReadVm( pFOld );
        Fnc_FunctionWriteVm( pFNew, pVm );

        // duplicate the cover
        pCvr = Fnc_FunctionReadCvr( pFOld );
        if ( pCvr )
            pCvr = Cvr_CoverDup( pCvr );
        Fnc_FunctionWriteCvr( pFNew, pCvr );

        // duplicate the relation
        pMvr = Fnc_FunctionReadMvr( pFOld );
        if ( pMvr )
            pMvr = Mvr_RelationDup( pMvr );
        Fnc_FunctionWriteMvr( pFNew, pMvr );
    }
    else
    {
        // transfer these objects into the new manager
        assert( 0 );
    }
}

/**Function*************************************************************

  Synopsis    [Deletes all functionality.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_FunctionClean( Fnc_Function_t * pF )
{
    if ( pF->pCvr )
        Cvr_CoverFree( pF->pCvr );
    if ( pF->pMvr )
        Mvr_RelationFree( pF->pMvr );
    if ( pF->pGlo )
    {
        assert( 0 );
    }
//    if ( pF->pEnv )
//        Dcmn_NodeFreeEnv( pF->pEnv );
    memset( pF, 0, sizeof(Fnc_Function_t) );
}

/**Function*************************************************************

  Synopsis    [Deletes all functionality.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_FunctionDelete( Fnc_Function_t * pF )
{
    Fnc_FunctionClean( pF );
    FREE( pF );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


