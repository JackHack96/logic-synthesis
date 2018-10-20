/**CFile****************************************************************

  FileName    [ioReadNet.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Creates the network structure from the node fanin/fanout information.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioReadNet.c,v 1.33 2005/04/10 23:27:01 alanmi Exp $]

***********************************************************************/

#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Io_ReadNetworkName( Io_Read_t * p, Ntk_Network_t * pNet );
static int  Io_ReadNetworkSpec( Io_Read_t * p, Ntk_Network_t * pNet );
static int  Io_ReadNetworkCis( Io_Read_t * p, int Line, Ntk_Network_t * pNet );
static void Io_ReadNetworkCiSetValue( Io_Read_t * p, Ntk_Node_t * pNode );
static int  Io_ReadNetworkInternalNode( Io_Read_t * p, int Node, Ntk_Network_t * pNet );
static int  Io_ReadNetworkLatchInputOutput( Io_Read_t * p, int Latch, Ntk_Network_t * pNet );
static int  Io_ReadNetworkInternalNodeMvs( Io_Read_t * p, Ntk_Node_t * pNode );
static int  Io_ReadNetworkCos( Io_Read_t * p, int Line, Ntk_Network_t * pNet );
static int  Io_ReadNetworkLatch( Io_Read_t * p, int Latch, Ntk_Network_t * pNet );
static int  Io_ReadNetworkInputArr( Io_Read_t * p, int Line, Ntk_Network_t * pNet );
static int  Io_ReadNetworkDefaultInputArr( Io_Read_t * p, Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Compiles the network from information derived by preparsing.]

  Description [This procedure compiles the network from the information
  collected from the BLIF/BLIF-MVS/BLIF-MV file during preparsing.
  The information is available in the Io_Read_t structure and includes:
  (1) The network type (BLIF, BLIF-MVS, BLIF-MV)
  (2) The 0-terminated lines for all dot-statements of the input file.
  (3) The array of Io_Node_t structures containing, for each node: 
      (a) The 0-terminated .names line. 
      (b) The first table line and the number of table lines. 
      (c) The name of the output.
      (d) The number of values of this node (for BLIF-MV).
      (e) The default value of this node (for BLIF-MV).
  (4) The array of latches.

  This procedure performs the following steps:
  (0) Allocates the network and assigns its name.
  (1) Adds CI nodes (PI nodes and latch outputs) as CI nodes.
  (2) Adds CO nodes (PO nodes and latch inputs) as internal nodes.
      During this step, we mark which CO nodes belong to latches only.
  (3) Goes through all the nodes in the file that have .names/.table
      directive (these nodes are the internal nodes and CO nodes)
      and adds then to the network as internal nodes
  (4) Connects the internal nodes using fanin/fanout pins
  (5) At this point, the network has correct connectivity, except for 
      the fact that the POs are represented as internal nodes
      and the pure CO nodes are missing. Network check is performed.
  (6) Derives the functionality of each internal node from file.
  (7) Adds pure CO nodes for each CO node and the latch input.
      As a result of this step, those internal nodes that represented
      the CO nodes become the internal nodes, which fanout into pure POs,
      which has CO names. The internal nodes lose their names to 
      the pure CO nodes and from now on, have no names.
  (8) Finally, the latches are added as CO/CI pairs corresponding
      to the latch input/output pairs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadNetworkStructure( Io_Read_t * p )
{
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
    Ntk_Latch_t * pLatch;
    int i;

    // (0) allocate the empty network
    pNet = Ntk_NetworkAlloc( p->pMvsis );
    // read the network name
    if ( !p->fParsingExdcNet ) // EXDC network has no name
    {
        if ( Io_ReadNetworkName( p, pNet ) )
            goto failure;
        if ( Io_ReadNetworkSpec( p, pNet ) )
            goto failure;
    }

    // (0.5) set default input arrival times before creating PIs
    // so that they are given correct default values on creation
    if ( !p->fParsingExdcNet ) // EXDC network has no name
    {
        if ( Io_ReadNetworkDefaultInputArr( p, pNet ) )
        goto failure;
    }

    // (1) create the primary inputs as PI nodes
    for ( i = 0; i < p->nDotInputs; i++ )
        if ( Io_ReadNetworkCis( p, i, pNet ) )
            goto failure;

    // (1.5) handle the arrival time of each signal
    for ( i = 0; i < p->nDotInputArr; i++ )
        if ( Io_ReadNetworkInputArr( p, i, pNet ) )
            goto failure;

    // (2) create the primary outputs as internal nodes
    for ( i = 0; i < p->nDotOutputs; i++ )
        if ( Io_ReadNetworkCos( p, i, pNet )  )
            goto failure;
    // (1) create latch outputs as PI nodes
    // (2) create latch inputs as internal nodes
    for ( i = 0; i < p->nDotLatches; i++ )
        if ( Io_ReadNetworkLatchInputOutput( p, i, pNet ) )
            goto failure;

    // (3) create the node structures of the internal nodes
    for ( i = 0; i < p->nDotNodes; i++ )
        if ( Io_ReadNetworkInternalNode( p, i, pNet )  )
            goto failure;

    // set the number of values for the PI variables
    // (all other variables have been set by Io_ReadNetworkInternalNode)
    Ntk_NetworkForEachCi( pNet, pNode )
        Io_ReadNetworkCiSetValue( p, pNode );
    // allocate temporary storage
    Io_ReadStructAllocateAdditional( p );

    // (4) create the fanout structures for the fanin nodes of the internal nodes
    Ntk_NetworkForEachNode( pNet, pNode )
    {   // make sure all the internal nodes have Io_Node_t structures
        // which will be used below to create their functionality
        if ( Ntk_NodeReadData(pNode) == NULL )
        {
            p->LineCur = p->LineModel;
            sprintf( p->sError, "Node \"%s\" is not driven; binary constant 0 is assumed.", Ntk_NodeReadName(pNode) );
            Io_ReadPrintErrorMessage( p );
//            goto failure;
            Ntk_NodeSetValueNum( pNode, 2 );
        }
    }
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        // add the fanout structures to the fanins of the node
        Ntk_NodeAddFaninFanout( pNet, pNode );
        // create the variable map of the node
        Ntk_NodeAssignVm( pNode );
    }
/*   
    // (5) make sure that everything is okay with the network structure
    // makes no sense to run Ntk_NetworkAcyclic() here because there are no COs yet...
    if ( !Ntk_NetworkCheck( pNet ) )
    {
        p->LineCur = 0;
        sprintf( p->sError, "Network check has failed." );
        Io_ReadPrintErrorMessage( p );
        goto failure;
    } 
*/
    // (6) derive functionality for the internal nodes (MVR and/or MVSOP)
    Ntk_NetworkForEachNode( pNet, pNode )
        if ( Io_ReadNodeFunctions( p, pNode ) )
            goto failure;

    // (8) create latch reset tables and add the latches
    for ( i = 0; i < p->nDotLatches; i++ )
        if ( Io_ReadNetworkLatch( p, i, pNet ) )
            goto failure;

    // (7) introduce the pure PO nodes and connect them to the internal nodes
    for ( i = 0; i < array_n(p->aOutputs); i++ )
    {
        pNode = array_fetch( Ntk_Node_t *, p->aOutputs, i );
        if ( Ntk_NodeIsCi(pNode) )
        {
//            fprintf( Ntk_NodeReadMvsisOut(pNode), "Warning: input and output named \"%s\": ", Ntk_NodeReadName(pNode) );
            Ntk_NetworkTransformCiToCo( pNet, pNode );
//            fprintf( Ntk_NodeReadMvsisOut(pNode), "renaming input \"%s\".\n", Ntk_NodeReadName(pNode) );
            continue;
        }
        assert( Ntk_NodeIsInternal(pNode) );
        if ( Ntk_NodeIsBinaryBuffer(pNode) && Ntk_NodeReadFanoutNum(pNode) == 0 )
            Ntk_NetworkTransformNodeIntToCo( pNet, pNode );
        else
            Ntk_NetworkAddNodeCo( pNet, pNode, 1 );
    }

    // adjust latch input nodes 
    // they could change after the label COs are added
    Ntk_NetworkForEachLatch( pNet, pLatch )
        Ntk_LatchAdjustInput( pNet, pLatch );

    // (9) change the fanin order
    Ntk_NetworkOrderFanins( pNet );

    // use the default whenever possible
    Ntk_NetworkForceDefault( pNet );
    
    // if the spec is not given, set the current network as its own spec
    if ( Ntk_NetworkReadSpec(pNet) == NULL )//&& p->Type == IO_FILE_BLIF_MV )
        Ntk_NetworkSetSpec( pNet, Ntk_NetworkRegisterNewName(pNet, p->FileName) );

    // clean the data fiels (otherwise the network appears to have mapping binding)
    Ntk_NetworkForEachNode( pNet, pNode )
        Ntk_NodeSetData( pNode, NULL );

    // (10) make sure that everything is okay with the network structure
    if ( !Ntk_NetworkCheck( pNet ) )
    {
        p->LineCur = 0;
        sprintf( p->sError, "The second network check has failed." );
        Io_ReadPrintErrorMessage( p );
        goto failure;
    }
    return pNet;

failure:
    Ntk_NetworkDelete( pNet );
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkName( Io_Read_t * p, Ntk_Network_t * pNet )
{
    char * pName, * pTemp;
    assert( strncmp( p->pLines[p->LineModel], ".model", 6 ) == 0 );
    // skip the ".model" word and get the network name
    pTemp = strtok( p->pLines[p->LineModel] + 7, " \t" );
    // register the name
    pName = Ntk_NetworkRegisterNewName( pNet, pTemp );
    // assign the name
    Ntk_NetworkSetName( pNet, pName );
    // get the next token
    pTemp = strtok( NULL, " \t" );
    if ( pTemp != NULL )
    {
        p->LineCur = p->LineModel;
        sprintf( p->sError, "Trailing symbols on .model line (%s).", pTemp );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkSpec( Io_Read_t * p, Ntk_Network_t * pNet )
{
    char * pName, * pTemp;
    // check if there is a spec line
    if ( p->LineSpec < 0 )
        return 0;
    assert( strncmp( p->pLines[p->LineSpec], ".spec", 5 ) == 0 );
    // skip the ".spec" word and get the network name
    pTemp = strtok( p->pLines[p->LineSpec] + 6, " \t" );
    // register the name
    pName = Ntk_NetworkRegisterNewName( pNet, pTemp );
    // assign the name
    Ntk_NetworkSetSpec( pNet, pName );
    // get the next token
    pTemp = strtok( NULL, " \t" );
    if ( pTemp != NULL )
    {
        p->LineCur = p->LineSpec;
        sprintf( p->sError, "Trailing symbols on .spec line (%s).", pTemp );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Reads the list of primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkCis( Io_Read_t * p, int Line, Ntk_Network_t * pNet )
{
    char * pTemp;
    // make sure this is indeed the .inputs line
    assert( strncmp( p->pLines[p->pDotInputs[Line]], ".inputs", 7 ) == 0 );
    // skip the ".inputs" word
    pTemp = strtok( p->pLines[p->pDotInputs[Line]], " \t" );
    // read the PI names one by one
    while ( (pTemp = strtok( NULL, " \t" )) )
    {
        if ( Ntk_NetworkFindNodeByName( pNet, pTemp ) )
        {
            p->LineCur = p->pDotInputs[Line];
            sprintf( p->sError, "Re-definion of primary input \"%s\".", pTemp );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
        if ( p->fParsingExdcNet )
        {
            if ( !Ntk_NetworkFindNodeByName( p->pNet, pTemp ) )
            {
                p->LineCur = p->pDotInputs[Line];
                sprintf( p->sError, "CI \"%s\" of the EXDC network is missing in the primary network.", pTemp );
                Io_ReadPrintErrorMessage( p );
                return 1;
            }
        }
        Ntk_NetworkFindOrAddNodeByName( pNet, pTemp, MV_NODE_CI );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Reads the list of primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkCos( Io_Read_t * p, int Line, Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    char * pTemp;
    // make sure this is indeed the .outputs line
    assert( strncmp( p->pLines[p->pDotOutputs[Line]], ".outputs", 8 ) == 0 );
    // skip the ".outputs" word
    pTemp = strtok( p->pLines[p->pDotOutputs[Line]], " \t" );
    // read the PO names one by one
    while ( (pTemp = strtok( NULL, " \t" )) )
    {
        // in some benchmarks (for example, "c2670.blif")
        // some CO nodes have the same name as CI nodes
        // following the tradition of SIS, these CI nodes are renamed...
        pNode = Ntk_NetworkFindNodeByName( pNet, pTemp );
        if ( pNode && !Ntk_NodeIsCi(pNode) )
        {
            p->LineCur = p->pDotOutputs[Line];
            sprintf( p->sError, "Re-definion of primary output \"%s\".", pTemp );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
        if ( pNode == NULL )
            pNode = Ntk_NetworkFindOrAddNodeByName( pNet, pTemp, MV_NODE_INT );
        if ( p->fParsingExdcNet )
        {
            if ( !Ntk_NetworkFindNodeByName( p->pNet, pTemp ) )
            {
                p->LineCur = p->pDotInputs[Line];
                sprintf( p->sError, "CO \"%s\" of the EXDC network is missing in the primary network.", pTemp );
                Io_ReadPrintErrorMessage( p );
                return 1;
            }
        }
        array_insert_last( Ntk_Node_t *, p->aOutputs, pNode );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Adds the latch structure to the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkLatchInputOutput( Io_Read_t * p, int Latch, Ntk_Network_t * pNet )
{
    Io_Latch_t * pIoLatch;
    Ntk_Node_t * pInput, * pOutput;
 
    // get the Io_Latch_t pointer
    pIoLatch = p->pIoLatches + Latch;

    // check if latch output already exist as a PI
    pOutput = Ntk_NetworkFindNodeByName( pNet, pIoLatch->pOutputName );
    if ( pOutput == NULL )
    { // create a new PI node to be driven by the latch
        pOutput = Ntk_NetworkFindOrAddNodeByName( pNet, pIoLatch->pOutputName, MV_NODE_CI );
        // mark the PO as only belonging to the latch
        Ntk_NodeSetSubtype( pOutput, MV_BELONGS_TO_LATCH );
    }
    else 
    {
        assert( Ntk_NodeIsInternal(pOutput) );
        // this is a special case when the latch output is also a PO
        // transform the PO node to PI
        // this way, it will later be given a different name
        // this case is similar to those cases when
        // the same CI and CO name appears in some benchmarks 
        Ntk_NetworkTransformNodeIntToCi( pNet, pOutput );
        // mark the PO as only belonging to the latch
        Ntk_NodeSetSubtype( pOutput, MV_BELONGS_TO_LATCH );
    }
    assert( Ntk_NodeIsCi(pOutput) );

    // check if latch input already exist as a PO
    pInput = Ntk_NetworkFindNodeByName( pNet, pIoLatch->pInputName );
    if ( pInput == NULL )
    { // create a new internal node to drive the latch
        pInput = Ntk_NetworkFindOrAddNodeByName( pNet, pIoLatch->pInputName, MV_NODE_INT );
        // mark the PO as only belonging to the latch
        Ntk_NodeSetSubtype( pInput, MV_BELONGS_TO_LATCH );
        array_insert_last( Ntk_Node_t *, p->aOutputs, pInput );
    }
    assert( Ntk_NodeIsInternal(pInput) );
    // in some cases, the Latch input can be a PI
    // we need to treat this case by adding a buffer, 
    // which is currently not done...
    return 0;
}


/**Function*************************************************************

  Synopsis    [Reads the internal node.]

  Description [This procedure creates the internal node using the 
  information available from preparsing. This procedure does not
  derive the functionality of the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkInternalNode( Io_Read_t * p, int Node, Ntk_Network_t * pNet )
{
    Io_Node_t * pIoNode;
    Ntk_Node_t * pNode, * pFanin;
    char * pLine, * pTemp, * pPrev;

    // set the current parsing line
    pIoNode = p->pIoNodes + Node;
    p->LineCur = pIoNode->LineNames;

    // find or create the node by name
    pNode = Ntk_NetworkFindOrAddNodeByName( pNet, pIoNode->pOutput, MV_NODE_INT );
    // make sure the node is not assigned yet
    if ( Ntk_NodeReadFaninNum(pNode) )
    {
        sprintf( p->sError, "Node \"%s\" is multiply defined.", Ntk_NodeReadName(pNode) );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }

    // get the .names line of the node
    pLine = p->pLines[ pIoNode->LineNames ];
    // make sure this is indeed the .names line
    assert( (strncmp( pLine, ".names", 6 ) == 0) || 
        ( (p->Type == IO_FILE_BLIF_MV) && (strncmp( pLine, ".table", 6 ) == 0) ) );

    // skip the ".names" word
    pTemp = strtok( pLine, " \t" );
    pPrev = NULL;
    // read the fanins names, one by one
    while ( (pTemp = strtok( NULL, " \t" )) )
    {
        if ( pPrev )
        {
            // find or create the fanin by name
            pFanin = Ntk_NetworkFindOrAddNodeByName( pNet, pPrev, MV_NODE_INT );
            // add the fanin to the fanin list of the node
            Ntk_NodeAddFanin( pNode, pFanin );
        }
        // store the current pointer
        pPrev = pTemp;
    }
    // make sure the last signal is the output
    assert( strcmp( pPrev, Ntk_NodeReadName(pNode) ) == 0 );

    // store the pointer to the Io_Node_t structure with the node
    pIoNode = p->pIoNodes + Node;
    Ntk_NodeSetData( pNode, (char*)pIoNode );

    // if this is BLIF-MVS, we need to get the number of values 
    // of each non-binary variable, including the PIs
    if ( p->Type == IO_FILE_BLIF_MVS )
        if ( Io_ReadNetworkInternalNodeMvs( p, pNode ) )
            return 1;

    // assign the default value and the number of values
    Ntk_NodeSetValueNum( pNode, pIoNode->nValues );

    // update the largest fanin count 
    if ( p->nFaninsMax < Ntk_NodeReadFaninNum(pNode) ) 
        p->nFaninsMax = Ntk_NodeReadFaninNum(pNode);
    return 0;
}


/**Function*************************************************************

  Synopsis    [Adds the latch structure to the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkLatch( Io_Read_t * p, int Latch, Ntk_Network_t * pNet )
{
    Io_Latch_t * pIoLatch;
    Io_Node_t * pIoNode;
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pNode, * pFanin;
    char * pLine, * pTemp, * pPrev;

    // get the IO latch pointer
    pIoLatch = p->pIoLatches + Latch;
    pIoNode = &pIoLatch->IoNode;

    if ( p->Type == IO_FILE_BLIF )
    {
        pLatch = Ntk_LatchCreate( pNet, NULL, pIoLatch->ResValue, pIoLatch->pInputName, pIoLatch->pOutputName );
        Ntk_NetworkAddLatch( pNet, pLatch );
        return 0;
    }

    // set the current parsing line
    p->LineCur = pIoNode->LineNames;

    // read the reset table
    // create the new node
    pNode = Ntk_NodeCreate( pNet, NULL, MV_NODE_INT, pIoNode->nValues );
    Ntk_NodeSetData( pNode, (char*)pIoNode );

    // get the .res line of the latch
    pLine = p->pLines[ pIoNode->LineNames ];
    // get the first word on the line
    pTemp = strtok( pLine, " \t" );
    assert( strncmp( pTemp, ".r", 2 ) == 0 );
    // read the fanins names, one by one
    pPrev = NULL;
    while ( (pTemp = strtok( NULL, " \t" )) )
    {
        if ( pPrev )
        {
            // get the node with this name
            if ( (pFanin = Ntk_NetworkFindNodeByName( pNet, pPrev )) == NULL )
            {
                sprintf( p->sError, "Fanin \"%s\" of reset table is not in the network.", pPrev );
                Io_ReadPrintErrorMessage( p );
                return 1;
            }
            // add the fanin
            Ntk_NodeAddFanin( pNode, pFanin );
        }
        // store the current pointer
        pPrev = pTemp;
    }
    // make sure the last signal is the output
    assert( strcmp( pPrev, pIoLatch->pOutputName ) == 0 );

    // update the largest fanin count 
    if ( p->nFaninsMax < Ntk_NodeReadFaninNum(pNode) ) 
        p->nFaninsMax = Ntk_NodeReadFaninNum(pNode);

    // create the variable map of the node
    Ntk_NodeAssignVm( pNode );

    // derive the functionality of this node
    Ntk_NodeSetName( pNode, pIoLatch->pOutputName );
    Io_ReadNodeFunctions( p, pNode );
    Ntk_NodeSetName( pNode, NULL );

    // finally add the latch with this node
    pLatch = Ntk_LatchCreate( pNet, pNode, pIoLatch->ResValue, pIoLatch->pInputName, pIoLatch->pOutputName );
    Ntk_NetworkAddLatch( pNet, pLatch );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sets the number of values of the primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadNetworkCiSetValue( Io_Read_t * p, Ntk_Node_t * pNode )
{
    int nValues;
    nValues = 2;
    if ( p->tName2Values )
        st_lookup( p->tName2Values, (char *)Ntk_NodeReadName(pNode), (char**)&nValues );
    Ntk_NodeSetValueNum( pNode, nValues );
    if ( p->nValuesMax < nValues )
        p->nValuesMax = nValues;
}


/**Function*************************************************************

  Synopsis    [Determine the number of values by scanning the first line of the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkInternalNodeMvs( Io_Read_t * p, Ntk_Node_t * pNode )
{
    Io_Node_t * pIoNode;
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pFanin;
    char * pEnd, * pTemp;
    int nValues, i;

    // remember the place in the first line where the end should be
    pIoNode = (Io_Node_t *)Ntk_NodeReadData(pNode);
    assert( pIoNode->LineTable + 1 < p->nLines );

    // set the current parsing line
    p->LineCur = pIoNode->LineTable;

    // start iterating through the cubes 
    // and at the same time iterate through the fanins
    pTemp = strtok( p->pLines[ pIoNode->LineTable ], " \t" );
    nValues = strlen(pTemp);

    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        if ( pTemp == NULL )
        {
            sprintf( p->sError, "Not enough literals in the cube." );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
        if ( nValues > 2 )
            Io_ReadLinesAddNumValues( p, Ntk_NodeReadName(pFanin), nValues );

        pTemp = strtok( NULL, " \t" );
        nValues = strlen(pTemp);
    }

    // add the output name
    if ( pTemp == NULL )
    {
        sprintf( p->sError, "Not enough literals in the cube." );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    if ( nValues > 2 )
        Io_ReadLinesAddNumValues( p, pIoNode->pOutput, nValues );

    // set the given number of values at the node
    pIoNode->nValues = nValues;
    // get the default value
    pIoNode->Default = -1;
    for ( i = 0; i < nValues; i++ )
        if ( pTemp[i] == 'D' || pTemp[i] == 'd' )
            pIoNode->Default = i;

    // check if something remains in the cube
    pTemp = strtok( NULL, " \t" );
    if ( pTemp != NULL )
    {
        sprintf( p->sError, "Trailing symbols after the last literal (%s).", pTemp );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }

    // strtok() has broken the cube into pieces; fix it
    pEnd = p->pLines[ pIoNode->LineTable + 1 ] - 1;
    for ( pTemp = p->pLines[ pIoNode->LineTable ]; pTemp < pEnd; pTemp++ )
        if ( *pTemp == '\0' )
            *pTemp = ' ';
    // make sure there is 0 before the beginning of the next cube
    *pTemp = '\0';
    return 0;
}

/**Function*************************************************************

  Synopsis    [Reads a .input_arrival line]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkInputArr( Io_Read_t * p, int Line, Ntk_Network_t * pNet )
{
    char * pTemp, * pFoo;
    Ntk_Node_t * pNodePI;
    double dFall, dRise;

    // make sure this is indeed the .inputs line
    assert( strncmp( p->pLines[p->pDotInputArr[Line]], ".input_arrival", 14 ) == 0 );

    // skip the ".input_arrival" word
    pTemp = strtok( p->pLines[p->pDotInputArr[Line]], " \t" );

    pTemp = strtok( NULL, " \t" );
    if ( pTemp == NULL )
    {
	sprintf( p->sError, ".input_arrival: Need primary input name." );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }

    pNodePI = Ntk_NetworkFindNodeByName( pNet, pTemp );
    
    // TODO: To also check MV_BELONGS_TO_NET 
    if ( pNodePI == NULL || !Ntk_NodeIsCi(pNodePI)  )
    {
	sprintf( p->sError, ".input_arrival: No primary input called %s found.", pTemp );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }

    pTemp = strtok( NULL, " \t" );

    if ( pTemp == NULL ) 
    {
	sprintf( p->sError, ".input_arrival: Need value for rise time." );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }

    dRise = strtod( pTemp, &pFoo );

    if ( *pFoo != '\0' )
    {
	sprintf( p->sError, ".input_arrival: Bad value (%s) for rise time.", pTemp );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }
	  
    pTemp = strtok( NULL, " \t" );

    if ( pTemp == NULL ) 
    {
	sprintf( p->sError, ".input_arrival: Need value for fall time." );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }

    dFall = strtod( pTemp, &pFoo );

    if ( *pFoo != '\0' )
    {
	sprintf( p->sError, ".input_arrival: Bad value (%s) for fall time.", pTemp );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }

    pTemp = strtok( NULL, " \t" );

    if ( pTemp && *pTemp )
    {
	sprintf( p->sError, ".input_arrival: Ignoring trailing characters (%s).", pTemp );
	Io_ReadPrintErrorMessage( p );
    }

    Ntk_NodeSetArrTimeRise( pNodePI, dRise );
    Ntk_NodeSetArrTimeFall( pNodePI, dFall );

    // printf ( "\nINFO: Node %s, Rise %g, Fall %g\n", Ntk_NodeGetNameLong( pNodePI ), dRise, dFall );

    return 0;
}

/**Function*************************************************************

  Synopsis    [Reads a .default_input_arrival line]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNetworkDefaultInputArr( Io_Read_t * p, Ntk_Network_t * pNet )
{
    char * pTemp, * pFoo;
    double dFall, dRise;

    if ( p->pLineDefaultInputArrival == NULL )
	    return 0;

    // make sure this is indeed the .inputs line
    assert( strncmp( p->pLineDefaultInputArrival, ".default_input_arrival", 22 ) == 0 );

    // skip the ".default_input_arrival" word
    pTemp = strtok( p->pLineDefaultInputArrival, " \t" );

    pTemp = strtok( NULL, " \t" );

    if ( pTemp == NULL ) 
    {
	sprintf( p->sError, ".default_input_arrival: Need value for rise time." );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }

    dRise = strtod( pTemp, &pFoo );

    if ( *pFoo != '\0' )
    {
	sprintf( p->sError, ".default_input_arrival: Bad value (%s) for rise time.", pTemp );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }
	  
    pTemp = strtok( NULL, " \t" );

    if ( pTemp == NULL ) 
    {
	sprintf( p->sError, ".default_input_arrival: Need value for fall time." );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }

    dFall = strtod( pTemp, &pFoo );

    if ( *pFoo != '\0' )
    {
	sprintf( p->sError, ".default_input_arrival: Bad value (%s) for fall time.", pTemp );
	Io_ReadPrintErrorMessage( p );
	return 1;
    }

    pTemp = strtok( NULL, " \t" );

    if ( pTemp && *pTemp )
    {
	sprintf( p->sError, ".default_input_arrival: Bad trailing characters (%s).", pTemp );
	Io_ReadPrintErrorMessage( p );
        return 1;
    }

    Ntk_NetworkSetDefaultArrTimeRise( pNet, dRise );
    Ntk_NetworkSetDefaultArrTimeFall( pNet, dFall );
    
    // printf ( "\nINFO: Default: Rise %g, Fall %g\n", dRise, dFall );

    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


