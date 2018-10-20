/**CFile****************************************************************

  FileName    [fmbsSimul.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [Computing subset of the care set using simulation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbsSimul.c,v 1.2 2004/10/18 01:39:23 alanmi Exp $]

***********************************************************************/

#include "fmbsInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Fmbs_SimulateRound( Fmbs_Manager_t * p );

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmbs_Simulation( Fmbs_Manager_t * p )
{
    int nCaresNew, nCaresAll, nMints, i;
    // clean the care set
    nMints = (1<<p->nFanins);
    memset( p->pCareSet, 0, sizeof(char) * nMints );
//    return 0;

    // count the number of care minterms found
    nCaresAll = 0;
    do {
        nCaresNew = 0;
        for ( i = 0; i < 10; i++ )
            nCaresNew += Fmbs_SimulateRound( p );
        nCaresAll += nCaresNew;
    } while ( nCaresNew > 0 );
    return nCaresAll;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmbs_SimulateRound( Fmbs_Manager_t * p )
{
    Sh_Node_t ** ppInputs, ** ppOutputs, ** ppNodes;
    Sh_Node_t * pConst1;
    int nInputs, nOutputs, nNodes;
    unsigned Data, Data1, Data2, MintIn;
    unsigned DataIn[100];
    int nCareNew, i, k;

    ppNodes   = Sh_NetworkReadNodes( p->pShNet );
    nNodes    = Sh_NetworkReadNodeNum( p->pShNet );
    ppInputs  = Sh_NetworkReadInputs( p->pShNet );
    nInputs   = Sh_NetworkReadInputNum( p->pShNet );
    ppOutputs = Sh_NetworkReadOutputs( p->pShNet );
    nOutputs  = Sh_NetworkReadOutputNum( p->pShNet );

    // assign random number to the inputs
    for ( i = 0; i < nInputs; i++ )
    {
        Data = rand();
        Data *= Data;
        Sh_NodeSetData( ppInputs[i], Data );
    }
    // simulate all the way through the nodes
    for ( i = 0; i < nNodes; i++ )
    {
        Data1 = Sh_NodeReadDataCompl( Sh_NodeReadOne(ppNodes[i]) );
        Data2 = Sh_NodeReadDataCompl( Sh_NodeReadTwo(ppNodes[i]) );
        Sh_NodeSetData( ppNodes[i], Data1 & Data2 );
    }

    // consider a special case of simulation
    // when the output of Miter is a constant
    pConst1 = Sh_ManagerReadConst1( Sh_NetworkReadManager(p->pShNet) );
    assert( ppOutputs[nOutputs-1] != Sh_Not(pConst1) );
    if ( ppOutputs[nOutputs-1] == pConst1 )
        Sh_NodeSetData( pConst1, (unsigned)(~0) );

    // get the final data
    Data = Sh_NodeReadDataCompl( ppOutputs[nOutputs-1] );
    if ( Data == 0 )
        return 0;

    // collect the bit pattern from the intermediate nodes
    for ( i = 0; i < nOutputs - 1; i++ )
        DataIn[i] = Sh_NodeReadDataCompl( ppOutputs[i] );

    // get the care bit patterns
    nCareNew = 0;
    for ( i = 0; i < 32; i++ )
    {
        // skip the don't-care pattern
        if ( (Data & (1<<i)) == 0 )
            continue;
        // get the local minterm
        MintIn = 0;
        for ( k = 0; k < nOutputs - 1; k++ )
            if ( DataIn[k] & (1<<i) )
                MintIn |= (1<<k);
        // check if this minterm was already found
        if ( p->pCareSet[MintIn] )
            continue;
        // this is a new care minterm
        p->pCareSet[MintIn] = 1;
        nCareNew++;
    }
    return nCareNew;
}


extern Sh_Node_t * Sh_NodeExistAbstract( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t ** pgVars, int nVars );
extern Sh_Node_t * Sh_NodeExistAbstractOne( Sh_Manager_t * pMan, Sh_Node_t * gNode, Sh_Node_t * pVar );


/**Function*************************************************************

  Synopsis    [Compute complete don't-cares using AIGs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmbs_Computation( Fmbs_Manager_t * p )
{
    Sh_Node_t ** ppInputs, ** ppOutputs, ** ppNodes;
    Sh_Node_t * gConst1, * gRoot, * gMap, * gPart, * gTemp;
    int nInputs, nOutputs, nNodes;
    int i, clk = clock();

    ppNodes   = Sh_NetworkReadNodes( p->pShNet );
    nNodes    = Sh_NetworkReadNodeNum( p->pShNet );
    ppInputs  = Sh_NetworkReadInputs( p->pShNet );
    nInputs   = Sh_NetworkReadInputNum( p->pShNet );
    ppOutputs = Sh_NetworkReadOutputs( p->pShNet );
    nOutputs  = Sh_NetworkReadOutputNum( p->pShNet );
    gConst1   = Sh_ManagerReadConst1( Sh_NetworkReadManager(p->pShNet) );

    // create mapping
    gMap = gConst1;  Sh_Ref( gMap );
    for ( i = 0; i < nOutputs - 1; i++ )
    {
        gPart = Sh_NodeExor( p->pShMan, ppOutputs[i], ppInputs[nInputs + i] ); Sh_Ref( gPart );
        gMap  = Sh_NodeAnd( p->pShMan, Sh_Not(gPart), gTemp = gMap );          Sh_Ref( gMap );
        Sh_RecursiveDeref( p->pShMan, gTemp );
        Sh_RecursiveDeref( p->pShMan, gPart );
    }
    // get the final result
    gRoot = Sh_NodeOr( p->pShMan, Sh_Not(gMap), ppOutputs[nOutputs-1] );    Sh_Ref( gRoot );
//printf( "Map     : Nodes = %7d.\n", Sh_NodeCountNodes(p->pShMan, gMap) );
//printf( "ODC     : Nodes = %7d.\n", Sh_NodeCountNodes(p->pShMan, ppOutputs[nOutputs-1]) );
//printf( "Root    : Nodes = %7d.\n", Sh_NodeCountNodes(p->pShMan, gRoot) );
    Sh_RecursiveDeref( p->pShMan, gMap );


if ( Sh_NodeCountNodes(p->pShMan, gRoot) > 10 )
{
    gTemp = ppOutputs[nOutputs-1];
    ppOutputs[nOutputs-1] = gRoot;
    Sh_NetworkShow( p->pShNet, "cdc_aig" );
    ppOutputs[nOutputs-1] = gTemp;

//    Sh_NodeWriteBlif( p->pShMan, gRoot, "root.blif" );
    printf( "Nodes = %7d.\n", Sh_NodeCountNodes(p->pShMan, gRoot) );
    i = 1;
}

    // quantify
    gRoot = Sh_Not(gRoot);
    for ( i = 0; i < nInputs; i++ )
    {
        gRoot = Sh_NodeExistAbstractOne( p->pShMan, gTemp = gRoot, ppInputs[i] );  Sh_Ref( gRoot );           
        Sh_RecursiveDeref( p->pShMan, gTemp );
//printf( "Iter %2d : Nodes = %7d.\n", i, Sh_NodeCountNodes(p->pShMan, gRoot) );
    }
    gRoot = Sh_Not(gRoot);



    Sh_RecursiveDeref( p->pShMan, gRoot );
//PRT( "AIG-based", clock() - clk );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


