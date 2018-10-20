/**CFile****************************************************************

  FileName    [ntkiStrash.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Derives the structurally hashed network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiStrash.c,v 1.8 2004/10/15 05:41:04 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "sh.h"
#include "wn.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Network_t * Ntk_NetworkStrashDerive( Ntk_Network_t * pNet, 
    Wn_Window_t * pWnd, Sh_Network_t * pShNet );
static Ntk_Node_t * Ntk_NetworkStrashDerive_rec( Sh_Node_t * pShNode, 
    Ntk_Network_t * pNetNew, Ntk_Node_t * ppInputs[] );

extern Mfs_NetworkMarkNDCones( Ntk_Network_t * pNet );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transforms the current network into the strashed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkStrash( Ntk_Network_t * pNet )
{
    Ntk_Network_t * pNetNew = NULL;
    Wn_Window_t * pWnd;
    Sh_Network_t * pShNet;
    Sh_Manager_t * pShMan;

    pShMan = Sh_ManagerCreate( 100, 10000, 0 );
    Sh_ManagerTwoLevelEnable( pShMan );

    pWnd    = Wn_WindowCreateFromNetwork(pNet);
    Mfs_NetworkMarkNDCones( pNet );
    pShNet  = Wn_WindowStrash( pShMan, pWnd, 0 );
    pNetNew = Ntk_NetworkStrashDerive( pNet, pWnd, pShNet );
    Sh_NetworkFree( pShNet );
    Wn_WindowDelete( pWnd );

    Sh_ManagerFree( pShMan, 1 );
    return pNetNew;
}

/**Function*************************************************************

  Synopsis    [Creates MVSIS network from the strashed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkStrashDerive( Ntk_Network_t * pNet, 
    Wn_Window_t * pWnd, Sh_Network_t * pShNet )
{
    Sh_Node_t ** ppShRoots;
    Ntk_Network_t * pNetNew;
    Ntk_Node_t ** ppInputs, ** ppOutputs;
    Ntk_Node_t * pNodeNew, * pNode, * pDriver;
    Ntk_Latch_t * pLatch, * pLatchNew;
    char * pName;
    unsigned Pols[32];
    int nInputs, nOutputs, iValue;
    int Default, i, v;
    bool fCompl;
    int clk;

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
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
    }

    // get parameters
    nInputs   = Vm_VarMapReadValuesInNum( Wn_WindowReadVmL(pWnd) );
    ppInputs  = ALLOC( Ntk_Node_t *, nInputs );
    nOutputs  = Vm_VarMapReadValuesInNum( Wn_WindowReadVmR(pWnd) );
    ppOutputs = ALLOC( Ntk_Node_t *, nOutputs );

    // create the array of buffers, which translate MV CIs into binary vars
    iValue = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pNodeNew = Ntk_NetworkFindNodeByName( pNetNew, pNode->pName );
        assert( pNodeNew );
        if ( pNode->nValues == 2 )
        {
            ppInputs[iValue++] = NULL;
            ppInputs[iValue++] = pNodeNew;
        }
        else
        {
            for ( v = 0; v < pNode->nValues; v++ )
            {
                // create one literal node for value v
                Pols[0] = (FT_MV_MASK(pNode->nValues) ^ (1 << v));
                Pols[1] = (1 << v);
                ppInputs[iValue] = Ntk_NodeCreateOneInputNode( pNetNew, 
                    pNodeNew, pNode->nValues, 2, Pols );
                // add this node to the network
                Ntk_NetworkAddNode( pNetNew, ppInputs[iValue], 1 );
                iValue++;
            }
        }
    }

    // clean the data fields
    Sh_NetworkInterleaveNodes( pShNet );
    Sh_NetworkCleanData( pShNet );
    // recursively create the nodes
    ppShRoots = Sh_NetworkReadOutputs( pShNet );
    for ( i = 0; i < nOutputs; i++ )
    {
        // check if the output is complemented
        fCompl = Sh_IsComplement(ppShRoots[i]);
        ppOutputs[i] = Ntk_NetworkStrashDerive_rec( Sh_Regular(ppShRoots[i]), 
            pNetNew, ppInputs );
        if ( fCompl )
        { // add inverter
            Pols[0] = 2;
            Pols[1] = 1;
            ppOutputs[i] = Ntk_NodeCreateOneInputNode( pNetNew, ppOutputs[i], 2, 2, Pols );
            Ntk_NetworkAddNode( pNetNew, ppOutputs[i], 1 );
        }
    }

    // create the CO nodes
    iValue = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        if ( pDriver->Type == MV_NODE_CI )
        {
            pNodeNew = Ntk_NodeCreate( pNetNew, pNode->pName, MV_NODE_CO, 2 );
            Ntk_NodeAddFanin( pNodeNew, pDriver->pCopy );
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            continue;
        }

        if ( pNode->nValues == 2 )
        {
            // skip the negative polarity
            iValue++;
            // assign the name to this node
            if ( ppOutputs[iValue]->pName == NULL )
            {
                pName = Ntk_NetworkRegisterNewName( pNetNew, pNode->pName );
                Ntk_NodeAssignName( ppOutputs[iValue], pName );
                // add the CO node for the positive polarity
                pNodeNew = Ntk_NetworkAddNodeCo( pNetNew, ppOutputs[iValue], 1 );
            }
            else
            {
                assert( ppOutputs[iValue]->Type == MV_NODE_CI );
                pNodeNew = Ntk_NodeCreate( pNetNew, pNode->pName, MV_NODE_CO, 2 );
                Ntk_NodeAddFanin( pNodeNew, ppOutputs[iValue] );
                Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            }
            iValue++;
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
            pNodeNew = Ntk_NodeCreateCollector( pNetNew, ppOutputs + iValue, pNode->nValues, Default );
            Ntk_NodeOrderFanins( pNodeNew );

            // add the collector node to the network
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            // assign the name to this node
            pName = Ntk_NetworkRegisterNewName( pNetNew, pNode->pName );
            Ntk_NodeAssignName( pNodeNew, pName );
            // add the CO node
            pNodeNew = Ntk_NetworkAddNodeCo( pNetNew, pNodeNew, 1 );
            // increment the number of values
            iValue += pNode->nValues;
        }
        pNodeNew->Subtype = pNode->Subtype;
        // set the pointer from the old CO node to the new CO node
        pNode->pCopy = pNodeNew;
    }
    FREE( ppInputs );
    FREE( ppOutputs );

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

//PRT( "Strash", clock() - clk );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );
    // uncomment this line to generate .bench format
//    Ntk_NetworkOrderFanins( pNetNew );

    // sweep to remove useless nodes
    // comment out this line to generate .bench format
    Ntk_NetworkSweep( pNetNew, 1, 1, 0, 0 );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkStrash(): Network check has failed.\n" );
    return pNetNew;
}


/**Function*************************************************************

  Synopsis    [Creates MVSIS network from the strashed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkStrashDerive_rec( Sh_Node_t * pShNode, 
    Ntk_Network_t * pNetNew, Ntk_Node_t * ppInputs[] )
{
    Sh_Node_t * pShNode1, * pShNode2, * pTemp;
    Ntk_Node_t * ppNodes[2], * pNode, * pNodeNew;
    int fCompl1, fCompl2, Type;
    int fWriteAnds = 0; // set this flag to 1 to generate networks of only ANDs and INVs (needed to write .bench format)
    unsigned Pol = 1;

    assert( !Sh_IsComplement(pShNode) );

    // get the children
    pShNode1 = Sh_NodeReadOne(pShNode);
    pShNode2 = Sh_NodeReadTwo(pShNode);

    // if the node is visited return the resulting Ntk_Node
    pNode = (Ntk_Node_t *)Sh_NodeReadData( pShNode );
    if ( pNode )
        return pNode;

    if ( Sh_NodeIsConst(pShNode) )
    {
        // create the constant-1 node
        pNode = Ntk_NodeCreateConstant( pNetNew, 2, (unsigned)2 );
        // add this node to the network
        Ntk_NetworkAddNode( pNetNew, pNode, 1 );
        // save the node
        Sh_NodeSetData( pShNode, (unsigned)pNode );
        return pNode;
    }
    
    if ( Sh_NodeIsVar(pShNode) )
    {
        // get the leaf node
//        pNode = ppInputs[ (int)pShNode2 ];
        pNode = ppInputs[ Sh_NodeReadIndex(pShNode) ];
        return pNode;
    }

    assert( Sh_NodeIsAnd(pShNode) );
    // get the children nodes
    ppNodes[0] = Ntk_NetworkStrashDerive_rec( Sh_Regular(pShNode1), pNetNew, ppInputs );
    ppNodes[1] = Ntk_NetworkStrashDerive_rec( Sh_Regular(pShNode2), pNetNew, ppInputs );
    // order them
    if ( Ntk_NodeCompareByNameAndId( ppNodes, ppNodes + 1 ) >= 0 )
    {
        pNode      = ppNodes[0];
        ppNodes[0] = ppNodes[1];
        ppNodes[1] = pNode;

        pTemp    = pShNode1;
        pShNode1 = pShNode2;
        pShNode2 = pTemp;
    }
    // derive complemented attributes
    fCompl1 = Sh_IsComplement( pShNode1 );
    fCompl2 = Sh_IsComplement( pShNode2 );

    if ( fWriteAnds )
    {
        // create the intertors
        if ( fCompl1 )
        {
            pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, ppNodes[0], 1 );
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            ppNodes[0] = pNodeNew;
        }
        if ( fCompl2 )
        {
            pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, ppNodes[1], 1 );
            Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
            ppNodes[1] = pNodeNew;
        }
        // create the AND gate
        Type = 8;
    }
    else
    {
        if ( !fCompl1 && !fCompl2 )        // AND( a,  b  )
            Type = 8;  //  1000
        else if ( fCompl1 && !fCompl2 )    // AND( a,  b' )
            Type = 4;  //  0100 
        else if ( !fCompl1 && fCompl2 )    // AND( a', b  )
            Type = 2;  //  0010
        else  // if ( fCompl1 && fCompl2 ) // AND( a', b' )
            Type = 1;  //  0001
    }

    // create the two-input node
    pNode = Ntk_NodeCreateTwoInputBinary( pNetNew, ppNodes, Type );
    // add the node to the network
    Ntk_NetworkAddNode( pNetNew, pNode, 1 );
    // save the node in the Sh node
    Sh_NodeSetData( pShNode, (unsigned)pNode );
    return pNode;
}





    
/**Function*************************************************************

  Synopsis    [Verifies the functionality of the networks by strashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkComputeGlobalFunctions( DdManager * dd, 
    Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, 
    Mva_FuncSet_t ** ppRes1, Mva_FuncSet_t ** ppRes2 )
{
/*
    Sh_Manager_t * pManSh;
    Fm_Manager_t * pManFm;
    Wn_Window_t * pWnd1, * pWnd2;
    Sh_Network_t * pShNet1, * pShNet2;
    char ** ppLeaves, ** ppRoots;
    Mva_FuncSet_t * pGlo1, * pGlo2;

    // collect the managers
    pManSh = Ntk_NetworkReadManSh(pNet1);
    pManFm = Ntk_NetworkReadManFm(pNet1);
    // collect the CI/CO names
    Ntk_NetworkCollectIoNames( pNet1, &ppLeaves, &ppRoots );
    // create windows by name
    pWnd1   = Wn_WindowCreateFromNetworkNames( pNet1, ppLeaves, ppRoots );
    pWnd2   = Wn_WindowCreateFromNetworkNames( pNet2, ppLeaves, ppRoots );
    // strash the windows
    pShNet1  = Wn_WindowStrash( pManSh, pWnd1, 0 );
    pShNet2  = Wn_WindowStrash( pManSh, pWnd1, 0 );
    // derive the global functions
    Fm_GlobalComputeTwo( dd, pManFm, pShNet1, pShNet2, &pGlo1, &pGlo2 );
    // free the trashed networks
    Sh_NetworkFree( pShNet1 );
    Sh_NetworkFree( pShNet2 );
    // delete the windows
    Wn_WindowDelete( pWnd1 );
    Wn_WindowDelete( pWnd2 );

    // return the global functions
    *ppRes1 = pGlo1;
    *ppRes2 = pGlo2;
*/
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


