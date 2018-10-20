/**CFile****************************************************************

  FileName    [mfsWnd.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Performs simplification of MV networks using complete flexiblitity.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mfsWnd.c,v 1.8 2004/04/12 20:41:09 alanmi Exp $]

***********************************************************************/

#include "mfsInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void             Mfs_NetworkMfsNode( Fmw_Manager_t * pManFm, Sh_Manager_t * pManSh, Mfs_Params_t * p, Ntk_Node_t * pNode );
static Mvr_Relation_t * Mfs_NetworkFlexNode( Fmw_Manager_t * pManFm, Sh_Manager_t * pManSh, Mfs_Params_t * p, Ntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Simplifies the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_NetworkMfs( Mfs_Params_t * p, Ntk_Network_t * pNet )
{
    Sh_Manager_t * pManSh;
    Fmw_Manager_t * pManFm;
    ProgressBar * pProgress;
    Ntk_Node_t * pNode;
    Ntk_Node_t ** ppNodes;
    int nNodes, iNode;
 
timeGlobalFirst = 0;
timeGlobal      = 0;
timeContain     = 0;
timeImage       = 0;
timeImagePrep   = 0;
timeStrash      = 0;
timeSopMin      = 0;
timeResub       = 0;
timeOther       = clock();

    pManSh  = Sh_ManagerCreate( 100, 100000, 1 );
    pManFm  = Fmw_ManagerAlloc( Ntk_NetworkReadManVm(pNet), Ntk_NetworkReadManVmx(pNet),  Ntk_NetworkReadManMvr(pNet) );

    // sweep the buffers/intervers
    Ntk_NetworkSweep( pNet, 1, 1, 0, 0 );
    // compute parameters
    Mfs_NetworkMfsPreprocess( p, pNet );

    // get the type of the current cost function
    p->AcceptType = Ntk_NetworkGetAcceptType( pNet );
    
    // go through the nodes and set the defaults
    nNodes = Ntk_NetworkReadNodeIntNum( pNet );
    ppNodes = Ntk_NetworkOrderNodesByLevel( pNet, p->fOrderItoO );
    // start the progress var
    if ( !p->fVerbose )
        pProgress = Extra_ProgressBarStart( stdout, nNodes );
    for ( iNode = 0; iNode < nNodes; iNode++ )
    {
        pNode = ppNodes[iNode];
//if ( pNode->pName &&  strcmp( pNode->pName, "l2" ) == 0 )
//{
//   int i = 0;
//    Io_WriteNetwork( pNet, "temp.mv", 3 );
//}
       Mfs_NetworkMfsNode( pManFm, pManSh, p, pNode );
       if ( iNode % 5 == 0 && !p->fVerbose )
            Extra_ProgressBarUpdate( pProgress, iNode, NULL );
    }
    FREE( ppNodes );
    if ( !p->fVerbose )
        Extra_ProgressBarStop( pProgress );

    Fmw_ManagerFree( pManFm );
    Sh_ManagerFree( pManSh, 1 );

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

    // sweep the buffers/intervers
    Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );
    if ( !Ntk_NetworkCheck( pNet ) )
        printf( "Mfs_NetworkMfs(): Network check has failed.\n" );
}

/**Function*************************************************************

  Synopsis    [Simplifies the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_NetworkMfsOne( Mfs_Params_t * p, Ntk_Node_t * pNode )
{
    Sh_Manager_t * pManSh;
    Fmw_Manager_t * pManFm;

    pManSh  = Sh_ManagerCreate( 100, 100000, 1 );
    pManFm  = Fmw_ManagerAlloc( Ntk_NetworkReadManVm(pNode->pNet), 
        Ntk_NetworkReadManVmx(pNode->pNet),  Ntk_NetworkReadManMvr(pNode->pNet) );

    Mfs_NetworkMfsPreprocess( p, pNode->pNet );
    Mfs_NetworkMfsNode( pManFm, pManSh, p, pNode );

    Fmw_ManagerFree( pManFm );
    Sh_ManagerFree( pManSh, 1 );
}

/**Function*************************************************************

  Synopsis    [Simplifies the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_NetworkMfsNode( Fmw_Manager_t * pManFm, Sh_Manager_t * pManSh, Mfs_Params_t * p, Ntk_Node_t * pNode )
{
    Mvr_Relation_t * pFlex;
    FILE * pOutput;

    pOutput = Ntk_NodeReadMvsisOut( pNode );

    //. skip the nodes that do not fanout
    if ( !Ntk_NodeHasCoInTfo( pNode ) )
    {
        if ( p->fShowKMap )
            fprintf( pOutput, "Node \"%s\" does not fanout to COs; simplification is not performed.\n", Ntk_NodeGetNamePrintable(pNode) );
        return;
    }
    // skip constant and single-input nodes
    if ( Ntk_NodeReadFaninNum( pNode ) < 2 )
    {
        if ( p->fShowKMap )
            fprintf( pOutput, "Node \"%s\" has less than two fanins; simplification is not performed.\n", Ntk_NodeGetNamePrintable(pNode) );
        return;
    }

    // get the MVSIS output stream
    pOutput = Ntk_NodeReadMvsisOut( pNode );

    // derive the flexibility
    pFlex = Mfs_NetworkFlexNode( pManFm, pManSh, p, pNode );
    if ( pFlex == NULL )
    {
        if ( p->fVerbose )
        fprintf( pOutput, "Mfs_NetworkMfs(): Flexibility computation timeout for node \"%s\".\n",
            Ntk_NodeGetNamePrintable(pNode) );
        return;
    }
    if ( p->fShowKMap )
    {
        fprintf( pOutput, "Original " );
        Ntk_NodePrintMvr( pOutput, pNode );
        fprintf( pOutput, "Derived " );
        Ntk_NodePrintFlex( pOutput, pNode, pFlex );
    }

    // check whether flexibility is well defined
    if ( !Mvr_RelationIsWellDefined( pFlex ) )
    {
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
Mvr_Relation_t * Mfs_NetworkFlexNode( Fmw_Manager_t * pManFm, Sh_Manager_t * pManSh, Mfs_Params_t * p, Ntk_Node_t * pNode )
{
    Wn_Window_t * pWnd, * pWndC;
    Sh_Network_t * pShNet;
    Mvr_Relation_t * pFlex;
    int Count1, Count2;

    // get the window
    pWndC = Wn_WindowCreateFromNode( pNode );
    if ( p->fUseWindow )
    {
//        pWnd = Wn_WindowDerive( pWndC, p->nLevelsTfi, p->nLevelsTfo );
        pWnd = Wn_WindowDeriveForNode( pNode, p->nLevelsTfi, p->nLevelsTfo );
        Wn_WindowSetWndCore( pWnd, pWndC );
//        Wn_WindowDelete( pWndC );

        if ( Wn_WindowReadLeafNum( pWnd ) > p->nTermsLimit ||
             Wn_WindowReadRootNum( pWnd ) > p->nTermsLimit )
        {
            Wn_WindowDelete( pWnd );
            return NULL;
        }
    }
    else
    {
        pWnd = Wn_WindowCreateFromNetwork( pNode->pNet );
        Wn_WindowSetWndCore( pWnd, pWndC );
    }

    // strash the window
    pShNet = Wn_WindowStrash( pManSh, pWnd, 1 );
    // set the parameters
    Fmw_ManagerSetBinary    ( pManFm, p->fBinary );
    Fmw_ManagerSetSetInputs ( pManFm, p->fSetInputs );
    Fmw_ManagerSetSetOutputs( pManFm, p->fSetOutputs );
    Fmw_ManagerSetVerbose   ( pManFm, p->fVerbose );

    // make sure that the core of the network is the same as node
    Count1 =  Vm_VarMapReadVarsInNum( Sh_NetworkReadVmLC(pShNet) );
    Count2 =  Vm_VarMapReadVarsInNum( Ntk_NodeReadFuncVm(pNode) );
    assert( Vm_VarMapReadVarsInNum( Sh_NetworkReadVmLC(pShNet) ) == 
            Vm_VarMapReadVarsInNum( Ntk_NodeReadFuncVm(pNode) ) );

    // compute the flexibility
    pFlex = Fm_FlexibilityCompute( pManFm, pShNet, Ntk_NodeGetFuncMvr(pNode) );

    // undo the strashed network
    Sh_NetworkFree( pShNet );
    Wn_WindowDelete( pWnd );
    return pFlex;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


