/**CFile****************************************************************

  FileName    [ntkiMap.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Converts the network to and from the tech-mapping data structure.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiMap.c,v 1.34 2005/07/08 01:01:22 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "mio.h"
#include "mapper.h"
#include "mntk.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Map_Man_t * Ntk_NetworkNet2MapTrust( Ntk_Network_t * pNet );

// procedure provided by the mapping manager to convert to Mntk network
extern Mntk_Man_t * Map_ConvertMappingToMntk( Map_Man_t * pMan );
// procedure to conver Mntk to MVSIS network
extern Ntk_Network_t * Ntk_NetworkFromMntk( Mntk_Man_t * pMan, Ntk_Network_t * pNet );

// loads the arrival times from MVSIS network into the mapper
static void Ntk_NodeSetInputArrivalTimes( Map_Man_t * pMapMan, Ntk_Network_t * pNet );

static int s_Time = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs technology mapping using a library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkMap( Ntk_Network_t * pNet, char * pFileNameOut, int fTrust, int fFuncRed, 
    int fChoicing, int fAreaRecovery, bool fVerbose, bool fSweep, bool fEnableRefactoring, 
    int nIterations, bool fEnableADT, float DelayTarget, int fUseSops )
{
    extern Mio_Library_t * s_pLib;
    Fraig_Man_t * pManFraig;
    Mntk_Man_t * pManMntk;
    Map_Man_t * pMan;
    Ntk_Network_t * pNetMap = NULL, * pNetNew = NULL;
    Ntk_Node_t * pNode;
    char ** ppOutputNames;
    int * pInputArrivalsDiscrete;
    double dAndGateDelay = 1.0;
    int clk, clkTotal = clock();
    int fFeedBack = 1; // use solver feedback
    int fDoSparse = 1; // no sparse functions
    int fBalance  = 0; // no balancing 
    int i;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only map binary networks.\n" );
        return NULL;
    }

    // check that the library is available
    if ( s_pLib == NULL )
    {
        printf( "The current library is not available.\n" );
        return 0;
    }

    // derive the supergate library
    if ( s_pSuperLib == NULL && s_pLib )
    {
        printf( "The supergate library is not specified but the genlib library %s is loaded.\n", Mio_LibraryReadName(s_pLib) );
        printf( "Simple supergates will be generated automatically from the genlib library.\n" );
        Map_SuperLibDeriveFromGenlib( s_pLib );
    }

    // get the delay of the inverter
    dAndGateDelay = Mio_LibraryReadDelayNand2Max( s_pLib );
    assert( dAndGateDelay > 0.0 );

clk = clock();
    if ( fTrust )
    {
        pMan = Ntk_NetworkNet2MapTrust( pNet );
        if ( pMan == NULL )
            return NULL;
    }
    else
    {
    // derive the FRAIG manager containing the network
        if ( (fFuncRed && fChoicing) || !fUseSops )
            pManFraig = Ntk_NetworkFraig( pNet, fFuncRed, fBalance, fDoSparse, fChoicing, 0 );
        else
        { // decompose the network using SOPs
            pNetNew = Ntk_NetworkBalance( pNet, dAndGateDelay );
            pManFraig = Ntk_NetworkFraig( pNetNew, fFuncRed, fBalance, fDoSparse, fChoicing, 0 );
            Ntk_NetworkDelete( pNetNew );
        }

        // Add associative choices to the Fraig
        if ( fEnableADT )
        {
    //        Fraig_ManAddChoices( pManFraig, fVerbose, 6 );
        }
        else if ( fEnableRefactoring )
        {
    //        Fraig_AddChoices( pManFraig );
        }

        // discretize the arrival times for balancing FRAIGs
        pInputArrivalsDiscrete = ALLOC( int, Ntk_NetworkReadCiNum(pNet) );
        i = 0;
        Ntk_NetworkForEachCi( pNet, pNode )
            pInputArrivalsDiscrete[i++] = Ntk_NodeGetArrivalTimeDiscrete( pNode, dAndGateDelay );

        // load FRAIG into the mapping manager
        if ( fFuncRed && fChoicing )
            pMan = Map_ManDupFraig( pManFraig );
        else
            pMan = Map_ManBalanceFraig( pManFraig, pInputArrivalsDiscrete );
        free( pInputArrivalsDiscrete );
        if ( pMan == NULL )
        {
            Fraig_ManSetVerbose(pManFraig, 0);
            Fraig_ManFree( pManFraig );
            return NULL;
        }
        Fraig_ManSetVerbose( pManFraig, 0 );
        Fraig_ManFree( pManFraig );
    }
Map_ManSetTimeToMap( pMan, clock() - clk );

    // set mapping parameters
    Map_ManSetObeyFanoutLimits( pMan, 0 );
    Map_ManSetNumIterations( pMan, nIterations );
    Map_ManSetAreaRecovery( pMan, fAreaRecovery );
    Map_ManSetDelayTarget( pMan, DelayTarget );
    Map_ManSetVerbose( pMan, fVerbose );

    // get the arrival times of the leaves
    Ntk_NodeSetInputArrivalTimes( pMan, pNet );

    // collect output names
    ppOutputNames = ALLOC( char *, Ntk_NetworkReadCoNum(pNet) );
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        ppOutputNames[i++] = pNode->pName;
    Map_ManSetOutputNames( pMan, ppOutputNames );

    // perform the mapping
    if ( !Map_Mapping( pMan ) )
    {
        Map_ManFree( pMan );
        return NULL;
    }
    Map_ManCleanData( pMan );

    // derive the new network
clk = clock();
    pManMntk = Map_ConvertMappingToMntk( pMan );
    pNetMap = Ntk_NetworkFromMntk( pManMntk, pNet );
    Mntk_ManFree( pManMntk );
Map_ManSetTimeToNet( pMan, clock() - clk );

clk = clock();
    if ( fSweep )
    {
        Net_NetworkEquiv( pNetMap, 1, 1, 0, 0 );        
        Ntk_NetworkSweep( pNetMap, 0, 0, 0, 0 );
    }
Map_ManSetTimeSweep( pMan, clock() - clk );

    // set the total runtime
Map_ManSetTimeTotal( pMan, clock() - clkTotal );
s_Time = clock() - clkTotal;

    // write the network into file if requested
    if ( pFileNameOut != NULL )
        Io_NetworkWriteMappedBlif( pNetMap, pFileNameOut );

    // print the area stats
    if ( Map_ManReadVerbose(pMan) )
        Ntk_NetworkMapStats( pNetMap );

    // print stats and get rid of the mapping info
    if ( Map_ManReadVerbose(pMan) )
        Map_ManPrintTimeStats( pMan );

    Map_ManFree( pMan );
    return pNetMap;
}

/**Function*************************************************************

  Synopsis    [Returns the delay of the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeSetInputArrivalTimes( Map_Man_t * pMan, Ntk_Network_t * pNet )
{
    Map_Time_t * pInputArrivals;
    Ntk_Node_t * pNode;
    int i;

    // get the arrival times of the leaves
    pInputArrivals = Map_ManReadInputArrivals( pMan );
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



/**Function*************************************************************

  Synopsis    [Get the FRAIG node with phase.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkIsMapped( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Ntk_NodeBinding_t * pBinding;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) <= 1 )
            continue;
        // get the binding of this node
        pBinding = (Ntk_NodeBinding_t *)Ntk_NodeReadMap( pNode );
        if ( pBinding == NULL )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Print area stats of the mapped network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkMapStats( Ntk_Network_t * pNetNew )
{
    int nInvs = 0;
    float AreaTotal = 0.0;
    Ntk_Node_t * pNode, * pDriver;
    Ntk_NodeBinding_t * pBinding;
    float ArrivalMax, ArrivalCur;

    // count the area, the number of inverters, etc
    AreaTotal = 0;
    nInvs = 0;
    Ntk_NetworkForEachNode( pNetNew, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) == 0 )
            continue;
        nInvs += (Ntk_NodeReadFaninNum( pNode ) == 1);
        pBinding = (Ntk_NodeBinding_t *)Ntk_NodeReadMap( pNode );
        if ( pBinding )
            AreaTotal += (float)Mio_GateReadArea( (Mio_Gate_t *)Ntk_NodeBindingReadGate(pBinding) );
    }

    // get the maximum delay
    ArrivalMax = 0.0;
    Ntk_NetworkForEachCoDriver( pNetNew, pNode, pDriver )
    {
        pBinding = (Ntk_NodeBinding_t *)Ntk_NodeReadMap( pDriver );
        if ( pBinding )
        {
            ArrivalCur = Ntk_NodeBindingReadArrival( pBinding );
            ArrivalMax = (ArrivalMax > ArrivalCur)? ArrivalMax : ArrivalCur;
        }
    }

    // print the stats
    printf( "Area = %5.2f. ", AreaTotal );
    printf( "Gates = %d. ", Ntk_NetworkReadNodeIntNum(pNetNew) );
    printf( "Invs = %d.   ", nInvs );
    printf( "Max delay = %4.2f.", ArrivalMax );
    printf( "\n" );

    Map_ManPrintStatsToFile( pNetNew->pName, AreaTotal, ArrivalMax, s_Time );
}


#define NTK_MAP_READ(pNode)      ((Map_Node_t *)pNode->pCopy)
#define NTK_MAP_WRITE(pNode,p)   (pNode->pCopy = (Ntk_Node_t *)p)

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Man_t * Ntk_NetworkNet2MapTrust( Ntk_Network_t * pNet )
{
    Map_Man_t * pMan;
    ProgressBar * pProgress;
    Map_Node_t ** pgInputs, ** pgOutputs, * gNode, * gNode2;
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
    pMan = Map_ManCreate( nInputs, nOutputs, 0 );
    pgInputs  = Map_ManReadInputs( pMan );
    pgOutputs = Map_ManReadOutputs( pMan );

    // clean the data
    Ntk_NetworkForEachNode( pNet, pNode )
        pNode->pCopy = NULL;

    // set the leaves
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        gNode = pgInputs[i++];
        NTK_MAP_WRITE( pNode, gNode );
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
            gNode = Map_ManReadConst1(pMan);
            if ( Ntk_NodeReadConstant(pNode) == 1 )
                NTK_MAP_WRITE( pNode, gNode );
            else
                NTK_MAP_WRITE( pNode, Map_Not(gNode) );
        }
        else if ( Ntk_NodeReadFaninNum(pNode) == 1 ) // buf or inv
        {
            pFanin = Ntk_NodeReadFaninNode(pNode, 0);
            gNode = NTK_MAP_READ(pFanin);
            if ( Ntk_NodeIsBinaryBuffer(pNode) )
                NTK_MAP_WRITE( pNode, gNode );
            else
                NTK_MAP_WRITE( pNode, Map_Not(gNode) );
        }
        else if ( Mvc_CoverReadCubeNum(pMvc) == 1 ) // AND
        {
            assert( Ntk_NodeReadFaninNum(pNode) == 2 );

            // get the first fanin
            pFanin  = Ntk_NodeReadFaninNode(pNode, 0);
            gNode  = NTK_MAP_READ(pFanin);
            Value = Mvc_CubeVarValue( Mvc_CoverReadCubeHead(pMvc), 0 );
            assert( Value == 1 || Value == 2 );
            if ( Value == 1 )
                gNode = Map_Not( gNode );

            // get the second fanins
            pFanin2 = Ntk_NodeReadFaninNode(pNode, 1);
            gNode2 = NTK_MAP_READ(pFanin2);
            Value = Mvc_CubeVarValue( Mvc_CoverReadCubeHead(pMvc), 1 );
            assert( Value == 1 || Value == 2 );
            if ( Value == 1 )
                gNode2 = Map_Not( gNode2 );

            // create the AND node
            NTK_MAP_WRITE( pNode, Map_NodeAnd(pMan, gNode, gNode2) );
        }
        else // OR
        {
            // select the node with the smallest logic level
            gNode = NULL;
            FaninSel = -1;
            iFanin = 0;
            Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            {
                gNode2 = NTK_MAP_READ(pFanin);
                if ( gNode == NULL || Map_NodeReadLevel(gNode) > Map_NodeReadLevel(gNode2) )
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
                gNode2 = NTK_MAP_READ(pFanin);
                if ( gNode == gNode2 )
                    continue;
                Map_NodeSetChoice( pMan, gNode, gNode2 );
            }

            // set the output of this node
            NTK_MAP_WRITE( pNode, Map_NotCond(gNode, fCompl) );
        }

        if ( ++i % 1000 == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );

    }
    Extra_ProgressBarStop( pProgress );

    // set outputs in the FRAIG manager
    i = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        gNode = NTK_MAP_READ( pDriver );
//        Map_ManSetPo( pMan, gNode );  // Map_Ref( gNode );
        // no need to ref because the node is held by the driver
        pgOutputs[i++] = gNode;
    }
    // make sure that choicing went well
    assert( Map_ManCheckConsistency( pMan ) );
    return pMan;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


