/**CFile****************************************************************

  FileName    [resmCore.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmCore.c,v 1.2 2005/02/28 05:34:33 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Resm_ResynthesizeInt( Resm_Man_t * p );
static void Resm_ResynthesizeLoop( Resm_Man_t * p );
static void Resm_ResynthesizeStart( Resm_Man_t * p );
static void Resm_ResynthesizeStop( Resm_Man_t * p );
static void Resm_ResynthesizeTestSat( Resm_Man_t * p );
static void Resm_ProcessDupFanins( Resm_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs resynthesis of the mapped netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_Resynthesize( Resm_Man_t * p )
{
//    Resm_ResynthesizeTestSat( p );
//    return 1;
    Resm_ResynthesizeStart( p );
    Resm_ResynthesizeInt( p );
    Resm_ResynthesizeStop( p );
//Resm_NodeBddsFree();
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prepares the mapping data structure for synthesis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_ResynthesizeStart( Resm_Man_t * p )
{
//    Resm_NodeVec_t * vNodes;
    float AreaTemp;
    int i;

    // compute the static area
    p->fAreaStatic = Resm_ComputeStaticArea( p );

    // create the pseudo-node whose fanins are POs
    p->pPseudo = Resm_NodeAlloc( p, p->pOutputs, p->nOutputs, Mio_GateCreatePseudo(p->nOutputs) );
    p->pPseudo->nRefs[1] = 1;

    // create fanouts and references (also computes area)
    p->fAreaGlo = Resm_NodeInsertFaninFanouts( p->pPseudo, 1 );
    assert( Resm_VerifyRefs( p ) );
    // compute area independently
    AreaTemp = Resm_MappingArea( p );
    assert( p->fAreaGlo == AreaTemp );

    // set primary input arrival times
    for ( i = 0; i < p->nInputs; i++ )
        p->pInputs[i]->tArrival = p->pInputArrivals[i];

    // compute the arrival times for all the nodes
    Resm_ComputeArrivalAll( p, 0 );
    printf( "Initial area = %4.2f.  Static area = %4.2f.  Initial arrival time = %4.2f.\n", 
        p->fAreaGlo, p->fAreaStatic, p->pPseudo->tArrival.Worst );

    // set the resynthesis limits
    // if both limits are set to 0, assume resynthesis for area
    if ( p->DelayLimit == 0 && p->AreaLimit == 0 )
        p->DelayLimit = p->pPseudo->tArrival.Worst;
    // if one of the limits is 0, assume this parameter should not be resynthesized
    else if ( p->DelayLimit == 0 && p->AreaLimit > 0 )
        p->DelayLimit = p->pPseudo->tArrival.Worst;
    else if ( p->DelayLimit > 0 && p->AreaLimit == 0 )
        p->AreaLimit = p->fAreaGlo;

    // if the delay limit is larger, relaxing
    if ( p->DelayLimit > p->pPseudo->tArrival.Worst )
    {
        printf( "Relaxing the delay from %4.2f to %4.2f.\n", p->pPseudo->tArrival.Worst, p->DelayLimit );
        p->fRequiredStart = p->DelayLimit;
    }
    else if ( p->DelayLimit < p->pPseudo->tArrival.Worst )
    {
        p->fRequiredStart = p->pPseudo->tArrival.Worst;
        printf( "Performing resynthesis for delay from %4.2f to %4.2f.\n", p->pPseudo->tArrival.Worst, p->DelayLimit );
    }
    else
        p->fRequiredStart = p->pPseudo->tArrival.Worst;


    // compute the required times
    Resm_TimeComputeRequiredGlobal( p );

    // verify the arrival/required times
    Resm_ComputeArrivalAll( p, 1 );
}

/**Function*************************************************************

  Synopsis    [Prepares the mapping data structure for synthesis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_ResynthesizeStop( Resm_Man_t * p )
{
//    Resm_NodeVec_t * vNodes;
    float AreaTemp, AreaTemp2;
    int i; 

    // transfer from the pseudo-gate to the POs
    assert( p->nOutputs == p->pPseudo->nLeaves );
    for ( i = 0; i < p->nOutputs; i++ )
        p->pOutputs[i] = p->pPseudo->ppLeaves[i];
/*
    vNodes = Resm_CollectNodesDfs( p );
    for ( i = 0; i < vNodes->nSize; i++ )
        printf( "(%d,%d) ", vNodes->pArray[i]->nRefs[0], vNodes->pArray[i]->nRefs[1] );
    Resm_NodeVecFree( vNodes );
*/
    // when resynthesis is over, dereference the pseudo-node and compute area
    AreaTemp2 = Resm_MappingArea(p);
    AreaTemp = Resm_NodeRemoveFaninFanouts( p->pPseudo, 1 );
    p->pPseudo->nRefs[1] = 0;
    printf( "Final area, independently computed = %4.2f (%4.2f).\n", AreaTemp2, AreaTemp );

    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        assert( Resm_NodeReadRefTotal( p->vNodesAll->pArray[i] ) == 0 );
}



/**Function*************************************************************

  Synopsis    [Performs resynthesis of the mapped netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Resm_ResynthesizeInt( Resm_Man_t * p )
{
    int clk = clock();

    // initialize the gains to be zero
    p->fAreaGain = 0;
    p->fRequiredGain = 0;

    // determine the window of criticality (expressed in percents of max delay)
    p->fDelayWindow = p->fRequiredGlo * p->CritWindow / (float)100.0;

    // set the runtime limits
    p->timeStart = clock();
    p->timeStop  = clock() + (int)(p->TimeLimit * CLOCKS_PER_SEC);

    // check if delay oriented resynthesis is needed
    if ( clock() < p->timeStop )
    {
        p->fDoingArea = 0;
        if ( p->DelayLimit != 0 )
        {
            if ( p->DelayLimit < p->pPseudo->tArrival.Worst ) // delay is relaxed
                Resm_ResynthesizeLoop( p );
            else // delay is okay
                printf( "Resynthesis for delay is not performed...\n" );
        }
    }
    else
        printf( "Runtime limit is reached. Resynthesis for delay is not performed.\n" );


    // check if resynthesis for area is needed
    if ( clock() < p->timeStop )
    {
        p->fDoingArea = 1;
        if ( p->AreaLimit < p->fAreaGlo - p->fAreaGain  )
        {
            printf( "Performing resynthesis for area from %4.2f to %4.2f...\n", 
                p->fAreaGlo - p->fAreaGain, p->AreaLimit );
            Resm_ResynthesizeLoop( p );
        }
        else // area is okay
            printf( "Resynthesis for area is not performed...\n" );
    }
    else
        printf( "Runtime limit is reached. Resynthesis for area is not performed.\n" );

    // compute images for all the cuts
p->timeTotal = clock() - clk;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs resynthesis for delay.]

  Description []
               

  SeeAlso     []
  SideEffects []

***********************************************************************/
void Resm_ResynthesizeLoop( Resm_Man_t * p )
{
    ProgressBar * pProgress;
    char Buffer[100];
    Resm_Node_t * pNode;
    int fChange, TimePrint, nIter, i;

//    Resm_ProcessDupFanins( p );
    // iterate as long as there are changes
    nIter = 0;
    do
    {
        nIter++;
        // go through the zero-slack nodes
        pProgress = Extra_ProgressBarStart( stdout, p->vNodesAll->nSize );
        TimePrint = clock() + CLOCKS_PER_SEC/4;
        fChange = 0;
        for ( i = 0; i < p->vNodesAll->nSize; i++ )
        {
            if ( i % 10 == 0 && clock() > TimePrint )
            {
                if ( p->fDoingArea )
                    sprintf( Buffer, "Area %5.2f %% ", 100.0 * p->fAreaGain / p->fAreaGlo );
                else
                    sprintf( Buffer, "Delay %5.2f %% ", 100.0 * p->fRequiredGain / p->fRequiredStart );
                Extra_ProgressBarUpdate( pProgress, i, Buffer  );
                TimePrint = clock() + CLOCKS_PER_SEC/4;
            }
            // quit if the runtime limit is reached
            if ( clock() >= p->timeStop )
            {
                fChange = 0;
                printf( "Runtime limit is reached. Quitting resynthesis loop.\n" );
                break;
            }
            // quit if the goal is achieved
            if (  p->fDoingArea && p->fAreaGlo - p->fAreaGain <= p->AreaLimit || 
                 !p->fDoingArea && p->fRequiredGlo <= p->DelayLimit )
            {
                fChange = 0;
                printf( "The goal of resynthesis for %s is achieved.\n", (p->fDoingArea? "area" : "delay") );
                break;
            }
            // get the node
            pNode = p->vNodesAll->pArray[i];
            // resynthesize nodes used in the mapping
            if ( Resm_NodeReadRefTotal(pNode) > 0 && Resm_NodeIsCritical(pNode) )
            {
                assert( pNode->tArrival.Rise < pNode->tRequired[1].Rise + p->fEpsilon );
                assert( pNode->tArrival.Fall < pNode->tRequired[1].Fall + p->fEpsilon );
                // resynthesize the node and mark the change
                if ( Resm_NodeResynthesize( pNode ) )
                    fChange = 1;
            }
        }
        Extra_ProgressBarStop( pProgress );
        printf( "Round %d : Total area = %4.2f. Max arrival = %4.2f.\n", 
            nIter, p->fAreaGlo - p->fAreaGain, p->fRequiredGlo );
        fflush( stdout );
        break; // performs only one loop
    } while ( fChange );
}


/**Function*************************************************************

  Synopsis    [This procedure test SAT-based image computation using topological cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_ResynthesizeTestSat( Resm_Man_t * p )
{
    Resm_Node_t * ppLeaves[10];
    Resm_Node_t * pNode;
    Resm_Node_t * pNodeNew;
    int i, k, fPrint = 1;

    // collect all the nodes
    Resm_NodeVecClear( p->vWinCands );
    Resm_NodeVecClear( p->vWinNodes );
    Resm_NodeVecClear( p->vWinTotal );
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
    {
        pNode = p->vNodesAll->pArray[i];
        if ( !Resm_NodeIsVar(pNode) )
            Resm_NodeVecPush( p->vWinNodes, pNode );
        Resm_NodeVecPush( p->vWinTotal, pNode );
    }

    // simulate the created window
    Resm_SimulateWindow( p );

    // go through the nodes and the cuts of each node
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
    {
        pNode = p->vNodesAll->pArray[i];
        if ( Resm_NodeIsVar(pNode) )
            continue;
        for ( k = 0; k < (int)pNode->nLeaves; k++ )
            ppLeaves[k] = Resm_Regular(pNode->ppLeaves[k]);
        // check the cut (skip the topological test)
        pNodeNew = Resm_CheckCut( pNode, ppLeaves, pNode->nLeaves, 0 );
        if ( pNodeNew == NULL )
        {
            printf( "Did not work for node %d.\n", pNode->Num );
            continue;
        }

        if ( fPrint )
        {
            Extra_PrintBinary( stdout, pNodeNew->uTruthTemp, (1 << pNode->nLeaves) ); printf( "\n" );
            Extra_PrintBinary( stdout, p->uRes0All, (1 << pNode->nLeaves) ); printf( "\n" );
            Extra_PrintBinary( stdout, p->uRes1All, (1 << pNode->nLeaves) ); printf( "\n" );
            printf( "\n" );
        }
        Resm_NodeFree( pNodeNew );
    }
}


/**Function*************************************************************

  Synopsis    [Processes the nodes with duplicated fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_ProcessDupFanins( Resm_Man_t * p )
{
    Resm_Node_t * pNode, * pNodeRet;
    int fAcceptModePrev, fChange, i;
    // synthesize the nodes with duplicated fanins
    fChange = 0;
    fAcceptModePrev = p->fAcceptMode;
    p->fAcceptMode = 2;
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
    {
        pNode = p->vNodesAll->pArray[i];
        if ( !Resm_CheckNoDuplicates( pNode, pNode->ppLeaves, pNode->nLeaves ) )
        {
            printf( "Currently the resynthesizer cannot process mapped nodes with duplicated fanins.\n" );    
            printf( "It may also be the case when one fanins is the inverter of another fanin.\n" );   
            return;

            // because resynthesis may lead to the increase in the max arrival time
            // of the network, set the required time for this not be infinity
            pNode->tRequired[0].Fall   = RESM_FLOAT_LARGE;
            pNode->tRequired[0].Rise   = RESM_FLOAT_LARGE;
            pNode->tRequired[0].Worst  = RESM_FLOAT_LARGE;
            pNode->tRequired[1].Fall   = RESM_FLOAT_LARGE;
            pNode->tRequired[1].Rise   = RESM_FLOAT_LARGE;
            pNode->tRequired[1].Worst  = RESM_FLOAT_LARGE;
            // resynthesize the node under the relaxed conditions
            pNodeRet = Resm_NodeResynthesizeFanin( pNode, NULL );
            if ( pNodeRet == NULL )
            {
                printf( "Cannot remove duplicated fanins. Quitting...\n" );    
                printf( "Make sure that the supergate library is rich enough.\n" );    
                return;
            }
            fChange++;
        }
    }
    p->fAcceptMode = fAcceptModePrev;
    if ( fChange )
        printf( "Before resynthesis, replaced %d nodes with duplicated fanins.\n", fChange );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


