/**CFile****************************************************************

  FileName    [fraigMan.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Implementation of the FRAIG manager.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigMan.c,v 1.11 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

int timeSelect;
int timeAssign;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the new FRAIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Man_t * Fraig_ManCreate()
{
    Fraig_Man_t * p;

    // set the random seed for simulation
    srand( 0xFEEDDEAF );

    // start the manager
    p = ALLOC( Fraig_Man_t, 1 );
    memset( p, 0, sizeof(Fraig_Man_t) );

    // set the default parameters
    p->fFuncRed  = 1;    // enables functional reduction (otherwise, only one-level hashing is performed)
    p->fFeedBack = 1;    // enables solver feedback (the use of counter-examples in simulation)
    p->fDoSparse = 1;    // performs equivalence checking for sparse functions (whose sim-info is 0)
    p->fRefCount = 0;    // disables reference counting (useful for one-pass contruction of FRAIGs)
    p->fChoicing = 0;    // disable accumulation of structural choices (keeps only the first choice)
    p->fVerbose  = 0;    // disable verbose output
//    p->nBTLimit  = -1;    // -1 means infinite backtrack limit
    p->nBTLimit  = 5;    // -1 means infinite backtrack limit
    p->nTravIds  = 1;
    p->nTravIds2 = 1;
    p->fUseImps  = 0;
    p->fBalance  = 1;
    p->nTravIds  = 1;

    // start memory managers
    p->mmNodes  = Fraig_MemFixedStart( sizeof(Fraig_Node_t), 10000, 100 );
    p->mmSims   = Fraig_MemFixedStart( sizeof(Fraig_Sims_t), 10000, 100 );
/*
    {
        int i, v;
        // create and clean simulation info for the 12-variable functions
        for ( i = 0; i < 12; i++ )
        {
            p->pVarSims[i] = (Fraig_Sims_t *)Fraig_MemFixedEntryFetch( p->mmSims );
            memset( p->pVarSims[i], 0, sizeof(Fraig_Sims_t) );
        }
        // set up the simulation
        assert( FRAIG_SIM_ROUNDS == 127 );
        for ( i = 0; i < (1<<12) - 32; i++ )
        for ( v = 0; v < 12; v++ )
            if ( i & (1 << v) )
                Fraig_BitStringSetBit( p->pVarSims[v]->uTests, i );

        for ( i = 0; i < 12; i++ )
        {
//            Extra_PrintBinary( stdout, p->pVarSims[i]->uTests, 60 ); printf( "\n" );
        }
//        printf( "\n" );
    }
*/

    // allocate node arrays
    p->vInputs  = Fraig_NodeVecAlloc( 1000 );    // the array of primary inputs
    p->vNodes   = Fraig_NodeVecAlloc( 1000 );    // the array of internal nodes
    p->vOutputs = Fraig_NodeVecAlloc( 1000 );    // the array of primary outputs

    // start the tables
    p->pTableS  = Fraig_HashTableCreate( 1000 );    // hashing by structure
    p->pTableF  = Fraig_HashTableCreate( 1000 );    // hashing by function
    p->pTableF0 = Fraig_HashTableCreate( 50   );    // hashing by function

    // SAT solver feedback data structures
    Fraig_FeedBackInit( p );
    p->vProj    = Msat_IntVecAlloc( 10 ); 

//    malloc( 1313 );

    // create the constant node
    p->pConst1  = Fraig_NodeCreateConst( p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Man_t * Fraig_ManClone( Fraig_Man_t * pMan )
{
    Fraig_Man_t * pManNew;
    pManNew = Fraig_ManCreate();
    pManNew->fFuncRed   = pMan->fFuncRed;       
    pManNew->fFeedBack  = pMan->fFeedBack;       
    pManNew->fDoSparse  = pMan->fDoSparse;       
    pManNew->fRefCount  = pMan->fRefCount;       
    pManNew->fChoicing  = pMan->fChoicing;       
    pManNew->fVerbose   = pMan->fVerbose;   
    pManNew->nBTLimit   = pMan->nBTLimit;
    return pManNew;
}

/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManFree( Fraig_Man_t * p )
{
    int i;

    if ( p->fVerbose )   
        Fraig_ManPrintTimeStats( p );

    for ( i = 0; i < p->vNodes->nSize; i++ )
    {
        if ( p->vNodes->pArray[i]->vFanins )
        {
            Fraig_NodeVecFree( p->vNodes->pArray[i]->vFanins );
            p->vNodes->pArray[i]->vFanins = NULL;
        }
/*
        if ( p->vNodes->pArray[i]->vFanouts )
        {
            Fraig_NodeVecFree( p->vNodes->pArray[i]->vFanouts );
            p->vNodes->pArray[i]->vFanouts = NULL;
        }
*/
    }

//    if ( p->fVerbose )   Fraig_PrintLevelStats( p );
//    if ( p->fVerbose )   Fraig_PrintChoicingStats( p );
//    if ( p->nResubs )
//printf( "Resub attempts = %d. Resub real = %d.\n", p->nResubs, p->nResubsReal );

//printf( "Calls = %d. Proofs = %d. Counters = %d. Zeros = %d. Fails = %d.\n", 
//       p->nSatCalls, p->nSatProof, p->nSatCounter, p->nSatZeros, p->nSatFails );

    if ( p->fRefCount )  Fraig_ManagerCheckZeroRefs( p );
    if ( p->fVerbose )   
    if ( p->pSat )       Msat_SolverPrintStats( p->pSat );
//    if ( p->fVerbose && p->fChoicing )
    if ( p->fChoicing )
        Fraig_ManReportChoices( p );

//        Fraig_TablePrintStatsS( p );
//        Fraig_TablePrintStatsF( p );
//        Fraig_TablePrintStatsF0( p );

    if ( p->vInputs )    Fraig_NodeVecFree( p->vInputs );
    if ( p->vNodes )     Fraig_NodeVecFree( p->vNodes );
    if ( p->vOutputs )   Fraig_NodeVecFree( p->vOutputs );

    if ( p->pTableS )    Fraig_HashTableFree( p->pTableS );
    if ( p->pTableF )    Fraig_HashTableFree( p->pTableF );
    if ( p->pTableF0 )   Fraig_HashTableFree( p->pTableF0 );

    if ( p->pSat )       Msat_SolverFree( p->pSat );
    if ( p->vProj )      Msat_IntVecFree( p->vProj );
    if ( p->vCones )     Fraig_NodeVecFree( p->vCones );
    if ( p->vPatsReal )  Msat_IntVecFree( p->vPatsReal );

    Fraig_MemFixedStop( p->mmNodes, 0 );
    Fraig_MemFixedStop( p->mmSims, 0 );

    if ( p->pSuppS )
    {
        FREE( p->pSuppS[0] );
        FREE( p->pSuppS );
    }
    if ( p->pSuppF )
    {
        FREE( p->pSuppF[0] );
        FREE( p->pSuppF );
    }

    FREE( p->ppOutputNames );
    FREE( p->ppInputNames );
    FREE( p );
}

 
/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManPrintTimeStats( Fraig_Man_t * p )
{
    double nMemory;
    int clk = clock();

//    Fraig_ManPrintRefs( p );

//    printf( "Number of 0s = %d. Number of 1s = %d.\n", p->nSimplifies0, p->nSimplifies1 );
//    printf( "Implies   0s = %d. Implies   1s = %d.\n", p->nImplies0,    p->nImplies1 );

    nMemory = ((double)(p->vInputs->nSize + p->vNodes->nSize) * 
        (sizeof(Fraig_Node_t) + sizeof(Fraig_Sims_t) /*+ p->nSuppWords*sizeof(unsigned)*/))/(1<<20);
    printf( "Proofs = %d (%d). Counter = %d (%d). Fails = %d (%d). Zeros = %d. Perms = %d. Simwords = %d.\n", 
        p->nSatProof, p->nSatProofImp, 
        p->nSatCounter, p->nSatCounterImp, 
        p->nSatFails, p->nSatFailsImp, 
        p->nSatZeros, p->iWordPerm, FRAIG_SIM_ROUNDS );
    printf( "Nodes = %d (%d). Mux = %d. (Exor = %d.) ClaVars = %d. Mem = %0.2f Mb.\n", 
        Fraig_CountNodes(p,0), p->vNodes->nSize, Fraig_ManCountMuxes(p), Fraig_ManCountExors(p), p->nVarsClauses, nMemory );
    Fraig_PrintTime( "AIG simulation  ", p->timeSims  );
    Fraig_PrintTime( "AIG traversal   ", p->timeTrav  );
    Fraig_PrintTime( "Solver feedback ", p->timeFeed  );
    Fraig_PrintTime( "SAT solving     ", p->timeSat   );
    Fraig_PrintTime( "Network update  ", p->timeToNet );
    Fraig_PrintTime( "TOTAL RUNTIME   ", p->timeTotal );
    if ( p->time1 > 0 ) { Fraig_PrintTime( "time1", p->time1 ); }
    if ( p->time2 > 0 ) { Fraig_PrintTime( "time2", p->time2 ); }
    if ( p->time3 > 0 ) { Fraig_PrintTime( "time3", p->time3 ); }
    if ( p->time4 > 0 ) { Fraig_PrintTime( "time4", p->time4 ); }

    PRT( "Selection ", timeSelect );
    PRT( "Assignment", timeAssign );

//    Fraig_FeedBackTest( p );
//    Fraig_FeedBackCompress( p );
//PRT( "Compressing patterns", clock() - clk );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

