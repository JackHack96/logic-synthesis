/**CFile****************************************************************

  FileName    [fmbApiRel.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [APIs of the flexibility manager (relations).]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbApiRel.c,v 1.2 2004/04/08 05:05:08 alanmi Exp $]

***********************************************************************/

#include "fmbInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Fmb_ManagerComputeGlobalDcNodeRel( Fmb_Manager_t * p, Ntk_Node_t * pPivots[], int nPivots )
{
//    Wn_Window_t * pWndCore;
    Ntk_Network_t * pNet = pPivots[0]->pNet;
    Ntk_Node_t * pNode;
    DdManager * dd = p->ddGlo;
    DdNode * bFlex;
    int nInputs, nSuppMax, i;

    for ( i = 0; i < nPivots; i++ )
    {
        assert( pPivots[i]->Type == MV_NODE_INT );
    }

    // get the window
    if ( p->pPars->nLevelTfi == -1 || p->pPars->nLevelTfo == -1 )
        p->pWnd  = Wn_WindowCreateFromNetwork( p->pNet );
    else   
    {
        Ntk_NetworkCreateSpecialFromArray( pNet, pPivots, nPivots );
//        pWndCore = Wn_WindowCreateFromNodeArray( pNet );
//        p->pWnd  = Wn_WindowDerive( pWndCore, p->pPars->nLevelTfi, p->pPars->nLevelTfo );
        p->pWnd = Wn_WindowDeriveForNode( pNet->pOrder, p->pPars->nLevelTfi, p->pPars->nLevelTfo );
        // will not work!!!
    }
    Wn_WindowCreateVarMaps( p->pWnd );

    p->pVmS  = Vm_VarMapCreateBinary( Ntk_NetworkReadManVm(pNet), nPivots, 0 );
    p->pVmL  = NULL;
    p->pVmG  = Vm_VarMapCreateInputOutput( Wn_WindowReadVmR(p->pWnd), p->pVmS );

    p->pVmxS = Vmx_VarMapLookup( Ntk_NetworkReadManVmx(pNet), p->pVmS, -1, NULL );
    p->pVmxL = NULL;
    p->pVmxG = Vmx_VarMapLookup( Ntk_NetworkReadManVmx(pNet), p->pVmG, -1, NULL );

    // get the window parameters
    nInputs  = Wn_WindowReadLeafNum( p->pWnd );
    nSuppMax = Wn_WindowLargestFaninCount( p->pWnd );

    // setup the manager
    Fmb_ManagerResize( p, nInputs, nSuppMax, nPivots );

    // compute the global functions
    pNode = Fmb_UtilsComputeGlobal( p );
    Ntk_NodeDelete( pNode );

    // set global inputs (GloZ) for the given node
    for ( i = 0; i < nPivots; i++ )
        Ntk_NodeWriteFuncBinGloZ( pPivots[i], p->pbVars0[nInputs + i] );

    // collect the nodes
    Wn_WindowDfs( p->pWnd );
    //Ntk_NetworkPrintSpecial( pPivot->pNet );

    // compute GloZ BDDs for these nodes
p->timeTemp = clock();
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( Fmb_UtilsHasZFanin( pNode ) )
            Ntk_NodeGlobalComputeZBinary( pNode );
    }

p->timeGloZ += clock() - p->timeTemp;

    // derive the flexibility
    // it is important that this call is before GloZ is derefed
p->timeTemp = clock();
    bFlex = Fmb_FlexComputeGlobalRel( p );
p->timeFlex += clock() - p->timeTemp;
    if ( bFlex == NULL ) // the flexibility computation has timed out
        bFlex = Fmb_UtilsRelationGlobalCompute( p, pPivots, nPivots, p->pVmxG ); 
    Cudd_Ref( bFlex );

    // deref GloZ BDDs
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        Ntk_NodeDerefFuncBinGloZ( pNode );
   
    // make sure that the global flexibility is well defined
    // one of the reasons why the domain may be computed incorrectly
    // is when the global BDDs are not updated properly arter a node has changed..
    assert( Fmb_UtilsRelationDomainCheck( dd, bFlex, p->pVmxG ) ); 

    Cudd_Deref( bFlex );
    return bFlex;
}

/**Function*************************************************************

  Synopsis    [Computes complete flexibility of the node.]

  Description [Returns the complete flexibility of a node expressed
  as an MV relation with the same relation parameters as the original
  MV relation of the node. (The returned relation may be not well-defined
  if the network was out of spec at the moment of calling this procedure.)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvr_Relation_t * Fmb_ManagerComputeLocalDcNodeRel( Fmb_Manager_t * p, 
    Ntk_Node_t * pPivots[], int nPivots, Ntk_Node_t * ppFanins[], int nFanins )
{
    Ntk_Network_t * pNet;
    Mvr_Relation_t * pMvrLoc;
    DdNode * bGlobal;
    int nOutputs;

    nOutputs = (nPivots == 1)? 0: nPivots;
    pNet = pPivots[0]->pNet;

    bGlobal = Fmb_ManagerComputeGlobalDcNodeRel( p, pPivots, nPivots );  Cudd_Ref( bGlobal );
    // set the fanins
//    nFanins = Ntk_NodeReadFaninNum( pNode );
//    ppFanins = ALLOC( Ntk_Node_t *, nFanins );
//    nFanins = Ntk_NodeReadFanins( pNode, ppFanins );

    // create local variable map
    p->pVmL = Vm_VarMapCreateBinary( Ntk_NetworkReadManVm(pNet), nFanins, 0 );
    p->pVmL = Vm_VarMapCreateInputOutput( p->pVmL, p->pVmS );
    p->pVmxL = Vmx_VarMapLookup( Ntk_NetworkReadManVmx(pNet), p->pVmL, -1, NULL );

p->timeTemp = clock();
    pMvrLoc  = Fmb_FlexComputeLocalRel( p, bGlobal, ppFanins, nFanins );  
p->timeImage += clock() - p->timeTemp;
    Cudd_RecursiveDeref( p->ddGlo, bGlobal );
//    FREE( ppFanins );
    Fmb_ManagerCleanCurrentWindow( p );
    return pMvrLoc;
}


////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


