/**CFile****************************************************************

  FileName    [ntkiBalance.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Creates a balanced netlist of AND2 gates.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiBalance.c,v 1.4 2005/02/28 05:34:25 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define Ntk_IsComplement(p)    (((int)((long) (p) & 01)))
#define Ntk_Regular(p)         ((Ntk_Node_t *)((unsigned)(p) & ~01)) 
#define Ntk_Not(p)             ((Ntk_Node_t *)((long)(p) ^ 01)) 
#define Ntk_NotCond(p,c)       ((Ntk_Node_t *)((long)(p) ^ (c)))

static void         Ntk_NetworkBalanceOne( Ntk_Network_t * pNetNew, Ntk_Node_t * pNode );
static Ntk_Node_t * Ntk_NetworkBalanceCover( Ntk_Network_t * pNetNew, Ntk_Node_t * pNode, Mvc_Cover_t * pMvc, Ntk_Node_t * ppInputs[] );
static Ntk_Node_t * Ntk_NetworkBalanceProduct( Ntk_Network_t * pNetNew, Ntk_Node_t ** ppNodes, int nNodes, int fOrGate );
static int          Ntk_NodeCompareByDelay( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 );
static Ntk_Node_t * Ntk_NetworkBalanceCreateAnd2( Ntk_Network_t * pNetNew, Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, int fCompl1, int fCompl2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Decomposes the network into a balances netlist of AND2 gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkBalance( Ntk_Network_t * pNet, double dAndGateDelay )
{
    char * pName;
    Ntk_Network_t * pNetNew;
    Ntk_Node_t ** ppInputs, ** ppOutputs;
    Ntk_Node_t * pNodeNew, * pNode, * pDriver;
    Ntk_Latch_t * pLatch, * pLatchNew;
    unsigned Pols[32];
    int Default, v;
    int clk;

	// We sweep because the following code doesn't do things like
	// constant propagation, and trivial redundancy removal, but assumes that
	// the network is free of those
	Ntk_NetworkSweep( pNet, 1, 1, 0, 0 );

clk = clock();

    // allocate the empty network
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );
    // register the name 
    pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNet->pName );
    // register the network spec file name
    if ( pNet->pSpec )
        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, pNet->pSpec );

    // copy and add the CI nodes
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pNodeNew = Ntk_NodeDup( pNetNew, pNode ); // sets pNode->pCopy = pNodeNew;
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }

    // create the array of buffers, which translate MV CIs into binary vars
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        // allocate the array of elementary binary nodes
        ppOutputs = ALLOC( Ntk_Node_t *, pNode->nValues );
        pNode->pData = (char *)ppOutputs;

        pNodeNew = Ntk_NetworkFindNodeByName( pNetNew, pNode->pName );
        assert( pNodeNew );
        if ( pNode->nValues == 2 )
        {
            ppOutputs[0] = Ntk_Not( pNodeNew );
            ppOutputs[1] = pNodeNew;
            // set the delay of the node
            pNodeNew->pData = (char *) Ntk_NodeGetArrivalTimeDiscrete( pNode, dAndGateDelay );
        }
        else
        {
            for ( v = 0; v < pNode->nValues; v++ )
            {
                // create one literal node for value v
                Pols[0] = (FT_MV_MASK(pNode->nValues) ^ (1 << v));
                Pols[1] = (1 << v);
                ppOutputs[v] = Ntk_NodeCreateOneInputNode( pNetNew, 
                    pNodeNew, pNode->nValues, 2, Pols );
                // add this node to the network
                Ntk_NetworkAddNode( pNetNew, ppOutputs[v], 1 );
                // set the delay of the node
                ppOutputs[v]->pData = (char *) Ntk_NodeGetArrivalTimeDiscrete( pNode, dAndGateDelay );
            }
        }
    }

    // this memory will be use for the temporary storage of the nodes
    // it should be more than the max cubes plus max fanin values in the node
    pNetNew->pData = (char *)ALLOC( Ntk_Node_t *, 100000 );

    // go through the internal nodes in the DFS order
    Ntk_NetworkDfs( pNet, 1 );
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        // transform the FF into a cone of AND2s and add them to the network
        Ntk_NetworkBalanceOne( pNetNew, pNode );
    }
    free( pNetNew->pData );


    // go through the primary outputs and create the nodes
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        // get hold of the outputs of the driver
        ppOutputs    = (Ntk_Node_t **)pDriver->pData;
        // create the inputs of the CO node
        ppInputs     = ALLOC( Ntk_Node_t *, pNode->nValues );
        pNode->pData = (char *)ppInputs;

        if ( pNode->nValues == 2 )
        {
            ppInputs[0] = NULL;
            if ( Ntk_IsComplement(ppOutputs[1]) )
            {
                ppInputs[1] = Ntk_NodeCreateOneInputBinary( pNetNew, Ntk_Regular(ppOutputs[1]), 1 );
                Ntk_NetworkAddNode( pNetNew, ppInputs[1], 1 );
            }
            else
                ppInputs[1] = ppOutputs[1];
        }
        else
        {
            // create the node's inputs
            for ( v = 0; v < pNode->nValues; v++ )
            {
                // check if the output is complemented
                if ( Ntk_IsComplement(ppOutputs[v]) )
                { // add inverter
                    ppInputs[v] = Ntk_NodeCreateOneInputBinary( pNetNew, Ntk_Regular(ppOutputs[v]), 1 );
                    Ntk_NetworkAddNode( pNetNew, ppInputs[v], 1 );
                }
                else
                    ppInputs[v] = ppOutputs[v];
            }
        }
    }


    // create the CO nodes
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // get hold of the outputs of the driver
        ppOutputs = (Ntk_Node_t **)pNode->pData;
        if ( pNode->nValues == 2 )
        {
            // skip the negative polarity
            assert( ppOutputs[0] == NULL );
            // treat the special case of the CI node
            if ( ppOutputs[1]->Type == MV_NODE_CI )
            {
                pNodeNew = Ntk_NodeCreate( pNetNew, pNode->pName, MV_NODE_CO, 2 );
                Ntk_NodeAddFanin( pNodeNew, ppOutputs[1] );
                Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            }
            else
            {
                // assign the name to this node
                pName = Ntk_NetworkRegisterNewName( pNetNew, pNode->pName );
                Ntk_NodeAssignName( ppOutputs[1], pName );
                // add the CO node for the positive polarity
                pNodeNew = Ntk_NetworkAddNodeCo( pNetNew, ppOutputs[1], 1 );
            }
        }
        else
        {
/*
            // get the default value of the driver
            Default = -1;
            if ( pDriver->Type == MV_NODE_INT )
                Default = Ntk_NodeReadDefault( pDriver );
            // clean the default value
            if ( Default >= 0 )
                ppOutputs[iValue + Default] = NULL;
*/
            Default = -1;

            // create the collector node
            pNodeNew = Ntk_NodeCreateCollector( pNetNew, ppOutputs, pNode->nValues, Default );
            Ntk_NodeOrderFanins( pNodeNew );

            // add the collector node to the network
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            // assign the name to this node
            pName = Ntk_NetworkRegisterNewName( pNetNew, pNode->pName );
            Ntk_NodeAssignName( pNodeNew, pName );
            // add the CO node
            pNodeNew = Ntk_NetworkAddNodeCo( pNetNew, pNodeNew, 1 );
        }
        pNodeNew->Subtype = pNode->Subtype;
        // set the pointer from the old CO node to the new CO node
        pNode->pCopy = pNodeNew;
    }

    // clean the pData field
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        ppOutputs = (Ntk_Node_t **)pNode->pData;
        free( ppOutputs );
    }

    // copy and add the latches if present
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        // get the new latch
        pLatchNew = Ntk_LatchDup( pNetNew, pLatch );
        // set the correct inputs/outputs
        pLatchNew->pInput  = pLatch->pInput->pCopy;
        pLatchNew->pOutput = pLatch->pOutput->pCopy;
        // add the new latch to the network
        Ntk_NetworkAddLatch( pNetNew, pLatchNew );
    }

    // copy the EXDC network
    if ( pNet->pNetExdc )
        pNetNew->pNetExdc = Ntk_NetworkDup( pNet->pNetExdc, pNet->pMan );

//PRT( "Balance", clock() - clk );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );
    Ntk_NetworkOrderFanins( pNetNew );

    // sweep to remove useless nodes
    Ntk_NetworkSweep( pNetNew, 1, 1, 0, 0 );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkBalance(): Network check has failed.\n" );
    return pNetNew;
}


/**Function*************************************************************

  Synopsis    [Balances the covers of one MV node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkBalanceOne( Ntk_Network_t * pNetNew, Ntk_Node_t * pNode )
{
    Ntk_Node_t ** ppOutputs, ** ppInputs;
    Ntk_Node_t * pRoot, * pFanin;
    Ntk_Pin_t * pPin;
    Mvc_Cover_t ** ppIsets, * pMvc;
    Cvr_Cover_t * pCvr, * pCvrNew;
    Mvr_Relation_t * pMvr;
    int nValues, iValue, v;

    assert( pNode->Type == MV_NODE_INT );

    // count the sum total of input values
    nValues = Vm_VarMapReadValuesInNum( Ntk_NodeReadFuncVm(pNode) );

    // collect the fanin inputs
    ppInputs = ALLOC( Ntk_Node_t *, nValues );
    iValue = 0;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        ppOutputs = (Ntk_Node_t **)pFanin->pData;
        for ( v = 0; v < pFanin->nValues; v++ )
            ppInputs[iValue++] = ppOutputs[v];
    }
    assert( iValue == nValues );

    // allocate the array of elementary binary nodes
    ppOutputs = ALLOC( Ntk_Node_t *, pNode->nValues );
    pNode->pData = (char *)ppOutputs;

    // get the set of i-sets
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    ppIsets = Cvr_CoverReadIsets( pCvr );

    // go through the covers of the node
    if ( pNode->nValues == 2 && (ppIsets[0] == NULL || ppIsets[1] == NULL) )
    {
        if ( ppIsets[0] == NULL )
        {
            pRoot        = Ntk_NetworkBalanceCover( pNetNew, pNode, ppIsets[1], ppInputs );
            ppOutputs[0] = Ntk_Not( pRoot );
            ppOutputs[1] = pRoot;
        }
        else
        {
            pRoot        = Ntk_NetworkBalanceCover( pNetNew, pNode, ppIsets[0], ppInputs );
            ppOutputs[0] = pRoot;
            ppOutputs[1] = Ntk_Not( pRoot );
        }
    }
    else
    {
        for ( v = 0; v < pNode->nValues; v++ )
        {
            pCvrNew = NULL;
            if ( ppIsets[v] == NULL )
            {   // get the default value
                // get hold of Mvr
                pMvr = Ntk_NodeGetFuncMvr( pNode );
                // derive the new Cvr
                pCvrNew = Fnc_FunctionDeriveCvrFromMvr( Ntk_NodeReadManMvc(pNode), pMvr, 0 ); // don't use the default
                // get the default value
                pMvc = Cvr_CoverReadIsets(pCvrNew)[v];
//                Mvc_CoverPrint( pMvc );
            }
            else
                pMvc = ppIsets[v];

            // balance the cover
            ppOutputs[v] = Ntk_NetworkBalanceCover( pNetNew, pNode, pMvc, ppInputs );

            // delete the temporary cover if allocated
            if ( pCvrNew )
                Cvr_CoverFree( pCvrNew );
        }
    }
    FREE( ppInputs );
}


/**Function*************************************************************

  Synopsis    [Balances the representation of one cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkBalanceCover( Ntk_Network_t * pNetNew, Ntk_Node_t * pNode, 
    Mvc_Cover_t * pMvc, Ntk_Node_t * ppInputs[] )
{
    Ntk_Node_t ** ppNodesAnd, ** ppNodesOr;
    Vm_VarMap_t * pVm;
    Ntk_Node_t * ppNodesTemp[32];
    Ntk_Node_t * pRoot;
    Mvc_Cube_t * pCube;
    int * pValues, * pValuesFirst;
    int nBitValues, nVarsIn;
    int iCube, iAnd, iOr;
    int v, i;

    if ( Mvc_CoverIsEmpty(pMvc) )
    {
        pRoot = Ntk_NodeCreateConstant( pNetNew, 2, 1 );
        Ntk_NetworkAddNode( pNetNew, pRoot, 1 );
        pRoot->pData = NULL;
        return pRoot;
    }

    if ( Mvc_CoverIsTautology(pMvc) )
    {
        pRoot = Ntk_NodeCreateConstant( pNetNew, 2, 2 );
        Ntk_NetworkAddNode( pNetNew, pRoot, 1 );
        pRoot->pData = NULL;
        return pRoot;
    }

    // get data about variables
    pVm = Ntk_NodeReadFuncVm( pNode );
    pValues = Vm_VarMapReadValuesArray( pVm );
    pValuesFirst = Vm_VarMapReadValuesFirstArray( pVm );

    // allocate room for the temporary nodes of AND plane and OR plane
    nVarsIn    = Vm_VarMapReadVarsInNum( pVm );

    // get the number of cubes
    ppNodesAnd = (Ntk_Node_t **)pNetNew->pData; 
    ppNodesOr  = ppNodesAnd + nVarsIn; 

    // balance each product of the cover
    iCube = 0;
    Mvc_CoverForEachCube( pMvc, pCube )
    {
        iAnd = 0;
        for ( v = 0; v < nVarsIn; v++ )
        {
            // check how many bits are set in this var
            nBitValues = 0;
            for ( i = 0; i < pValues[v]; i++ )
                nBitValues += Mvc_CubeBitValue( pCube, pValuesFirst[v] + i );
            assert( nBitValues >= 0 );

            // if this is a full literal, skip it
            if ( nBitValues == pValues[v] )
                continue;

            // treat the case when the OR gate is not needed
            if ( nBitValues > 1 )
            {
                // create the OR gate
                iOr = 0;
                for ( i = 0; i < pValues[v]; i++ )
                    if ( Mvc_CubeBitValue( pCube, pValuesFirst[v] + i ) )
                        ppNodesTemp[iOr++] = ppInputs[ pValuesFirst[v] + i ];
                // create the OR gate
                ppNodesAnd[iAnd++] = Ntk_NetworkBalanceProduct( pNetNew, ppNodesTemp, iOr, 1 );
            }
            else
            {
                // find the only bit value set
                for ( i = 0; i < pValues[v]; i++ )
                    if ( Mvc_CubeBitValue( pCube, pValuesFirst[v] + i ) )
                        break;
                ppNodesAnd[iAnd++] = ppInputs[ pValuesFirst[v] + i ];
            }
        }
        // create the AND gate
        if ( iAnd > 1 )
            ppNodesOr[iCube++] = Ntk_NetworkBalanceProduct( pNetNew, ppNodesAnd, iAnd, 0 );
        else
            ppNodesOr[iCube++] = ppNodesAnd[0];
    }

    // create the OR gate for the cube
    if ( iCube > 1 )
        pRoot = Ntk_NetworkBalanceProduct( pNetNew, ppNodesOr, iCube, 1 );
    else
        pRoot = ppNodesOr[0];
    return pRoot;
}


/**Function*************************************************************

  Synopsis    [Balances the representation of one product or sum.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkBalanceProduct( Ntk_Network_t * pNetNew, Ntk_Node_t ** ppNodes, int nNodes, int fOrGate )
{
    Ntk_Node_t * pNode1, * pNode2;
    Ntk_Node_t * pRoot, * pTemp;
    int i;

    // complement the pointers
    if ( fOrGate )
    {
        for ( i = 0; i < nNodes; i++ )
            ppNodes[i] = Ntk_Not( ppNodes[i] );
    }

    //for ( i = 0; i < nNodes; i++ )
    //	printf ("Node %s has arrival %d\n", Ntk_NodeGetNameLong( Ntk_Regular( ppNodes[i] ) ), (int) Ntk_Regular ( ppNodes[i] )->pData );

    // q-sort the pointers by delay (the latest arriving go first)
    qsort( (void *)ppNodes, nNodes, sizeof(Ntk_Node_t *), 
            (int (*)(const void *, const void *)) Ntk_NodeCompareByDelay );
    assert( Ntk_NodeCompareByDelay( ppNodes, ppNodes + nNodes - 1 ) <= 0 );

    while ( nNodes > 1 )
    {
	    // Merge earliest two signals

	    pNode1 = Ntk_Regular(ppNodes[nNodes-2]);
	    pNode2 = Ntk_Regular(ppNodes[nNodes-1]);

	    assert( pNode1->pData >= pNode2->pData );

	    // create a new gate
	    pRoot = Ntk_NetworkBalanceCreateAnd2( pNetNew, pNode1, pNode2, 
			    Ntk_IsComplement(ppNodes[nNodes-2]), Ntk_IsComplement(ppNodes[nNodes-1]) );
	    // place it into the array
	    ppNodes[nNodes-2] = pRoot;

	    // Now push this newly introduced node into
	    // the correct position in the sorted array
	    i = nNodes-2;
	    while ( (i-1 >= 0) && ( Ntk_Regular( ppNodes[i-1] )->pData < Ntk_Regular( ppNodes[i] )->pData) )
	    {
		    pTemp        = ppNodes[i-1];
		    ppNodes[i-1] = ppNodes[i];
		    ppNodes[i]   = pTemp;
		    i--;
	    }

	    // decrement the number of nodes
	    nNodes--;
    }

    // assign the root gate
    pRoot = ppNodes[0];

    if ( fOrGate )
        pRoot = Ntk_Not( pRoot );
    return pRoot;
}


/**Function*************************************************************

  Synopsis    [Compares the nodes according to the accepted order.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeCompareByDelay( Ntk_Node_t ** ppN1, Ntk_Node_t ** ppN2 )
{
    Ntk_Node_t * pNode1 = Ntk_Regular(*ppN1);
    Ntk_Node_t * pNode2 = Ntk_Regular(*ppN2);
    // compare using number of values
    if ( pNode1->pData < pNode2->pData )
        return 1;
    if ( pNode1->pData > pNode2->pData )
        return -1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Creates one AND2 gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkBalanceCreateAnd2( Ntk_Network_t * pNetNew, 
    Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, int fCompl1, int fCompl2 )
{
    Ntk_Node_t * ppNodes[2] = { pNode1, pNode2 };
    Ntk_Node_t * pNode;
    int Delay1, Delay2, DelayMax;
    int Type;

    assert( pNode1 != pNode2 );

    if ( !fCompl1 && !fCompl2 )        // AND( a,  b  )
        Type = 8;  //  1000
    else if ( fCompl1 && !fCompl2 )    // AND( a,  b' )
        Type = 4;  //  0100 
    else if ( !fCompl1 && fCompl2 )    // AND( a', b  )
        Type = 2;  //  0010
    else  // if ( fCompl1 && fCompl2 ) // AND( a', b' )
        Type = 1;  //  0001

    // create the two-input node
    pNode = Ntk_NodeCreateTwoInputBinary( pNetNew, ppNodes, Type );
    // add the node to the network
    Ntk_NetworkAddNode( pNetNew, pNode, 1 );
    // set the delay of the node
    Delay1 = (int)pNode1->pData;
    Delay2 = (int)pNode2->pData;
    DelayMax = (Delay1 > Delay2)? Delay1 : Delay2;
    pNode->pData = (char *)(DelayMax + 1);
    return pNode;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


