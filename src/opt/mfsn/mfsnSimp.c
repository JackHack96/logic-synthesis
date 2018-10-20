/**CFile****************************************************************

  FileName    [mfsnSimp.c]

  PackageName [Simplification of networks using complete flexiblity.]

  Synopsis    [The kernel of network simplification package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 13, 2003.]

  Revision    [$Id: mfsnSimp.c,v 1.2 2003/11/18 18:55:15 alanmi Exp $]

***********************************************************************/

#include "mfsnInt.h"

// Resource limits used in this file:
// (other resource limits are described in file "dcmFlex.c"

// If the BDD represeting the on-set, the dc-set, or the off-set becomes larger 
// than this, the node is skipped
#define BDD_LARGEST_SIZE         1000   // in BDD nodes

// If the number of cubes in the ZDD of the ISOP computed for the on-set, 
// the dc-set, or the off-set becomes larger than this, the node is skipped
#define ZDD_LARGEST_CUBE_COUNT   1000   // in miliseconds

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Mfsn_NetworkMfsNodeSpec( Dcmn_Man_t * p, Ntk_Node_t * pNode, int fUseProduct, int fUseNsSpec, DdNode * bRel );

static int s_nNodes;
static double s_TotalRatioBef;
static double s_TotalRatioAft;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mfsn_NetworkMfs( Mfsn_Params_t * p, Ntk_Network_t * pNet )
{
//    ProgressBar * pProgress;
    Dcmn_Man_t * pMan;                     // the DC manager
    Dcmn_Par_t * pPars;                    // the set of parameters
	DdManager * dd;                        // the pointer to the local BDD manager
    DdNode * bRel = NULL;
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    int nNodes, iNode;
    int fDfsFromOutputs;

    s_nNodes = 0;
    s_TotalRatioBef = 0.0;
    s_TotalRatioAft = 0.0;
    
    // set the parameters
    pPars = ALLOC( Dcmn_Par_t, 1 );
    memset( pPars, 0, sizeof(Dcmn_Par_t) );
    pPars->TypeSpec    = p->TypeSpec;   
    pPars->TypeFlex    = p->TypeFlex;   
    pPars->nBddSizeMax = p->nBddSizeMax;// determines the limit after which the global BDD construction aborts
    pPars->fSweep      = p->fSweep;     // enable/disable sweeping at the end
    pPars->fUseExdc    = p->fUseExdc;   // set to 1 if the EXDC is to be used
    pPars->fDynEnable  = p->fDynEnable; // enables dynmamic variable reordering
    pPars->fVerbose    = p->fVerbose;   // to enable the output of puzzling messages

    // perform the network sweep
    if ( p->fSweep )
        Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );

    // start the flexibility manager
    pMan = Dcmn_ManagerStart( pNet, pPars );
    dd   = Dcmn_ManagerReadDdGlo(pMan);

    // compute the ratio of minterms in the spec
    if ( p->fGlobal )
    {
        // get the relation
        //Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
        bRel = Dcmn_UtilsNetworkDeriveRelation( pMan, 0 );   Cudd_Ref( bRel );
//    PRB( dd, bRel );
        printf( "Average node ND = %.4f %%. Spec volume = %.4f %%\n", 
            Dcmn_UtilsNetworkGetAverageNdRatio(pMan), 
            Dcmn_UtilsNetworkGetRelationRatio( pMan, bRel ) );
        Cudd_RecursiveDeref( dd, bRel );
        // stop the manager
        Dcmn_ManagerStop( pMan );
        return 1;
    }
/*
    if ( p->TypeFlex == DCMN_TYPE_NS )
    {
        printf( "NS flexibility computation is not supported.\n" );
        Dcmn_ManagerStop( pMan );
        return 1;
    }
*/

    // precompute the NS spec for all nodes
    if ( p->TypeSpec == DCMN_TYPE_NS )
    {
        bRel = Dcmn_UtilsNetworkDeriveRelation( pMan, 0 );   Cudd_Ref( bRel );
    }


    // order the internal nodes
    if ( p->fDir == DCMN_DIR_FROM_OUTPUTS )
        fDfsFromOutputs =  0;
    else if ( p->fDir == DCMN_DIR_FROM_INPUTS )
        fDfsFromOutputs =  1;
    else if ( p->fDir == DCMN_DIR_RANDOM )
        fDfsFromOutputs = -1;
    if ( fDfsFromOutputs >= 0 )
        ppNodes = Ntk_NetworkOrderNodesByLevel( pNet, fDfsFromOutputs );
    else
        ppNodes = Ntk_NetworkCollectInternal( pNet );

    // start the progress var
//    if ( !p->fVerbose )
//        pProgress = Extra_ProgressBarStart( stdout, nNodes );
    nNodes  = Ntk_NetworkReadNodeIntNum( pNet );
    for ( iNode = 0; iNode < nNodes; iNode++ )
    {
        pNode = ppNodes[iNode];
        Mfsn_NetworkMfsNodeSpec( pMan, pNode, p->fUseProduct, p->TypeSpec == DCMN_TYPE_NS, bRel );
//       if ( iNode % 5 == 0 && !p->fVerbose )
//            Extra_ProgressBarUpdate( pProgress, iNode );
    }
    FREE( ppNodes );
//    if ( !p->fVerbose )
//        Extra_ProgressBarStop( pProgress );


    // precompute the NS spec for all nodes
    if ( p->TypeSpec == DCMN_TYPE_NS )
    {
        Cudd_RecursiveDeref( dd, bRel );
    }

    // stop the manager
    Dcmn_ManagerStop( pMan );
    // sweep the buffers/intervers
    if ( p->fSweep )
        Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );


    s_TotalRatioBef = s_TotalRatioBef / s_nNodes;
    s_TotalRatioAft = s_TotalRatioAft / s_nNodes;
    printf( "AVERAGE:                             Ratio before = %7.3f %%. Ratio after = %7.3f %%.\n", 
           s_TotalRatioBef, s_TotalRatioAft );

    return 1;
}

/**Function*************************************************************

  Synopsis    [Simplification for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mfsn_NetworkMfsOne( Mfsn_Params_t * p, Ntk_Node_t * pNode )
{
//    ProgressBar * pProgress;
    Dcmn_Man_t * pMan;                     // the DC manager
    Dcmn_Par_t * pPars;                    // the set of parameters
	DdManager * dd;                        // the pointer to the local BDD manager
    Ntk_Network_t * pNet = pNode->pNet;
    DdNode * bRel = NULL;

    if ( p->TypeFlex == DCMN_TYPE_NS )
    {
        printf( "NS flexibility computation is not supported.\n" );
        return 1;
    }
    // set the parameters
    pPars = ALLOC( Dcmn_Par_t, 1 );
    memset( pPars, 0, sizeof(Dcmn_Par_t) );
    pPars->TypeSpec    = p->TypeSpec;   
    pPars->TypeFlex    = p->TypeFlex;   
    pPars->nBddSizeMax = p->nBddSizeMax;// determines the limit after which the global BDD construction aborts
    pPars->fSweep      = p->fSweep;     // enable/disable sweeping at the end
    pPars->fUseExdc    = p->fUseExdc;   // set to 1 if the EXDC is to be used
    pPars->fDynEnable  = p->fDynEnable; // enables dynmamic variable reordering
    pPars->fVerbose    = p->fVerbose;   // to enable the output of puzzling messages

    // perform the network sweep
    if ( p->fSweep )
        Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );


    // start the flexibility manager
    pMan = Dcmn_ManagerStart( pNet, pPars );
    dd   = Dcmn_ManagerReadDdGlo(pMan);

    // precompute the NS spec for all nodes
    if ( p->TypeSpec == DCMN_TYPE_NS )
    {
        bRel = Dcmn_UtilsNetworkDeriveRelation( pMan, 0 );   Cudd_Ref( bRel );
    }

    Mfsn_NetworkMfsNodeSpec( pMan, pNode, p->fUseProduct, p->TypeSpec == DCMN_TYPE_NS, bRel );

    // precompute the NS spec for all nodes
    if ( p->TypeSpec == DCMN_TYPE_NS )
    {
        Cudd_RecursiveDeref( dd, bRel );
    }

    // stop the manager
    Dcmn_ManagerStop( pMan );

    // sweep the buffers/intervers
    if ( p->fSweep )
        Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );

    return 1;
}

/**Function*************************************************************

  Synopsis    [Simplifies the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfsn_NetworkMfsNodeSpec( Dcmn_Man_t * p, Ntk_Node_t * pNode, int fUseProduct, int fUseNsSpec, DdNode * bSpecNs )
{
    Mvr_Relation_t * pFlex;
    FILE * pOutput;
    double RatioBef, RatioAft;

    //. skip the nodes that do not fanout
    if ( !Ntk_NodeHasCoInTfo( pNode ) )
        return;
    // skip constant and single-input nodes
//    if ( Ntk_NodeReadFaninNum( pNode ) < 2 )
//        return;

    // get the MVSIS output stream
    pOutput = Ntk_NodeReadMvsisOut( pNode );

    // derive the flexibility
    if ( fUseNsSpec )
        pFlex = Dcmn_ManagerComputeLocalDcNodeUseNsSpec( p, pNode, bSpecNs );
    else if ( fUseProduct )
        pFlex = Dcmn_ManagerComputeLocalDcNodeUseProduct( p, pNode );
    else
        pFlex = Dcmn_ManagerComputeLocalDcNode( p, pNode );

    if ( pFlex == NULL )
    {
//        if ( p->fVerbose )
        fprintf( pOutput, "Mfs_NetworkMfs(): Flexibility computation timeout for node \"%s\".\n",
            Ntk_NodeGetNamePrintable(pNode) );
        return;
    }


// check whether flexibility is well defined
if ( !Mvr_RelationIsWellDefined( pFlex ) )
{
//Ntk_NodePrintMvr( stdout, pNode );
//Ntk_NodePrintFlex( stdout, pNode, pFlex );
    fprintf( pOutput, "Local  flexibility of node \"%s\" is not well defined.\n", 
        Ntk_NodeGetNamePrintable(pNode) );
//    Mvr_RelationFree( pFlex );
//    return;

    if ( Dcmn_ManagerReadVerbose( p ) )
    {
    Ntk_NodePrintMvr( stdout, pNode );
    Ntk_NodePrintFlex( stdout, pNode, pFlex );
    //PRB( dd, Ntk_NodeReadFuncMvr(pNode) );
    }
}
else
{

    if ( Dcmn_ManagerReadVerbose( p ) )
    {
    Ntk_NodePrintMvr( stdout, pNode );
    Ntk_NodePrintFlex( stdout, pNode, pFlex );
    //PRB( dd, Ntk_NodeReadFuncMvr(pNode) );
    }

    RatioBef = Mvr_RelationFlexRatio( Ntk_NodeReadFuncMvr(pNode) );
    RatioAft = Mvr_RelationFlexRatio( pFlex );

    printf( "Node %30s. Ratio before = %7.3f %%. Ratio after = %7.3f %%.\n", 
           Ntk_NodeGetNamePrintable(pNode), RatioBef, RatioAft );

    s_TotalRatioBef += RatioBef;
    s_TotalRatioAft += RatioAft;
    s_nNodes++;
}

    // minimize the node
//    Mfs_NetworkMfsNodeMinimize( p, pNode, pFlex );
    Mvr_RelationFree( pFlex );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

