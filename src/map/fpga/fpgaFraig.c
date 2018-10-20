/**CFile****************************************************************

  FileName    [fpgaFraig.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: fpgaFraig.c,v 1.7 2005/07/08 01:01:23 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int bit_count[256] = {
  0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transfers the contents of the FRAIG manager into the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Man_t * Fpga_ManDupFraig( Fraig_Man_t * pMan )
{
    Fpga_Man_t * pManMap;
    Fraig_Node_t ** ppNodes, ** ppInputs, ** ppOutputs;
    Fraig_Node_t * pNode1, * pNode2, * pNodeE, * pNodeR;
    Fpga_Node_t ** pInputsMap, ** pOutputsMap;
    Fpga_Node_t * pNodeMap, * pNodeMap1, * pNodeMap2, * pNodeMapE, * pNodeMapR;
    int nNodes, nInputs, nOutputs, i, k, fCompl;
    int nChoiceNodes, nChoices;
    unsigned * pSimInfo;
    int nBytes, nTotal, nOnes;

    // get the nodes
    nInputs   = Fraig_ManReadInputNum( pMan );
    nOutputs  = Fraig_ManReadOutputNum( pMan );
    nNodes    = Fraig_ManReadNodeNum( pMan );
    ppInputs  = Fraig_ManReadInputs( pMan );
    ppOutputs = Fraig_ManReadOutputs( pMan );
    ppNodes   = Fraig_ManReadNodes( pMan );

    // start the mapping manager 
    pManMap  = Fpga_ManCreate( nInputs, nOutputs, Fraig_ManReadVerbose(pMan) );
    if ( pManMap == NULL )
        return NULL;

    // read the parameters
    pInputsMap  = Fpga_ManReadInputs( pManMap );
    pOutputsMap = Fpga_ManReadOutputs( pManMap );
    // set the primary inputs
    for ( i = 0; i < nInputs; i++ )
        Fraig_NodeSetData0( ppInputs[i], (Fraig_Node_t *)pInputsMap[i] );
    // create the internal nodes
    for ( i = 1; i < nNodes; i++ )
    {
        if ( Fraig_NodeIsVar(ppNodes[i]) )
            continue;
        pNode1    = Fraig_NodeReadOne(ppNodes[i]);
        pNode2    = Fraig_NodeReadTwo(ppNodes[i]);
        pNodeMap1 = (Fpga_Node_t *)Fraig_NodeReadData0( Fraig_Regular(pNode1) );
        pNodeMap2 = (Fpga_Node_t *)Fraig_NodeReadData0( Fraig_Regular(pNode2) );
        pNodeMap  = Fpga_NodeAnd( pManMap, 
            Fpga_NotCond(pNodeMap1, Fraig_IsComplement(pNode1)), 
            Fpga_NotCond(pNodeMap2, Fraig_IsComplement(pNode2)) ); 
        Fraig_NodeSetData0( ppNodes[i], (Fraig_Node_t *)pNodeMap );
    }
    // create the choice nodes
    nChoices = nChoiceNodes = 0;
    for ( i = 1; i < nNodes; i++ )
    {
        if ( Fraig_NodeIsVar(ppNodes[i]) )
            continue;
        // get the equivalent node for this FRAIG node
        pNodeE = Fraig_NodeReadNextE( ppNodes[i] );
        if ( pNodeE == NULL )
            continue;
        // get the representative node for the equivalent FRAIG node
        pNodeR = Fraig_NodeReadRepr( pNodeE );
        assert( pNodeR );
        // get the three corresponding mapper nodes
        pNodeMap = (Fpga_Node_t *)Fraig_NodeReadData0( ppNodes[i] );
        pNodeMapE = (Fpga_Node_t *)Fraig_NodeReadData0( pNodeE );
        pNodeMapR = (Fpga_Node_t *)Fraig_NodeReadData0( pNodeR );
        // set the equivalent and the representative nodes
        Fpga_NodeSetNextE( pNodeMap, pNodeMapE );
        Fpga_NodeSetRepr( pNodeMapE, pNodeMapR );
        // set the relative phase of pNodeE compared to pNodeR
//        Fpga_NodeSetPhase( pNodeMapE, Fraig_NodeComparePhase(pNodeE, pNodeR) );
        nChoiceNodes += (pNodeMap == pNodeMapR);
        nChoices++;
    }

    // copy the outputs
    for ( i = 0; i < nOutputs; i++ )
    {
        if ( Fraig_Regular(ppOutputs[i]) == Fraig_ManReadConst1(pMan) )
            pNodeMap = Fpga_ManReadConst1(pManMap);
        else
            pNodeMap = (Fpga_Node_t *)Fraig_NodeReadData0( Fraig_Regular(ppOutputs[i]) );
        pOutputsMap[i] = Fpga_NotCond(pNodeMap, Fraig_IsComplement(ppOutputs[i]));
    }
    assert( Fpga_ManCheckConsistency( pManMap ) );

    // store the simulation info
    pManMap->nSimRounds = Fraig_ManReadSimRounds( pMan ) + 1;     // the number of words in the simulation info
    pManMap->pSimInfo = ALLOC( unsigned *, pManMap->nInputs );
    pManMap->pSimInfo[0] = ALLOC( unsigned, pManMap->nSimRounds * pManMap->nInputs );
    for ( i = 1; i < pManMap->nInputs; i++ )
        pManMap->pSimInfo[i] = pManMap->pSimInfo[i-1] + pManMap->nSimRounds;
    for ( i = 0; i < pManMap->nInputs; i++ )
    {
        fCompl = Fraig_NodeReadSimInv( ppInputs[i] );
        pSimInfo = Fraig_NodeReadSimInfo( ppInputs[i] );
        if ( fCompl )
            for ( k = 0; k < pManMap->nSimRounds - 1; k++ )
                pManMap->pSimInfo[i][k] = ~pSimInfo[k];
        else
            for ( k = 0; k < pManMap->nSimRounds - 1; k++ )
                pManMap->pSimInfo[i][k] = pSimInfo[k];
        pManMap->pSimInfo[i][pManMap->nSimRounds - 1] = FPGA_RANDOM_UNSIGNED;
    }

    // copy the switching info
    nBytes = sizeof(unsigned) * (pManMap->nSimRounds - 1);
    nTotal = nBytes * 8;
    for ( i = 0; i < pManMap->nInputs; i++ )
    {
        nOnes = Fraig_NodeReadNumOnes( ppInputs[i] );
        pManMap->vNodesAll->pArray[i]->SwitchProb = (float)2.0 * nOnes * (nTotal - nOnes) / nTotal / nTotal;
    }
    for ( k = 1; k < nNodes; k++ )
    {
        if ( Fraig_NodeIsVar(ppNodes[k]) )
            continue;
        nOnes = Fraig_NodeReadNumOnes( ppNodes[k] );
        pManMap->vNodesAll->pArray[i++]->SwitchProb = (float)2.0 * nOnes * (nTotal - nOnes) / nTotal / nTotal;
    }
    assert( i == nNodes - 1 );

    // set the statistics of choice nodes and choices
    Fpga_ManSetChoiceNodeNum( pManMap, nChoiceNodes );
    Fpga_ManSetChoiceNum( pManMap, nChoices );
    return pManMap;
}

/**Function*************************************************************

  Synopsis    [Balances the contents of the FRAIG manager into the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Man_t * Fpga_ManBalanceFraig( Fraig_Man_t * pMan, int * pInputArrivals )
{
    Fpga_Man_t * pManMap;
    Fraig_Man_t * pManNew;
    pManNew = Fraig_ManBalance( pMan, 0, pInputArrivals );
    pManMap = Fpga_ManDupFraig( pManNew );
    Fraig_ManSetVerbose( pManNew, 0 );
    Fraig_ManFree( pManNew );
    return pManMap;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


