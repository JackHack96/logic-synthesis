/**CFile****************************************************************

  FileName    [ioWriteSplit.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Writes into a file individual logic cones.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioWriteSplit.c,v 1.11 2004/02/19 05:48:11 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static char * IoNetworkSplitOneWrite( Ntk_Network_t * pNet, Ntk_Node_t * pNodeCo, int OutNum, bool fAllInputs, bool fWriteBlif );
static void IoNetworkSplitOneGetParams( Ntk_Node_t * pNodeCo, int * pOutSuppCur, int * pNodeFanCur );
static char * IoNetworkSplitGetGenericName( Ntk_Node_t * pNode, int OutNum );
static char * IoNetworkSplitGetFileName( Ntk_Node_t * pNode, int OutNum, int FileType );
static char * IoNetworkGetRealName( char * pName );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the given output into a BLIF-MV file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void IoNetworkSplit( Ntk_Network_t * pNet, int Output, int OutSuppMin, 
                    int NodeFanMax, bool fAllInputs, bool fWriteBlif, bool fVerbose )
{
    Ntk_Node_t * pNode;
    char * pName;
	int OutSuppCur;
	int NodeFanCur;
	int Counter;

    if ( fWriteBlif && !Ntk_NetworkIsBinary(pNet) )
    {
		fprintf( Ntk_NetworkReadMvsisOut(pNet), "Cannot write MV network into BLIF file; BLIF-MV format is assumed.\n" );
        fWriteBlif = 0;
    }

	if ( Output != -1 )
	{
		if ( Output < 0 || Output >= Ntk_NetworkReadCoNum(pNet) )
		{
			fprintf( Ntk_NetworkReadMvsisOut(pNet), "Selected output number (%d) is not okay.\n", Output );
            return;
		}

		// write one output
		pNode = Ntk_NetworkReadCoNode( pNet, Output );
		pName = IoNetworkSplitOneWrite( pNet, pNode, Output, fAllInputs, fWriteBlif );
		fprintf( Ntk_NetworkReadMvsisOut(pNet), "Output #%d \"%s\" of network \"%s\" is written into file \"%s\".\n", 
            Output, Ntk_NodeReadName(pNode), Ntk_NetworkReadName(pNet), pName);
	}
	else
	{
		// write all outputs
		Counter = 0;
		Ntk_NetworkForEachCo( pNet, pNode )
		{
			if ( OutSuppMin == 0 && NodeFanMax == 1000 )
				IoNetworkSplitOneWrite( pNet, pNode, Counter, fAllInputs, fWriteBlif );
			else
			{
				// get the support size and the max fanin count of nodes for this output
				IoNetworkSplitOneGetParams( pNode, &OutSuppCur, &NodeFanCur );
				if ( OutSuppCur >= OutSuppMin && NodeFanCur <= NodeFanMax )
				{
					fprintf( Ntk_NetworkReadMvsisOut(pNet), "Output #%3d:  Support = %2d. Max fanin count = %2d.\n", Counter, OutSuppCur, NodeFanCur );
					IoNetworkSplitOneWrite( pNet, pNode, Counter, fAllInputs, fWriteBlif );
				}
			}
			Counter++;
		}
		if ( OutSuppMin == 0 && NodeFanMax == 1000 )
			fprintf( Ntk_NetworkReadMvsisOut(pNet), "All outputs of pNetwork \"%s\" are written into separate files.\n", Ntk_NetworkReadName(pNet) );
		else
			fprintf( Ntk_NetworkReadMvsisOut(pNet), "Selected outputs of pNetwork \"%s\" are written into separate files.\n", Ntk_NetworkReadName(pNet) );
	}
}


/**Function*************************************************************

  Synopsis    [Writes the given output into a BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * IoNetworkSplitOneWrite( Ntk_Network_t * pNet, Ntk_Node_t * pNodeCo, int OutNum, bool fAllInputs, bool fWriteBlif )
{
    int FileType = fWriteBlif? IO_FILE_BLIF: IO_FILE_BLIF_MV;
	char FileName[500];
    char * pGenericName, * pFileName;
	Ntk_Node_t * pNode;
	FILE * pFile; 

	// get the file name
    pGenericName = IoNetworkSplitGetGenericName( pNodeCo, OutNum );
    pFileName = IoNetworkSplitGetFileName( pNodeCo, OutNum, FileType );

    // start the file
	sprintf( FileName, "%s", pFileName );
	pFile = fopen( FileName, "w" );

    // collect the DFS array of the CO node
  	Ntk_NetworkDfsNodes( pNet, &pNodeCo, 1, 1 );

	// write the header
	fprintf( pFile, ".model %s\n", pGenericName );

    // write the primary input names
	fprintf( pFile, ".inputs" );
	if ( fAllInputs )
        Io_WriteNetworkCis( pFile, pNet, 1 );
	else
        Io_WriteNetworkCisSpecial( pFile, pNet );
	fprintf( pFile, "\n" );

	// write the primary output name
	fprintf( pFile, ".outputs" );
	fprintf( pFile, " %s", Ntk_NodeReadName(pNodeCo) );
	fprintf( pFile, "\n" );

    if ( FileType == IO_FILE_BLIF_MV )
    {
        // write .mv directives for CIs
	    if ( fAllInputs )
        {
            Ntk_NetworkForEachCi( pNet, pNode )
                if ( Ntk_NodeReadValueNum(pNode) > 2 )
                    fprintf( pFile, ".mv %s %d\n", Ntk_NodeReadName(pNode), Ntk_NodeReadValueNum(pNode) );
        }
        else
        {
            Ntk_NetworkForEachNodeSpecial( pNet, pNode )
                if ( Ntk_NodeIsCi(pNode) && Ntk_NodeReadValueNum(pNode) > 2 )
                    fprintf( pFile, ".mv %s %d\n", Ntk_NodeReadName(pNode), Ntk_NodeReadValueNum(pNode) );
        }

        // write .mv directives for COs
        if ( Ntk_NodeReadValueNum(pNodeCo) > 2 )
            fprintf( pFile, ".mv %s %d\n", Ntk_NodeReadName(pNodeCo), Ntk_NodeReadValueNum(pNodeCo) );

        // write all the .mv directives for the internal nodes on top of the file
        Ntk_NetworkForEachNodeSpecial( pNet, pNode )
            if ( Ntk_NodeIsInternal( pNode ) )
            {
                // if an internal node has a single CO fanout, 
                // the name of this node is the same as CO name
                // and the node's .mv is already written above as the PO's .mv
                if ( Ntk_NodeHasCoName(pNode) )
                        continue;
                // otherwise, write the .mv directive if it is non-binary
                if ( Ntk_NodeReadValueNum(pNode) > 2 )
                    fprintf( pFile, ".mv %s %d\n", Ntk_NodeGetNameLong(pNode), Ntk_NodeReadValueNum(pNode) );
            }
    }

	// write the nodes
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( Ntk_NodeIsInternal( pNode ) )
    		Io_WriteNetworkNode( pFile, pNode, FileType, 0 );

	fprintf( pFile, ".end\n\n" );
	fclose( pFile );
    return pGenericName;
}

/**Function*************************************************************

  Synopsis    [Writes the given output into a BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void IoNetworkSplitOneGetParams( Ntk_Node_t * pNodeCo, int * pOutSuppCur, int * pNodeFanCur )
{
	Ntk_Node_t * pNode;
	int OutSuppCur;
	int NodeFanCur;

	NodeFanCur = 0;
	OutSuppCur = 0;

  	Ntk_NetworkDfsNodes( Ntk_NodeReadNetwork(pNodeCo), &pNodeCo, 1, 1 );
    Ntk_NetworkForEachNodeSpecial( Ntk_NodeReadNetwork(pNodeCo), pNode )
		if ( Ntk_NodeIsCi(pNode) )
			OutSuppCur++;
		else if ( NodeFanCur < Ntk_NodeReadFaninNum(pNode) )
			NodeFanCur = Ntk_NodeReadFaninNum(pNode);

	*pOutSuppCur = OutSuppCur;
	*pNodeFanCur = NodeFanCur;
}

/**Function*************************************************************

  Synopsis    [Writes the given output into a BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * IoNetworkSplitGetGenericName( Ntk_Node_t * pNode, int OutNum )
{
    static char Buffer[500];
    Ntk_Network_t * pNet;
    char * pNetName;
	int nDigits, nOutputs;

    // get the network
    pNet = Ntk_NodeReadNetwork( pNode );
    pNetName = IoNetworkGetRealName( Ntk_NetworkReadName(pNet) );
	// get the number of digits in the output number to be used in the file name
	nOutputs = Ntk_NetworkReadCoNum( pNet );
	for ( nDigits = 0; nOutputs; nOutputs /= 10, nDigits++ );
	// collect all the nodes in the transitive fanin
	sprintf( Buffer, "%s_%0*d_%s", 
        pNetName, nDigits, OutNum, Ntk_NodeGetNamePrintable(pNode) );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Writes the given output into a BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * IoNetworkSplitGetFileName( Ntk_Node_t * pNode, int OutNum, int FileType )
{
    static char Buffer[500];
    char * pCur, * pName;
    pName = IoNetworkSplitGetGenericName(pNode, OutNum);
    // overwrite strange chars with underscores
    for ( pCur = pName; *pCur; pCur++ )
        if ( *pCur == '<' || *pCur == '>' || *pCur == '%' || *pCur == '$' || *pCur == '~' )
            *pCur = '_';
    // create the file name
	sprintf( Buffer, "%s.%s", pName, (FileType == IO_FILE_BLIF)? "blif": "mv" );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Gets the network name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * IoNetworkGetRealName( char * pName )
{
    static char Buffer[500];
    char * pToken, * pTokenPrev1, * pTokenPrev2;

    strcpy( Buffer, pName );
    // get the last two tokens
    pTokenPrev1 = NULL;
    pTokenPrev2 = NULL;
    pToken = strtok( Buffer, "\\/.\t\r\n%$" );
    while ( pToken )
    {
        pTokenPrev1 = pTokenPrev2;
        pTokenPrev2 = pToken;
        pToken = strtok( NULL, "\\/.\t\r\n%$" );
    }
    if ( pTokenPrev1 && (
        strcmp( pTokenPrev2, "iscas" ) == 0 || 
        strcmp( pTokenPrev2, "bench" ) == 0 ||
        strcmp( pTokenPrev2, "blif" ) == 0 ||
        strcmp( pTokenPrev2, "mv" ) == 0 ||
        strcmp( pTokenPrev2, "pla" ) == 0 )   )
        return pTokenPrev1;
    return pTokenPrev2;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


