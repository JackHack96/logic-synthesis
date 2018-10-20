/**CFile****************************************************************

  FileName    [aigMan.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [AND-INV graph package with built in sat_sweep.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 10, 2004.]

  Revision    [$Id: aigMan.c,v 1.2 2004/07/29 04:54:47 alanmi Exp $]

***********************************************************************/

#include "aigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads parameters from the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Node_t **   Aig_ManReadInputs ( Aig_Man_t * p )                     { return p->pInputs;    }
Aig_Node_t **   Aig_ManReadOutputs( Aig_Man_t * p )                     { return p->pOutputs;   }
int             Aig_ManReadInputNum( Aig_Man_t * p )                    { return p->nInputs;   }
int             Aig_ManReadOutputNum( Aig_Man_t * p )                   { return p->nOutputs;   }
Aig_Node_t *    Aig_ManReadConst1 ( Aig_Man_t * p )                     { return p->pConst1;    }
bool            Aig_ManReadVerbose( Aig_Man_t * p )                     { return p->fVerbose;   }
char *          Aig_ManReadVarsInt( Aig_Man_t * p )                     { return (char *)p->vVarsInt;   }
char *          Aig_ManReadSat( Aig_Man_t * p )                         { return (char *)p->pSat;   }
void            Aig_ManSetTimeToAig( Aig_Man_t * p, int Time )          { p->timeToAig = Time;  }
void            Aig_ManSetTimeToNet( Aig_Man_t * p, int Time )          { p->timeToNet = Time;  }
void            Aig_ManSetTimeTotal( Aig_Man_t * p, int Time )          { p->timeTotal = Time;  }
void            Aig_ManSetOutputNames( Aig_Man_t * p, char ** ppNames ) { p->ppOutputNames = ppNames; }
void            Aig_ManSetOneLevel( Aig_Man_t * p, int fOneLevel )      { p->fOneLevel = fOneLevel; }


/**Function*************************************************************

  Synopsis    [Reads parameters from the mapping node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *          Aig_NodeReadData0( Aig_Node_t * p )                   { return p->pData0;    }
char *          Aig_NodeReadData1( Aig_Node_t * p )                   { return p->pData1;    }
int             Aig_NodeReadNum( Aig_Node_t * p )                     { return p->Num;       }
Aig_Node_t *    Aig_NodeReadOne( Aig_Node_t * p )                     { return p->p1;        }
Aig_Node_t *    Aig_NodeReadTwo( Aig_Node_t * p )                     { return p->p2;        }
void            Aig_NodeSetData0( Aig_Node_t * p, char * pData )      { p->pData0  = pData;  }
void            Aig_NodeSetData1( Aig_Node_t * p, char * pData )      { p->pData1  = pData;  }


/**Function*************************************************************

  Synopsis    [Create the mapping manager.]

  Description [The number of inputs and outputs is assumed to be
  known is advance. It is much simpler to have them fixed upfront.
  When it comes to representing the object graph in the form of
  AIG, the resulting manager is similar to the regular AIG manager, 
  except that it does not use reference counting (and therefore
  does not have garbage collections). It does have table resizing.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManCreate( int nInputs, int nOutputs, int fVerbose )
{
    Aig_Man_t * p;
    int i;

    // set the random seed for simulation
    srand( time(NULL) );

    // start the manager
    p = ALLOC( Aig_Man_t, 1 );
    memset( p, 0, sizeof(Aig_Man_t) );
    p->fVerbose  = fVerbose;

//    printf( "Node = %d bytes. Cut = %d bytes.\n", sizeof(Aig_Node_t), sizeof(Aig_Sims_t) ); 
    p->mmNodes = Extra_MmFixedStart( sizeof(Aig_Node_t), 10000, 100 );
    p->mmSims  = Extra_MmFixedStart( sizeof(Aig_Sims_t), 10000, 100 );

    // allocate data for SAT solving
    p->pSat      = Sat_SolverAlloc( 1000, 1, 1, 1, 1, 0 );
    p->vNodes    = Aig_NodeVecAlloc( 1000 );   
    p->vVarsInt  = Sat_IntVecAlloc( 1000 );   
    p->vProj     = Sat_IntVecAlloc( 10 );   

    // allocate the support info
    p->nSuppWords = nInputs / 32 + ((nInputs % 32) > 0);
    p->mmSupps = Extra_MmFixedStart( sizeof(unsigned) * p->nSuppWords, 10000, 100 );

    // start the tables
    p->pTableS = Aig_HashTableCreate();       // hashing by structure
    p->pTableF = Aig_HashTableCreate();       // hashing by function
    p->pVec    = Aig_NodeVecAlloc( 10000 );   // hashing by number

    // create the constant node
    p->pConst1 = Aig_NodeCreate( p, NULL, NULL );
    // set the simulation info of the node
    p->pConst1->pSims->fInv  = 1;
    p->pConst1->pSims->uHash = 0;
    for ( i = 0; i < NSIMS; i++ )
        p->pConst1->pSims->uTests[i] = 0;
    // insert it into the hash table
    Aig_HashTableLookupF( p->pTableF, p->pConst1 );

    // create the PI nodes
    p->nInputs = nInputs;
    p->pInputs = ALLOC( Aig_Node_t *, nInputs );
    for ( i = 0; i < nInputs; i++ )
    {
        p->pInputs[i] = Aig_NodeCreate( p, NULL, NULL );
        if ( Aig_HashTableLookupF( p->pTableF, p->pInputs[i] ) )
        {
            // unlikely as it may seem, the PI node may have 
            // exactly the same random test vectors as another PI; re-derive them
            printf( "Warning: Replacing the PI node.\n" );
            i--;
        }
    }

    // create the place for the output nodes
    p->nOutputs = nOutputs;
    p->pOutputs = ALLOC( Aig_Node_t *, nOutputs );
    memset( p->pOutputs, 0, sizeof(Aig_Node_t *) * nOutputs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManFree( Aig_Man_t * p )
{
    if ( p->fVerbose )   Aig_ManPrintTimeStats( p );
    if ( p->fVerbose )   Sat_SolverPrintStats( p->pSat );

    if ( p->pSat )       Sat_SolverFree( p->pSat );
    if ( p->vNodes )     Aig_NodeVecFree( p->vNodes );
    if ( p->vVarsInt )   Sat_IntVecFree( p->vVarsInt );
    if ( p->vProj )      Sat_IntVecFree( p->vProj );

    if ( p->pVec )       Aig_NodeVecFree( p->pVec );
    if ( p->pTableS )    Aig_HashTableFree( p->pTableS );
    if ( p->pTableF )    Aig_HashTableFree( p->pTableF );
    Extra_MmFixedStop( p->mmNodes, 0 );
    Extra_MmFixedStop( p->mmSims, 0 );
    Extra_MmFixedStop( p->mmSupps, 0 );
    FREE( p->pInputs );
    FREE( p->pOutputs );
    FREE( p->ppOutputNames );
    FREE( p );
}


/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPrintTimeStats( Aig_Man_t * p )
{
    printf( "Proofs = %d. Satisfies = %d. Zeros = %d. Nodes = %d. Memory = %0.3f Mb.\n", 
        p->nSatProof, p->nSatCounter, p->nSatZeros, p->pVec->nSize,
        ((double)p->pVec->nSize * (sizeof(Aig_Node_t) + sizeof(Aig_Sims_t)))/(1<<20) );
    PRT( "AIG simulation  ", p->timeSims  );
    PRT( "AIG traversal   ", p->timeTrav  );
    PRT( "SAT solving     ", p->timeSat   );
    PRT( "Network update  ", p->timeToNet );
    PRT( "TOTAL RUNTIME   ", p->timeTotal );
//    PRT( "time1", p->time1 );
//    PRT( "time2", p->time2 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


