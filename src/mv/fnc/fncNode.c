/**CFile****************************************************************

  FileName    [fncNode.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Manipulation of the node functionality.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fncNode.c,v 1.14 2003/11/18 18:55:04 alanmi Exp $]

***********************************************************************/

#include "fncInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reading the contents of the node functionality fields.]

  Description [These functions do not analize the contents, just read it as it is.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t *     Fnc_FunctionReadVm ( Fnc_Function_t * pF ) { return pF->pVm;  }
Cvr_Cover_t *     Fnc_FunctionReadCvr( Fnc_Function_t * pF ) { return pF->pCvr; }
Mvr_Relation_t *  Fnc_FunctionReadMvr( Fnc_Function_t * pF ) { return pF->pMvr; }
DdNode **         Fnc_FunctionReadGlo( Fnc_Function_t * pF ) { return pF->pGlo; }
        
/**Function*************************************************************

  Synopsis    [Returns the variable map of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Fnc_FunctionGetVm( Fnc_Function_t * pF )
{
    assert( pF->pVm );
    return pF->pVm;
}

/**Function*************************************************************

  Synopsis    [Returns MV SOP of the node.]

  Description [Looks at the node's Cvr. If it exits, returns it.
  If it does not exist, creates a new one and returns it.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cvr_Cover_t * Fnc_FunctionGetCvr( Fnc_Manager_t * pMan, Fnc_Function_t * pF )
{
    if ( !pF->pCvr )
    {
        assert( pF->pMvr );
        pF->pCvr = Fnc_FunctionDeriveCvrFromMvr( pMan->pManMvc, pF->pMvr, 1 );
    }
    return pF->pCvr;
}


/**Function*************************************************************

  Synopsis    [Returns MV relation of the node.]

  Description [Looks at the node's Mvr. If it exits, returns it.
  If it does not exist, creates a new one and returns it. If after
  construction, Mvr was found to be not well-defined, the non-defined
  minterms are assumed to be don't-cares. In this case, the flag
  is raised (pMvr->fMark) and the current Cvr is freed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fnc_FunctionGetMvr( Fnc_Manager_t * pMan, Fnc_Function_t * pF )
{
    if ( !pF->pMvr )
    {
        assert( pF->pCvr );
        pF->pMvr = Fnc_FunctionDeriveMvrFromCvr( pMan->pManMvr, pMan->pManVmx, pF->pCvr );
        if ( pF->pMvr->fMark ) 
        { // don't-care have been added -> Cvr became invalid
            Cvr_CoverFree( pF->pCvr );
            pF->pCvr = NULL;
        }
    }
    return pF->pMvr;
}

/**Function*************************************************************

  Synopsis    [Returns MV relation of the node.]

  Description [Looks at the node's Mvr. If it exits, returns it.
  If it does not exist, creates a new one and returns it.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode ** Fnc_FunctionGetGlo( Fnc_Manager_t * pMan, Fnc_Function_t * pF )
{
    // here we will add the call to compute the global relation of the node
    assert( pF->pGlo );
    return pF->pGlo;
}




/**Function*************************************************************

  Synopsis    [Writing the contents of the node functionality fields.]

  Description [These functions free the pointer, if it is different from
  the pointer that is being written and then set the new pointer.
  Other pointers are not updated.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_FunctionWriteVm ( Fnc_Function_t * pF, Vm_VarMap_t * pVm ) 
{ 
    if ( pF->pVm != pVm )
    {
        Fnc_FunctionFreeVm( pF );
        pF->pVm = pVm;   
    }
}
void Fnc_FunctionWriteCvr ( Fnc_Function_t * pF, Cvr_Cover_t * pCvr ) 
{ 
    if ( pF->pCvr != pCvr )
    {
        Fnc_FunctionFreeCvr( pF );
        pF->pCvr = pCvr;   
    }
}
void Fnc_FunctionWriteMvr( Fnc_Function_t * pF, Mvr_Relation_t * pMvr ) 
{ 
    if ( pF->pMvr != pMvr )
    {
        Fnc_FunctionFreeMvr( pF );
        pF->pMvr = pMvr;   
    }
}
void Fnc_FunctionWriteGlo( Fnc_Function_t * pF, DdNode ** pGlo ) 
{ 
//  if ( pF->pGlo != pGlo )
//  {
//      Fnc_FunctionFreeGlo( pF );
//      pF->pGlo = pGlo;   
//  }
    pF->pGlo = pGlo;
}


/**Function*************************************************************

  Synopsis    [Updating the node's functionality.]

  Description [These functions do the same as the Write() series but
  additionally they invalidate other representations.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_FunctionSetVm( Fnc_Function_t * pF, Vm_VarMap_t * pVm ) 
{ 
    if ( pF->pVm != pVm )
    {
        Fnc_FunctionFreeVm( pF );
        pF->pVm = pVm;   
    }
    Fnc_FunctionFreeCvr( pF );
    Fnc_FunctionFreeMvr( pF );
}
void Fnc_FunctionSetCvr( Fnc_Function_t * pF, Cvr_Cover_t * pCvr ) 
{ 
    if ( pF->pCvr != pCvr )
    {
        Fnc_FunctionFreeCvr( pF );
        pF->pCvr = pCvr;   
    }
    Fnc_FunctionFreeMvr( pF );
}
void Fnc_FunctionSetMvr( Fnc_Function_t * pF, Mvr_Relation_t * pMvr ) 
{ 
    if ( pF->pMvr != pMvr )
    {
        Fnc_FunctionFreeMvr( pF );
        pF->pMvr = pMvr;   
    }
    Fnc_FunctionFreeCvr( pF );
}
void Fnc_FunctionSetGlo( Fnc_Function_t * pF, DdNode ** pGlo ) 
{ 
    if ( pF->pGlo != pGlo )
    {
        Fnc_FunctionFreeGlo( pF );
        pF->pGlo = pGlo;   
    }
    Fnc_FunctionFreeCvr( pF );
    Fnc_FunctionFreeMvr( pF );
}

/**Function*************************************************************

  Synopsis    [Invalidate objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fnc_FunctionFreeVm( Fnc_Function_t * pF )
{
    if ( pF->pVm )
    {
        pF->pVm = NULL;
    }
}
void Fnc_FunctionFreeCvr( Fnc_Function_t * pF )
{
    if ( pF->pCvr )
    {
        Cvr_CoverFree( pF->pCvr );
        pF->pCvr = NULL;
    }
}
void Fnc_FunctionFreeMvr( Fnc_Function_t * pF )
{
    if ( pF->pMvr )
    {
        Mvr_RelationFree( pF->pMvr );
        pF->pMvr = NULL;
    }
}
void Fnc_FunctionFreeGlo( Fnc_Function_t * pF )
{
    if ( pF->pGlo )
    {
        assert( 0 );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


