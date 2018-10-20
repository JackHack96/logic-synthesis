/**CFile****************************************************************

  FileName    [ntkiCollapse.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Collapsing of the network into two levels.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiCollapseNew.c,v 1.2 2003/12/07 04:49:11 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Network_t * Ntk_NetworkCollapseInternal( Ntk_Network_t * pNet, bool fVerbose );
static Ntk_Node_t * Ntk_NetworkCollapseDeriveNode( DdManager * dd, Ntk_Network_t * pNetNew, 
    Ntk_Node_t * pNodeGlo, Ntk_Node_t * pNodeCo, Vm_VarMap_t * pVm );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkCollapseNew( Ntk_Network_t * pNet, bool fVerbose )
{
    Ntk_Network_t * pNetCol;
    bool fDynEnable = 1;
    bool fLatchOnly = 0;

    // free the old global manager if necessary
    Ntk_NetworkSetDdGlo( pNet, NULL );
    // compute the global functions
    Ntk_NetworkGlobalComputeCo( pNet, fDynEnable, fLatchOnly, fVerbose );
    // create the collapsed network
    pNetCol = Ntk_NetworkCollapseInternal( pNet, fVerbose );
    // undo the global functions
    Ntk_NetworkGlobalDeref( pNet );
    return pNetCol;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkCollapseInternal( Ntk_Network_t * pNet, bool fVerbose )
{
    DdManager * dd;
    Vm_VarMap_t * pVm;
    Ntk_Network_t * pNetNew;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pNodeGlo; 
    Ntk_Node_t * pNode, * pNodeNew, * pNodeNewCo;
    Ntk_Latch_t * pLatch, * pLatchNew;
    int i;

    dd   = Ntk_NetworkReadDdGlo( pNet );
    pVm  = Vmx_VarMapReadVm( Ntk_NetworkReadVmxGlo( pNet ) );

    // allocate the empty collapsed network
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );
    // register the name 
    pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNet->pName );
    // register the network spec file name
    if ( pNet->pSpec )
        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, pNet->pSpec );

    // copy and add the CI nodes
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }

    // create the global node
    pNodeGlo = Ntk_NodeCreateFromNetwork( pNet, NULL );

    // adjust the global node to point to the CIs of the new network
    Ntk_NodeForEachFanin( pNodeGlo, pPin, pNode )
    {
        pPin->pNode = Ntk_NetworkFindNodeByName( pNetNew, pNode->pName );
        assert( pPin->pNode );
    }

    // create and add the internal and CO nodes
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // create the internal node
        pNodeNew = Ntk_NetworkCollapseDeriveNode( dd, pNetNew, pNodeGlo, pNode, pVm );
        // add this node to the network
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // add the CO node
//        Ntk_NetworkAddNodeCo( pNetNew, pNodeNew, 1 );
        // copy the CO node (it is important if latches are present - see below)
        pNodeNewCo = Ntk_NodeDup( pNetNew, pNode );
        // set the only fanin to be equal to the new internal node
        pNodeNewCo->lFanins.pHead->pNode = pNodeNew;
        // add the CO node
        Ntk_NetworkAddNode( pNetNew, pNodeNewCo, 1 );
        i++;
    }
    Ntk_NodeDelete( pNodeGlo );

    // copy and add the latches if present
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        // get the new latch
        pLatchNew = Ntk_LatchDup( pNetNew, pLatch );
        // set the correct inputs/outputs
        pLatchNew->pInput  = pLatch->pInput->pCopy;
        pLatchNew->pOutput = pLatch->pOutput->pCopy;
        // add the new latch to the network
        Ntk_NetworkAddLatch( pNetNew, pLatchNew );
    }

    // copy the EXDC network
    if ( pNet->pNetExdc )
        pNetNew->pNetExdc = Ntk_NetworkDup( pNet->pNetExdc, pNet->pMan );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNetNew), "Ntk_NetworkCollapseInternal(): Network check has failed.\n" );
    return pNetNew;
}

/**Function*************************************************************

  Synopsis    [Creates the internal nodes from the MDDs.]

  Description [The procedure takes the manager and the MDDs of for each value
  (the number of values is the same as pNodeCo->nValues). The global node
  is the node in terms of CIs of the network. The CO node (pNodeCo) is the one
  for which the global MDDs are given. The variable map contains the CI space
  used to derive the global MDDs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkCollapseDeriveNode( DdManager * dd, Ntk_Network_t * pNetNew,
    Ntk_Node_t * pNodeGlo, Ntk_Node_t * pNodeCo, Vm_VarMap_t * pVm )
{
    DdNode ** pbMdds;
    Mvr_Manager_t * pManMvr;
    Vmx_Manager_t * pManVmx;
    Mvr_Relation_t * pMvr;
    Ntk_Node_t * pNode;

    // get the managers
    pManMvr = Ntk_NetworkReadManMvr( pNodeGlo->pNet );
    pManVmx = Ntk_NetworkReadManVmx( pNodeGlo->pNet );

    // get the global MDDs
    pbMdds = Ntk_NodeReadFuncGlob( pNodeCo );

    // create the relation
    pMvr = Mvr_RelationCreateFromGlobal( dd, pManMvr, pManVmx, pVm, pbMdds, pNodeCo->nValues );

    // create the new node
    pNode = Ntk_NodeDup( pNetNew, pNodeGlo );
    pNode->Type = MV_NODE_INT;
    pNode->nValues = pNodeCo->nValues;
    // copy the CO node's name
//    pNode->pName = Ntk_NetworkRegisterNewName( pNodeGlo->pNet, pNodeCo->pName );

    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pNode, Mvr_RelationReadVm(pMvr) );
    Ntk_NodeWriteFuncMvr( pNode, pMvr );

    // order the fanins alphabetically
    Ntk_NodeOrderFanins( pNode );
    // make the node minimum base
    Ntk_NodeMakeMinimumBase( pNode );
    return pNode;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


