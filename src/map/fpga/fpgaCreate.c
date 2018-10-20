/**CFile****************************************************************

  FileName    [fpgaCreate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaCreate.c,v 1.9 2005/07/08 01:01:23 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void            Fpga_TableCreate( Fpga_Man_t * p );
static void            Fpga_TableResize( Fpga_Man_t * p );
static Fpga_Node_t *   Fpga_TableLookup( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );

extern Fpga_LutLib_t * s_pLutLib;
extern Fpga_LutLib_t    s_LutLib;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads parameters of the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Fpga_ManReadInputNum( Fpga_Man_t * p )                    { return p->nInputs;    }
int             Fpga_ManReadOutputNum( Fpga_Man_t * p )                   { return p->nOutputs;   }
Fpga_Node_t **  Fpga_ManReadInputs ( Fpga_Man_t * p )                     { return p->pInputs;    }
Fpga_Node_t **  Fpga_ManReadOutputs( Fpga_Man_t * p )                     { return p->pOutputs;   }
Fpga_Node_t *   Fpga_ManReadConst1 ( Fpga_Man_t * p )                     { return p->pConst1;    }
float *         Fpga_ManReadInputArrivals( Fpga_Man_t * p )               { return p->pInputArrivals;}
int             Fpga_ManReadVerbose( Fpga_Man_t * p )                     { return p->fVerbose;   }
float *         Fpga_ManReadLutAreas( Fpga_Man_t * p )                    { return p->pLutLib->pLutAreas;  }
void            Fpga_ManSetTimeToMap( Fpga_Man_t * p, int Time )          { p->timeToMap = Time;  }
void            Fpga_ManSetTimeToNet( Fpga_Man_t * p, int Time )          { p->timeToNet = Time;  }
void            Fpga_ManSetTimeTotal( Fpga_Man_t * p, int Time )          { p->timeTotal = Time;  }
void            Fpga_ManSetOutputNames( Fpga_Man_t * p, char ** ppNames ) { p->ppOutputNames = ppNames; }
void            Fpga_ManSetTree( Fpga_Man_t * p, int fTree )              { p->fTree = fTree;           }
void            Fpga_ManSetPower( Fpga_Man_t * p, int fPower )              { p->fPower = fPower;           }
void            Fpga_ManSetAreaRecovery( Fpga_Man_t * p, int fAreaRecovery ) { p->fAreaRecovery = fAreaRecovery;}
void            Fpga_ManSetResyn( Fpga_Man_t * p, int fResynthesis )         { p->fResynthesis = fResynthesis;  }
void            Fpga_ManSetDelayLimit( Fpga_Man_t * p, float DelayLimit )    { p->DelayLimit   = DelayLimit;    }
void            Fpga_ManSetAreaLimit( Fpga_Man_t * p, float AreaLimit )      { p->AreaLimit    = AreaLimit;     }
void            Fpga_ManSetTimeLimit( Fpga_Man_t * p, float TimeLimit )      { p->TimeLimit    = TimeLimit;     }
void            Fpga_ManSetChoiceNodeNum( Fpga_Man_t * p, int nChoiceNodes ) { p->nChoiceNodes = nChoiceNodes;  }  
void            Fpga_ManSetChoiceNum( Fpga_Man_t * p, int nChoices )         { p->nChoices = nChoices;          }   
void            Fpga_ManSetVerbose( Fpga_Man_t * p, int fVerbose )           { p->fVerbose = fVerbose;          }   
void            Fpga_ManSetLatchNum( Fpga_Man_t * p, int nLatches )          { p->nLatches = nLatches;          }   
void            Fpga_ManSetSequential( Fpga_Man_t * p, int fSequential )     { p->fSequential = fSequential;    }   
void            Fpga_ManSetName( Fpga_Man_t * p, char * pFileName )          { p->pFileName = pFileName;        }   

/**Function*************************************************************

  Synopsis    [Reads the parameters of the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Fpga_LibReadLutMax( Fpga_LutLib_t * pLib )  { return pLib->LutMax; }

/**Function*************************************************************

  Synopsis    [Reads parameters of the mapping node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *          Fpga_NodeReadData0( Fpga_Node_t * p )                   { return p->pData0;    }
//char *          Fpga_NodeReadData1( Fpga_Node_t * p )                   { return p->pData1;    }
int             Fpga_NodeReadNum( Fpga_Node_t * p )                     { return p->Num;       }
int             Fpga_NodeReadLevel( Fpga_Node_t * p )                   { return Fpga_Regular(p)->Level;  }
Fpga_Cut_t *    Fpga_NodeReadCuts( Fpga_Node_t * p )                    { return p->pCuts;     }
Fpga_Cut_t *    Fpga_NodeReadCutBest( Fpga_Node_t * p )                 { return p->pCutBest;  }
Fpga_Node_t *   Fpga_NodeReadOne( Fpga_Node_t * p )                     { return p->p1;        }
Fpga_Node_t *   Fpga_NodeReadTwo( Fpga_Node_t * p )                     { return p->p2;        }
void            Fpga_NodeSetData0( Fpga_Node_t * p, char * pData )      { p->pData0  = pData;  }
//void            Fpga_NodeSetData1( Fpga_Node_t * p, char * pData )      { p->pData1  = pData;  }
void            Fpga_NodeSetNextE( Fpga_Node_t * p, Fpga_Node_t * pNextE ) { p->pNextE = pNextE;}
void            Fpga_NodeSetRepr( Fpga_Node_t * p, Fpga_Node_t * pRepr ) { p->pRepr = pRepr;    }

/**Function*************************************************************

  Synopsis    [Checks the type of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Fpga_NodeIsConst( Fpga_Node_t * p )    {  return (Fpga_Regular(p))->Num == -1;    }
int             Fpga_NodeIsVar( Fpga_Node_t * p )      {  return (Fpga_Regular(p))->p1 == NULL && (Fpga_Regular(p))->Num >= 0; }
int             Fpga_NodeIsAnd( Fpga_Node_t * p )      {  return (Fpga_Regular(p))->p1 != NULL;  }
int             Fpga_NodeComparePhase( Fpga_Node_t * p1, Fpga_Node_t * p2 ) { assert( !Fpga_IsComplement(p1) ); assert( !Fpga_IsComplement(p2) ); return p1->fInv ^ p2->fInv; }

/**Function*************************************************************

  Synopsis    [Reads parameters from the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Fpga_CutReadLeavesNum( Fpga_Cut_t * p )            {  return p->nLeaves;  }
Fpga_Node_t **  Fpga_CutReadLeaves( Fpga_Cut_t * p )               {  return p->ppLeaves; }


/**Function*************************************************************

  Synopsis    [Create the mapping manager.]

  Description [The number of inputs and outputs is assumed to be
  known is advance. It is much simpler to have them fixed upfront.
  When it comes to representing the object graph in the form of
  AIG, the resulting manager is similar to the regular AIG manager, 
  except that it does not use reference counting (and therefore
  does not have garbage collections). It does have table resizing.
  The data structure is more flexible to represent additional 
  information needed for mapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Man_t * Fpga_ManCreate( int nInputs, int nOutputs, int fVerbose )
{
    Fpga_Man_t * p;
    int i;

    // derive the supergate library
//    if ( s_pLutLib == NULL )
//    {
//        printf( "The supergate library is not specified. Use \"read_lut\".\n" );
//        return NULL;
//    }

    // if the LUT library does not exist, create a default one
    if ( s_pLutLib == NULL )
    {
        printf ( "Using default LUT library. Run \"read_lut -h\" for details.\n" );
        s_pLutLib = Fpga_LutLibDup( &s_LutLib ); 
    }

    // start the manager
    p = ALLOC( Fpga_Man_t, 1 );
    memset( p, 0, sizeof(Fpga_Man_t) );
    p->pLutLib   = s_pLutLib;
    p->nVarsMax  = s_pLutLib->LutMax;
    p->fVerbose  = fVerbose;
    p->fAreaRecovery = 1;
    p->fTree     = 0;
    p->fRefCount = 1;
    p->fEpsilon  = (float)0.00001;

    Fpga_TableCreate( p );
//if ( p->fVerbose )
//    printf( "Node = %d (%d) bytes. Cut = %d bytes.\n", sizeof(Fpga_Node_t), FPGA_NUM_BYTES(sizeof(Fpga_Node_t)), sizeof(Fpga_Cut_t) ); 
    p->mmNodes  = Extra_MmFixedStart( FPGA_NUM_BYTES(sizeof(Fpga_Node_t)), 10000, 100 );
    p->mmCuts   = Extra_MmFixedStart( sizeof(Fpga_Cut_t),  10000, 100 );

    assert( p->nVarsMax > 0 );
    Fpga_MappingSetupTruthTables( p->uTruths );

    // make sure the constant node will get index -1
    p->nNodes = -1;
    // create the constant node
    p->pConst1 = Fpga_NodeCreate( p, NULL, NULL );
    p->vNodesAll = Fpga_NodeVecAlloc( 100 );

    // create the PI nodes
    p->nInputs = nInputs;
    p->pInputs = ALLOC( Fpga_Node_t *, nInputs );
    for ( i = 0; i < nInputs; i++ )
        p->pInputs[i] = Fpga_NodeCreate( p, NULL, NULL );
    p->pInputArrivals = ALLOC( float, nInputs );
    memset( p->pInputArrivals, 0, sizeof(float) * nInputs );

    // create the place for the output nodes
    p->nOutputs = nOutputs;
    p->pOutputs = ALLOC( Fpga_Node_t *, nOutputs );
    memset( p->pOutputs, 0, sizeof(Fpga_Node_t *) * nOutputs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManFree( Fpga_Man_t * p )
{
    Fpga_ManStats( p );

//    int i;
//    for ( i = 0; i < p->vNodesAll->nSize; i++ )
//        Fpga_NodeVecFree( p->vNodesAll->pArray[i]->vFanouts );
//    Fpga_NodeVecFree( p->pConst1->vFanouts );
    if ( p->vAnds )    
        Fpga_NodeVecFree( p->vAnds );
    if ( p->vNodesAll )    
        Fpga_NodeVecFree( p->vNodesAll );
    Extra_MmFixedStop( p->mmNodes, 0 );
    Extra_MmFixedStop( p->mmCuts, 0 );
    FREE( p->pInputArrivals );
    FREE( p->pInputs );
    FREE( p->pOutputs );
    FREE( p->pBins );
    FREE( p->ppOutputNames );
    if ( p->pSimInfo )
    {
    FREE( p->pSimInfo[0] );
    FREE( p->pSimInfo );
    }
    FREE( p );
}


/**Function*************************************************************

  Synopsis    [Prints runtime statistics of the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManPrintTimeStats( Fpga_Man_t * p )
{
    extern char * pNetName;
    extern int TotalLuts;
//    FILE * pTable;

    
/*
    pTable = fopen( "stats.txt", "a+" );
    fprintf( pTable, "%s ", pNetName );
    fprintf( pTable, "%.0f ", p->fRequiredGlo );
//    fprintf( pTable, "%.0f ", p->fAreaGlo );//+ (float)nOutputInvs );
    fprintf( pTable, "%.0f ", (float)TotalLuts );
    fprintf( pTable, "%4.2f\n", (float)(p->timeTotal-p->timeToMap)/(float)(CLOCKS_PER_SEC) );
    fclose( pTable );
*/

//    printf( "N-canonical = %d. Matchings = %d.  ", p->nCanons, p->nMatches );
//    printf( "Choice nodes = %d. Choices = %d.\n", p->nChoiceNodes, p->nChoices );
    PRT( "ToMap", p->timeToMap );
    PRT( "Cuts ", p->timeCuts );
    PRT( "Match", p->timeMatch );
    PRT( "Area ", p->timeRecover );
    PRT( "ToNet", p->timeToNet );
    PRT( "TOTAL", p->timeTotal );
    if ( p->time1 ) { PRT( "time1", p->time1 ); }
    if ( p->time2 ) { PRT( "time2", p->time2 ); }
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description [This procedure should be called to create the constant
  node and the PI nodes first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeCreate( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    Fpga_Node_t * pNode;
    // create the node
    pNode = (Fpga_Node_t *)Extra_MmFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Fpga_Node_t) );
    // set very large required time
    pNode->tRequired = FPGA_FLOAT_LARGE;
    pNode->aEstFanouts = -1;
    pNode->p1  = p1; 
    pNode->p2  = p2;
    // set the number of this node
    pNode->Num = p->nNodes++;
    // place to store the fanouts
//    pNode->vFanouts = Fpga_NodeVecAlloc( 5 );
    // store this node in the internal array
    if ( pNode->Num >= 0 )
        Fpga_NodeVecPush( p->vNodesAll, pNode );
    else
        pNode->fInv = 1;
    // set the level of this node
    if ( p1 ) 
    {
        // create the fanout info
        Fpga_NodeAddFaninFanout( Fpga_Regular(p1), pNode );
        Fpga_NodeAddFaninFanout( Fpga_Regular(p2), pNode );
        // compute the level
        pNode->Level = 1 + FPGA_MAX(Fpga_Regular(p1)->Level, Fpga_Regular(p2)->Level);
        pNode->fInv  = Fpga_NodeIsSimComplement(p1) & Fpga_NodeIsSimComplement(p2);
    }
    // reference the inputs (will be used to compute the number of fanouts)
    if ( p->fRefCount )
    {
        if ( p1 ) Fpga_NodeRef(p1);
        if ( p2 ) Fpga_NodeRef(p2);
    }
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Create the unique table of AND gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TableCreate( Fpga_Man_t * pMan )
{
    assert( pMan->pBins == NULL );
    pMan->nBins = Cudd_Prime(50000);
    pMan->pBins = ALLOC( Fpga_Node_t *, pMan->nBins );
    memset( pMan->pBins, 0, sizeof(Fpga_Node_t *) * pMan->nBins );
    pMan->nNodes = 0;
}

/**Function*************************************************************

  Synopsis    [Looks up the AND2 node in the unique table.]

  Description [This procedure implements one-level hashing. All the nodes
  are hashed by their children. If the node with the same children was already
  created, it is returned by the call to this procedure. If it does not exist,
  this procedure creates a new node with these children. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_TableLookup( Fpga_Man_t * pMan, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    Fpga_Node_t * pEnt;
    unsigned Key;

    if ( p1 == p2 )
        return p1;
    if ( p1 == Fpga_Not(p2) )
        return Fpga_Not(pMan->pConst1);
    if ( Fpga_NodeIsConst(p1) )
    {
        if ( p1 == pMan->pConst1 )
            return p2;
        return Fpga_Not(pMan->pConst1);
    }
    if ( Fpga_NodeIsConst(p2) )
    {
        if ( p2 == pMan->pConst1 )
            return p1;
        return Fpga_Not(pMan->pConst1);
    }

    if ( Fpga_Regular(p1)->Num > Fpga_Regular(p2)->Num )
        pEnt = p1, p1 = p2, p2 = pEnt;

    Key = hashKey2( p1, p2, pMan->nBins );
    for ( pEnt = pMan->pBins[Key]; pEnt; pEnt = pEnt->pNext )
        if ( pEnt->p1 == p1 && pEnt->p2 == p2 )
            return pEnt;
    // resize the table
    if ( pMan->nNodes >= 2 * pMan->nBins )
    {
        Fpga_TableResize( pMan );
        Key = hashKey2( p1, p2, pMan->nBins );
    }
    // create the new node
    pEnt = Fpga_NodeCreate( pMan, p1, p2 );
    // add the node to the corresponding linked list in the table
    pEnt->pNext = pMan->pBins[Key];
    pMan->pBins[Key] = pEnt;
    return pEnt;
}


/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TableResize( Fpga_Man_t * pMan )
{
    Fpga_Node_t ** pBinsNew;
    Fpga_Node_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i, clk;
    unsigned Key;

clk = clock();
    // get the new table size
    nBinsNew = Cudd_Prime(2 * pMan->nBins); 
    // allocate a new array
    pBinsNew = ALLOC( Fpga_Node_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Fpga_Node_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < pMan->nBins; i++ )
        for ( pEnt = pMan->pBins[i], pEnt2 = pEnt? pEnt->pNext: NULL; pEnt; 
              pEnt = pEnt2, pEnt2 = pEnt? pEnt->pNext: NULL )
        {
            Key = hashKey2( pEnt->p1, pEnt->p2, nBinsNew );
            pEnt->pNext = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == pMan->nNodes - pMan->nInputs );
    if ( pMan->fVerbose )
    {
//        printf( "Increasing the unique table size from %6d to %6d. ", pMan->nBins, nBinsNew );
//        PRT( "Time", clock() - clk );
    }
    // replace the table and the parameters
    free( pMan->pBins );
    pMan->pBins = pBinsNew;
    pMan->nBins = nBinsNew;
}



/**Function*************************************************************

  Synopsis    [Elementary AND operation on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeAnd( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    Fpga_Node_t * pNode;
    pNode = Fpga_TableLookup( p, p1, p2 );     
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Elementary OR operation on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeOr( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    Fpga_Node_t * pNode;
    pNode = Fpga_Not( Fpga_TableLookup( p, Fpga_Not(p1), Fpga_Not(p2) ) );  
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Elementary EXOR operation on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeExor( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    return Fpga_NodeMux( p, p1, Fpga_Not(p2), p2 );
}

/**Function*************************************************************

  Synopsis    [Elementary MUX operation on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeMux( Fpga_Man_t * p, Fpga_Node_t * pC, Fpga_Node_t * pT, Fpga_Node_t * pE )
{
    Fpga_Node_t * pAnd1, * pAnd2, * pRes;
    pAnd1 = Fpga_TableLookup( p, pC,           pT ); 
    pAnd2 = Fpga_TableLookup( p, Fpga_Not(pC), pE ); 
    pRes  = Fpga_NodeOr( p, pAnd1, pAnd2 );           
    return pRes;
}


/**Function*************************************************************

  Synopsis    [Sets the node to be equivalent to the given one.]

  Description [This procedure is a work-around for the equivalence check.
  Does not verify the equivalence. Use at the user's risk.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeSetChoice( Fpga_Man_t * pMan, Fpga_Node_t * pNodeOld, Fpga_Node_t * pNodeNew )
{
    pNodeNew->pNextE = pNodeOld->pNextE;
    pNodeOld->pNextE = pNodeNew;
    pNodeNew->pRepr  = pNodeOld;
}


    
/**Function*************************************************************

  Synopsis    [Prints some interesting stats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManStats( Fpga_Man_t * p )
{
    FILE * pTable;
    pTable = fopen( "stats.txt", "a+" );
    fprintf( pTable, "%s ", p->pFileName );
    fprintf( pTable, "%4d ", p->nInputs - p->nLatches );
    fprintf( pTable, "%4d ", p->nOutputs - p->nLatches );
    fprintf( pTable, "%4d ", p->nLatches );
    fprintf( pTable, "%7d ", p->vAnds->nSize );
    fprintf( pTable, "%7d ", Fpga_CutCountAll(p) );
    fprintf( pTable, "%2d\n", (int)p->fRequiredGlo );
    fclose( pTable );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

