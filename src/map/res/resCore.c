/**CFile****************************************************************

  FileName    [resCore.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resCore.c,v 1.1 2005/01/23 06:59:46 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Res_ResynthesizeLoop( Res_Man_t * p, float DelayLimit, float AreaLimit );
static void Res_ResynthesizePrepare( Fpga_Man_t * pMan );
static void Res_ResynthesizeTestSat( Res_Man_t * pMan );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs resynthesis of the mapped netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_Resynthesize( Fpga_Man_t * pMan, float DelayLimit, float AreaLimit, float RuntimeLimit, int fGlobalCones )
{
    Res_Man_t * p;
    int clk = clock();

    if ( DelayLimit == 0 )
        DelayLimit = FPGA_FLOAT_LARGE;
    if ( AreaLimit == 0 )
        AreaLimit = FPGA_FLOAT_LARGE;
    if ( RuntimeLimit == 0 )
        RuntimeLimit = FPGA_FLOAT_LARGE;

//    return;

    if ( RuntimeLimit == FPGA_FLOAT_LARGE )
//        RuntimeLimit = 3600 * 24; // seconds
        RuntimeLimit = 100; // seconds

    // we assume that the mapping is assigned but the timing is not set; here we set it
    Res_ResynthesizePrepare( pMan );
    pMan->fRequiredStart = pMan->fRequiredGlo;

//    printf( "\n" );
    printf( "Original: Total area = %4.2f. Max arrival time = %4.2f.\n", 
        pMan->fAreaGlo - pMan->fAreaGain, pMan->fRequiredGlo );

    // start the resynthesis manager
    p = Res_ManStart( pMan );
    p->fGlobalCones = fGlobalCones;
    // compute true supports
    Res_ManGetTrueSupports( p );

/*
    Res_ResynthesizeTestSat( p );
    // compute images for all the cuts
p->timeTotal = clock() - clk;
    Res_ManStop( p );
    return;
*/

    if ( DelayLimit == FPGA_FLOAT_LARGE && AreaLimit == FPGA_FLOAT_LARGE )
    {
        DelayLimit = pMan->fRequiredGlo * (float)0.5;
        AreaLimit  = pMan->fAreaGlo * (float)0.5;
    }

    // determine the window of criticality (5% of max delay)
    pMan->fDelayWindow = pMan->fRequiredGlo * (float)0.02;
    // if the delays in the FPGA library are discrete, make sure the window is at least 1
    if ( Fpga_LutLibDelaysAreDiscrete( pMan->pLutLib ) )
        if ( pMan->fDelayWindow < 1 )
            pMan->fDelayWindow = 1;

    // set the runtime limits
    p->timeStart = clock();
    p->timeStop  = clock() + (int)(RuntimeLimit * CLOCKS_PER_SEC);

    // check if delay oriented resynthesis is needed
    if ( clock() < p->timeStop )
    {
        p->fDoingArea = 0;

        // set area limit to be 50% below the current area
//        DelayLimit = pMan->fRequiredGlo * (float)1.2;
//        DelayLimit = pMan->fRequiredGlo * (float)0.5;

        if ( DelayLimit != FPGA_FLOAT_LARGE )
        {
            if ( pMan->fRequiredGlo < DelayLimit ) // delay is relaxed
            {
                printf( "Relaxing delay from %4.2f to %4.2f...\n", 
                    pMan->fRequiredGlo, DelayLimit );
//                pMan->fRequiredShift = pMan->fRequiredGlo - DelayLimit;
                pMan->fRequiredGlo = DelayLimit;
                Fpga_TimeComputeRequired( pMan, pMan->fRequiredGlo );
            }
            else if ( pMan->fRequiredGlo > DelayLimit ) // delay is tightened
            {
                printf( "Performing resynthesis for delay from %4.2f to %4.2f...\n", pMan->fRequiredGlo, DelayLimit );
//                pMan->fRequiredShift = 0;
                Res_ResynthesizeLoop( p, DelayLimit, AreaLimit );
            }
            else if ( pMan->fRequiredGlo == DelayLimit ) // delay is the same
            {
                printf( "Resynthesis for delay is not performed...\n" );
//                pMan->fRequiredShift = 0;
            }
        }
    }
    else
        printf( "Runtime limit is reached. Resynthesis for delay is not performed.\n" );


    // check if resynthesis for area is needed
    if ( clock() < p->timeStop )
    {
        p->fDoingArea = 1;

        // set area limit to be 50% below the current area
//        AreaLimit = (pMan->fAreaGlo - pMan->fAreaGain) * (float)0.5;

        if ( pMan->fAreaGlo > AreaLimit )
        {
            printf( "Performing resynthesis for area from %4.2f to %4.2f...\n", 
                pMan->fAreaGlo - pMan->fAreaGain, AreaLimit );
            Res_ResynthesizeLoop( p, DelayLimit, AreaLimit );
        }
    }
    else
        printf( "Runtime limit is reached. Resynthesis for area is not performed.\n" );


    printf( "Final area, independently computed = %4.2f.\n", Fpga_MappingArea(pMan) );

    // compute images for all the cuts
p->timeTotal = clock() - clk;
    Res_ManStop( p );
}

/**Function*************************************************************

  Synopsis    [Performs resynthesis for delay.]

  Description []
               

  SeeAlso     []
  SideEffects []

***********************************************************************/
void Res_ResynthesizeLoop( Res_Man_t * p, float DelayLimit, float AreaLimit )
{
    ProgressBar * pProgress;
    Fpga_Node_t * pNode;
    int fChange, TimePrint, nIter, i;
 
//    assert( DelayLimit < p->pMan->fRequiredGlo );
    // iterate as long as there are changes
    nIter = 0;
    do
    {
        nIter++;
        // go through the zero-slack nodes
        pProgress = Extra_ProgressBarStart( stdout, p->pMan->vAnds->nSize );
        TimePrint = clock() + CLOCKS_PER_SEC;
        fChange = 0;
        for ( i = 0; i < p->pMan->vAnds->nSize; i++ )
        {
            if ( i % 10 == 0 && clock() > TimePrint )
            {
                Extra_ProgressBarUpdate( pProgress, i, p->fDoingArea? "Area ..." : "Delay ..." );
                TimePrint = clock() + CLOCKS_PER_SEC;
            }
            // quit if the runtime limit is reached
            if ( clock() >= p->timeStop )
            {
                fChange = 0;
                printf( "Runtime limit is reached. Quitting resynthesis loop.\n" );
                break;
            }
            // quit if the goal is achieved
            if ( p->pMan->fAreaGlo - p->pMan->fAreaGain <= AreaLimit && p->pMan->fRequiredGlo <= DelayLimit )
            {
                fChange = 0;
                printf( "The goal of resynthesis is achieved. Quitting resynthesis loop.\n" );
                break;
            }
            // get the node
            pNode = p->pMan->vAnds->pArray[i];
            // skip nodes that are not used in the mapping
            if ( pNode->nRefs == 0 )
                continue;
            // skip nodes whose level is less than two
            if ( pNode->Level < 2 )
                continue;
//            if ( pNode->pCutBest->nLeaves > 3 )
//                continue;
//            if ( pNode->pCutBest->nLeaves < 3 )
//                continue;

            assert( pNode->pCutBest->tArrival <= pNode->tRequired );
            if ( Res_NodeIsCritical( p, pNode ) )
            {
                // resynthesize the node and mark the change
                if ( Res_NodeResynthesize( p, pNode ) )
                    fChange = 1;
            }
        }
        Extra_ProgressBarStop( pProgress );
        printf( "Round %d : Total area = %4.2f. Max arrival time = %4.2f.\n", 
            nIter, p->pMan->fAreaGlo - p->pMan->fAreaGain, p->pMan->fRequiredGlo );
        fflush( stdout );
    } while ( fChange );
}

/**Function*************************************************************

  Synopsis    [Prepares the mapping data structure for synthesis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_ResynthesizePrepare( Fpga_Man_t * pMan )
{
    Fpga_NodeVec_t * vNodes;
    Fpga_Node_t * pNode;
    int i;

    // clean references and compute them from scretch
    Fpga_MappingSetRefsAndArea( pMan );

    // collect the nodes used in the mapping
    vNodes = Fpga_MappingDfsCuts( pMan );
    // for each cut used in the mapping, in the topological order
    pMan->fAreaGlo = 0;
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pNode = vNodes->pArray[i];
        // compute the arrival times
        pNode->pCutBest->tArrival = Fpga_TimeCutComputeArrival( pMan, pNode->pCutBest );
        // create references and compute area
        pNode->pCutBest->aFlow = Fpga_CutGetAreaRefed(pMan, pNode->pCutBest);
        pMan->fAreaGlo += pMan->pLutLib->pLutAreas[pNode->pCutBest->nLeaves];
        // add the fanouts
//        Fpga_CutInsertFanouts( pMan, pNode, pNode->pCutBest );
    }
    pMan->fAreaGain = 0;
    // verify areas
/*
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        float Area;
        pNode = vNodes->pArray[i];
        Area = Fpga_CutGetAreaRefed(pMan, pNode->pCutBest);
        assert( pNode->pCutBest->aFlow == Area );
    }
*/
    Fpga_NodeVecFree( vNodes );

    if ( pMan->vAnds == NULL )
        pMan->vAnds = Fpga_MappingDfs( pMan, 1 );

    // compute the required times
    Fpga_TimeComputeRequiredGlobal( pMan );
//    pMan->fRequiredShift = 0;

    // clean signatures
    for ( i = 0; i < pMan->vNodesAll->nSize; i++ )
        if ( pMan->vNodesAll->pArray[i]->pCutBest )
            pMan->vNodesAll->pArray[i]->pCutBest->uSign = 0;
}


/**Function*************************************************************

  Synopsis    [This procedure test SAT-based image computation using topological cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_ResynthesizeTestSat( Res_Man_t * p )
{
    Fpga_Node_t * pNode;
    Fpga_Cut_t * pCutNew, * pCut;
    int i, fPrint = 0;

    // go through the nodes and the cuts of each node
    for ( i = 0; i < p->pMan->vAnds->nSize; i++ )
    {
        pNode = p->pMan->vAnds->pArray[i];
        if ( pNode->nRefs == 0 )
            continue;
        if ( pNode->Level < 2 )
            continue;
        for ( pCut = pNode->pCuts->pNext; pCut; pCut = pCut->pNext )
        {
//            if ( pCut->nLeaves > 5 )
//                continue;
            // check the cut (skip the topological test)
            pCutNew = Res_CheckCut( p, pNode, pCut->ppLeaves, pCut->nLeaves, 0 );
            Fpga_CutFree( p->pMan, pCutNew );
            // get the truth table of the cut
            Fpga_TruthsCut( p->pMan, pCut, pCut->uTruthTemp );
            // print the truth tables
            if ( fPrint )
            {
                Extra_PrintBinary( stdout, pCut->uTruthTemp, (1 << pCut->nLeaves) ); printf( "\n" );
                Extra_PrintBinary( stdout, p->uRes0All, (1 << pCut->nLeaves) ); printf( "\n" );
                Extra_PrintBinary( stdout, p->uRes1All, (1 << pCut->nLeaves) ); printf( "\n" );
                printf( "\n" );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


