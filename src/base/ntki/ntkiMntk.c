/**CFile****************************************************************

  FileName    [ntkiMntk.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to convert between the tech-ind and mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiMntk.c,v 1.2 2005/03/01 05:36:03 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "mio.h"
#include "mapper.h"
#include "mntk.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// macros to access the structurally hashed representation of the node
#define MNTK_STRASH_READ(pNode)      ((Mntk_Node_t *)pNode->pCopy)
#define MNTK_STRASH_WRITE(pNode,p)   (pNode->pCopy = (Ntk_Node_t *)p)

// the procedures exported from this file
extern Mntk_Man_t *    Ntk_NetworkToMntk( Ntk_Network_t * pNet, int fVerbose );
extern Ntk_Network_t * Ntk_NetworkFromMntk( Mntk_Man_t * pMan, Ntk_Network_t * pNet );

static Ntk_Node_t *    Ntk_NodeFromMntk_rec( Mntk_Man_t * pMan, Mntk_Node_t * pMapNode, Ntk_Network_t * pNet );
static Mntk_Node_t *   Ntk_ReadNodeWithPhase_rec( Ntk_Node_t * pNode );
static Ntk_Node_t *    Ntk_ReadNonTrivialNode_rec( Ntk_Node_t * pNode );
static int             Ntk_NetworkHasDupFanins( Ntk_Network_t * pNet );
static void            Ntk_NodeSetInputArrivalTimes( Mntk_Man_t * pMan, Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs technology mapping using the supergate library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
Ntk_Network_t * Ntk_NetworkResm( Ntk_Network_t * pNet, char * pFileNameOut, 
    float DelayLimit, float AreaLimit, float TimeLimit, int fGlobalCones, 
    int fUseThree, int CritWindow, int ConeDepth, int ConeSize, int fVerbose )
{
    extern Mio_Library_t * s_pLib;
    Mntk_Man_t * pMan;
    Ntk_Network_t * pNetMap = NULL;
    Ntk_Node_t * pNode;
    int clk, clkTotal = clock();
    int i;

    // derive the resynthesis manager containing the network
clk = clock();
    pMan = Ntk_NetworkToMntk( pNet, fVerbose );
Mntk_ManSetTimeToMap( pMan, clock() - clk );

    Mntk_ManSetGlobalCones( pMan, fGlobalCones );
    Mntk_ManSetUseThree( pMan, fUseThree );
    Mntk_ManSetCritWindow( pMan, CritWindow );
    Mntk_ManSetConeDepth( pMan, ConeDepth );
    Mntk_ManSetConeSize( pMan, ConeSize );
    Mntk_ManSetDelayLimit( pMan, DelayLimit );   
    Mntk_ManSetAreaLimit( pMan, AreaLimit );  
    Mntk_ManSetTimeLimit( pMan, TimeLimit );  

    // perform the mapping
    if ( !Mntk_Resynthesize( pMan ) )
    {
        Mntk_ManFree( pMan );
        return NULL;
    }
    Mntk_ManCleanData( pMan );

    // derive the new network
clk = clock();
    pNetMap = Ntk_NetworkFromMntk( pMan, pNet );
Mntk_ManSetTimeToNet( pMan, clock() - clk );
    // set the total runtime
Mntk_ManSetTimeTotal( pMan, clock() - clkTotal );

    if ( pFileNameOut != 0 )
        Io_NetworkWriteMappedBlif( pNetMap, pFileNameOut );

    if ( fVerbose )
        Ntk_NetworkMapStats( pNetMap );

    // print stats and get rid of the mapping info
//    if ( fVerbose )
        Mntk_ManPrintTimeStats( pMan );
    Mntk_ManFree( pMan );
    return pNetMap;
}
*/

/**Function*************************************************************

  Synopsis    [Constructs the MVSIS network for the best mapping in the manager.]

  Description [Assuming that the manager contains full information about 
  the mapping, which was performed, this procedure creates the MVSIS network,
  which correspondes to the selected mapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Man_t * Ntk_NetworkToMntk( Ntk_Network_t * pNet, int fVerbose )
{
    extern Mio_Library_t * s_pLib;
    char ** ppOutputNames;
    Ntk_Node_t * pNode, * pDriver, ** pArray;
    Mntk_Node_t * gNode, * gConst1;
    Mntk_Man_t * p;
    Mntk_Node_t * pFanins[MNTK_MAX_FANINS];
    Mntk_Node_t ** pInputs, ** pOutputs;
    Ntk_NodeBinding_t * pBinding;
    int i, nFanins, nInputs, nOutputs;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only map binary networks.\n" );
        return NULL;
    }

    if ( Ntk_NetworkHasDupFanins( pNet ) )
    {
        printf( "Currently the resynthesizer cannot process mapped nodes with duplicated fanins.\n" );    
        printf( "This message may also appear when one fanin is the inverter of another fanin.\n" );   
        return NULL;
    }

    if ( s_pSuperLib == NULL && s_pLib )
    {
        printf( "The supergate library is not specified but the genlib library %s is loaded.\n", Mio_LibraryReadName(s_pLib) );
        printf( "Simple supergates will be generated automatically from the genlib library.\n" );
        Map_SuperLibDeriveFromGenlib( s_pLib );
    }

    if ( !Ntk_NetworkIsMapped( pNet ) )
    {
        printf( "The network is not mapped using a standard cell library.\n" );
        printf( "The gates from the current library will be attached to the nodes.\n" );
        if ( !Ntk_NetworkAttach( pNet ) )
            return NULL;
    }

    // start the manager
    nInputs  = Ntk_NetworkReadCiNum(pNet);
    nOutputs = Ntk_NetworkReadCoNum(pNet);
    p = Mntk_ManStart( nInputs, nOutputs, 1000, fVerbose );
    pInputs  = Mntk_ManReadInputs( p );
    pOutputs = Mntk_ManReadOutputs( p );
    gConst1  = Mntk_ManReadConst1( p );
    // set the primary inputs
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        MNTK_STRASH_WRITE( pNode, pInputs[i++] );
    assert( i == nInputs );
    // go through the internal nodes
    Ntk_NetworkDfs( pNet, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        if ( Ntk_NodeReadFaninNum(pNode) == 0 )
        { // treat the case of constant node
            if ( Ntk_NodeReadConstant(pNode) == 1 )
                MNTK_STRASH_WRITE( pNode, gConst1 );
            else
                MNTK_STRASH_WRITE( pNode, Mntk_Not(gConst1) );
            continue;
        }
        if ( Ntk_NodeReadFaninNum(pNode) < 2 )
            continue;
        // get the binding
        pBinding = Ntk_NodeReadMap( pNode );
        pArray = Ntk_NodeBindingReadFanInArray( pBinding ); 
        // save the array
        nFanins = Ntk_NodeReadFaninNum( pNode );
        for ( i = 0; i < nFanins; i++ )
            pFanins[i] = Ntk_ReadNodeWithPhase_rec( pArray[i] );
//            pFanins[i] = MNTK_STRASH_READ( pArray[i] );
        gNode = Mntk_NodeAlloc( p, pFanins, nFanins, (Mio_Gate_t *)Ntk_NodeBindingReadGate(pBinding) );
/*
        {
            int k, m;
            for ( k = 0; k < nFanins; k++ ) 
            for ( m = k+1; m < nFanins; m++ ) 
                if ( Mntk_Regular(pFanins[k]) == Mntk_Regular(pFanins[m]) )
                    printf( "Node %8s. Fanins = %d. Fanin (%8s,%8s) is duplicated.\n", 
                        Ntk_NodeGetNamePrintable(pNode), nFanins, 
                        Ntk_NodeGetNamePrintable(pArray[k]), 
                        Ntk_NodeGetNamePrintable(pArray[m]) );
        }
*/
        Mntk_NodeRegister( gNode );
        MNTK_STRASH_WRITE( pNode, gNode );
    }
    // set the primary outputs
    i = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
        pOutputs[i++] = Ntk_ReadNodeWithPhase_rec( pDriver );
//        pOutputs[i++] = MNTK_STRASH_READ( pDriver );
    assert( i == nOutputs );

    
    // set the arrival times of the leaves
    Ntk_NodeSetInputArrivalTimes( p, pNet );

    // collect output names
    ppOutputNames = ALLOC( char *, Ntk_NetworkReadCoNum(pNet) );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        ppOutputNames[i++] = pNode->pName;
    Mntk_ManSetOutputNames( p, ppOutputNames );
    return p;
}

/**Function*************************************************************

  Synopsis    [Constructs the MVSIS network for the best mapping in the manager.]

  Description [Assuming that the manager contains full information about 
  the mapping, which was performed, this procedure creates the MVSIS network,
  which correspondes to the selected mapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkFromMntk( Mntk_Man_t * pMan, Ntk_Network_t * pNet )
{
    Mio_Gate_t * pGateInv = Mio_LibraryReadInv(Mntk_ManReadGenLib(pMan));
    float DelayInv = (float)Mio_GateReadDelayMax( pGateInv );
    Ntk_NodeBinding_t * pBinding;
    Map_Time_t * pTime;
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNode, * pNodeNew, * pNodePoNew;
    Ntk_Latch_t * pLatch, * pLatchNew;
    Mntk_Node_t ** ppInputs;
    Mntk_Node_t ** ppOutputs;
    int nInputs, nOutputs, i;

    // allocate the empty network
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );

    // register the name 
    if ( pNet->pName )
        pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNet->pName );
    // register the network spec file name
    if ( pNet->pSpec )
        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, pNet->pSpec );

    // copy default arrival times
    pNetNew->dDefaultArrTimeRise = pNet->dDefaultArrTimeRise;
    pNetNew->dDefaultArrTimeFall = pNet->dDefaultArrTimeFall;

    // copy and add the CI nodes
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }

    // set the PI nodes in the mapping nodes
    nInputs = Ntk_NetworkReadCiNum( pNet );
    ppInputs = Mntk_ManReadInputs( pMan );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        Mntk_NodeSetData1( ppInputs[i++], (char *)pNode->pCopy );
    assert( i == nInputs );

    // map the nodes in DFS order starting from the POs
    // the resulting nodes are added to the network and save in mapping nodes
    nOutputs = Ntk_NetworkReadCoNum( pNet );
    ppOutputs = Mntk_ManReadOutputs( pMan );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // get the node representing the mapped functionality of this output
        pNodeNew = Ntk_NodeFromMntk_rec( pMan, ppOutputs[i], pNetNew );
        pTime = Mntk_NodeReadTimeArrival( Mntk_Regular(ppOutputs[i]), !Mntk_IsComplement(ppOutputs[i]) );
        // if this node is a CI, we need to add a buffer/inverter depending on polarity
        if ( pNodeNew->Type == MV_NODE_CI )
        {
            // Since a CI node cannot be a CO node, we need to add buffer or inverter
            // We always add an inverter in this case. If we required a buffer then we add a
            // second inverter
            
            pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, pNodeNew, 1 );
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
//            Ntk_NodeSetMap( pNodeNew, Ntk_NodeBindingCreate( (char *)Mio_LibraryReadInv(Mntk_ManReadGenLib(pMan)), pNodeNew ) );
            pBinding = Ntk_NodeBindingCreate( (char *)pGateInv, pNodeNew, pTime->Worst + DelayInv );
            Ntk_NodeSetMap( pNodeNew, pBinding );

            if ( !Mntk_IsComplement(ppOutputs[i]) )
            {
                // Add second inverter to get buffer
                pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, pNodeNew, 1 );
                Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
//                Ntk_NodeSetMap( pNodeNew, Ntk_NodeBindingCreate( (char *)Mio_LibraryReadInv(Mntk_ManReadGenLib(pMan)), pNodeNew ) );
                pBinding = Ntk_NodeBindingCreate( (char *)pGateInv, pNodeNew, pTime->Worst + 2 * DelayInv );
                Ntk_NodeSetMap( pNodeNew, pBinding );
            }
        }
        Ntk_NodeAssignName( pNodeNew, Ntk_NetworkRegisterNewName(pNetNew, pNode->pName) );
        // add a new PO node
        pNodePoNew = Ntk_NetworkAddNodeCo( pNetNew, pNodeNew, 1 );
        // set subtype of the PO node
        pNodePoNew->Subtype = pNode->Subtype;
        // remember the new node in the old node
        pNode->pCopy = pNodePoNew;
        i++;
    }
    assert( i == nOutputs );

    // copy and add the latches
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

#if 0
    // write the network before reodering fanins
    if ( fWrite && pNet->pSpec )
    {
        char * pFileName, * pFileGeneric;
        pFileGeneric = Extra_FileNameGeneric( pNet->pSpec );
        pFileName = Extra_FileNameAppend( pFileGeneric, "_m.blif" );
        free( pFileGeneric );
        // write the network
        Io_WriteNetwork( pNetNew, pFileName, IO_FILE_BLIF, 1 );
        printf( "The mapped network was written into file \"%s\".\n", pFileName );
    }
#endif

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );

    // here is a subtle point: 
    // if we order fanins, the mapping info is lost
    // if we don't order fanins, the network check will fail
    // in this case, node collapsing (eliminate, etc) 
    // cannot be performed but other optimization are fine
    Ntk_NetworkOrderFanins( pNetNew );

    if ( !Ntk_NetworkCheck( pNetNew ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkFromMntk(): Network check has failed.\n" );

    return pNetNew;
}

/**Function*************************************************************

  Synopsis    [Constructs one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NodeFromMntk_rec( Mntk_Man_t * pMan, Mntk_Node_t * pMapNode, Ntk_Network_t * pNet )
{
    Mio_Gate_t * pGateInv = Mio_LibraryReadInv(Mntk_ManReadGenLib(pMan));
    Ntk_NodeBinding_t * pBinding;
    Map_Time_t * pTime;
    Mntk_Node_t ** ppLeaves;
    Mntk_Node_t * pMapNodeR;
    Ntk_Node_t * pFanin, * pResNode;
    int nLeaves, fCompl, i;
    Vm_VarMap_t * pVm;
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** ppIsets;

    // get the real node and its complemented attribute
    pMapNodeR = Mntk_Regular(pMapNode);
    fCompl = Mntk_IsComplement(pMapNode);

    // if the positive phase of the node is needed and it is mapped, return it
    if ( fCompl && (pResNode = (Ntk_Node_t *)Mntk_NodeReadData0(pMapNodeR)) )
        return pResNode;
    // if the negative phase of the node is needed and it is mapped, return it
    if ( !fCompl && (pResNode = (Ntk_Node_t *)Mntk_NodeReadData1(pMapNodeR)) )
        return pResNode;

    // the node is the constant node, return a constant node
    if ( Mntk_NodeIsConst(pMapNodeR) ) // constant node
    {
        // create the constant node
        pResNode = Ntk_NodeCreateConstant( pNet, 2, 2-fCompl ); // constant
        Ntk_NetworkAddNode( pNet, pResNode, 1 );
        pBinding = Ntk_NodeBindingCreate( (char *)NULL, pResNode, 0.0 );
        Ntk_NodeSetMap( pResNode, pBinding );
        // set this node in the corresponding phase
        // in the future, if this phase is needed, it will be returned
        if ( fCompl )
            Mntk_NodeSetData0( pMapNodeR, (char *)pResNode );
        else
            Mntk_NodeSetData1( pMapNodeR, (char *)pResNode );
        return pResNode;
    }

    // get the cut and other info corresponding to this polarity
    if ( fCompl )
    { // the node is complemented
        // implement the positive phase
        pResNode = Ntk_NodeFromMntk_rec( pMan, Mntk_Not(pMapNode), pNet );
        assert( pResNode );
        // add the inverter to derive the negative phase
        pResNode = Ntk_NodeCreateOneInputBinary( pNet, pResNode, 1 );
        Ntk_NetworkAddNode( pNet, pResNode, 1 );
//        Ntk_NodeSetMap( pResNode, Ntk_NodeBindingCreate( (char *)Mio_LibraryReadInv(Mntk_ManReadGenLib(pMan)), pResNode ) );
        pTime = Mntk_NodeReadTimeArrival( pMapNodeR, 0 );
        pBinding = Ntk_NodeBindingCreate( (char *)pGateInv, pResNode, pTime->Worst );
        Ntk_NodeSetMap( pResNode, pBinding );

        // set the node
        Mntk_NodeSetData0( pMapNodeR, (char *)pResNode );
        return pResNode;
    }

    // start the new node
    pResNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, 2 );
    // collect the network nodes corresponding to the fanins of the gate
    nLeaves  = Mntk_NodeReadLeavesNum( pMapNodeR );
    ppLeaves = Mntk_NodeReadLeaves( pMapNodeR );
    for ( i = 0; i < nLeaves; i++ )
    {
        pFanin = Ntk_NodeFromMntk_rec( pMan, ppLeaves[i], pNet );
        Ntk_NodeAddFanin( pResNode, pFanin );
    }

    // derive the functionality
    pVm = Vm_VarMapCreateBinary( Ntk_NetworkReadManVm(pNet), nLeaves, 1 );
    Ntk_NodeSetFuncVm( pResNode, pVm );    
    // create Cvr for this node
    ppIsets = ALLOC( Mvc_Cover_t *, 2 );
    ppIsets[0] = NULL; 
    ppIsets[1] = Mvc_CoverDup( Mio_GateReadMvc( Mntk_NodeReadGate(pMapNodeR) ) );
    pCvr = Cvr_CoverCreate( pVm, ppIsets );
    // set the current representation at the node
    Ntk_NodeWriteFuncVm( pResNode, pVm );
    Ntk_NodeWriteFuncCvr( pResNode, pCvr );
    // save the pointer to the gate in the node
//    Ntk_NodeSetMap( pResNode, Ntk_NodeBindingCreate( (char *)Mntk_NodeReadGate(pMapNodeR), pResNode ));
    pTime = Mntk_NodeReadTimeArrival( pMapNodeR, 1 );
    pBinding = Ntk_NodeBindingCreate( (char *)Mntk_NodeReadGate(pMapNodeR), pResNode, pTime->Worst );
    Ntk_NodeSetMap( pResNode, pBinding );
    // add the node to the network
    Ntk_NetworkAddNode( pNet, pResNode, 1 );

    // save the node's implementation in the required polarity
    Mntk_NodeSetData1( pMapNodeR, (char *)pResNode );
    // return the resulting implementation
    return pResNode;
}

/**Function*************************************************************

  Synopsis    [Get the RESM node with phase.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Ntk_ReadNodeWithPhase_rec( Ntk_Node_t * pNode )
{
    Mntk_Node_t * gNode;
    // stop at large nodes
    if ( Ntk_NodeReadFaninNum(pNode) != 1 )
        return MNTK_STRASH_READ(pNode);
    // stop at the PIs
    if ( pNode->Type == MV_NODE_CI )
        return MNTK_STRASH_READ(pNode);
    // get the fanin's node
    gNode = Ntk_ReadNodeWithPhase_rec( Ntk_NodeReadFaninNode(pNode,0) );
    if ( Ntk_NodeIsBinaryBuffer(pNode) )
        return gNode;
    // complement if it is the invertor
    return Mntk_Not(gNode);
}

/**Function*************************************************************

  Synopsis    [Get the RESM node with phase.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_ReadNonTrivialNode_rec( Ntk_Node_t * pNode )
{
    // stop at large nodes
    if ( Ntk_NodeReadFaninNum(pNode) > 1 )
        return pNode;
    // stop at the PIs
    if ( pNode->Type == MV_NODE_CI )
        return pNode;
    return Ntk_ReadNonTrivialNode_rec( Ntk_NodeReadFaninNode(pNode,0) );
}

/**Function*************************************************************

  Synopsis    [Get the RESM node with phase.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkHasDupFanins( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Ntk_Node_t * pFanins[100];
    int nFanins, i, k;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        nFanins = Ntk_NodeReadFaninNum(pNode);
        if ( nFanins < 2 )
            continue;
        if ( nFanins > 100 )
            return 1;
        // get the fanin array
        Ntk_NodeReadFanins( pNode, pFanins );
        // convert each node in the array
        for ( i = 0; i < nFanins; i++ )
            pFanins[i] = Ntk_ReadNonTrivialNode_rec( pFanins[i] );
        // compare the nodes pair-wise
        for ( i = 0; i < nFanins; i++ )
            for ( k = i + 1; k < nFanins; k++ )
                if ( pFanins[i] == pFanins[k] )
                    return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the delay of the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetInputArrivalTimes( Mntk_Man_t * pMan, Ntk_Network_t * pNet )
{
    Map_Time_t * pInputArrivals;
    Ntk_Node_t * pNode;
    int i;

    // get the arrival times of the leaves
    pInputArrivals = Mntk_ManReadInputArrivals( pMan );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pInputArrivals[i].Rise  = (float)Ntk_NodeReadArrTimeRise( pNode );
        pInputArrivals[i].Fall  = (float)Ntk_NodeReadArrTimeFall( pNode );
        pInputArrivals[i].Worst = (pInputArrivals[i].Rise > pInputArrivals[i].Fall)? 
                                   pInputArrivals[i].Rise : pInputArrivals[i].Fall;
        i++;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


