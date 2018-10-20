/**CFile****************************************************************

  FileName    [fmbApiFunc.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [APIs of the flexibility manager (functions).]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbApiFunc.c,v 1.6 2004/05/12 04:30:11 alanmi Exp $]

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
DdNode * Fmb_ManagerComputeGlobalDcNode( Fmb_Manager_t * p, Ntk_Node_t * pPivot )
{
//    Wn_Window_t * pWndCore;
    Ntk_Node_t * pNode;
    DdNode * bFlex;
    DdManager * dd = p->ddGlo;
    int nInputs, nSuppMax;
    int nLevelTfi, nLevelTfo;
    int clk = clock();

    assert( pPivot->Type == MV_NODE_INT );

    // get the window
clk = clock();
    if ( p->pPars->nLevelTfi == -1 || p->pPars->nLevelTfo == -1 )
        p->pWnd  = Wn_WindowCreateFromNetwork( p->pNet );
    else        
    {
        nLevelTfi = p->pPars->nLevelTfi;
        nLevelTfo = p->pPars->nLevelTfo;
        p->pWnd = Wn_WindowDeriveForNode( pPivot, nLevelTfi, nLevelTfo );
//printf( "Starting %d x %d.\n", Wn_WindowReadLeafNum(p->pWnd), Wn_WindowReadRootNum(p->pWnd) );
        // rescale the window down until it fits into these limites
        while ( Wn_WindowReadLeafNum(p->pWnd) > 30 || Wn_WindowReadRootNum(p->pWnd) > 15 )
        {
            Wn_WindowDelete( p->pWnd );
            // get the new parameters
            nLevelTfi = (nLevelTfi > 0)? nLevelTfi - 1 : nLevelTfi;
            nLevelTfo = (nLevelTfo > 0)? nLevelTfo - 1 : nLevelTfo;
            // get the new window
            p->pWnd = Wn_WindowDeriveForNode( pPivot, nLevelTfi, nLevelTfo );
            if ( nLevelTfi == 0 && nLevelTfo == 0 )
                break;
        }
    }
    Wn_WindowCreateVarMaps( p->pWnd );
//printf( "Window = %d x %d.\n", Wn_WindowReadLeafNum(p->pWnd),  Wn_WindowReadRootNum(p->pWnd) );
p->timeWnd += clock() - clk;

    p->pVmS  = NULL;
    p->pVmL  = NULL;
    p->pVmG  = Wn_WindowReadVmL( p->pWnd );

    p->pVmxS = NULL;
    p->pVmxL = NULL;
    p->pVmxG = Vmx_VarMapLookup( Ntk_NetworkReadManVmx(pPivot->pNet), p->pVmG, -1, NULL );

    // setup the manager
    nInputs = Vm_VarMapReadVarsInNum( p->pVmG );
    nSuppMax = Wn_WindowLargestFaninCount( p->pWnd );
    Fmb_ManagerResize( p, nInputs, nSuppMax, 1 );

    // compute the global functions
    pNode = Fmb_UtilsComputeGlobal( p );
    Ntk_NodeDelete( pNode );

    // set the global inputs (GloZ) for the given node
    Ntk_NodeWriteFuncBinGloZ( pPivot, p->pbVars0[nInputs] );

    // collect the nodes in the TFO of the given node
//    Ntk_NetworkDfsFromNode( p->pNet, pPivot );
//    assert( p->pNet->pOrder == pPivot );
    //Ntk_NetworkPrintSpecial( pPivot->pNet );
    Wn_WindowDfs( p->pWnd );

    // compute GloZ BDDs for these nodes
p->timeTemp = clock();
    Ntk_NetworkForEachNodeSpecial( p->pNet, pNode )
    {
        if ( pNode == pPivot )
            continue;
        if ( Fmb_UtilsHasZFanin( pNode ) )
            Ntk_NodeGlobalComputeZBinary( pNode );
    }
p->timeGloZ += clock() - p->timeTemp;

    // derive the flexibility
    // it is important that this call is before GloZ is derefed
p->timeTemp = clock();
    bFlex = Fmb_FlexComputeGlobal( p );
p->timeFlex += clock() - p->timeTemp;
    if ( bFlex == NULL ) // the flexibility computation has timed out
        bFlex = b0; 
    Cudd_Ref( bFlex );

    // deref GloZ BDDs
    Ntk_NetworkForEachNodeSpecial( p->pNet, pNode )
        Ntk_NodeDerefFuncBinGloZ( pNode );

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
Mvr_Relation_t * Fmb_ManagerComputeLocalDcNode( Fmb_Manager_t * p, Ntk_Node_t * pNode, Ntk_Node_t * ppFanins[], int nFanins )
{
    Mvr_Relation_t * pMvrLoc;
//    Mvr_Relation_t * pMvrLoc2;
    DdNode * bGlobal;
    int clkTotal;
clkTotal = clock();

    bGlobal = Fmb_ManagerComputeGlobalDcNode( p, pNode );  Cudd_Ref( bGlobal );

    // get the local variable map
    p->pVmL = Vm_VarMapCreateBinary( Ntk_NodeReadManVm(pNode), nFanins, 0 );
    p->pVmxL = Vmx_VarMapLookup( Ntk_NodeReadManVmx(pNode), p->pVmL, -1, NULL );
p->timeTemp = clock();
    // image the global flexibility
    pMvrLoc  = Fmb_FlexComputeLocal( p, pNode, bGlobal, ppFanins, nFanins );  
p->timeImage += clock() - p->timeTemp;
    Cudd_RecursiveDeref( p->ddGlo, bGlobal );

//    if ( p->pPars->fUseSat )
//    Fmb_ManagerComputeLocalDcNodeSat( p );

    Fmb_ManagerCleanCurrentWindow( p );
//    Mvr_RelationPrintKmap( stdout, pMvrLoc, NULL );

/*
    if ( pFmbs == NULL )
    {
        Pars.fUseExdc   = p->pPars->fUseExdc;
        Pars.fDynEnable = p->pPars->fDynEnable;
        Pars.fVerbose   = p->pPars->fVerbose;
        Pars.nLevelTfi  = p->pPars->nLevelTfi;
        Pars.nLevelTfo  = p->pPars->nLevelTfo;
        Pars.fUseSat    = p->pPars->fUseSat;
        pFmbs = Fmbs_ManagerStart( p->pNet, &Pars );
    }
    else
        Fmbs_ManagerSetNetwork( pFmbs, p->pNet );
*/

//    pMvrLoc2 = Fmbs_ManagerComputeLocalDcNode( pFmbs, pNode, ppFanins, nFanins );

//    if ( Mvr_RelationReadRel(pMvrLoc) != Mvr_RelationReadRel(pMvrLoc2) )
//    {
//        printf( "Verification failed!\n" );
//        Mvr_RelationPrintKmap( stdout, pMvrLoc, NULL );
//        Mvr_RelationPrintKmap( stdout, pMvrLoc2, NULL );
//    }

//    Mvr_RelationFree( pMvrLoc2 );
//    Mvr_RelationPrintKmap( stdout, pMvrLoc2, NULL );


    return pMvrLoc;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


