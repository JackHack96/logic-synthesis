/**CFile****************************************************************

  FileName    [ntkiFraigChoice.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Creates the choice network out of several MVSIS networks.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiFraigChoice.c,v 1.10 2005/05/18 04:14:37 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "fraigInt.h"
#include "mio.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int             Ntk_NetworkChoiceOne( Fraig_Man_t * pMan, Ntk_Network_t * pNet );
static Ntk_Network_t * Ntk_NetworkChoiceToNet( Ntk_Network_t * pNetRepres, Fraig_Man_t * pMan );
static Ntk_Node_t *    Ntk_NetworkChoiceCreateOr_rec( Ntk_Network_t * pNetNew, Fraig_Node_t * gNode );
static Ntk_Node_t *    Ntk_NetworkChoiceCreateAnd_rec( Ntk_Network_t * pNetNew, Fraig_Node_t * gNode );

extern Ntk_Network_t * Ntk_NetworkDeriveFraig( Mv_Frame_t * pMvsis, Ntk_Network_t * pNet, 
    int fMulti, int fAreaDup, int fWriteAnds, int fFuncRed, int fBalance, int fDoSparse, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the AIG with choice nodes for several networks.]

  Description [The networks should be functionally equivalent but 
  structurally different. The current network is given (pNet). The
  list of file names for other numbers is in (pNames,nNames). The 
  returned network is an AIG with OR-gates to denote choice nodes.]
               
  SideEffects [We assume that all networks have the same ordering of PIs/POs!]

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkChoice( Mv_Frame_t * pMvsis, Ntk_Network_t * pNet, char * pNames[], int nNames, 
                                  int fFuncRed, int fBalance, int fDoSparse, int fPickOne, int fVerbose )
{
    extern Mio_Library_t * s_pLib;
    FILE * pFile;
    Fraig_Man_t * pMan;//, * pTemp;
    Ntk_Network_t * pNetNew, * pNetTemp;
    int i, clk, RetValue, clkTotal = clock();
    int fRefCount = 0; // no ref counting
    int fChoicing = 1; // create choice nodes

    if ( pNet == NULL )
        return NULL;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only choice binary networks.\n" );
        return NULL;
    }

    // start the manager
    pMan = Fraig_ManCreate();

    // set the parameters of the manager 
    Fraig_ManSetFuncRed ( pMan, fFuncRed );       
//    Fraig_ManSetFeedBack( pMan, fFeedBack );      
    Fraig_ManSetDoSparse( pMan, fDoSparse );      
    Fraig_ManSetRefCount( pMan, fRefCount );      
    Fraig_ManSetChoicing( pMan, fChoicing );      
    Fraig_ManSetVerbose ( pMan, fVerbose );       

    // add the current network
clk = clock();
    Ntk_NetworkChoiceOne( pMan, pNet );
PRT( "Fraiging current network", clock() - clk );
    // reference the outputs
//    for ( i = 0; i < pMan->nOutputs; i++ )
//        Fraig_Ref( pMan->pOutputs[i] );

    // strash the remaining networks
    for ( i = 0; i < nNames; i++ )
    {
        // make sure the file can be opened
        pFile = fopen( pNames[i], "r" );
        if ( pFile == NULL )
        {
            fprintf( stdout, "Cannot open the network in file \"%s\".\n", pNames[i] );
            continue;
        }
        fclose( pFile );
        // read the network using the same functionality manager
        pNetNew = Io_ReadNetwork( pMvsis, pNames[i] );
        if ( pNetNew == NULL )
        {
            fprintf( stdout, "Error while reading the network from file \"%s\".\n", pNames[i] );
            continue;
        }
        // quit if the network is not binary
        if ( !Ntk_NetworkIsBinary(pNetNew) )
        {
            fprintf( stdout, "Cannot choice an MV network from file \"%s\".\n", pNames[i] );
            continue;
        }
        // balance the AIG
        pNetNew = Ntk_NetworkDeriveFraig( pMvsis, pNetTemp = pNetNew, 0, 0, 0, 0, fBalance, 0, 0 );
        Ntk_NetworkDelete( pNetTemp );

        // strash the network into the same AIG manager
clk = clock();
        if ( RetValue = Ntk_NetworkChoiceOne( pMan, pNetNew ) )
            fprintf( stdout, "Skipping %d outputs, for which network \"%s\" does not verify.\n", RetValue, pNames[i] );
PRT( "Fraiging another network", clock() - clk );
        // delete the temporary network
        Ntk_NetworkFraigDeref( pMan, pNetNew ); // the outputs are held by the current network
        Ntk_NetworkDelete( pNetNew );
    }

    // select one best choice out of many and balance the resulting graph
    if ( fPickOne )
    {
//        pMan = Fraig_ManSelect( pTemp = pMan );
//        Fraig_ManFree( pTemp );
//        pMan = Fraig_ManBalance( pTemp = pMan, 0, NULL );
//        Fraig_ManFree( pTemp );
    }

    // transform the AIG into the equivalent network
clk = clock();
    pNetNew = Ntk_NetworkChoiceToNet( pNet, pMan );
Fraig_ManSetTimeToNet( pMan, clock() - clk );
Fraig_ManSetTimeTotal( pMan, clock() - clkTotal );

{
//extern void Fraig_ManRetime( Fraig_Man_t * pMan, int nLatches, int fVerbose );
//int clk = clock();
//Fraig_ManRetime( pMan, Ntk_NetworkReadLatchNum(pNet), 0 );
//PRT( "Time", clock() - clk );
}

    // dereference the network
    Ntk_NetworkFraigDeref( pMan, pNet );
    // dereference the outputs
//    for ( i = 0; i < pMan->nOutputs; i++ )
//        Fraig_RecursiveDeref( pMan, pMan->pOutputs[i] );

//Fraig_PrintChoiceNodeLevels( pMan );

    // free the manager
    Fraig_ManFree( pMan );
    return pNetNew;
}

/**Function*************************************************************

  Synopsis    [Adds one network to the choice graph.]

  Description [Returns 1 if the non-verify is detected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkChoiceOne( Fraig_Man_t * pMan, Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode, * pDriver;
    Fraig_Node_t * gNode;
    int i, RetValue = 0;

    if ( pMan->vOutputs->nSize == 0 ) // the first call
    {
        Ntk_NetworkFraigInt( pMan, pNet, 1 );
        return 0;
    }

    // choice without putting the outputs
    Ntk_NetworkFraigInt( pMan, pNet, 0 );

    // compare the outputs
    i = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        gNode = NTK_STRASH_READ( pDriver ); // ref of gNode is held by pDriver
        if ( pMan->vOutputs->pArray[i] != gNode )
        {
            if ( Fraig_NodesAreEqual( pMan, pMan->vOutputs->pArray[i], gNode, -1 ) )
                Fraig_NodeSetChoice( pMan, pMan->vOutputs->pArray[i], gNode );
            else
                RetValue++;
        }
        i++;
    }
    assert( i == pMan->vOutputs->nSize );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Derives MVSIS network from the choice graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t *  Ntk_NetworkChoiceToNet( Ntk_Network_t * pNet, Fraig_Man_t * pMan )
{
    Ntk_Network_t * pNetNew;
    Ntk_Latch_t * pLatch, * pLatchNew;
    Fraig_Node_t ** pgInputs, ** pgOutputs;
    Ntk_Node_t * pNodeNew, * pNode, * pOutput;
    char * pName;
    int nInputs, nOutputs, i;
    int clk = clock();

    // get parameters of AIG
    nInputs   = Ntk_NetworkReadCiNum(pNet);
    nOutputs  = Ntk_NetworkReadCoNum(pNet);
    pgInputs  = Fraig_ManReadInputs( pMan );
    pgOutputs = Fraig_ManReadOutputs( pMan );

    // allocate the empty network
    pNetNew = Ntk_NetworkAlloc( pNet->pMvsis );
    // register the name 
    pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, pNet->pName );
    // register the network spec file name
    if ( pNet->pSpec )
        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, pNet->pSpec );

    // copy and add the CI nodes
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        pNodeNew = Ntk_NodeDup( pNetNew, pNode );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // set the PI node into the corresponding AIG node
        Fraig_NodeSetData0( pgInputs[i], (Fraig_Node_t *)pNodeNew );
        Fraig_NodeSetData1( pgInputs[i], (Fraig_Node_t *)pNodeNew );
        i++;
    }
    // add the constant node
    if ( Fraig_NodeReadNumRefs( Fraig_ManReadConst1(pMan) ) > 1 )
    {
        pNodeNew = Ntk_NodeCreateConstant( pNetNew, 2, 2 );
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // set the PI node into the corresponding AIG node
        Fraig_NodeSetData0( Fraig_ManReadConst1(pMan), (Fraig_Node_t *)pNodeNew );
        Fraig_NodeSetData1( Fraig_ManReadConst1(pMan), (Fraig_Node_t *)pNodeNew );
    }

    // recursively create the nodes
    i = 0;
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // check if the node is the constant node
        if ( Fraig_NodeIsConst(pgOutputs[i]) )
        {
            pOutput = Ntk_NodeCreateConstant( pNetNew, 2, 2-Fraig_IsComplement(pgOutputs[i]) );
            Ntk_NetworkAddNode( pNetNew, pOutput, 1 );
        }
        else
        {
            // create the corresponding output
            pOutput = Ntk_NetworkChoiceCreateOr_rec( pNetNew, Fraig_Regular(pgOutputs[i]) );
            // check if the output is complemented
            if ( Fraig_IsComplement(pgOutputs[i]) )
            { // add inverter
                pOutput = Ntk_NodeCreateOneInputBinary( pNetNew, pOutput, 1 );
                Ntk_NetworkAddNode( pNetNew, pOutput, 1 );
            }
            if ( pOutput->Type == MV_NODE_CI )
            { // add buffer
                pOutput = Ntk_NodeCreateOneInputBinary( pNetNew, pOutput, 0 );
                Ntk_NetworkAddNode( pNetNew, pOutput, 1 );
            }
        }
        // assign the name to this node
        pName = Ntk_NetworkRegisterNewName( pNetNew, pNode->pName );
        Ntk_NodeAssignName( pOutput, pName );
        // create the CO node
        pNodeNew = Ntk_NetworkAddNodeCo( pNetNew, pOutput, 1 );
        pNodeNew->Subtype = pNode->Subtype;
        // remember it in the old node
        pNode->pCopy = pNodeNew;
        i++;
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

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );
    Ntk_NetworkOrderFanins( pNetNew );

    // sweep to remove useless nodes
//    Ntk_NetworkSweep( pNetNew, 1, 1, 0, 0 );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Ntk_NetworkChoice(): Network check has failed.\n" );
    return pNetNew;
}


/**Function*************************************************************

  Synopsis    [Creates MVSIS network from the strashed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkChoiceCreateOr_rec( Ntk_Network_t * pNetNew, Fraig_Node_t * gNode )
{
    Ntk_Node_t ** ppNodes, * pNode;
    Fraig_Node_t * gTemp;
    int * pCompls, Counter;

    assert( !Fraig_IsComplement(gNode) );
    // if the node is visited, return the resulting OR-gate
    pNode = (Ntk_Node_t *)Fraig_NodeReadData1( gNode );
    if ( pNode )
        return pNode;
    assert( Fraig_NodeIsAnd(gNode) );

    // if the node has no choice node associated with it, return AND gate
    if ( gNode->pNextE == NULL )
        return Ntk_NetworkChoiceCreateAnd_rec( pNetNew, gNode );

    // count the number of choices
    Counter = 0;
    for ( gTemp = gNode; gTemp; gTemp = gTemp->pNextE )
        Counter++;

    // get the children nodes
    ppNodes = ALLOC( Ntk_Node_t *, Counter );
    pCompls = ALLOC( int, Counter );
    Counter = 0;
    for ( gTemp = gNode; gTemp; gTemp = gTemp->pNextE )
    {
        pCompls[Counter] = Fraig_NodeComparePhase( gNode, gTemp );
        ppNodes[Counter] = Ntk_NetworkChoiceCreateAnd_rec( pNetNew, gTemp );
        Counter++;
    }
    // create the binary OR gate with inputs in the corresponding polatiry
    pNode = Ntk_NodeCreateSimpleBinary( pNetNew, ppNodes, pCompls, Counter, 1 );
    free( ppNodes );
    free( pCompls );

    // add this node to the network
    Ntk_NetworkAddNode( pNetNew, pNode, 1 );
    // save the node
    Fraig_NodeSetData1( gNode, (Fraig_Node_t *)pNode );
    return pNode;
}


/**Function*************************************************************

  Synopsis    [Creates MVSIS network from the strashed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkChoiceCreateAnd_rec( Ntk_Network_t * pNetNew, Fraig_Node_t * gNode )
{
    Fraig_Node_t * gNode1, * gNode2, * pTemp;
    Ntk_Node_t * ppNodes[2], * pNode;
    int fCompl1, fCompl2, Type;

    assert( !Fraig_IsComplement(gNode) );
    // if the node is visited, return the resulting Ntk_Node
    pNode = (Ntk_Node_t *)Fraig_NodeReadData0( gNode );
    if ( pNode )
        return pNode;

    if ( Fraig_NodeIsConst(gNode) )
    {
        // create the constant-1 node
        pNode = Ntk_NodeCreateConstant( pNetNew, 2, (unsigned)2 );
        // add this node to the network
        Ntk_NetworkAddNode( pNetNew, pNode, 1 );
        // save the node
        Fraig_NodeSetData0( gNode, (Fraig_Node_t *)pNode );
        return pNode;
    }
    assert( Fraig_NodeIsAnd(gNode) );
    
    // get the children
    gNode1 = Fraig_NodeReadOne(gNode);
    gNode2 = Fraig_NodeReadTwo(gNode);

    // get the children nodes
    ppNodes[0] = Ntk_NetworkChoiceCreateOr_rec( pNetNew, Fraig_Regular(gNode1) );
    ppNodes[1] = Ntk_NetworkChoiceCreateOr_rec( pNetNew, Fraig_Regular(gNode2) );
    // order them
    if ( Ntk_NodeCompareByNameAndId( ppNodes, ppNodes + 1 ) >= 0 )
    {
        pNode      = ppNodes[0];
        ppNodes[0] = ppNodes[1];
        ppNodes[1] = pNode;

        pTemp  = gNode1;
        gNode1 = gNode2;
        gNode2 = pTemp;
    }
    // derive complemented attributes
    fCompl1 = Fraig_IsComplement( gNode1 );
    fCompl2 = Fraig_IsComplement( gNode2 );

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
    // save the node in the AIG node
    Fraig_NodeSetData0( gNode, (Fraig_Node_t *)pNode );
    return pNode;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


