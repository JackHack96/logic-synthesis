/**CFile****************************************************************

  FileName    [ntkMinBase.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkMinBase.c,v 1.26 2003/09/01 04:55:58 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Removes the redundant fanins from the fanin list.]

  Description [Analizes the local relation for the presence of redundant 
  fanins. If there is no redundant fanins, makes no change to the relation.
  If redundant fanins are found, remaps the relation, and removes the 
  redundant fanins from the fanin list. If the node is in the network,
  removes the redundant fanin's fanout connections. Returns 1 if the 
  node has been changed, otherwise returns 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Ntk_NodeMakeMinimumBase( Ntk_Node_t * pNode )
{
    Vm_VarMap_t * pVm, * pVmNew;
    Mvr_Relation_t * pMvr, * pMvrNew;
    Cvr_Cover_t    * pCvr, * pCvrNew;
    int * pSuppMv, nSuppMv;

    // get the pointers to the current objects of the node
    pVm  = Ntk_NodeGetFuncVm( pNode );  // the MV var map of the node
    pSuppMv = Vm_VarMapGetStorageSupport1( pVm );
    
    pMvr = Ntk_NodeReadFuncMvr( pNode ); // the local relation used for making min base
    
    // BDD-based method is given higher priority
    if ( pMvr ) {
        // get the MV support of the relation
        nSuppMv = Mvr_RelationSupport( pMvr, pSuppMv );
    }
    else {
        // minimum based with MV SOP
        pCvr = Ntk_NodeGetFuncCvr( pNode );
        nSuppMv = Cvr_CoverSupport( pCvr, pSuppMv );
    }
    
    // check for the situation when the node is already min base
    if ( nSuppMv == Vm_VarMapReadVarsNum( pVm ) )
        return 0;
    // some variables can be reduced
    
    // update the fanin space of the node
    Ntk_NodeReduceFanins( pNode, pSuppMv );
    
    // special case for constant node
    if ( nSuppMv == 0 ) {
        pMvr = Ntk_NodeGetFuncMvr( pNode );
    }
    
    if ( pMvr ) {
        // create the new relation
        pMvrNew = Mvr_RelationCreateMinimumBase( pMvr, pSuppMv );
        pVmNew  = Mvr_RelationReadVm( pMvrNew );
        Ntk_NodeSetFuncVm( pNode, pVmNew );
        Ntk_NodeWriteFuncMvr( pNode, pMvrNew );
    }
    else {
        
        pCvrNew = Cvr_CoverCreateMinimumBase( pCvr, pSuppMv );
        pVmNew  = Cvr_CoverReadVm( pCvrNew );
        Ntk_NodeSetFuncVm( pNode, pVmNew );
        Ntk_NodeWriteFuncCvr( pNode, pCvrNew );
    }
    return 1;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


