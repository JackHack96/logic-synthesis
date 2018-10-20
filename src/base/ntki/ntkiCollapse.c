/**CFile****************************************************************

  FileName    [ntkiCollapse.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Collapsing the network using global MDDs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiCollapse.c,v 1.7 2003/12/07 04:49:11 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Node_t * Ntk_NetworkCollapseDeriveNode( DdManager * dd, DdNode ** pbMdds, 
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
Ntk_Network_t * Ntk_NetworkCollapse( Ntk_Network_t * pNet, bool fVerbose )
{
    DdManager * dd;
    DdNode *** ppMdds;
    DdNode *** ppMddsExdc;
    char ** psCiNames;
    Vm_VarMap_t * pVm;
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNode, * pNodeGlo, * pNodeNew, * pNodeNewCo;
    Ntk_Latch_t * pLatch, * pLatchNew;
    int * pValuesOut, nOuts, i;

    if ( pNet == NULL )
        return NULL;

    // start the manager, in which the global MDDs will be stored
    dd = Cudd_Init( 0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    // get the number of outputs 
    nOuts = Ntk_NetworkReadCoNum( pNet );
    // get the output values
    pValuesOut = Ntk_NetworkGlobalGetOutputValues( pNet );

    // derive the global MDDs and the ext var map
    Ntk_NetworkGlobalMdd( dd, pNet, &pVm, &psCiNames, &ppMdds, &ppMddsExdc );
    if ( ppMddsExdc )
    {   // global MDDs of the EXDC network are binary functions
        // convert the EXDC MDDs into the standard MV format
        ppMddsExdc = Ntk_NetworkGlobalMddConvertExdc( dd, pValuesOut, nOuts, ppMddsExdc );
        // minimize the MDD using the EXDC as a don't-care
        Ntk_NetworkGlobalMddMinimize( dd, pValuesOut, nOuts, ppMdds, ppMddsExdc );
        Ntk_NetworkGlobalMddDeref( dd, pValuesOut, nOuts, ppMddsExdc ); 
    }
printf( "The number of BDD nodes = %d.\n", Cudd_ReadNodeCount(dd) );

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

    // create the global internal node with the given ordering of CIs
    pNodeGlo = Ntk_NodeCreateFromNetwork( pNetNew, psCiNames );
    FREE( psCiNames );

    // create and add the internal and CO nodes
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // create the internal node
        pNodeNew = Ntk_NetworkCollapseDeriveNode( dd, ppMdds[i], pNodeGlo, pNode, pVm );
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

    // delete the global node
    Ntk_NodeDelete( pNodeGlo );

    // delete the global MDDs
    Ntk_NetworkGlobalMddDeref( dd, pValuesOut, nOuts, ppMdds ); 
    FREE( pValuesOut );
    // remove the temporary manager
//  Cudd_PrintInfo( dd, stdout );
//    Extra_StopManager( dd );
	Cudd_Quit( dd );

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
    assert( Ntk_NetworkCheck( pNetNew ) );
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
Ntk_Node_t * Ntk_NetworkCollapseDeriveNode( DdManager * dd, DdNode ** pbMdds, 
    Ntk_Node_t * pNodeGlo, Ntk_Node_t * pNodeCo, Vm_VarMap_t * pVm )
{
    Mvr_Manager_t * pManMvr;
    Vmx_Manager_t * pManVmx;
    Mvr_Relation_t * pMvr;
    Ntk_Node_t * pNode;

    // get the managers
    pManMvr = Ntk_NetworkReadManMvr( pNodeGlo->pNet );
    pManVmx = Ntk_NetworkReadManVmx( pNodeGlo->pNet );

    // create the relation
    pMvr = Mvr_RelationCreateFromGlobal( dd, pManMvr, pManVmx, pVm, pbMdds, pNodeCo->nValues );

    // create the new node
    pNode = Ntk_NodeDup( pNodeGlo->pNet, pNodeGlo );
    // set the number of values
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


