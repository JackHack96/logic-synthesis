/**CFile****************************************************************

  FileName    [mfsSpec.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Performs simplification of MV networks using complete flexiblitity.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mfsSpec.c,v 1.7 2004/04/12 20:41:09 alanmi Exp $]

***********************************************************************/

#include "mfsInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void             Mfs_NetworkMfsNodeSpec( Fms_Manager_t * pMan, Mfs_Params_t * p, Ntk_Node_t * pNode, Wn_Window_t * pWnd, Sh_Manager_t * pManSh );
static Mvr_Relation_t * Mfs_NetworkFlexNodeSpec( Fms_Manager_t * pMan, Ntk_Node_t * pNode, Wn_Window_t * pWnd, Sh_Manager_t * pManSh );
static void             Mfs_NetworkMarkNDCones_rec( Ntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Simplifies the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_NetworkMfsSpec( Mfs_Params_t * p, Ntk_Network_t * pNet )
{
    Ntk_Network_t * pNetSpec = NULL;
    Sh_Manager_t * pManSh;
    Fms_Manager_t * pMan;
    Wn_Window_t * pWnd, * pWndSpec = NULL;
    Sh_Network_t * pShNet;
    Ntk_Node_t ** ppNodes;
    ProgressBar * pProgress;
    Ntk_Node_t * pNode;
    int nNodes, iNode;
    int clk;

timeGlobalFirst = 0;
timeGlobal      = 0;
timeContain     = 0;
timeImage       = 0;
timeImagePrep   = 0;
timeStrash      = 0;
timeSopMin      = 0;
timeResub       = 0;
timeOther       = clock();
 
    // sweep the buffers/intervers
    Ntk_NetworkSweep( pNet, 0, 0, 0, 0 );

    // compute parameters
    Mfs_NetworkMfsPreprocess( p, pNet );

    // if the network is ND, mark the TFO code of every ND node
    if ( p->fNonDet )
        Mfs_NetworkMarkNDCones( pNet );


    // collect the global BDDs
    pMan = Fms_ManagerAlloc( Ntk_NetworkReadManVm(pNet), Ntk_NetworkReadManVmx(pNet),  Ntk_NetworkReadManMvr(pNet) );
    // set the parameters
    Fms_ManagerSetBinary    ( pMan, p->fBinary );
    Fms_ManagerSetSetInputs ( pMan, p->fSetInputs );
    Fms_ManagerSetSetOutputs( pMan, p->fSetOutputs );
    Fms_ManagerSetVerbose   ( pMan, p->fVerbose );

    // create the strashing manager
    pManSh = Sh_ManagerCreate( 100, 100000, 1 );

    // build the global BDDs
    if ( p->fUseSpec && pNet->pSpec ) 
    {
        printf( "Using external spec \"%s\".\n", pNet->pSpec );
        pNetSpec = Io_ReadNetwork( Ntk_NetworkReadMvsis(pNet), pNet->pSpec );

        // mark the ND cones in the spec network
        Mfs_NetworkMarkNDCones( pNetSpec );

        pWndSpec = Wn_WindowCreateFromNetwork( pNetSpec );
        pWnd = Wn_WindowCreateFromNetwork( pNet );
        // create the MV var maps for the window
        Wn_WindowCreateVarMaps( pWnd );

        pShNet = Wn_WindowStrash( pManSh, pWndSpec, 0 );
        Ntk_NetworkDelete( pNetSpec );

        // clean the previous strashings
        Wn_WindowStrashClean( pWnd );
    }
    else
    {
        pWnd = Wn_WindowCreateFromNetwork( pNet );
        pShNet = Wn_WindowStrash( pManSh, pWnd, 0 );
    }

clk = clock();
    if ( !Fms_FlexibilityPrepare( pMan, pShNet ) )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Global BDD computation for the network timed out after 10 sec.\n" );
        Sh_NetworkFree( pShNet );
        Wn_WindowDelete( pWnd );
        return;
    }
timeGlobalFirst += clock() - clk;

    // get the type of the current cost function
    p->AcceptType = Ntk_NetworkGetAcceptType( pNet );

    // go through the nodes and set the defaults
    nNodes  = Ntk_NetworkReadNodeIntNum( pNet );
    ppNodes = Ntk_NetworkOrderNodesByLevel( pNet, p->fOrderItoO );
    // start the progress var
    if ( !p->fVerbose )
        pProgress = Extra_ProgressBarStart( stdout, nNodes );
    for ( iNode = 0; iNode < nNodes; iNode++ )
    {
        pNode = ppNodes[iNode];
//if ( strcmp( pNode->lFanouts.pHead->pNode->pName, "pe2" ) == 0 )
//{
//    int i = 0;
//    Io_WriteNetwork( pNet, "temp.mv", 1 );
//}
       Mfs_NetworkMfsNodeSpec( pMan, p, pNode, pWnd, pManSh );

/*
       // if the node has become ND as a result of "mfs", check this fact
       if ( !p->fBinary && !p->fNonDet && 
            Mvr_RelationIsND( Ntk_NodeGetFuncMvr(pNode) )  )
       {
           p->fNonDet = 1;
           p->fSetOutputs = 1;
           Fms_ManagerSetSetOutputs( pMan, p->fSetOutputs );
       }
*/
       // if ND, need to update the ND TFOs
       Mfs_NetworkMarkNDCones( pNet );

       if ( iNode % 5 == 0 && !p->fVerbose )
            Extra_ProgressBarUpdate( pProgress, iNode, NULL );
    }
    FREE( ppNodes );
    if ( !p->fVerbose )
        Extra_ProgressBarStop( pProgress );
 
    // sweep the buffers/intervers
    Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );

    Wn_WindowDelete( pWnd );
    Sh_NetworkFree( pShNet );
    Sh_ManagerFree( pManSh, 1 );
    Fms_ManagerFree( pMan );

    if ( pWndSpec )
        Wn_WindowDelete( pWndSpec );

    // print the timing stats
    if ( p->fVerbose )
    {
        timeOther = clock() - timeOther - timeGlobalFirst - 
            timeGlobal - timeContain - timeImage - timeImagePrep - 
            timeStrash - timeSopMin - timeResub;
        PRT( "Global1 ",  timeGlobalFirst );
        PRT( "Global  ",  timeGlobal );
        PRT( "Contain ",  timeContain );
        PRT( "Image   ",  timeImage );
        PRT( "ImageP  ",  timeImagePrep );
        PRT( "Strash  ",  timeStrash );
        PRT( "SOP min ",  timeSopMin );
        PRT( "Resub   ",  timeResub );
        PRT( "Other   ",  timeOther );
    }

    if ( !Ntk_NetworkCheck( pNet ) )
        printf( "Mfs_NetworkMfsSpec(): Network check has failed.\n" );
}

/**Function*************************************************************

  Synopsis    [Simplifies the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_NetworkMfsNodeSpec( Fms_Manager_t * pMan, Mfs_Params_t * p, Ntk_Node_t * pNode, Wn_Window_t * pWnd, Sh_Manager_t * pManSh )
{
    Mvr_Relation_t * pFlex;
    FILE * pOutput;

    //. skip the nodes that do not fanout
    if ( !Ntk_NodeHasCoInTfo( pNode ) )
        return;
    // skip constant and single-input nodes
    if ( Ntk_NodeReadFaninNum( pNode ) < 2 )
        return;

    // get the MVSIS output stream
    pOutput = Ntk_NodeReadMvsisOut( pNode );

    // derive the flexibility
    pFlex = Mfs_NetworkFlexNodeSpec( pMan, pNode, pWnd, pManSh );
    if ( pFlex == NULL )
    {
        if ( p->fVerbose )
        fprintf( pOutput, "Mfs_NetworkMfs(): Flexibility computation timeout for node \"%s\".\n",
            Ntk_NodeGetNamePrintable(pNode) );
        return;
    }
//Ntk_NodePrintMvr( stdout, pNode );
//Ntk_NodePrintFlex( stdout, pNode, pFlex );

    // check whether flexibility is well defined
    if ( !Mvr_RelationIsWellDefined( pFlex ) )
    {
//Ntk_NodePrintMvr( stdout, pNode );
//Ntk_NodePrintFlex( stdout, pNode, pFlex );
        fprintf( pOutput, "Mfs_NetworkMfs(): Flexibility of node \"%s\" is not well defined.\n", 
            Ntk_NodeGetNamePrintable(pNode) );
        Mvr_RelationFree( pFlex );
        return;
    }

    // minimize the node
    Mfs_NetworkMfsNodeMinimize( p, pNode, pFlex );
    Mvr_RelationFree( pFlex );
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
Mvr_Relation_t * Mfs_NetworkFlexNodeSpec( Fms_Manager_t * pMan, Ntk_Node_t * pNode, Wn_Window_t * pWnd, Sh_Manager_t * pManSh )
{
    Wn_Window_t * pWndC;
    Sh_Network_t * pShNet;
    Mvr_Relation_t * pFlex;
    int clk;

    // get the core window
    pWndC = Wn_WindowCreateFromNode( pNode );
    // strash the main window using the core window
clk = clock();
    pShNet = Wn_WindowStrashCore( pManSh, pWnd, pWndC );
timeStrash += clock() - clk;

    // delete the core window
    Wn_WindowDelete( pWndC );

    // compute the flexibility
    pFlex = Fms_FlexibilityCompute( pMan, pShNet, Ntk_NodeGetFuncMvr(pNode) );

    // undo the strashed network
    Sh_NetworkFree( pShNet );
    return pFlex;
}


/**Function*************************************************************

  Synopsis    [Mark the TFO cones of the ND nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_NetworkMarkNDCones( Ntk_Network_t * pNet )
{
/*
    Ntk_Node_t * pNode;

    // reset the node attributes
    Ntk_NetworkForEachNode( pNet, pNode )
        pNode->fNdTfi = 0;
    // mark the TFO cones
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        // check if this node is already in the TFO cone
        if ( pNode->fNdTfi )
            continue;
        if ( pNode->nValues != 2 && Mvr_RelationIsND( Ntk_NodeGetFuncMvr(pNode) ) )
            Mfs_NetworkMarkNDCones_rec( pNode );
    }
//    Ntk_NetworkForEachNode( pNet, pNode )
//        printf( "Node %s.  Status = %d.\n", Ntk_NodeGetNamePrintable(pNode), pNode->fNdTfi );
*/
}

/**Function*************************************************************

  Synopsis    [Mark the TFO cones of the ND nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_NetworkMarkNDCones_rec( Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanout;
    Ntk_Pin_t * pPin;

    // return if this is a CO node
    if ( Ntk_NodeIsCo(pNode) )
        return;
    // mark the node
    pNode->fNdTfi = 1;
    // traverse the TFO
    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
    {
        // check if this node is already in the TFO cone
        if ( pFanout->fNdTfi )
            continue;
        Mfs_NetworkMarkNDCones_rec( pFanout );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


