/**CFile****************************************************************

  FileName    [remCreate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic resynthesis engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: resmCreate.c,v 1.1 2005/01/23 06:59:50 alanmi Exp $]

***********************************************************************/

#include "resmInt.h"

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
Resm_Node_t *   Resm_ManReadConst1( Resm_Man_t * p )                      { return p->pConst1;    }
Resm_Node_t **  Resm_ManReadInputs ( Resm_Man_t * p )                     { return p->pInputs;    }
Resm_Node_t **  Resm_ManReadOutputs( Resm_Man_t * p )                     { return p->pOutputs;   }
Mio_Library_t * Resm_ManReadGenLib ( Resm_Man_t * p )                     { return Map_SuperLibReadGenLib(p->pSuperLib); }
Map_Time_t *    Resm_ManReadInputArrivals( Resm_Man_t * p )               { return p->pInputArrivals;}
bool            Resm_ManReadVerbose( Resm_Man_t * p )                     { return p->fVerbose;   }
void            Resm_ManSetTimeToMap( Resm_Man_t * p, int Time )          { p->timeToMap = Time;  }
void            Resm_ManSetTimeToNet( Resm_Man_t * p, int Time )          { p->timeToNet = Time;  }
void            Resm_ManSetTimeTotal( Resm_Man_t * p, int Time )          { p->timeTotal = Time;  }
void            Resm_ManSetOutputNames( Resm_Man_t * p, char ** ppNames ) { p->ppOutputNames = ppNames; }
void            Resm_ManSetVerbose( Resm_Man_t * p, int fVerbose )        { p->fVerbose = fVerbose;     }   
void            Resm_ManSetGlobalCones( Resm_Man_t * p, int fGlobalCones ){ p->fGlobalCones = fGlobalCones; }   
void            Resm_ManSetUseThree( Resm_Man_t * p, int fUseThree )      { p->fUseThree = fUseThree;   }   
void            Resm_ManSetCritWindow( Resm_Man_t * p, int CritWindow )   { p->CritWindow = CritWindow; }   
void            Resm_ManSetConeDepth( Resm_Man_t * p, int ConeDepth )     { p->ConeDepth = ConeDepth;   }   
void            Resm_ManSetConeSize( Resm_Man_t * p, int ConeSize )       { p->ConeSize = ConeSize;     }   
void            Resm_ManSetDelayLimit( Resm_Man_t * p, float DelayLimit ) { p->DelayLimit = DelayLimit; }   
void            Resm_ManSetAreaLimit( Resm_Man_t * p, float AreaLimit )   { p->AreaLimit  = AreaLimit;  }
void            Resm_ManSetTimeLimit( Resm_Man_t * p, float TimeLimit )   { p->TimeLimit  = TimeLimit;  }

Mio_Gate_t *    Resm_NodeReadGate( Resm_Node_t * p )                      { return p->pGate;      }
int             Resm_NodeReadLeavesNum( Resm_Node_t * p )                 { return p->nLeaves;    }
Resm_Node_t **  Resm_NodeReadLeaves( Resm_Node_t * p )                    { return p->ppLeaves;   }
char *          Resm_NodeReadData0( Resm_Node_t * p )                     { return p->pData[0];   }
char *          Resm_NodeReadData1( Resm_Node_t * p )                     { return p->pData[1];   }
int             Resm_NodeReadNum( Resm_Node_t * p )                       { return p->Num;        }
void            Resm_NodeSetData0( Resm_Node_t * p, char * pData )        { p->pData[0] = pData;  }
void            Resm_NodeSetData1( Resm_Node_t * p, char * pData )        { p->pData[1] = pData;  }
int             Resm_NodeIsConst( Resm_Node_t * p )                       { return (int)(p->Num == -1); }
int             Resm_NodeIsVar( Resm_Node_t * p )                         { return (int)(p->nLeaves==0 && p->Num >=0); }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Resm_Man_t * Resm_ManStart( int nInputs, int nOutputs, int nNodesMax, int fVerbose )
{
    extern Map_SuperLib_t * s_pSuperLib;
    Resm_Man_t * p;
    int i;

    // derive the supergate library
    if ( s_pSuperLib == NULL )
    {
        printf( "The supergate library is not specified. Use \"read_library\" or \"read_super\".\n" );
        return NULL;
    }

//    srand( time(NULL) );

    // start the manager
    p = ALLOC( Resm_Man_t, 1 );
    memset( p, 0, sizeof(Resm_Man_t) );
    p->pSuperLib    = s_pSuperLib;
    p->nVarsMax     = Map_SuperLibReadVarsMax(p->pSuperLib);
    p->AreaInv      = Map_SuperLibReadAreaInv(p->pSuperLib);
    p->tDelayInv    = Map_SuperLibReadDelayInv(p->pSuperLib);
    Map_MappingSetupTruthTables( p->uTruths );

    p->nVarsRes     = nNodesMax;
    p->nSimRounds   = RESM_SIM_ROUNDS;
    p->fVerbose     = fVerbose;
    p->fGlobalCones = 1;
    p->fEpsilon     = (float)0.00001;

    // allocate memory managers
    p->mmNodes  = Extra_MmFixedStart( sizeof(Resm_Node_t),              10000, 100 );
    p->mmSims   = Extra_MmFixedStart( sizeof(unsigned) * p->nSimRounds, 10000, 100 );

    // allocate array for all nodes
    p->vNodesAll = Resm_NodeVecAlloc( 100 );

    // create the constant node
    // make sure the constant node will get index -1
    p->nNodes = -1;
    p->pConst1 = Resm_NodeAlloc( p, NULL, 0, NULL );
    Resm_NodeRegister( p->pConst1 );

    // create the PI nodes
    p->nInputs = nInputs;
    p->pInputs = ALLOC( Resm_Node_t *, nInputs );
    for ( i = 0; i < nInputs; i++ )
    {
        p->pInputs[i] = Resm_NodeAlloc( p, NULL, 0, NULL );
        Resm_NodeRegister( p->pInputs[i] );
    }
    p->pInputArrivals = ALLOC( Map_Time_t, nInputs );
    memset( p->pInputArrivals, 0, sizeof(Map_Time_t) * nInputs );

    // create the place for the output nodes
    p->nOutputs = nOutputs;
    p->pOutputs = ALLOC( Resm_Node_t *, nOutputs );
    memset( p->pOutputs, 0, sizeof(Resm_Node_t *) * nOutputs );

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
//    p->vLevels    = Resm_MappingLevelize( pMan, pMan->vAnds );
    p->vWinNodes  = Resm_NodeVecAlloc( 200 );
    p->vWinCands  = Resm_NodeVecAlloc( 200 );
    p->vWinTotal  = Resm_NodeVecAlloc( 200 );
    p->vWinVisit  = Resm_NodeVecAlloc( 200 );
    p->vWinTotalCone  = Resm_NodeVecAlloc( 200 );
    p->vWinNodesCone  = Resm_NodeVecAlloc( 200 );

    // allocate the stack used for incremental timing propagation
//    p->pvStack = ALLOC( Resm_NodeVec_t *, p->vLevels->nSize );
//    for ( i = 0; i < p->vLevels->nSize; i++ )
//        p->pvStack[i] = Resm_NodeVecAlloc( 10 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_ManFree( Resm_Man_t * p )
{
    int i;
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        Resm_NodeVecFree( p->vNodesAll->pArray[i]->vFanouts );
    Resm_NodeVecFree( p->vNodesAll );

    // remove the nodes that are not in the general list
    if ( p->pPseudo )
        Mio_GateDelete( p->pPseudo->pGate );
    Resm_NodeFree( p->pConst1 );
    Resm_NodeFree( p->pPseudo );

    // remove the memory managers
    Extra_MmFixedStop( p->mmNodes, 0 );
    Extra_MmFixedStop( p->mmSims,  0 );

    Resm_NodeVecFree( p->vWinNodes  );
    Resm_NodeVecFree( p->vWinCands  );
    Resm_NodeVecFree( p->vWinVisit );
    Resm_NodeVecFree( p->vWinTotal  );
    Resm_NodeVecFree( p->vWinTotalCone  );
    Resm_NodeVecFree( p->vWinNodesCone  );
    if ( p->vLevels )
    {         
        if ( p->pvStack )
        {
            for ( i = 0; i < p->vLevels->nSize; i++ )
                Resm_NodeVecFree( p->pvStack[i] );
            free( p->pvStack );
        }
        Resm_NodeVecFree( p->vLevels );
    }

    Sat_IntVecFree( p->vLits );  
    Sat_IntVecFree( p->vProj );  
    Sat_IntVecFree( p->vVars );   
    Sat_IntVecFree( p->vVarMap );   
    Sat_SolverFree( p->pSatImg );
    FREE( p->pInputArrivals );
    FREE( p->pInputs );
    FREE( p->pOutputs );
    FREE( p->ppOutputNames );
    FREE( p );
}

 
/**Function*************************************************************

  Synopsis    [Prints the statitistic.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_ManPrintTimeStats( Resm_Man_t * p )
{
    double nMemory = 0;
//    printf( "RESYNTHESIS STATS:\n" ); 
//    printf( "\n" );
//    printf( "Memory = %0.3f Mb." );
//    nMemory = ((double)p->vAnds->nSize * (sizeof(Fraig_Node_t) + sizeof(Fraig_Sims_t) + p->nSuppWords*sizeof(unsigned)))/(1<<20);
    printf( "Nodes = %d.  AREA GAIN = %4.2f (%.2f %%)  DELAY GAIN = %4.2f (%.2f %%)\n", 
        p->vNodesAll->nSize - p->nInputs, 
        p->fAreaGain,     100.0 * p->fAreaGain / p->fAreaGlo,
        p->fRequiredGain, 100.0 * p->fRequiredGain / p->fRequiredStart );
    printf( "NodesTried = %d. FaninsTried = %d. Use= %d.\n", 
        p->nNodesTried, p->nFaninsTried, p->nCutsUsed );
    printf( "CTry= %d. CTop= %d. CSim= %d. Proof= %d. Counter= %d. Fail= %d.\n", 
         p->nCutsTried, p->nCutsTop, p->nCutsSim, p->nSatSuccess, p->nSatFailure, p->nFails );
    printf( "CutsUsed0 = %2d. CutsUsed1 = %2d. CutsUsed2 = %2d. CutsUsed3 = %2d.\n", 
        p->nCutsGen[0], p->nCutsGen[1], p->nCutsGen[2], p->nCutsGen[3] );
    PRT( "Logic selection ", p->timeLogic  );  // time to select the nodes
    PRT( "Simulation      ", p->timeSimul  );  // time to simulate the nodes
    PRT( "Resubstitution  ", p->timeResub  );  // time to perform resubstitution
//    PRT( "  Top filtering ", p->timeFilTop );  // time to filter the node sets
    PRT( "  Sim filtering ", p->timeFilSim );  // time to filter the node sets
    PRT( "  Sat filtering ", p->timeFilSat );  // time to prove the node sets
    PRT( "  TOT filtering ", p->timeFilTop + p->timeFilSim + p->timeFilSat );
    PRT( "Cut matching    ", p->timeMatch  );  // time to match the cuts
    PRT( "Network update  ", p->timeUpdate );  // time to update the network and timing
    PRT( "TOTAL RUNTIME   ", p->timeTotal  );  // the total time
    if ( p->time1 > 0 ) { PRT( "time1", p->time1 ); }
    if ( p->time2 > 0 ) { PRT( "time2", p->time2 ); }
    if ( p->time3 > 0 ) { PRT( "time3", p->time3 ); }
//    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Get the FRAIG node with phase.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Resm_ManCleanData( Resm_Man_t * p )
{
    int i;
    for ( i = 0; i < p->vNodesAll->nSize; i++ )
        p->vNodesAll->pArray[i]->pData[0] = p->vNodesAll->pArray[i]->pData[1] = NULL;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


