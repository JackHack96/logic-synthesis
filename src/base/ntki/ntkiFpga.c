/**CFile****************************************************************

  FileName    [ntkiFpga.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Performs mapping into variable-sized-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiFpga.c,v 1.13 2005/07/08 01:01:22 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "fraig.h"
#include "fpga.h"
#include "res.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Fpga_Man_t *    Ntk_NetworkNet2FpgaTrust( Ntk_Network_t * pNet );
static Ntk_Network_t * Ntk_NetworkConvertToNetFpga( Fpga_Man_t * pMan, Ntk_Network_t * pNet, int fVerbose );
static void            Ntk_NetworkMapAreaStatsFpga( Ntk_Network_t * pNetNew, Fpga_Man_t * pMan );
static Ntk_Node_t *    Ntk_NetworkMapNodeFpga_rec( Fpga_Man_t * pMan, Fpga_Node_t * pMapNode, Ntk_Network_t * pNet );
static Mvc_Cover_t *   Mvc_CoverCreateFromTruthTable( Vm_VarMap_t * pVm, Mvc_Manager_t * pMan, unsigned * uTruth, int nVars );
static float           Ntk_NodeGetArrivalTimeFpga( Ntk_Node_t * pNode );

static int             Ntk_NetworkIsKFeasible( Ntk_Network_t * pNet );
static void            Ntk_NetworkSetInitialMapping( Ntk_Network_t * pNet, Fpga_Man_t * pMan );

// temporary storage for the Mvc_Data_t objects, used to derive SOPs from truth tables
static Mvc_Data_t *    s_pDatas[10] = {NULL}; 
static Mvc_Cover_t *   s_pMvcs[10] = {NULL}; 
// procedure to deallocate the data objects
static void            Ntk_NetworkFreeData();

//static s_DupCounter = 0;

//char * pNetName;
//int TotalLuts;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs technology mapping for variable-size-LUTs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkFpga( Ntk_Network_t * pNet, bool fTrust, int fFuncRed, int fChoicing, int fPower, int fAreaRecovery, bool fVerbose, int fResynthesis, float DelayLimit, float AreaLimit, float TimeLimit, int fGlobalCones, int fSequential )
{
    Fraig_Man_t * pManFraig;
    Ntk_Network_t * pNetMap = NULL;
    Ntk_Node_t * pNode;
    char ** ppOutputNames;
    Fpga_Man_t * pMan;
    int * pInputArrivals = NULL;
    int clk, clkTotal = clock();
    int fFeedBack = 1; // use solver feedback
    int fDoSparse = 1; // no sparse functions
    int fBalance  = 0; // no balancing 
    int i;

//    pNetName = pNet->pSpec;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only map binary networks.\n" );
        return NULL;
    }

    if ( fResynthesis && TimeLimit && fChoicing )
    {
        printf( "Cannot use the current mapping when choicing is enabled.\n" );
        return NULL;
    }
    if ( fResynthesis && TimeLimit && !Ntk_NetworkIsKFeasible( pNet ) )
    {
        printf( "The network is not K-feasible according to the selected LUT library.\n" );
        printf( "Cannot use the current LUT-mapping as the starting point.\n" );
        return NULL;
    }

clk = clock();
    if ( fTrust )
    {
        pMan = Ntk_NetworkNet2FpgaTrust( pNet );
        if ( pMan == NULL )
            return NULL;
    }
    else
    {
        // derive the FRAIG manager containing the network
        pManFraig = Ntk_NetworkFraig( pNet, fFuncRed, fBalance, fDoSparse, fChoicing, 0 );
        // load FRAIG into the mapping manager
        if ( fResynthesis && TimeLimit ) 
        {
            pMan = Fpga_ManDupFraig( pManFraig );
            Ntk_NetworkSetInitialMapping( pNet, pMan );
        }
        else
        {
            if ( fFuncRed && fChoicing )
                pMan = Fpga_ManDupFraig( pManFraig );
            else
                pMan = Fpga_ManBalanceFraig( pManFraig, pInputArrivals );
            if ( pMan == NULL )
            {
                Fraig_ManSetVerbose(pManFraig, 0);
                Fraig_ManFree( pManFraig );
                return NULL;
            }
        }
        Fraig_ManSetVerbose( pManFraig, 0 );
        Fraig_ManFree( pManFraig );
    }
Fpga_ManSetTimeToMap( pMan, clock() - clk );

    Fpga_ManSetPower( pMan, fPower );
    Fpga_ManSetAreaRecovery( pMan, fAreaRecovery );
    Fpga_ManSetResyn( pMan, fResynthesis );
    Fpga_ManSetDelayLimit( pMan, DelayLimit );
    Fpga_ManSetAreaLimit( pMan, AreaLimit );
    Fpga_ManSetTimeLimit( pMan, TimeLimit );
    Fpga_ManSetVerbose( pMan, fVerbose );
    Fpga_ManSetLatchNum( pMan, Ntk_NetworkReadLatchNum(pNet) );
    Fpga_ManSetSequential( pMan, fSequential );
    Fpga_ManSetName( pMan, pNet->pSpec );


    // collect output names
    ppOutputNames = ALLOC( char *, Ntk_NetworkReadCoNum(pNet) );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        ppOutputNames[i++] = pNode->pName;
    Fpga_ManSetOutputNames( pMan, ppOutputNames );

    // perform the mapping
    if ( fResynthesis && TimeLimit ) 
    {
        Res_Resynthesize( pMan, DelayLimit, AreaLimit, TimeLimit, fGlobalCones );
    }
    else
    {
        if ( !Fpga_Mapping( pMan ) )
        {
            Fpga_ManFree( pMan );
            return NULL;
        }
    }
    Fpga_ManCleanData0( pMan );

clk = clock();
//Fpga_ManGetTrueSupports( pMan );
//PRT( "supports", clock() - clk );

    // derive the new network
clk = clock();
//    s_DupCounter = 0;
    pNetMap = Ntk_NetworkConvertToNetFpga( pMan, pNet, fVerbose );
Fpga_ManSetTimeToNet( pMan, clock() - clk );
    // set the total runtime
Fpga_ManSetTimeTotal( pMan, clock() - clkTotal );

    // print stats and get rid of the mapping info
    if ( fVerbose )
    {
        Ntk_NetworkMapAreaStatsFpga( pNetMap, pMan );
        Fpga_ManPrintTimeStats( pMan );
    }
    Fpga_ManFree( pMan );
    return pNetMap;
}



/**Function*************************************************************

  Synopsis    [Constructs the MVSIS network with the currrently selected mapping.]

  Description [Assuming that the manager contains full information about 
  the mapping, which was performed, this procedure creates the MVSIS network,
  which correspondes to the selected mapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkConvertToNetFpga( Fpga_Man_t * pMan, Ntk_Network_t * pNet, int fVerbose )
{
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNode, * pNodeNew, * pNodePoNew;
//    Ntk_Node_t * pDriver, * pFanin;
    Ntk_Latch_t * pLatch, * pLatchNew;
    Fpga_Node_t ** ppInputs;
    Fpga_Node_t ** ppOutputs;
    int nInputs, nOutputs, i, nSupers = 0;
    extern bool Ntk_NetworkSweepFpga( Ntk_Network_t * pNet );

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
    ppInputs = Fpga_ManReadInputs( pMan );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
        Fpga_NodeSetData0( ppInputs[i++], (char *)pNode->pCopy );
    assert( i == nInputs );

    // map the nodes in DFS order starting from the POs
    // the resulting nodes are added to the network and save in mapping nodes
    nOutputs = Ntk_NetworkReadCoNum( pNet );
    ppOutputs = Fpga_ManReadOutputs( pMan );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // get the node representing the mapped functionality of this output
        pNodeNew = Ntk_NetworkMapNodeFpga_rec( pMan, Fpga_Regular(ppOutputs[i]), pNetNew );
        // if this node is a CI, we need to add a buffer/inverter depending on polarity
        if ( Fpga_IsComplement(ppOutputs[i]) )
        { // add inverter
            pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, pNodeNew, 1 );
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        }
        if ( pNodeNew->Type == MV_NODE_CI )
        { // add buffer
            pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, pNodeNew, 0 );
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
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
/*
    // collapse the invertors in the POs
    Ntk_NetworkForEachCoDriver( pNetNew, pNode, pDriver )
    {
        pFanin = Ntk_NodeReadFaninNode( pDriver, 0 );
        if ( Ntk_NodeReadFaninNum(pDriver) == 1 && pFanin->Type == MV_NODE_INT )
        {
            // leave inverters at the multiple fanout points
//            if ( Ntk_NodeReadFanoutNum( pFanin ) > 1 )
//                continue;
//            // collapse fanin into the driver
//            Ntk_NetworkCollapseNodes( pDriver, pFanin );
//            // delete the deriver if it has no fanouts
//            Ntk_NetworkDeleteNode( pNetNew, pFanin, 1, 1 );

            // duplicate nodes at the multiple fanout points
            // collapse fanin into the driver
            Ntk_NetworkCollapseNodes( pDriver, pFanin );
            // delete the deriver if it has no fanouts
            if ( Ntk_NodeReadFanoutNum( pFanin ) == 0 )
                Ntk_NetworkDeleteNode( pNetNew, pFanin, 1, 1 );
//            else
//                s_DupCounter++;
        }
    }
*/
    Ntk_NetworkFreeData();
    // collapse the output invertors
    Ntk_NetworkSweepFpga( pNetNew );
    Ntk_NetworkSweep( pNetNew, 1, 1, 0, 0 );
    if ( !Ntk_NetworkCheck( pNetNew ) )
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkDup(): Network check has failed.\n" );

    return pNetNew;
}

/**Function*************************************************************

  Synopsis    [Returns the arrival time of the given PI node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Ntk_NodeGetArrivalTimeFpga( Ntk_Node_t * pNode )
{
    float Rise, Fall;
    Rise = (float)Ntk_NodeReadArrTimeRise( pNode );
    Fall = (float)Ntk_NodeReadArrTimeFall( pNode );
    return Rise > Fall? Rise : Fall;
}

/**Function*************************************************************

  Synopsis    [Print area stats of the mapped network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkMapAreaStatsFpga( Ntk_Network_t * pNetNew, Fpga_Man_t * pMan )
{
    Ntk_Node_t * pNodeNew, * pFanin;
    float AreaTotal = 0.0;
    int pCounters[FPGA_MAX_LUTSIZE] = { 0 };
    int nGatesTotal = 0;
    float * ppAreas;
    int nFanins, i;

    // count the area, the number of inverters, etc
    ppAreas = Fpga_ManReadLutAreas( pMan );
    Ntk_NetworkForEachNode( pNetNew, pNodeNew )
    {
        // skip single input nodes
        nFanins = Ntk_NodeReadFaninNum(pNodeNew);
        if ( nFanins == 0 )
            continue;
        // don't count the buffer of the PI (such nodes produce POs equal to PIs)
        pFanin = Ntk_NodeReadFaninNode(pNodeNew, 0);
        if ( pFanin->Type == MV_NODE_CI && Ntk_NodeIsBinaryBuffer( pNodeNew ) )
            continue;
        // don't count the invertors that drive the POs and 
        // can be collapsed into their driving nodes
        if ( nFanins == 1 && Ntk_NodeReadFanoutNum(pFanin) == 1 && Ntk_NodeIsCoDriver(pNodeNew) )
            continue;

        // count the node with the given number of inputs
        if ( nFanins < FPGA_MAX_LUTSIZE )
            pCounters[nFanins]++;
        AreaTotal += ppAreas[nFanins];
        nGatesTotal++;
    }

//    TotalLuts = nGatesTotal;

    // print the stats
    printf( "AREA = %5.2f. ", AreaTotal );
//    if ( pCounters[1] > 0 )
//        printf( "(Added %2d INVs.) ", pCounters[1] );
//    if ( s_DupCounter > 0 )
//        printf( "(Added %2d duplicated nodes.) ", s_DupCounter );

    printf( "Gates:  " );
    for ( i = 0; i < FPGA_MAX_LUTSIZE; i++ )
        if ( pCounters[i] )
            printf( "%d=%d ", i, pCounters[i] );
    printf( " Total = %d", nGatesTotal );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Construct one node of the mapped network.]

  Description [This procedure transforms the selected cut rooted at 
  AIG node pMapNode into an MVSIS node with the same functionality.
  The new node is added to the MVSIS network under construction.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkMapNodeFpga_rec( Fpga_Man_t * pMan, Fpga_Node_t * pMapNode, Ntk_Network_t * pNet )
{
    Fnc_Manager_t * pFnMan;
    Fpga_Cut_t * pCut;
    Fpga_Node_t ** ppLeaves;
    Ntk_Node_t *  pNetNodes[FPGA_MAX_LUTSIZE];
    Ntk_Node_t * pResNode;
    unsigned uTruth[2];
    unsigned uTruthDc[2];
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** ppIsets;
    Mvc_Cover_t * pMvc, * pMvcDc = NULL;
    Vm_VarMap_t * pVm;
    int nLeaves, i;

    assert( !Fpga_IsComplement(pMapNode) );

    // if the node is mapped, return it
    if ( pResNode = (Ntk_Node_t *)Fpga_NodeReadData0(pMapNode) )
        return pResNode;

    // the node is the constant node, create adn return a constant node
    if ( Fpga_NodeIsConst(pMapNode) ) // constant node
    {
        // create the constant node
        pResNode = Ntk_NodeCreateConstant( pNet, 2, 2 ); // constant
        Ntk_NetworkAddNode( pNet, pResNode, 1 );
        // set this node
        Fpga_NodeSetData0( pMapNode, (char *)pResNode );
        return pResNode;
    }

    // get data about the selected cut
    pCut     = Fpga_NodeReadCutBest( pMapNode );
    nLeaves  = Fpga_CutReadLeavesNum( pCut );
    ppLeaves = Fpga_CutReadLeaves( pCut );

    // implement (or collect) the network nodes corresponding to the fanins of the cut
    for ( i = 0; i < nLeaves; i++ )
        pNetNodes[i] = Ntk_NetworkMapNodeFpga_rec( pMan, ppLeaves[i], pNet );

    // create a new node
    pResNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, 2 );
    // set the fanin (do not connect the fanout to the fanin)
    for ( i = 0; i < nLeaves; i++ )
        Ntk_NodeAddFanin( pResNode, pNetNodes[i] );
    // get the functionality manager
    pFnMan = Ntk_NodeReadMan( pResNode );
    // compute the truth table
    Fpga_TruthsCut( pMan, pCut, uTruth );
    // create the variable map 
    pVm = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pFnMan), nLeaves, 1 );
    pMvc = Mvc_CoverCreateFromTruthTable( pVm, Fnc_ManagerReadManMvc(pFnMan), uTruth, nLeaves );
    // get the don't-care cover
    if ( Fpga_TruthsCutDontCare( pMan, pCut, uTruthDc ) )
        pMvcDc = Mvc_CoverCreateFromTruthTable( pVm, Fnc_ManagerReadManMvc(pFnMan), uTruthDc, nLeaves );
    // create the cover
    ppIsets = ALLOC( Mvc_Cover_t *, 2 );
    ppIsets[0] = NULL;
    ppIsets[1] = Cvr_IsetEspresso( pVm, pMvc, pMvcDc, 0, 0, 0 );
    pCvr = Cvr_CoverCreate( pVm, ppIsets );
    // set the variable map and the relation
    Ntk_NodeWriteFuncVm( pResNode, pVm );
    Ntk_NodeWriteFuncCvr( pResNode, pCvr );
    if ( pMvcDc )  Mvc_CoverFree( pMvcDc );

    // add the node to the network
    Ntk_NetworkAddNode( pNet, pResNode, 1 );
    Fpga_NodeSetData0( pMapNode, (char *)pResNode );
    // return the resulting implementation
    return pResNode;
}



/**Function*************************************************************

  Synopsis    [Macro to get the i-th bit of the bit string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#define Fpga_TruthHasBit(p,i)     ((p[(i)>>5] & (1<<((i) & 31))) > 0)


/**Function*************************************************************

  Synopsis    [Constructs the data object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Data_t * Ntk_NetworkGetData( Vm_VarMap_t * pVm, Mvc_Cover_t * pMvc )
{
    int nVars;
    assert( Vm_VarMapReadValuesInNum(pVm) == pMvc->nBits );
    nVars = pMvc->nBits / 2;
    if ( s_pDatas[nVars] == NULL )
    {
        s_pDatas[nVars] = Mvc_CoverDataAlloc( pVm, pMvc ); 
        s_pMvcs[nVars] = Mvc_CoverDup( pMvc );
    }
    return s_pDatas[nVars];
}

/**Function*************************************************************

  Synopsis    [Removes all data objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFreeData()
{
    int i;
    for ( i = 0; i < 10; i++ )
        if ( s_pDatas[i] )
        {
            Mvc_CoverDataFree( s_pDatas[i], s_pMvcs[i] );
            Mvc_CoverFree( s_pMvcs[i] );
            s_pDatas[i] = NULL;
            s_pMvcs[i] = NULL;
        }
}

/**Function*************************************************************

  Synopsis    [Constructs the cube cover from the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverCreateFromTruthTable( Vm_VarMap_t * pVm, Mvc_Manager_t * pMan, unsigned * puTruth, int nVars )
{
    Mvc_Data_t * pData; 
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pCube;
    int nMints, i, v;

    nMints = (1<<nVars);
    pMvc = Mvc_CoverAlloc( pMan, 2 * nVars );
    for ( i = 0; i < nMints; i++ )
    {
        if ( !Fpga_TruthHasBit( puTruth, i ) )
            continue;
        pCube = Mvc_CubeAlloc( pMvc );
        Mvc_CoverAddCubeTail( pMvc, pCube );
        Mvc_CubeBitClean( pCube );
        for ( v = 0; v < nVars; v++ )
        {
            if ( i & (1<<v) )
                Mvc_CubeBitInsert( pCube, 2*v + 1 ); 
            else
                Mvc_CubeBitInsert( pCube, 2*v ); 
        }
    }
    pData = Ntk_NetworkGetData( pVm, pMvc );
    Mvc_CoverDist1Merge( pData, pMvc );
    return pMvc;
}

/**Function*************************************************************

  Synopsis    [Constructs the cube cover from the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int CountSingles( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int Counter = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        Counter += (Ntk_NodeReadFaninNum(pNode) == 1);
    return Counter;

}

/**Function*************************************************************

  Synopsis    [Performs technology mapping for variable-size-LUTs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSetInitialMapping( Ntk_Network_t * pNet, Fpga_Man_t * pMan )
{
    Ntk_Node_t * pNode, * pFanin;
    Fraig_Node_t * gNode;
    Ntk_Pin_t * pPin;
    int pLeaves[10];
    int nLeaves, iRoot, iNode, v;
    int fPerformMapping = 1;

    // set the starting mapping
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        if ( Ntk_NodeReadFaninNum(pNode) <= 1 )
            continue;
        // collect the fanin numbers
        nLeaves = 0;
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        {
            gNode = NTK_STRASH_READ(pFanin);
            iNode = Fraig_NodeReadNum( Fraig_Regular(gNode) );
            if ( iNode >= 0 )
            {
                // make sure this node is not in the list
                for ( v = 0; v < nLeaves; v++ )
                    if ( pLeaves[v] == iNode )
                        break;
                // check the case when the node is unique
                if ( v == nLeaves )
                    pLeaves[nLeaves++] = iNode;
            }
        }
        // create the mapping
        gNode = NTK_STRASH_READ(pNode);
        iRoot = Fraig_NodeReadNum( Fraig_Regular(gNode) );
        Fpga_CutCreateFromNode( pMan, iRoot, pLeaves, nLeaves );
    }

    if ( fPerformMapping )
    {
        Fpga_ManSetVerbose( pMan, 0 );
        Fpga_Mapping( pMan );
    }
    else
        Fpga_MappingCreatePiCuts( pMan );

    // restore the used cuts
    Fpga_MappingSetUsedCuts( pMan );
}

/**Function*************************************************************

  Synopsis    [Checks if the network is K-feasible using the currently selected library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkIsKFeasible( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int LutMax;
    extern Fpga_LutLib_t * s_pLutLib;
    extern Fpga_LutLib_t s_LutLib;

    // if the LUT library does not exist, create a default one
    if ( s_pLutLib == NULL )
    {
        printf ( "Using default LUT library. Run \"read_lut -h\" for details.\n" );
        s_pLutLib = Fpga_LutLibDup( &s_LutLib ); 
    }

    if ( s_pLutLib == NULL )
    {
        printf( "The LUT-library is not selected.\n" );
        return 0;
    }

    LutMax = Fpga_LibReadLutMax(s_pLutLib);
    Ntk_NetworkForEachNode( pNet, pNode )
        if ( Ntk_NodeReadFaninNum(pNode) > LutMax )
            return 0;

    return 1;
}



#define NTK_FPGA_READ(pNode)      ((Fpga_Node_t *)pNode->pCopy)
#define NTK_FPGA_WRITE(pNode,p)   (pNode->pCopy = (Ntk_Node_t *)p)

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Man_t * Ntk_NetworkNet2FpgaTrust( Ntk_Network_t * pNet )
{
    Fpga_Man_t * pMan;
    ProgressBar * pProgress;
    Fpga_Node_t ** pgInputs, ** pgOutputs, * gNode, * gNode2;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pNode, * pDriver, * pFanin, * pFanin2;
    int nInputs, nOutputs, i, Value, iFanin, FaninSel, fCompl, nCubes;
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pCube;

    // check if the internal nodes look like ANDs and ORs
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        nCubes = Cvr_CoverReadCubeNum(Ntk_NodeReadFuncCvr(pNode));
        if ( nCubes == 1 || nCubes == Ntk_NodeReadFaninNum(pNode) )
            continue;
        printf( "The network does not look like an AIG with choices.\n" );
        return NULL;
    }

    // start the strashed network
    nInputs  = Ntk_NetworkReadCiNum(pNet);
    nOutputs = Ntk_NetworkReadCoNum(pNet);

    // start the manager
    pMan = Fpga_ManCreate( nInputs, nOutputs, 0 );
    pgInputs  = Fpga_ManReadInputs( pMan );
    pgOutputs = Fpga_ManReadOutputs( pMan );

    // clean the data
    Ntk_NetworkForEachNode( pNet, pNode )
        pNode->pCopy = NULL;

    // set the leaves
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        gNode = pgInputs[i++];
        NTK_FPGA_WRITE( pNode, gNode );
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

        if ( Ntk_NodeReadFaninNum(pNode) == 0 ) // contant node
        {
            gNode = Fpga_ManReadConst1(pMan);
            if ( Ntk_NodeReadConstant(pNode) == 1 )
                NTK_FPGA_WRITE( pNode, gNode );
            else
                NTK_FPGA_WRITE( pNode, Fpga_Not(gNode) );
        }
        else if ( Ntk_NodeReadFaninNum(pNode) == 1 ) // buf or inv
        {
            pFanin = Ntk_NodeReadFaninNode(pNode, 0);
            gNode = NTK_FPGA_READ(pFanin);
            if ( Ntk_NodeIsBinaryBuffer(pNode) )
                NTK_FPGA_WRITE( pNode, gNode );
            else
                NTK_FPGA_WRITE( pNode, Fpga_Not(gNode) );
        }
        else if ( Mvc_CoverReadCubeNum(pMvc) == 1 ) // AND
        {
            assert( Ntk_NodeReadFaninNum(pNode) == 2 );

            // get the first fanin
            pFanin  = Ntk_NodeReadFaninNode(pNode, 0);
            gNode  = NTK_FPGA_READ(pFanin);
            Value = Mvc_CubeVarValue( Mvc_CoverReadCubeHead(pMvc), 0 );
            assert( Value == 1 || Value == 2 );
            if ( Value == 1 )
                gNode = Fpga_Not( gNode );

            // get the second fanins
            pFanin2 = Ntk_NodeReadFaninNode(pNode, 1);
            gNode2 = NTK_FPGA_READ(pFanin2);
            Value = Mvc_CubeVarValue( Mvc_CoverReadCubeHead(pMvc), 1 );
            assert( Value == 1 || Value == 2 );
            if ( Value == 1 )
                gNode2 = Fpga_Not( gNode2 );

            // create the AND node
            NTK_FPGA_WRITE( pNode, Fpga_NodeAnd(pMan, gNode, gNode2) );
        }
        else // OR
        {
            // select the node with the smallest logic level
            gNode = NULL;
            FaninSel = -1;
            iFanin = 0;
            Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            {
                gNode2 = NTK_FPGA_READ(pFanin);
                if ( gNode == NULL || Fpga_NodeReadLevel(gNode) > Fpga_NodeReadLevel(gNode2) )
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
                gNode2 = NTK_FPGA_READ(pFanin);
                if ( gNode == gNode2 )
                    continue;
                Fpga_NodeSetChoice( pMan, gNode, gNode2 );
            }

            // set the output of this node
            NTK_FPGA_WRITE( pNode, Fpga_NotCond(gNode, fCompl) );
        }

        if ( ++i % 1000 == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );

    }
    Extra_ProgressBarStop( pProgress );

    // set outputs in the FRAIG manager
    i = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        gNode = NTK_FPGA_READ( pDriver );
//        Fpga_ManSetPo( pMan, gNode );  // Fpga_Ref( gNode );
        // no need to ref because the node is held by the driver
        pgOutputs[i++] = gNode;
    }
    // make sure that choicing went well
    assert( Fpga_ManCheckConsistency( pMan ) );
    return pMan;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


