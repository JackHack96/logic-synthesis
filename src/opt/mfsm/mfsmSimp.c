/**CFile****************************************************************

  FileName    [mfsnSimp.c]

  PackageName [Simplification of networks using complete flexiblity.]

  Synopsis    [The kernel of network simplification package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: mfsmSimp.c,v 1.7 2005/06/02 03:34:11 alanmi Exp $]

***********************************************************************/

#include "mfsmInt.h"
#include "sh.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Ver_NetworkSweep( Ntk_Network_t * pNet, int fVerbose );

static int Mfsm_NetworkMfsNodeMinimize( Mfsm_Params_t * p, Ntk_Node_t * pNode, Mvr_Relation_t * pFlex );

static int timeSopMin;
static int timeResub;
static int timeCdc;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mfsb_NetworkMfs( Mfsm_Params_t * p, Ntk_Network_t * pNet, Ntk_Node_t * ppNodesInit[], int nNodes )
{
    ProgressBar * pProgress = NULL;
    Fmb_Manager_t * pMan = NULL;
    Fmbs_Manager_t * pManS = NULL;
    Fmb_Params_t * pPars;
    Mvr_Relation_t * pFlex;
    Ntk_Node_t * pNode;
    Ntk_Node_t ** ppNodes;
    FILE * pOutput;
    int iNode;
    bool fChanges = 0;
    Ntk_Node_t ** ppFanins;
    int nFanins;
    int nAigDiff;
    int clk;

    timeSopMin = 0;
    timeResub = 0;
    timeCdc = 0;

    // get the MVSIS output stream
    pOutput = Ntk_NetworkReadMvsisOut( pNet );

    // perform the network sweep
    // do not sweep if we only minimize several nodes
    if ( ppNodesInit == NULL )
    {
        Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );
        // remove the equivalent nodes
//        if ( p->fSweep && !p->fPrint )
//            Ver_NetworkSweep( pNet, 0 );
    }
    
    // set the parameters
    pPars = ALLOC( Fmb_Params_t, 1 );
    memset( pPars, 0, sizeof(Fmb_Params_t) );
    pPars->fUseExdc    = p->fUseExdc;   // set to 1 if the EXDC is to be used
    pPars->fDynEnable  = p->fDynEnable; // enables dynmamic variable reordering
    pPars->fVerbose    = p->fVerbose;   // to enable the output of puzzling messages
    pPars->nLevelTfi   = p->nLevelTfi;  // the number of TFI logic levels
    pPars->nLevelTfo   = p->nLevelTfo;  // the number of TFO logic levels
    pPars->fUseSat     = p->fUseSat;      
    pPars->nNodesMax   = p->nNodesMax;  // the number of AND-gates

    // start the flexibility manager
    if ( pPars->fUseSat )
        pManS = Fmbs_ManagerStart( pNet, (Fmbs_Params_t *)pPars );
    else
        pMan = Fmb_ManagerStart( pNet, pPars );


    if ( ppNodesInit == NULL )
    {
        // order the internal nodes
        nNodes = Ntk_NetworkReadNodeIntNum(pNet);
        if ( p->fDir == MFSM_DIR_FROM_OUTPUTS )
            ppNodes = Ntk_NetworkOrderNodesByLevel( pNet, 0 ); // from outputs
        else if ( p->fDir == MFSM_DIR_FROM_INPUTS )
            ppNodes = Ntk_NetworkOrderNodesByLevel( pNet, 1 ); // from inputs
        else if ( p->fDir == MFSM_DIR_RANDOM )
            ppNodes = Ntk_NetworkCollectInternal( pNet );
        else { assert( 0 ); }
	    if ( !p->fVerbose && !p->fPrint )
            pProgress = Extra_ProgressBarStart( stdout, nNodes );
    }
    else
        ppNodes = ppNodesInit;

    // clockTotal = 0;

    // iterate through the nodes
    nAigDiff = 0;
    for ( iNode = 0; iNode < nNodes; iNode++ )
    {
        pNode = ppNodes[iNode];
        if ( Ntk_NodeReadType( pNode ) != MV_NODE_INT )
            continue;
        // skip constant and single-input nodes
        if ( Ntk_NodeReadFaninNum( pNode ) < 2 )
            continue;
        //. skip the nodes that do not fanout
        if ( !Ntk_NodeHasCoInTfo( pNode ) )
            continue;
        // skip the nodes that are too large
        if ( Ntk_NodeReadFaninNum( pNode ) > p->nFaninMax )
            continue;
        // the problem here is that we use set variables to propagate
        // the influence of the cut node on the COs
        // as a result, the default i-set should be computed each time
        // global MDDs are computed
        // for benchmarks like "alu4.blif" this is very slow

        // set the fanins
        nFanins = Ntk_NodeReadFaninNum( pNode );
        ppFanins = ALLOC( Ntk_Node_t *, nFanins );
        nFanins = Ntk_NodeReadFanins( pNode, ppFanins );

        // compute the flexibility
clk = clock();
        if ( pPars->fUseSat )
            pFlex = Fmbs_ManagerComputeLocalDcNode( pManS, pNode, ppFanins, nFanins );
        else
            pFlex = Fmb_ManagerComputeLocalDcNode( pMan, pNode, ppFanins, nFanins );
        FREE( ppFanins );
timeCdc += clock() - clk;

        if ( pFlex == NULL )
        {
            if ( p->fVerbose )
            fprintf( pOutput, "Mfs_NetworkMfs(): Flexibility computation timeout for node \"%s\".\n",
                Ntk_NodeGetNamePrintable(pNode) );
            continue;
        }
        // print the current relation and the flexibility
        if ( p->fPrint )
        {
            fprintf( pOutput, "The original relation at node \"%s\":\n", 
                Ntk_NodeGetNamePrintable(pNode) );
            Ntk_NodePrintFlex( stdout, pNode, Ntk_NodeGetFuncMvr(pNode) );

            fprintf( pOutput, "The flexibility at node \"%s\":\n", 
                Ntk_NodeGetNamePrintable(pNode) );
            Ntk_NodePrintFlex( stdout, pNode, pFlex );
            Mvr_RelationFree( pFlex );
            continue;
        }
        // check whether flexibility is well defined
        if ( !Mvr_RelationIsWellDefined( pFlex ) )
        {
//    Ntk_NodePrintMvr( stdout, pNode );
//    Ntk_NodePrintFlex( stdout, pNode, pFlex );
            fprintf( pOutput, "Mfsb_NetworkMfs(): Flexibility of node \"%s\" is not well defined.\n", 
                Ntk_NodeGetNamePrintable(pNode) );
            Mvr_RelationFree( pFlex );
            continue;
        }

/*
        // mininize the node
        {
            static Sh_Manager_t * pMan = NULL;
            DdNode * bOffSet, * bOnSet, * bCommon, * bTemp;
            int nNodesAig, nNodesFfs;
            DdManager * dd = Mvr_RelationReadDd(pFlex);

            bOffSet = Mvr_RelationGetIset( pFlex, 0 );  Cudd_Ref( bOffSet );
            bOnSet  = Mvr_RelationGetIset( pFlex, 1 );  Cudd_Ref( bOnSet );
            // get the common part
            bCommon = Cudd_bddAnd( dd, bOffSet, bOnSet );                  Cudd_Ref( bCommon );
            bOnSet = Cudd_bddAnd( dd, bTemp = bOnSet, Cudd_Not(bCommon) ); Cudd_Ref( bOnSet );
            Cudd_RecursiveDeref( dd, bTemp );
            bOffSet = Cudd_bddAnd( dd, bTemp = bOffSet, Cudd_Not(bCommon) ); Cudd_Ref( bOffSet );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCommon );

            if ( pMan == NULL )
            {
                pMan = Sh_ManagerCreate( dd->size );
                DecCache_SetUp( pMan, dd );
            }
            nNodesAig = BiDecomposeCount( pMan, dd, bOnSet, bOffSet );
            nNodesFfs = Cvr_CoverReadLitFacNum( Ntk_NodeGetFuncCvr(pNode) );
printf( "Node %s:   Aig = %3d.  Ffs = %3d.\n", Ntk_NodeGetNamePrintable(pNode), nNodesAig, nNodesFfs );

            Cudd_RecursiveDeref( dd, bOnSet );
            Cudd_RecursiveDeref( dd, bOffSet );
            if ( nNodesFfs > nNodesAig )
                nAigDiff += nNodesFfs - nNodesAig;
        }
*/

        if ( Mfsm_NetworkMfsNodeMinimize( p, pNode, pFlex ) )
        {
//            Fmb_ManagerUpdateGlobalFunctions( pMan, pNode );
            fChanges = 1;
        }
        Mvr_RelationFree( pFlex );

        // update the progress bar
        if ( pProgress && iNode % 50 == 0 )
            Extra_ProgressBarUpdate( pProgress, iNode, NULL );
    }
    if ( pProgress )
        Extra_ProgressBarStop( pProgress );
    FREE( ppNodes );
//printf( "Improvement in the number of AIG nodes = %d.\n", nAigDiff );
//PRT( "Bi-decomposition", clockTotal );

    if ( pPars->fUseSat )
    {
        Fmbs_ManagerAddToTimeSopMin( pManS, timeSopMin );
        Fmbs_ManagerAddToTimeResub( pManS, timeResub );
        Fmbs_ManagerAddToTimeCdc( pManS, timeCdc );
        if ( p->fVerbose )
            Fmbs_ManagerPrintTimeStats( pManS );
        Fmbs_ManagerStop( pManS );
    }
    else
    {
        Fmb_ManagerAddToTimeSopMin( pMan, timeSopMin );
        Fmb_ManagerAddToTimeResub( pMan, timeResub );
        Fmb_ManagerAddToTimeCdc( pMan, timeCdc );
        if ( p->fVerbose )
            Fmb_ManagerPrintTimeStats( pMan );
        Fmb_ManagerStop( pMan );
    }

    // perform the network sweep
    // do not sweep if we only minimize several nodes
    if ( ppNodesInit == NULL )
        Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );

    if ( !Ntk_NetworkCheck( pNet ) )
        printf( "Mfsb_NetworkMfsSpec(): Network check has failed.\n" );
    return fChanges;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mfsm_NetworkMfs( Mfsm_Params_t * p, Ntk_Network_t * pNet, Ntk_Node_t * ppNodesInit[], int nNodes )
{
    ProgressBar * pProgress = NULL;
    Fmm_Manager_t * pMan;
    Fmm_Params_t * pPars;
    Mvr_Relation_t * pFlex;
    Ntk_Node_t * pNode;
    Ntk_Node_t ** ppNodes;
    FILE * pOutput;
    int iNode;
    bool fChanges = 0;

    timeSopMin = 0;
    timeResub = 0;

    // get the MVSIS output stream
    pOutput = Ntk_NetworkReadMvsisOut( pNet );

    // perform the network sweep
    // do not sweep if we only minimize several nodes
    if ( ppNodesInit == NULL )
        Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );
    
    // set the parameters
    pPars = ALLOC( Fmm_Params_t, 1 );
    memset( pPars, 0, sizeof(Fmm_Params_t) );
    pPars->fUseExdc    = p->fUseExdc;   // set to 1 if the EXDC is to be used
    pPars->fDynEnable  = p->fDynEnable; // enables dynmamic variable reordering
    pPars->fVerbose    = p->fVerbose;   // to enable the output of puzzling messages
    pPars->nLevelTfi   = p->nLevelTfi;  // the number of TFI logic levels
    pPars->nLevelTfo   = p->nLevelTfo;  // the number of TFO logic levels
    pPars->fBinary     = p->fBinary;    // the network is pure binary (used to simplify computation)

    // start the flexibility manager
    pMan = Fmm_ManagerStart( pNet, pPars );

    // remove the equivalent nodes
    if ( p->fSweep && !p->fPrint )
    {
        Fmm_NetworkMergeEquivalentNets( pMan );
        Ntk_NetworkSweep( pNet, 0, 0, 0, 0 );
    }

    if ( ppNodesInit == NULL )
    {
        // order the internal nodes
        nNodes = Ntk_NetworkReadNodeIntNum(pNet);
        if ( p->fDir == MFSM_DIR_FROM_OUTPUTS )
            ppNodes = Ntk_NetworkOrderNodesByLevel( pNet, 0 ); // from outputs
        else if ( p->fDir == MFSM_DIR_FROM_INPUTS )
            ppNodes = Ntk_NetworkOrderNodesByLevel( pNet, 1 ); // from inputs
        else if ( p->fDir == MFSM_DIR_RANDOM )
            ppNodes = Ntk_NetworkCollectInternal( pNet );
        else { assert( 0 ); }
	    if ( !p->fVerbose && !p->fPrint )
            pProgress = Extra_ProgressBarStart( stdout, nNodes );
    }
    else
        ppNodes = ppNodesInit;


    // iterate through the nodes
    for ( iNode = 0; iNode < nNodes; iNode++ )
    {
        pNode = ppNodes[iNode];
        if ( Ntk_NodeReadType( pNode ) != MV_NODE_INT )
            continue;
        // skip constant and single-input nodes
        if ( Ntk_NodeReadFaninNum( pNode ) < 2 )
            continue;
        // skip the nodes that do not fanout
        if ( !Ntk_NodeHasCoInTfo( pNode ) )
            continue;
        // skip the nodes that are too large
        if ( Ntk_NodeReadFaninNum( pNode ) >= p->nFaninMax )
            continue;
        // the problem here is that we use set variables to propagate
        // the influence of the cut node on the COs
        // as a result, the default i-set should be computed each time
        // global MDDs are computed
        // for benchmarks like "alu4.blif" this is very slow

        // compute the flexibility
        pFlex = Fmm_ManagerComputeLocalDcNode( pMan, pNode );
        if ( pFlex == NULL )
        {
            if ( p->fVerbose )
            fprintf( pOutput, "Mfs_NetworkMfs(): Flexibility computation timeout for node \"%s\".\n",
                Ntk_NodeGetNamePrintable(pNode) );
            continue;
        }
        // print the current relation and the flexibility
        if ( p->fPrint )
        {
            fprintf( pOutput, "The original relation at node \"%s\":\n", 
                Ntk_NodeGetNamePrintable(pNode) );
            Ntk_NodePrintFlex( stdout, pNode, Ntk_NodeGetFuncMvr(pNode) );

            fprintf( pOutput, "The flexibility at node \"%s\":\n", 
                Ntk_NodeGetNamePrintable(pNode) );
            Ntk_NodePrintFlex( stdout, pNode, pFlex );
            Mvr_RelationFree( pFlex );
            continue;
        }
        // check whether flexibility is well defined
        if ( !Mvr_RelationIsWellDefined( pFlex ) )
        {
//    Ntk_NodePrintMvr( stdout, pNode );
//    Ntk_NodePrintFlex( stdout, pNode, pFlex );
            fprintf( pOutput, "Mfsm_NetworkMfs(): Flexibility of node \"%s\" is not well defined.\n", 
                Ntk_NodeGetNamePrintable(pNode) );
            Mvr_RelationFree( pFlex );
            continue;
        }
        // mininize the node
        if ( Mfsm_NetworkMfsNodeMinimize( p, pNode, pFlex ) )
        {
            Fmm_ManagerUpdateGlobalFunctions( pMan, pNode );
            fChanges = 1;
        }
        Mvr_RelationFree( pFlex );

        // update the progress bar
        if ( pProgress && iNode % 50 == 0 )
            Extra_ProgressBarUpdate( pProgress, iNode, NULL );
    }
    if ( pProgress )
        Extra_ProgressBarStop( pProgress );
    FREE( ppNodes );

Fmm_ManagerAddToTimeSopMin( pMan, timeSopMin );
Fmm_ManagerAddToTimeResub( pMan, timeResub );

    // print the timing stats
    if ( p->fVerbose )
        Fmm_ManagerPrintTimeStats( pMan );

    // stop the manager
    Fmm_ManagerStop( pMan );

    // perform the network sweep
    // do not sweep if we only minimize several nodes
    if ( ppNodesInit == NULL )
        Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );

    if ( !Ntk_NetworkCheck( pNet ) )
        printf( "Mfsm_NetworkMfsSpec(): Network check has failed.\n" );
    return fChanges;
}



/**Function*************************************************************

  Synopsis    [Minimizes the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mfsm_NetworkMfsNodeMinimize( Mfsm_Params_t * p, Ntk_Node_t * pNode, Mvr_Relation_t * pFlex )
{
    FILE * pOutput;
    Cvr_Cover_t * pCvr, * pCvrMin;
    double Ratio;
    int fChanges;
    int clk;

    // get the MVSIS output stream
    pOutput = Ntk_NodeReadMvsisOut( pNode );

    // read the current cover if present
    pCvr = Ntk_NodeReadFuncCvr( pNode );
    // minimize the flexibility with Espresso
clk = clock();
    pCvrMin = Fnc_FunctionMinimizeCvr( Ntk_NodeReadManMvc(pNode), pFlex, 0 ); // espresso
    // if failed, try minimizing using ISOP
    if ( pCvrMin == NULL )
        pCvrMin = Fnc_FunctionMinimizeCvr( Ntk_NodeReadManMvc(pNode), pFlex, 1 ); // isop
timeSopMin += clock() - clk;

    // print statistics about resubstitution
    if ( p->fVerbose )
    {
        pOutput = Ntk_NodeReadMvsisOut(pNode);
        fprintf( pOutput, "%5s : ", Ntk_NodeGetNamePrintable(pNode) );

        Ratio = Mvr_RelationFlexRatio( pFlex );
//        Ratio = 0.0;
        if ( pCvr )
            fprintf( pOutput, "CUR  cb= %3d lit= %3d  ", Cvr_CoverReadCubeNum(pCvr), Cvr_CoverReadLitFacNum(pCvr) );
        else
            fprintf( pOutput, "CUR  cb= %3s lit= %3s  ", "-", "-" );

        if ( pCvrMin )
            fprintf( pOutput, "MFS   dc   = %5.1f%%  cb= %3d lit= %3d   ", Ratio, Cvr_CoverReadCubeNum(pCvrMin), Cvr_CoverReadLitFacNum(pCvrMin) );
        else
            fprintf( pOutput, "MFS   dc   = %5.1f%%  cb= %3s lit= %3s   ", Ratio, "-", "-" );
//        fprintf( pOutput, "\n" );
    }

    // consider four situations depending on the present/absence of Cvr and CvrMin
    if ( pCvr && pCvrMin )
    { // both Cvrs are avaible - choose the best one

        // compare the minimized relation with the original one
        if ( Ntk_NodeTestAcceptCvr( pCvr, pCvrMin, p->AcceptType, 0 ) )
        { // accept the minimized node
            if ( p->fVerbose )
                fprintf( pOutput, "Accept\n" );
            // this will remove Mvr and old Cvr
            Ntk_NodeSetFuncCvr( pNode, pCvrMin );
//            Ntk_NodeMakeMinimumBase( pNode );
            fChanges = 1;
        }
        else
        { // reject the minimized node
            if ( p->fVerbose )
                fprintf( pOutput, "\n" );
            Cvr_CoverFree( pCvrMin );
            fChanges = 0;
        }
    }
    else if ( pCvr )
    { // only the old Cvr is available - leave it as it is
        if ( p->fVerbose )
            fprintf( pOutput, "\n" );
        fChanges = 0;
    }
    else if ( pCvrMin )
    { // only the new Cvr is available - set it
        if ( p->fVerbose )
            fprintf( pOutput, "Accept\n" );
        Ntk_NodeSetFuncCvr( pNode, pCvrMin );
//        Ntk_NodeMakeMinimumBase( pNode );
        fChanges = 1;
    }
    else
    { // none of them is available
        if ( p->fVerbose )
            fprintf( pOutput, "\n" );
        fprintf( pOutput, "Ntk_NetworkMfs(): Failed to minimize MV SOP of node \"%s\".\n", 
            Ntk_NodeGetNamePrintable(pNode) );
        fprintf( pOutput, "Currently the node does not have MV SOP, only MV relation.\n" );
        fprintf( pOutput, "The relation is the original one. Flexibility is not used.\n" );
        fChanges = 0;
    }

    // now, try resubstitution
clk = clock();
    if ( p->fResub )
    {
        if ( !Ntk_NetworkResubNode( pNode, pFlex, p->AcceptType, p->nCands, p->fVerbose ) )
        {
            // if the node is not resubstituted
            // and it has changed, mininize its support
            if ( fChanges )
                Ntk_NodeMakeMinimumBase( pNode );
        }
        else // the node is resubstituted; there is no need to minimize the support
            fChanges = 1;
    }
timeResub += clock() - clk;
    return fChanges;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

