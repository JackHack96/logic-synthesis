/**CFile****************************************************************

  FileName    [wnUtils.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various windowing utilities.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: wnUtils.c,v 1.3 2003/11/18 18:55:03 alanmi Exp $]

***********************************************************************/

#include "wnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Counts the largest fanin size of the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wn_WindowLargestFaninCount( Wn_Window_t * pWnd )
{
    Ntk_Node_t * pNode;
    int nSuppMax = 0;
    Wn_WindowDfs( pWnd );
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
    {
        if ( nSuppMax < Ntk_NodeReadFaninNum(pNode) )
            nSuppMax = Ntk_NodeReadFaninNum(pNode);
    }
    return nSuppMax;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the window has at least one ND node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Wn_WindowIsND( Wn_Window_t * pWnd )
{
    Mvr_Relation_t * pMvr;
    Ntk_Node_t * pNode;

    // if the window is composed of binary nodes, no need to look for special
    // IF THERE ARE NO INCOMPLETELY SPECIFIED NODES!!!

    // collect the nodes into the special list
    Wn_WindowDfs( pWnd );
    // assume that there is no need to collect the special nodes
    Ntk_NetworkForEachNodeSpecial( pWnd->pNet, pNode )
    {
        if ( pNode->nValues == 2 ) // ???
            continue;
        if ( pNode->Type != MV_NODE_INT )
            continue;
        pMvr = Ntk_NodeGetFuncMvr( pNode );
        if ( Mvr_RelationIsND( pMvr ) ) 
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the extended variable map for the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wn_WindowCreateVarMaps( Wn_Window_t * pWnd )
{
    Vm_Manager_t * pManVm;
    int * pValues;
    int nNodesMax;
    int i;

    // get the pointer to the Vm manager
    pManVm = Fnc_ManagerReadManVm(pWnd->pNet->pMan);

    // get the max number of leaves/roots/specials
    nNodesMax = -1;
    if ( nNodesMax < pWnd->nLeaves )
        nNodesMax = pWnd->nLeaves;
    if ( nNodesMax < pWnd->nRoots )
        nNodesMax = pWnd->nRoots;
    if ( nNodesMax < pWnd->nSpecials )
        nNodesMax = pWnd->nSpecials;
    // allocate temporary storage for var values
    pValues = ALLOC( int, nNodesMax );

    // create the map for the leaves
    for ( i = 0; i < pWnd->nLeaves; i++ )
        pValues[i] = pWnd->ppLeaves[i]->nValues;
    pWnd->pVmL = Vm_VarMapLookup( pManVm, pWnd->nLeaves, 0, pValues );  

    // create the map for the roots
    for ( i = 0; i < pWnd->nRoots; i++ )
        pValues[i] = pWnd->ppRoots[i]->nValues;
    pWnd->pVmR = Vm_VarMapLookup( pManVm, pWnd->nRoots, 0, pValues );  

    // create the map for the specials
    for ( i = 0; i < pWnd->nSpecials; i++ )
        pValues[i] = pWnd->ppSpecials[i]->nValues;
    pWnd->pVmS = Vm_VarMapLookup( pManVm, pWnd->nSpecials, 0, pValues );  

    FREE( pValues );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


