/**CFile****************************************************************

  FileName    [resCreate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resCreate.c,v 1.1 2005/01/23 06:59:46 alanmi Exp $]

***********************************************************************/

#include "resInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Res_Man_t * Res_ManStart( Fpga_Man_t * pMan )
{
    Res_Man_t * p;
    int i;

    // start the manager
    p = ALLOC( Res_Man_t, 1 );
    memset( p, 0, sizeof(Res_Man_t) );
    p->pMan     = pMan;
    p->fVerbose = pMan->fVerbose;
//    p->fGlobalCones = 1;

    // allocate memory manager
    p->mmSims   = Extra_MmFixedStart( sizeof(unsigned) * pMan->nSimRounds, 10000, 100 );

    // allocate the support info
    p->nSuppWords = (pMan->nInputs + sizeof(unsigned) * 8) / sizeof(unsigned) / 8;
    p->mmSupps = Extra_MmFixedStart( sizeof(unsigned) * p->nSuppWords, 10000, 100 );


    // add more inputs; rememeber the new node in the old node 
    for ( i = 0; i < pMan->nInputs; i++ )
        pMan->pInputs[i]->pData0 = (char *)Fpga_NodeCreate( p->pMan, NULL, NULL );

    // remember the current point
    if ( p->nVarsRes == 0 )
        p->nVarsRes = pMan->vAnds->nSize;

    // create arrays
    p->vVars    = Sat_IntVecAlloc( 2 * p->nVarsRes + 100 );   
    p->vVarMap  = Sat_IntVecAlloc( 2 * p->nVarsRes + 100 );  
    p->vProj    = Sat_IntVecAlloc( 10 );  
    p->vLits    = Sat_IntVecAlloc( 10 );  
    Sat_IntVecFill( p->vVarMap, p->nVarsRes, -1 );
    // start the solver
    p->pSatImg  = Sat_SolverAlloc( p->nVarsRes+1, 1, 1, 1, 1, 0 );
//    Sat_SolverClean( p->pSatImg, p->nVarsRes+1 );

    // levelize the netlist
    p->vLevels    = Fpga_MappingLevelize( pMan, pMan->vAnds );
    p->vWinNodes  = Fpga_NodeVecAlloc( 200 );
    p->vWinCands  = Fpga_NodeVecAlloc( 200 );
    p->vWinTotal  = Fpga_NodeVecAlloc( 200 );
    p->vWinVisit  = Fpga_NodeVecAlloc( 200 );

    // allocate the stack used for incremental timing propagation
    p->pvStack = ALLOC( Fpga_NodeVec_t *, p->vLevels->nSize );
    for ( i = 0; i < p->vLevels->nSize; i++ )
        p->pvStack[i] = Fpga_NodeVecAlloc( 10 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_ManStop( Res_Man_t * p )
{
    int i;
    Res_ManPrintTimeStats( p );

    Extra_MmFixedStop( p->mmSupps, 0 );
    Extra_MmFixedStop( p->mmSims, 0 );
    if ( p->pSuppF ) FREE( p->pSuppF[0] );
    FREE( p->pSuppF );
    if ( p->pSuppS ) FREE( p->pSuppS[0] );
    FREE( p->pSuppS );
    if ( p->pSimms ) FREE( p->pSimms[0] );
    FREE( p->pSimms );

    Fpga_NodeVecFree( p->vWinNodes  );
    Fpga_NodeVecFree( p->vWinCands  );
    Fpga_NodeVecFree( p->vWinVisit );
    Fpga_NodeVecFree( p->vWinTotal  );

    if ( p->vLevels )
    {         
        if ( p->pvStack )
        {
            for ( i = 0; i < p->vLevels->nSize; i++ )
                Fpga_NodeVecFree( p->pvStack[i] );
            free( p->pvStack );
        }
        Fpga_NodeVecFree( p->vLevels );
    }

    Sat_IntVecFree( p->vLits );  
    Sat_IntVecFree( p->vProj );  
    Sat_IntVecFree( p->vVars );   
    Sat_IntVecFree( p->vVarMap );   
    Sat_SolverFree( p->pSatImg );
    FREE( p );
}


/**Function*************************************************************

  Synopsis    [Prints the statitistic.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_ManPrintTimeStats( Res_Man_t * p )
{
    double nMemory = 0;
    p->pMan->fRequiredGain = p->pMan->fRequiredStart - p->pMan->fRequiredGlo;

//    printf( "\n" );
//    printf( "Memory = %0.3f Mb." );
//    nMemory = ((double)p->vAnds->nSize * (sizeof(Fraig_Node_t) + sizeof(Fraig_Sims_t) + p->nSuppWords*sizeof(unsigned)))/(1<<20);
    printf( "Nodes = %d (%d).  AREA GAIN = %4.2f (%.2f %%)  DELAY GAIN = %4.2f (%.2f %%)\n", 
        p->pMan->vNodesAll->nSize, p->pMan->vAnds->nSize, 
        p->pMan->fAreaGain,     100.0 * p->pMan->fAreaGain / p->pMan->fAreaGlo,
        p->pMan->fRequiredGain, 100.0 * p->pMan->fRequiredGain / p->pMan->fRequiredStart );
    printf( "Nodes = %d. CTry= %d. CTop= %d. CSim= %d. Use= %d. Proof= %d. Counter= %d. Fail= %d.\n", 
        p->nNodesTried, p->nCutsTried, p->nCutsTop, p->nCutsSim, p->nCutsUsed, p->nSatSuccess, p->nSatFailure, p->nFails );
    printf( "CutsUsed0 = %2d. CutsUsed1 = %2d. CutsUsed2 = %2d. CutsUsed3 = %2d.\n", 
        p->nCutsGen[0], p->nCutsGen[1], p->nCutsGen[2], p->nCutsGen[3] );
    PRT( "Logic selection ", p->timeSelect );  // time to select the nodes
    PRT( "Top filtering   ", p->timeFilTop );  // time to filter the node sets
    PRT( "Sim filtering   ", p->timeFilSim );  // time to filter the node sets
    PRT( "Sat filtering   ", p->timeFilSat );  // time to prove the node sets
    PRT( "TOTAL filtering ", p->timeFilTop + p->timeFilSim + p->timeFilSat );
    PRT( "Resubstitution  ", p->timeResub  );  // time to perform resubstitution
    PRT( "Network update  ", p->timeUpdate );  // time to update the network and timing
    PRT( "TOTAL RUNTIME   ", p->timeTotal  );  // the total time
    if ( p->time1 > 0 ) { PRT( "time1", p->time1 ); }
    if ( p->time2 > 0 ) { PRT( "time2", p->time2 ); }
//    printf( "\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


