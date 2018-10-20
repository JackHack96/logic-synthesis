/**CFile****************************************************************

  FileName    [mvrApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [APIs of the package to manipulate MV relations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvrApi.c,v 1.14 2003/11/18 18:55:05 alanmi Exp $]

***********************************************************************/

#include "mvrInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Basic relation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Manager_t *  Mvr_RelationReadMan( Mvr_Relation_t * pRel )      { return pRel->pMan; }
DdManager *      Mvr_RelationReadDd( Mvr_Relation_t * pRel )       { return pRel->pMan->pDdLoc; }
DdNode *         Mvr_RelationReadRel( Mvr_Relation_t * pRel )      { return pRel->bRel; }
Vmx_VarMap_t *   Mvr_RelationReadVmx( Mvr_Relation_t * pRel )      { return pRel->pVmx; }
Vm_VarMap_t *    Mvr_RelationReadVm( Mvr_Relation_t * pRel )       { return pRel->pVmx->pVm; }
int              Mvr_RelationReadNodes( Mvr_Relation_t * pRel )    { return pRel? pRel->nBddNodes: BDD_NODES_UNKNOWN; }
bool             Mvr_RelationReadMark( Mvr_Relation_t * pRel )     { return pRel->fMark; }
void             Mvr_RelationCleanMark( Mvr_Relation_t * pRel )    { pRel->fMark = 0; }

/**Function*************************************************************

  Synopsis    [Basic manager APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager *       Mvr_ManagerReadDdLoc( Mvr_Manager_t * pMan )     { return pMan->pDdLoc; }
reo_man *         Mvr_ManagerReadReo( Mvr_Manager_t * pMan )       { return pMan->pReo; }
Extra_PermMan_t * Mvr_ManagerReadPerm( Mvr_Manager_t * pMan )      { return pMan->pPerm; }

/**Function*************************************************************

  Synopsis    [Other simple APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvr_RelationGetNodes( Mvr_Relation_t * pMvr )
{ 
    if ( pMvr->nBddNodes != BDD_NODES_UNKNOWN )
        return pMvr->nBddNodes;
    return (pMvr->nBddNodes = Cudd_DagSize( pMvr->bRel )); 
}

/**Function*************************************************************

  Synopsis    [Basic manager APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvr_RelationWriteRel( Mvr_Relation_t * pRel, DdNode * bRel )
{ 
    pRel->bRel = bRel;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


