/**CFile****************************************************************

  FileName    [ntkiFraig.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Builds FRAIG for the network and replaces the network by FRAIG.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiFraig.c,v 1.15 2005/07/08 01:01:22 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "fraigInt.h"
#include "mio.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Ntk_Network_t * Ntk_NetworkFraigToNet( Ntk_Network_t * pNetRepres, Fraig_Man_t * pMan, int fMulti, int fAreaDup, int fWriteAnds );
static Ntk_Node_t *    Ntk_NetworkFraigCreateAnd_rec( Ntk_Network_t * pNetNew, Fraig_Node_t * gNode, int fWriteAnds );
static Ntk_Node_t *    Ntk_NetworkFraigCreateMultiAnd_rec( Ntk_Network_t * pNetNew, Fraig_Node_t * gNode, int fAreaDup );
static void            Ntk_NetworkFraigCollect_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fAreaDup, int fFirst );

static void            Ntk_NetworkFraigNode( Fraig_Man_t * pMan, Ntk_Node_t * pNode );
extern Fraig_Node_t *  Ntk_NetworkFraigFactor_rec( Fraig_Man_t * pMan, Ft_Tree_t * pFtTree, Ft_Node_t * pFtNode );

static int *           Ntk_NetworkCiGetDiscreteArrivalTimes( Ntk_Network_t * pNet );

static Fraig_Man_t *   Ntk_NetworkFraigTrust( Ntk_Network_t * pNet, int fFuncRed, int fBalance, int fDoSparse, int fChoicing, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transforms the network into its AIG without choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkDeriveFraig( Mv_Frame_t * pMvsis, Ntk_Network_t * pNet, int fMulti, int fAreaDup, int fWriteAnds, 
                                  int fFuncRed, int fBalance, int fDoSparse, int fTryProve, int fVerbose )
{
    Fraig_Man_t * pMan;
    Ntk_Network_t * pNetNew;
    int clk, clkTotal = clock();
    int fRefCount = 0; // no ref counting
    int fChoicing = 0; // create choice nodes
    extern void Fraig_ManProveMiter( Fraig_Man_t * p );

    if ( pNet == NULL )
        return NULL;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only fraig binary networks.\n" );
        return NULL;
    }

    if ( fTryProve )
        fBalance = 0;

    // derive the FRAIG manager containing the network
    pMan = Ntk_NetworkFraig( pNet, fFuncRed, fBalance, fDoSparse, fChoicing, fVerbose );
    // if multi-input gates are used, set the fanout free cones
    if ( fMulti )
        Fraig_ManMarkRealFanouts( pMan );

    Fraig_ManSetTryProve( pMan, fTryProve );
    Fraig_ManProveMiter( pMan );
PRT( "Total runtime   ", clock() - clkTotal );
  

    // transform the AIG into the equivalent network
clk = clock();
    pNetNew = Ntk_NetworkFraigToNet( pNet, pMan, fMulti, fAreaDup, fWriteAnds );
Fraig_ManSetTimeToNet( pMan, clock() - clk );
Fraig_ManSetTimeTotal( pMan, clock() - clkTotal );

{
extern void Fraig_DominatorsCompute2( Fraig_Man_t * pMan );
extern Fraig_NodeVec_t * Fraig_SymmsStructCompute( Fraig_Man_t * pMan );
extern Fraig_NodeVec_t * Fraig_SymmsSimCompute( Fraig_Man_t * pMan );
extern void Fraig_ManComputeImplications( Fraig_Man_t * p );
extern void Fraig_ManComputeSupports( Fraig_Man_t * p, int nSimLimit, int fExact, int fVerbose );
extern void Fraig_ManRetime( Fraig_Man_t * pMan, int nLatches, int fVerbose );
extern void Fraig_AddChoices( Fraig_Man_t * pMan );
extern void Fraig_ManCutSet( Fraig_Man_t * pMan, int nLatches );
//Fraig_AddChoices( pMan );
//Fraig_ManCutSet( pMan, Ntk_NetworkReadLatchNum(pNet) );
//Fraig_ManRetime( pMan, Ntk_NetworkReadLatchNum(pNet), 0 );
//Fraig_DominatorsCompute2( pMan );
//Fraig_SymmsStructCompute( pMan );
//Fraig_SymmsSimCompute( pMan );
//Fraig_ManComputeImplications( pMan );
//Fraig_ManComputeSupports( pMan, 1, 0 );

}

    // free the manager
    if ( fTryProve )
        Fraig_ManSetVerbose( pMan, fVerbose );
    else
        Fraig_ManSetVerbose( pMan, !fBalance );
    Fraig_ManFree( pMan );
    return pNetNew;
}

/**Function*************************************************************

  Synopsis    [Derives MVSIS network from the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t *  Ntk_NetworkFraigToNet( Ntk_Network_t * pNet, Fraig_Man_t * pMan, int fMulti, int fAreaDup, int fWriteAnds )
{
    Ntk_Network_t * pNetNew;
    Ntk_Latch_t * pLatch, * pLatchNew;
    Fraig_Node_t ** pgInputs, ** pgOutputs;
    Ntk_Node_t * pNodeNew, * pNode, * pOutput;
	char NameBuffer[500];
    char * pName;
    int nInputs, nOutputs, i;
    int clk = clock();

    // get parameters of AIG
    nInputs   = Ntk_NetworkReadCiNum(pNet);
    nOutputs  = Ntk_NetworkReadCoNum(pNet);
    pgInputs  = Fraig_ManReadInputs( pMan );
    pgOutputs = Fraig_ManReadOutputs( pMan );

    // allocate the empty network
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );
    // register the name 
    pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNet->pName );
    // register the network spec file name
    if ( pNet->pSpec )
        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, pNet->pSpec );

    // copy and add the CI nodes
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // set the PI node into the corresponding AIG node
        Fraig_NodeSetData0( pgInputs[i], (Fraig_Node_t *)pNodeNew );
        i++;
    }
    // add the constant node
    if ( Fraig_NodeReadNumRefs( Fraig_ManReadConst1(pMan) ) > 1 )
    {
        pNodeNew = Ntk_NodeCreateConstant( pNetNew, 2, 2 );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // add this node to the network
//        Ntk_NetworkAddNode( pNetNew, pNode, 1 );
        sprintf( NameBuffer, "const1");
        pName = Ntk_NetworkRegisterNewName(pNetNew, NameBuffer);
        Ntk_NodeAssignName(pNodeNew, pName);
        // set the PI node into the corresponding AIG node
        Fraig_NodeSetData0( Fraig_ManReadConst1(pMan), (Fraig_Node_t *)pNodeNew );
    }

    // recursively create the nodes
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // create the corresponding output
        if ( fMulti )
            pOutput = Ntk_NetworkFraigCreateMultiAnd_rec( pNetNew, Fraig_Regular(pgOutputs[i]), fAreaDup );
        else
            pOutput = Ntk_NetworkFraigCreateAnd_rec( pNetNew, Fraig_Regular(pgOutputs[i]), fWriteAnds );
        // check if the output is complemented
        if ( Fraig_IsComplement(pgOutputs[i]) )
        { 
          // name the original
          if (!Ntk_NodeReadName(pOutput))
            Ntk_NodeAssignName(pOutput, NULL);
          // add inverter
          pOutput = Ntk_NodeCreateOneInputBinary( pNetNew, pOutput, 1 );
          Ntk_NetworkAddNode( pNetNew, pOutput, 1 );
        }
        if ( pOutput->Type == MV_NODE_CI )
        { // add buffer
            if ( fWriteAnds )
            { 
              // name the original
              if (!Ntk_NodeReadName(pOutput))
                Ntk_NodeAssignName(pOutput, NULL);
              // add two inverters
              // this is the first
              pOutput = Ntk_NodeCreateOneInputBinary( pNetNew, pOutput, 1 );
              Ntk_NetworkAddNode( pNetNew, pOutput, 1 );
              // name the first inverter
              if (!Ntk_NodeReadName(pOutput))
                Ntk_NodeAssignName(pOutput, NULL);
              // this is the second inverter
              pOutput = Ntk_NodeCreateOneInputBinary( pNetNew, pOutput, 1 );
              Ntk_NetworkAddNode( pNetNew, pOutput, 1 );
            }
            else
            {
              // name the original
              if (!Ntk_NodeReadName(pOutput))
                Ntk_NodeAssignName(pOutput, NULL);
              // continue
              pOutput = Ntk_NodeCreateOneInputBinary( pNetNew, pOutput, 0 );
              Ntk_NetworkAddNode( pNetNew, pOutput, 1 );
            }
        }
        // name it
        pName = Ntk_NetworkRegisterNewName(pNetNew, pNode->pName);
        if (Ntk_NodeReadName(pOutput))
          Ntk_NodeRemoveName(pOutput);
        Ntk_NodeAssignName(pOutput, pName);
        // create the CO node
        pNodeNew = Ntk_NetworkAddNodeCo( pNetNew, pOutput, 1 );
        Ntk_NodeAssignName(pOutput, NULL);
        pNodeNew->Subtype = pNode->Subtype;
        // remember it in the old node
        pNode->pCopy = pNodeNew;
        i++;
    }

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
    Ntk_NetworkOrderFanins( pNetNew );

    // sweep to remove useless nodes
//    Ntk_NetworkSweep( pNetNew, 1, 1, 0, 0 );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkFraig(): Network check has failed.\n" );
    return pNetNew;
}


/**Function*************************************************************

  Synopsis    [Creates two-input MVSIS node from the AIG node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkFraigCreateAnd_rec( Ntk_Network_t * pNetNew, Fraig_Node_t * gNode, int fWriteAnds )
{
    Fraig_Node_t * gNode1, * gNode2, * pTemp;
    Ntk_Node_t * ppNodes[2], * pNode, * pNodeNew;
    int fCompl1, fCompl2, Type;
	char NameBuffer[500];
    char* pName;

    assert( !Fraig_IsComplement(gNode) );
//    if ( Fraig_NodeIsAnd(gNode) )
//printf( "AND node for %5d = AND(%5d, %5d).\n", gNode->Num, Fraig_Regular(gNode->p1)->Num, Fraig_Regular(gNode->p2)->Num );
    // if the node is visited, return the resulting Ntk_Node
    pNode = (Ntk_Node_t *)Fraig_NodeReadData0( gNode );
    if ( pNode )
        return pNode;

    if ( Fraig_NodeIsConst(gNode) )
    {
        // create the constant-1 node
        pNode = Ntk_NodeCreateConstant( pNetNew, 2, (unsigned)2 );
        // add this node to the network
        Ntk_NetworkAddNode( pNetNew, pNode, 1 );
        sprintf( NameBuffer, "const1");
        pName = Ntk_NetworkRegisterNewName(pNetNew, NameBuffer);
        Ntk_NodeAssignName(pNode, pName);
        // save the node
        Fraig_NodeSetData0( gNode, (Fraig_Node_t *)pNode );
        return pNode;
    }
    assert( Fraig_NodeIsAnd(gNode) );
    
    // get the children
    gNode1 = Fraig_NodeReadOne(gNode);
    gNode2 = Fraig_NodeReadTwo(gNode);

    // get the children nodes
    ppNodes[0] = Ntk_NetworkFraigCreateAnd_rec( pNetNew, Fraig_Regular(gNode1), fWriteAnds );
    ppNodes[1] = Ntk_NetworkFraigCreateAnd_rec( pNetNew, Fraig_Regular(gNode2), fWriteAnds );
    // order them
    if ( Ntk_NodeCompareByNameAndId( ppNodes, ppNodes + 1 ) >= 0 )
    {
        pNode      = ppNodes[0];
        ppNodes[0] = ppNodes[1];
        ppNodes[1] = pNode;

        pTemp  = gNode1;
        gNode1 = gNode2;
        gNode2 = pTemp;
    }
    // derive complemented attributes
    fCompl1 = Fraig_IsComplement( gNode1 );
    fCompl2 = Fraig_IsComplement( gNode2 );

    if ( fWriteAnds )
    {
        // create the intertors
        if ( fCompl1 )
        {
            pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, ppNodes[0], 1 );
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            sprintf( NameBuffer,
                     "inv%d:%d",
                     Fraig_NodeReadNum(Fraig_Regular(gNode)),
                     Fraig_NodeReadNum(Fraig_Regular(gNode1)) );
            pName = Ntk_NetworkRegisterNewName(pNetNew, NameBuffer);
			Ntk_NodeAssignName(pNodeNew, pName);

            ppNodes[0] = pNodeNew;
        }
        if ( fCompl2 )
        {
            pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, ppNodes[1], 1 );
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            sprintf( NameBuffer,
                     "inv%d:%d",
                     Fraig_NodeReadNum(Fraig_Regular(gNode)),
                     Fraig_NodeReadNum(Fraig_Regular(gNode2)) );
            pName = Ntk_NetworkRegisterNewName(pNetNew, NameBuffer);
			Ntk_NodeAssignName(pNodeNew, pName);

            ppNodes[1] = pNodeNew;
        }
        // create the AND gate
        Type = 8;
    }
    else
    {
        if ( !fCompl1 && !fCompl2 )        // AND( a,  b  )
            Type = 8;  //  1000
        else if ( fCompl1 && !fCompl2 )    // AND( a,  b' )
            Type = 4;  //  0100 
        else if ( !fCompl1 && fCompl2 )    // AND( a', b  )
            Type = 2;  //  0010
        else  // if ( fCompl1 && fCompl2 ) // AND( a', b' )
            Type = 1;  //  0001
    }

    // create the two-input node
    pNode = Ntk_NodeCreateTwoInputBinary( pNetNew, ppNodes, Type );
    // add the node to the network
    Ntk_NetworkAddNode( pNetNew, pNode, 1 );
	// Name the node
	sprintf( NameBuffer,
             "and%d",
             Fraig_NodeReadNum(gNode));
    pName = Ntk_NetworkRegisterNewName(pNetNew, NameBuffer);
    Ntk_NodeAssignName(pNode, pName);
    //Ntk_NodeAssignName(pNode, NULL);
    // save the node in the AIG node
    Fraig_NodeSetData0( gNode, (Fraig_Node_t *)pNode );
    return pNode;
}


/**Function*************************************************************

  Synopsis    [Creates multi-input MVSIS node from several AIG nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkFraigCreateMultiAnd_rec( Ntk_Network_t * pNetNew, Fraig_Node_t * gNode, int fAreaDup )
{
    Ntk_Node_t ** ppNodes, * pNode;
    Fraig_NodeVec_t * vNodes;
    int * pCompls, i;

    assert( !Fraig_IsComplement(gNode) );

    // if the node is visited, return the resulting OR-gate
    pNode = (Ntk_Node_t *)Fraig_NodeReadData0( gNode );
    if ( pNode )
        return pNode;
    assert( Fraig_NodeIsAnd(gNode) );

    // get the nodes
    vNodes = Fraig_NodeVecAlloc( 10 );
    Ntk_NetworkFraigCollect_rec( gNode, vNodes, fAreaDup, 1 );

    // get the children nodes
    ppNodes = ALLOC( Ntk_Node_t *, vNodes->nSize );
    pCompls = ALLOC( int, vNodes->nSize );
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pCompls[i] = Fraig_IsComplement(vNodes->pArray[i]);
        ppNodes[i] = Ntk_NetworkFraigCreateMultiAnd_rec( pNetNew, Fraig_Regular(vNodes->pArray[i]), fAreaDup );
    }
    // create the binary AND gate with inputs in the corresponding polatiry
    pNode = Ntk_NodeCreateSimpleBinary( pNetNew, ppNodes, pCompls, vNodes->nSize, 0 );
    free( ppNodes );
    free( pCompls );
    Fraig_NodeVecFree( vNodes );

    // add this node to the network
    Ntk_NetworkAddNode( pNetNew, pNode, 1 );
    // save the node
    Fraig_NodeSetData0( gNode, (Fraig_Node_t *)pNode );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Returns the array of nodes combined into one AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFraigCollect_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes, int fAreaDup, int fFirst )
{
    // if the new node is complemented, another gate begins
    if ( Fraig_IsComplement(pNode) )
    {
        Fraig_NodeVecPushUnique( vNodes, pNode );
        return;
    }
    // if pNew is the PI node, return
    if ( !Fraig_NodeIsAnd(pNode) )
    {
        Fraig_NodeVecPushUnique( vNodes, pNode );
        return;    
    }
//    if ( !fAreaDup && !fFirst && Fraig_NodeGetFanoutNum(pNode) > 1 ) // counts all fanouts
    if ( !fAreaDup && !fFirst && Fraig_NodeReadNumFanouts(pNode) > 1 ) // counts only useful fanouts
    {
        Fraig_NodeVecPushUnique( vNodes, pNode );
        return;    
    }
    // go through the branches
    Ntk_NetworkFraigCollect_rec( pNode->p1, vNodes, fAreaDup, 0 );
    Ntk_NetworkFraigCollect_rec( pNode->p2, vNodes, fAreaDup, 0 );
}








FILE * pTable;


/**Function*************************************************************

  Synopsis    [Transforms the network into its AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Man_t * Ntk_NetworkFraig( Ntk_Network_t * pNet, int fFuncRed, int fBalance, int fDoSparse, int fChoicing, int fVerbose )
{
    Fraig_Man_t * pMan, * pTemp;
    int fRefCount = 0; // no ref counting
    int clk, clkTotal = clock();

//    return Ntk_NetworkFraigTrust( pNet, fFuncRed, fBalance, fDoSparse, fChoicing, fVerbose );

    // start the manager
    pMan = Fraig_ManCreate();

    // save the IO names in the FRAIG manager
    Fraig_ManSetInputNames( pMan, Ntk_NetworkCollectIONames( pNet, 1 ) );
    Fraig_ManSetOutputNames( pMan, Ntk_NetworkCollectIONames( pNet, 0 ) );

    // set the parameters of the manager 
    Fraig_ManSetFuncRed ( pMan, fFuncRed );       
//    Fraig_ManSetFeedBack( pMan, fFeedBack );      
    Fraig_ManSetDoSparse( pMan, fDoSparse );      
    Fraig_ManSetRefCount( pMan, fRefCount );      
    Fraig_ManSetChoicing( pMan, fChoicing );      
    Fraig_ManSetVerbose ( pMan, fVerbose );       

    // add the current network
clk = clock();
    Ntk_NetworkFraigInt( pMan, pNet, 1 );
//PRT( "Fraiging", clock() - clk );
    // dereference the network
    Ntk_NetworkFraigDeref( pMan, pNet );

/*	
pTable = fopen( "stats.txt", "a+" );
fprintf( pTable, "%s ", pNet->pSpec );
fprintf( pTable, "%4.2f\n", (float)(clock() - clk)/(float)(CLOCKS_PER_SEC) );
fclose( pTable );
*/

    // balance the FRAIG (while putting it into a new manager)
    if ( fBalance )
    {
        int * pInputArrivals;
        pInputArrivals = Ntk_NetworkCiGetDiscreteArrivalTimes( pNet );
        pMan = Fraig_ManBalance( pTemp = pMan, 0, pInputArrivals );
        Fraig_ManFree( pTemp );
        free( pInputArrivals );
    }
    return pMan;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFraigInt( Fraig_Man_t * pMan, Ntk_Network_t * pNet, int fCopyOutputs )
{
    ProgressBar * pProgress;
    Fraig_Node_t ** pgInputs, ** pgOutputs, * gNode;
    Ntk_Node_t * pNode, * pDriver;
    int nInputs, nOutputs, i;

    // start the strashed network
    nInputs  = Ntk_NetworkReadCiNum(pNet);
    nOutputs = Ntk_NetworkReadCoNum(pNet);
    pgInputs  = Fraig_ManReadInputs( pMan );
    pgOutputs = Fraig_ManReadOutputs( pMan );

    // clean the data
    Ntk_NetworkForEachNode( pNet, pNode )
        pNode->pCopy = NULL;

    // set the leaves
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        gNode = Fraig_ManReadIthVar( pMan, i++ );
        NTK_STRASH_WRITE( pNode, gNode );
    }
    assert( i == Ntk_NetworkReadCiNum(pNet) );

    // put the nodes in the DFS order
    Ntk_NetworkDfs( pNet, 1 );
//    Ntk_NetworkDfsByLevel( pNet, 1 );
    // for some reason, it was slower and the quality was worse

    // build the AIG of the network for the internal nodes
    pProgress = Extra_ProgressBarStart( stdout, Ntk_NetworkReadNodeIntNum(pNet) );
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        Ntk_NetworkFraigNode( pMan, pNode );
        if ( ++i % 1000 == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );

    }
    Extra_ProgressBarStop( pProgress );

    if ( fCopyOutputs ) 
    {
        // set outputs in the FRAIG manager
        Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
        {
            gNode = NTK_STRASH_READ( pDriver );
            Fraig_ManSetPo( pMan, gNode );  // Fraig_Ref( gNode );
            // no need to ref because the node is held by the driver
        }
    }

    // make sure that choicing went well
    assert( Fraig_ManCheckConsistency( pMan ) );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFraigNode( Fraig_Man_t * pMan, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    Fraig_Node_t * gNode;
    Cvr_Cover_t * pCvr;
    Ft_Tree_t * pTree;
    int i;

    // get the factored form of this node
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    pTree = Cvr_CoverFactor( pCvr );

    // init the leaves the factored form
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
        pTree->uLeafData[i] = (unsigned)NTK_STRASH_READ( pFanin );

    // strash the factored form
    if ( pTree->pRoots[0] == NULL )
    {
        gNode = Ntk_NetworkFraigFactor_rec( pMan, pTree, pTree->pRoots[1] );  Fraig_Ref( gNode );
        NTK_STRASH_WRITE( pNode, gNode );
    }
    else
    {
        gNode = Ntk_NetworkFraigFactor_rec( pMan, pTree, pTree->pRoots[0] );  Fraig_Ref( gNode );
        NTK_STRASH_WRITE( pNode, Fraig_Not(gNode) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Ntk_NetworkFraigFactor_rec( Fraig_Man_t * pMan, Ft_Tree_t * pFtTree, Ft_Node_t * pFtNode )
{
    Fraig_Node_t * gNode1, * gNode2, * gNode;    
    if ( pFtNode->Type == FT_NODE_LEAF )
    {
        if ( pFtNode->uData == 1 )
            return Fraig_Not( pFtTree->uLeafData[pFtNode->VarNum] );
        if ( pFtNode->uData == 2 )
            return (Fraig_Node_t *)pFtTree->uLeafData[pFtNode->VarNum];
        assert( 0 );
        return NULL;
    }
    if ( pFtNode->Type == FT_NODE_AND )
    {
        gNode1 = Ntk_NetworkFraigFactor_rec( pMan, pFtTree, pFtNode->pOne );  Fraig_Ref( gNode1 );
        gNode2 = Ntk_NetworkFraigFactor_rec( pMan, pFtTree, pFtNode->pTwo );  Fraig_Ref( gNode2 );
        gNode = Fraig_NodeAnd( pMan, gNode1, gNode2 );                           Fraig_Ref( gNode );
        Fraig_RecursiveDeref( pMan, gNode1 );
        Fraig_RecursiveDeref( pMan, gNode2 );
        Fraig_Deref( gNode );
        return gNode;
    }
    if ( pFtNode->Type == FT_NODE_OR )
    {
        gNode1 = Ntk_NetworkFraigFactor_rec( pMan, pFtTree, pFtNode->pOne );  Fraig_Ref( gNode1 );
        gNode2 = Ntk_NetworkFraigFactor_rec( pMan, pFtTree, pFtNode->pTwo );  Fraig_Ref( gNode2 );
        gNode = Fraig_NodeOr( pMan, gNode1, gNode2 );                            Fraig_Ref( gNode );
        Fraig_RecursiveDeref( pMan, gNode1 );
        Fraig_RecursiveDeref( pMan, gNode2 );
        Fraig_Deref( gNode );
        return gNode;
    }
    if ( pFtNode->Type == FT_NODE_0 )
        return Fraig_Not( Fraig_ManReadConst1( pMan ) );
    if ( pFtNode->Type == FT_NODE_1 )
        return Fraig_ManReadConst1( pMan );
    assert( 0 ); // unknown node type
    return NULL;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFraigDeref( Fraig_Man_t * pMan, Ntk_Network_t * pNet )
{
    Fraig_Node_t * gNode;
    Ntk_Node_t * pNode;
    Ntk_NetworkDfs( pNet, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        gNode = NTK_STRASH_READ( pNode );
        Fraig_RecursiveDeref( pMan, gNode );
    }
}

/**Function*************************************************************

  Synopsis    [Returns discrete arrival times of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Ntk_NetworkCiGetDiscreteArrivalTimes( Ntk_Network_t * pNet )
{
    extern Mio_Library_t * s_pLib;
    int * pInputArrivalsDiscrete, i;
    Ntk_Node_t * pNode;
    double dAndGateDelay = 1.0;
    
    // get the delay of the inverter
    if ( s_pLib )
        dAndGateDelay = Mio_LibraryReadDelayNand2Max( s_pLib );

    // discretize the arrival times for balancing FRAIGs
    pInputArrivalsDiscrete = ALLOC( int, Ntk_NetworkReadCiNum(pNet) );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        pInputArrivalsDiscrete[i++] = Ntk_NodeGetArrivalTimeDiscrete( pNode, dAndGateDelay );
    return pInputArrivalsDiscrete;
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFraigIntTrust( Fraig_Man_t * pMan, Ntk_Network_t * pNet )
{
    ProgressBar * pProgress;
    Fraig_Node_t ** pgInputs, ** pgOutputs, * gNode, * gNode2;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pNode, * pDriver, * pFanin, * pFanin2;
    int nInputs, nOutputs, i, Value, iFanin, FaninSel, fCompl;
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pCube;

    // start the strashed network
    nInputs  = Ntk_NetworkReadCiNum(pNet);
    nOutputs = Ntk_NetworkReadCoNum(pNet);
    pgInputs  = Fraig_ManReadInputs( pMan );
    pgOutputs = Fraig_ManReadOutputs( pMan );

    // clean the data
    Ntk_NetworkForEachNode( pNet, pNode )
        pNode->pCopy = NULL;

    // set the leaves
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        gNode = Fraig_ManReadIthVar( pMan, i++ );
        NTK_STRASH_WRITE( pNode, gNode );
    }
    assert( i == Ntk_NetworkReadCiNum(pNet) );

    // put the nodes in the DFS order
    Ntk_NetworkDfs( pNet, 1 );

    // build the AIG of the network for the internal nodes
    pProgress = Extra_ProgressBarStart( stdout, Ntk_NetworkReadNodeIntNum(pNet) );
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
//        Ntk_NetworkFraigNode( pMan, pNode );
        pCvr = Ntk_NodeReadFuncCvr(pNode);
        pMvc = Cvr_CoverReadIsetByIndex(pCvr, 1);

        if ( Ntk_NodeReadFaninNum(pNode) == 1 ) // buf or inv
        {
            pFanin = Ntk_NodeReadFaninNode(pNode, 0);
            gNode = NTK_STRASH_READ(pFanin);
            if ( Ntk_NodeIsBinaryBuffer(pNode) )
                NTK_STRASH_WRITE( pNode, gNode );
            else
                NTK_STRASH_WRITE( pNode, Fraig_Not(gNode) );
        }
        else if ( Mvc_CoverReadCubeNum(pMvc) == 1 ) // AND
        {
            assert( Ntk_NodeReadFaninNum(pNode) == 2 );

            // get the first fanin
            pFanin  = Ntk_NodeReadFaninNode(pNode, 0);
            gNode  = NTK_STRASH_READ(pFanin);
            Value = Mvc_CubeVarValue( Mvc_CoverReadCubeHead(pMvc), 0 );
            assert( Value == 1 || Value == 2 );
            if ( Value == 1 )
                gNode = Fraig_Not( gNode );

            // get the second fanins
            pFanin2 = Ntk_NodeReadFaninNode(pNode, 1);
            gNode2 = NTK_STRASH_READ(pFanin2);
            Value = Mvc_CubeVarValue( Mvc_CoverReadCubeHead(pMvc), 1 );
            assert( Value == 1 || Value == 2 );
            if ( Value == 1 )
                gNode2 = Fraig_Not( gNode2 );

            // create the AND node
            NTK_STRASH_WRITE( pNode, Fraig_NodeAnd(pMan, gNode, gNode2) );
        }
        else // OR
        {
            // select the node with the smallest logic level
            gNode = NULL;
            FaninSel = -1;
            iFanin = 0;
            Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            {
                gNode2 = NTK_STRASH_READ(pFanin);
                if ( gNode == NULL || Fraig_Regular(gNode)->Level > Fraig_Regular(gNode2)->Level )
                {
                    gNode = gNode2;
                    FaninSel = iFanin;
                }
                iFanin++;
            }

            // get the phase of this node - go through the cubes
            assert( FaninSel != -1 );
            Mvc_CoverForEachCube( pMvc, pCube )
            {
                Value = Mvc_CubeVarValue( pCube, FaninSel );
                if ( Value == 1 || Value == 2 )
                {
                    fCompl = (Value == 1);
                    break;
                }
            }

            // collect the nodes
            assert( gNode != NULL );
            Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            {
                gNode2 = NTK_STRASH_READ(pFanin);
                if ( gNode == gNode2 )
                    continue;
                Fraig_NodeSetChoice( pMan, gNode, gNode2 );
            }

            // set the output of this node
            NTK_STRASH_WRITE( pNode, Fraig_NotCond(gNode, fCompl) );
        }

        if ( ++i % 1000 == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );

    }
    Extra_ProgressBarStop( pProgress );

    // set outputs in the FRAIG manager
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        gNode = NTK_STRASH_READ( pDriver );
        Fraig_ManSetPo( pMan, gNode );  // Fraig_Ref( gNode );
        // no need to ref because the node is held by the driver
    }
    // make sure that choicing went well
    assert( Fraig_ManCheckConsistency( pMan ) );
}

/**Function*************************************************************

  Synopsis    [Constructs FRAIG in the trusting mode.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Man_t * Ntk_NetworkFraigTrust( Ntk_Network_t * pNet, int fFuncRed, int fBalance, int fDoSparse, int fChoicing, int fVerbose )
{
    Fraig_Man_t * pMan;
    int fRefCount = 0; // no ref counting

    assert( fBalance == 0 );

    // start the manager
    pMan = Fraig_ManCreate();

    // save the IO names in the FRAIG manager
    Fraig_ManSetInputNames( pMan, Ntk_NetworkCollectIONames( pNet, 1 ) );
    Fraig_ManSetOutputNames( pMan, Ntk_NetworkCollectIONames( pNet, 0 ) );

    // disable functional reduction
    Fraig_ManSetFuncRed( pMan, 0 );  
    
    // add the current network
    Ntk_NetworkFraigIntTrust( pMan, pNet );
    // dereference the network
    Ntk_NetworkFraigDeref( pMan, pNet );

/*
    // set the parameters of the manager 
    Fraig_ManSetFuncRed ( pMan, fFuncRed );       
    Fraig_ManSetDoSparse( pMan, fDoSparse );      
    Fraig_ManSetRefCount( pMan, fRefCount );      
    Fraig_ManSetChoicing( pMan, fChoicing );      
    Fraig_ManSetVerbose ( pMan, fVerbose );       
*/
    return pMan;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


