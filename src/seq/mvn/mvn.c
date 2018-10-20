/**CFile****************************************************************

  FileNameIn  [mvn.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with FSMs on the network level.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvn.c,v 1.11 2005/06/02 03:34:21 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Mvn_CommandNetProduct     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetIOEncode    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetLatchSplit  ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetLatchExpose ( Mv_Frame_t * pMvsis, int argc, char ** argv );

static int Mvn_CommandNetExtractSeqDc( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetExtractSeqDc2( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetStgExtract  ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetStgExtract2 ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetSolve       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetSolve2      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetSplit       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Mvn_CommandNetSplit2      ( Mv_Frame_t * pMvsis, int argc, char ** argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_Init( Mv_Frame_t * pMvsis )
{
#ifdef BALM
    Cmd_CommandAdd( pMvsis, "MV network", "extract_aut",              Mvn_CommandNetStgExtract,    0 ); 
    Cmd_CommandAdd( pMvsis, "MV network", "latch_expose",             Mvn_CommandNetLatchExpose,   1 ); 
    Cmd_CommandAdd( pMvsis, "MV network", "latch_split",              Mvn_CommandNetSplit2,        0 ); 
    Cmd_CommandAdd( pMvsis, "MV network", "solve_fsm_equ",            Mvn_CommandNetSolve,         0 ); 
#else
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "net_product",    Mvn_CommandNetProduct,       1 ); 
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "io_encode",      Mvn_CommandNetIOEncode,      1 ); 
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "latch_split",    Mvn_CommandNetLatchSplit,    0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "latch_expose",   Mvn_CommandNetLatchExpose,   1 ); 

    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "extract_seq_dc", Mvn_CommandNetExtractSeqDc,  0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "_extract_seq_dc",Mvn_CommandNetExtractSeqDc2, 0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "stg_extract",    Mvn_CommandNetStgExtract,    0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "_stg_extract",   Mvn_CommandNetStgExtract2,   0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "solve",          Mvn_CommandNetSolve,         0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "_solve",         Mvn_CommandNetSolve2,        0 ); 
//    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "split",         Mvn_CommandNetSplit,         0 ); 
    Cmd_CommandAdd( pMvsis, "Sequential synthesis", "_split",         Mvn_CommandNetSplit2,        0 ); 
#endif

}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_End()
{
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_CommandNetProduct( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet, * pNet1, * pNet2;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn1, * FileNameIn2;
    char * ppNames1[100], * ppNames2[100];
    int fVerbose;
    int nPairs, c, i;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fprintf( pErr, "Implementation of this command is not finished.\n" );
    return 0;

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the file names
    if ( argc < 3 )
        goto usage;

    FileNameIn1 = argv[util_optind+0];
    FileNameIn2 = argv[util_optind+1];

    // check that the files are okay
    if ( (pFile = fopen( FileNameIn1, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn1 );
        goto usage;
    }
    fclose( pFile );
//    pNet1 = Io_ReadKiss( pMvsis, FileNameIn1, 1 );
    pNet1 = Io_ReadNetwork( pMvsis, FileNameIn1 );

    // check that the files are okay
    if ( (pFile = fopen( FileNameIn2, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn2 );
        goto usage;
    }
    fclose( pFile );
//    pNet2 = Io_ReadKiss( pMvsis, FileNameIn2, 1 );
    pNet2 = Io_ReadNetwork( pMvsis, FileNameIn2 );

    if ( pNet1 == NULL || pNet2 == NULL )
    {
        fprintf( pErr, "The product operation failed.\n" );
        Ntk_NetworkDelete( pNet1 );
        Ntk_NetworkDelete( pNet2 );
        return 1;
    }
    
    // get the name pairs
    nPairs = Mvn_ReadNamePairs( pErr, pNet1, pNet2, argv + util_optind + 2, argc - 3, ppNames1, ppNames2 );
    if ( nPairs == -1 )
    {
        fprintf( pErr, "Cannot correctly scan the name pairs.\n" );
        goto usage;
    }

    // create the network
    Mvn_NetAutomataProduct( pNet1, pNet2, ppNames1, ppNames2, nPairs );
    Ntk_NetworkDelete( pNet2 );
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNet1 );

    for ( i = 0; i < nPairs; i++ )
    {
        FREE( ppNames1[i] );
        FREE( ppNames2[i] );
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: net_product <file1> <file2> [in1=out2 [out1=in2 [...]]]\n");
	fprintf( pErr, "       derives MV network of the product of two automata given as MV networks\n" );
	fprintf( pErr, "       while asserting the equality of input/output signals\n" );
	fprintf( pErr, " <file1>  : the MV network file representing the first automaton\n" );
	fprintf( pErr, " <file2>  : the MV network file representing the second automaton\n" );
	fprintf( pErr, " in1=out2 : equation asserting equality of network signals\n" );
	fprintf( pErr, "            the first signal in an equation is from the first network\n" );
	fprintf( pErr, "            the second signal in an equation is from the second network\n" );
	fprintf( pErr, "            spaces are not allowed inside the equations\n" );
//	fprintf( pErr, "        -v     : toggles verbose [default = disabled]\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_CommandNetIOEncode( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    int fVerbose;
    int fEncodeLatches;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
//    fprintf( pErr, "Implementation of this command is not finished.\n" );
//    return 0;

    fVerbose = 0;
    fEncodeLatches = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "lhv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'l':
				fEncodeLatches ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // create the network
    if ( Mvn_NetworkIOEncode( pNet, fEncodeLatches ) )
    {
        fprintf( pErr, "Encoding PIs/POs has failed.\n" );
        return 1;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: io_encode [-l]\n");
	fprintf( pErr, "       logarithmically encodes PIs/POs of the MV network\n" );
	fprintf( pErr, "  -l : toggles encoding latches [default = %s]\n", (fEncodeLatches? "yes": "no") );
//	fprintf( pErr, "        -v     : toggles verbose [default = disabled]\n" );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_CommandNetLatchSplit( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    int fVerbose;
    int nLatches1, iLatch, c;
    char * FileNameIn1;
    char * FileNameIn2;
    int FileType;
    Ntk_Latch_t * pLatch;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    nLatches1 = -1;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nLatches1 = atoi(argv[util_optind]);
                util_optind++;
                if ( nLatches1 < 0 ) 
                    goto usage;
                break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1; 
    }

    // get the file names
    if ( argc < 3 )
        goto usage;

    FileNameIn1 = argv[util_optind];
    FileNameIn2 = argv[util_optind + 1];

    if ( nLatches1 == -1 )
        nLatches1 = Ntk_NetworkReadLatchNum(pNet) / 2;

    if ( Ntk_NetworkReadLatchNum(pNet) < 1 )
    {
        fprintf( pErr, "The number of latches is less than 2.\n" );
        return 1;
    }

    if ( nLatches1 > Ntk_NetworkReadLatchNum(pNet) )
    {
        fprintf( pErr, "The number of latches to be included in the first part is more than the total number of latches.\n" );
        return 1;
    }

    // remove EXDC if present
    if ( pNet->pNetExdc )
    {
        Ntk_NetworkDelete( pNet->pNetExdc );
        pNet->pNetExdc = NULL;
        fprintf( pErr, "The EXDC network is removed.\n" );
    }


    // make the outputs of the remaining latches look like PIs
    iLatch = 0; 
    Ntk_NetworkForEachLatch( pNet, pLatch )
        if ( iLatch++ >= nLatches1 )
             Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_NET );
    // get the file type
    FileType = Ntk_NetworkIsBinary(pNet)? IO_FILE_BLIF : IO_FILE_BLIF_MV;

    // write the first network
    Io_WriteNetwork( pNet, FileNameIn1, FileType, 1 );

    // change the outputs of the latches
    iLatch = 0; 
    Ntk_NetworkForEachLatch( pNet, pLatch )
        if ( iLatch++ >= nLatches1 )
             Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_LATCH );
        else
             Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_NET );

    // write the second network
    Io_WriteNetwork( pNet, FileNameIn2, FileType, 1 );

    // change the outputs of the latches
    Ntk_NetworkForEachLatch( pNet, pLatch )
         Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_LATCH );

    assert( Ntk_NetworkCheck( pNet ) );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: latch_split [-i num] <file_out1> <file_out2>\n");
	fprintf( pErr, "       splits the circuit into two parts\n" );
	fprintf( pErr, "    -i num  : the number of latches in the first part [default = half].\n" );
	fprintf( pErr, "<file_out1> : the output BLIF file for the first part\n" );
	fprintf( pErr, "<file_out2> : the output BLIF file for the second part\n" );
//	fprintf( pErr, "        -v     : toggles verbose [default = disabled]\n" );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_CommandNetLatchExpose( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Latch_t * pLatch;
    Ntk_Network_t * pNet;
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    FILE * pOut, * pErr;
    int fVerbose;
    int c;
    int fRemoveOuts;
    int nNodes;

	/* begin UNIVR */
	char * pNamesLatch = NULL;
	char ** pTemps, * pTemp;
    int nParts, iNum1, iNum2, ctrl = 0;
    int countLatch, i, k, nLatches, * pLatchNums;
	/* end UNIVR */

	pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);


    fRemoveOuts = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ohv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'o':
				fRemoveOuts ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

	    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1; 
    }

    // get the file names
//    if ( argc != 1 )
//        goto usage;

    if ( Ntk_NetworkReadLatchNum(pNet) < 0 )
    {
        fprintf( pErr, "The network has no latches.\n" );
        return 1;
    }

    // remove EXDC if present
    if ( pNet->pNetExdc )
    {
        Ntk_NetworkDelete( pNet->pNetExdc );
        pNet->pNetExdc = NULL;
        fprintf( pErr, "The EXDC network is removed.\n" );
    }

    // remove the network outputs
    if ( fRemoveOuts )
    {
        nNodes = 0;
        ppNodes = ALLOC( Ntk_Node_t *, Ntk_NetworkReadCoNum(pNet) );
        Ntk_NetworkForEachCo( pNet, pNode )
            if ( pNode->Subtype == MV_BELONGS_TO_NET )
                ppNodes[nNodes++] = pNode;
        for ( i = 0; i < nNodes; i++ )
            Ntk_NetworkDeleteNode( pNet, ppNodes[i], 1, 1 );
        FREE( ppNodes );
    }

	nLatches  = Ntk_NetworkReadLatchNum( pNet );
	pLatchNums = ALLOC( int, nLatches );


if (argc == 1 || (argc == 2 && fRemoveOuts == 1) ){
 pNamesLatch = ALLOC( char, nLatches );
 ctrl = 1;
 snprintf(pNamesLatch, 5, "%s%d", "0-", nLatches-1);
}
else{
pNamesLatch = argv[util_optind];
}

    // set the latch list to -1
    for ( i = 0; i < nLatches; i++ )
        pLatchNums[i] = -1;

    // split the latch list into comma-sep parts
    pTemps = ALLOC( char *, nLatches );
    pTemp  = strtok( pNamesLatch, "," );
    for ( nParts = 0; pTemp; nParts++ )
    {
        pTemps[nParts] = pTemp;
        pTemp = strtok( NULL, "," );
    }
    // parse the parts
    for ( i = 0; i < nParts; i++ )
    {
        pTemp = strtok( pTemps[i], "-" );
        iNum1 = atoi( pTemp );
        pTemp = strtok( NULL, "-" );
       
	    if ( iNum1 >= nLatches )
            {
                printf( "Latch number %d is too large (can only be 0-%d).\n", iNum1, nLatches-1 );
                free( pTemps );
                goto usage;
            }
		if ( pTemp == NULL )
        {
           
            if ( pLatchNums[iNum1] != -1 )
            {
                printf( "Number %d is listed multiple times in the latch list.\n", iNum1 );
                free( pTemps );
                goto usage;
            }
            pLatchNums[iNum1] = 1;
        }
        else
        {
            iNum2 = atoi( pTemp );
            for ( k = iNum1; k <= iNum2; k++ )
            {
                if ( k >= nLatches )
                {
                    printf( "Latch number %d is too large (can only be 0-%d).\n", iNum2, nLatches-1 );
                    free( pTemps );
                    goto usage;
                }
                if ( pLatchNums[k] != -1 )
                {
                    printf( "Number %d is listed multiple times in the latch list.\n", k );
                    free( pTemps );
                    goto usage;
                }
                pLatchNums[k] = 1;
            }
        }
    }
    free( pTemps );

 
   
	countLatch = 0;
       Ntk_NetworkForEachLatch( pNet, pLatch ){
		if (pLatchNums[countLatch] == 1) /* expose only the latches specified from the command line*/
         Ntk_NetworkTransformCiToCo( pNet, Ntk_LatchReadOutput(pLatch) );
		 countLatch++;
}
        /* end UNIVR*/

	Ntk_NetworkOrderFanins( pNet );
    assert( Ntk_NetworkCheck( pNet ) );

if (ctrl == 1) 
	FREE(pNamesLatch);
FREE(pLatchNums);

    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: latch_expose [-o] <latch_list>\n");
	fprintf( pErr, "       makes the latch outputs visible as POs of the network\n" );
    fprintf( pErr, "        -o     : remove the original POs from the network [default = %s]\n", fRemoveOuts? "yes" : "no" );
	fprintf( pErr, "<latch_list>   : the list of latches to expose [default = all]\n" );
	fprintf( pErr, "                 no spaces are allowed in the latch list\n" );
	fprintf( pErr, "                 the number of latches are zero-based\n" );
	fprintf( pErr, "                 for example: 0,3,5-7,9\n" );
//	fprintf( pErr, "        -v     : toggles verbose [default = disabled]\n" );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_CommandNetExtractSeqDc( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1; 
    }

    if ( Ntk_NetworkReadLatchNum(pNet) == 0 )
    {
        fprintf( pErr, "The current network does not have latches.\n" );
        return 1;
    }

    if ( Mvn_NetworkExtractSeqDcs( pNet ) )
    {
        fprintf( pErr, "Extracting sequential don't-cares has failed.\n" );
        return 1;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: extract_seq_dc\n");
	fprintf( pErr, "       extracts sequential don't-cares and adds them as the EXDC network\n" );
	fprintf( pErr, "        -v     : toggles verbose [default = disabled]\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_CommandNetExtractSeqDc2( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1; 
    }

    if ( Ntk_NetworkReadLatchNum(pNet) == 0 )
    {
        fprintf( pErr, "The current network does not have latches.\n" );
        return 1;
    }

    // create the network
    if ( Mvn_NetworkExtractSeqDcs( pNet ) )
    {
        fprintf( pErr, "Extracting sequential don't-cares has failed.\n" );
        return 1;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _extract_seq_dc\n");
	fprintf( pErr, "       extracts sequential don't-cares and adds them as the EXDC network\n" );
	fprintf( pErr, "        -v     : toggles verbose [default = disabled]\n" );
	return 1;					// error exit 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_CommandNetStgExtract( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    char * pFileName;
    int nStatesLimit;
    int fVerbose, c;
    int fUseFsm;
    int fWriteAut;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fUseFsm = 0;
    fVerbose = 0;
    nStatesLimit = 10000;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "shfvz" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                nStatesLimit = atoi(argv[util_optind]);
                util_optind++;
                if ( nStatesLimit < 0 ) 
                    goto usage;
                break;
			case 'f':
				fUseFsm ^= 1;
				break;
			case 'z':
				fWriteAut ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1; 
    }

    if ( Ntk_NetworkReadLatchNum(pNet) == 0 )
    {
        fprintf( pErr, "The current network does not have latches.\n" );
        return 1;
    }

    // get the file names
    if ( util_optind + 1 > argc )
        goto usage;

    pFileName = argv[util_optind];

    // create the network
    if ( Mvn_NetworkStgExtract( pNet, pFileName, nStatesLimit, fUseFsm, fWriteAut, fVerbose ) )
    {
        fprintf( pErr, "STG extraction has failed.\n" );
        goto usage;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: extract_aut [-s num] [-v] <file_out>\n");
	fprintf( pErr, "       extracts the STG of the current network as an automaton\n" );
    fprintf( pErr, " -s num     : the limit on the number of states [default = %d].\n", nStatesLimit );
//	fprintf( pErr, " -f         : toggles the output as FSM or automaton [default = %s]\n", (fUseFsm? "FSM": "automaton") );
//	fprintf( pErr, " -z         : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " -v         : toggles verbose output [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " <file_out> : the output file with the resulting automaton\n" );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_CommandNetStgExtract2( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    char * pFileName;
    int nStatesLimit;
    int fVerbose, c;
    int fUseFsm;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fUseFsm = 0;
    fVerbose = 0;
    nStatesLimit = 10000;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "shfv" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                nStatesLimit = atoi(argv[util_optind]);
                util_optind++;
                if ( nStatesLimit < 0 ) 
                    goto usage;
                break;
			case 'f':
				fUseFsm ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1; 
    }

    if ( Ntk_NetworkReadLatchNum(pNet) == 0 )
    {
        fprintf( pErr, "The current network does not have latches.\n" );
        return 1;
    }

    // get the file names
    if ( argc < 2 )
        goto usage;

    pFileName = argv[util_optind];

    // create the network
    if ( Mvn_NetworkStgExtract2( pNet, pFileName, nStatesLimit, fUseFsm, fVerbose ) )
    {
        fprintf( pErr, "STG extraction has failed.\n" );
        goto usage;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _stg_extract [-s num] [-f] [-v] <file_out>\n");
	fprintf( pErr, "       extracts the STG and writes it into a KISS file\n" );
	fprintf( pErr, "       the PIs/POs should be binary; the latches can be multi-valued\n" );
	fprintf( pErr, "       (to convert to binary PIs/POs, run command \"io_encode\")\n" );
    fprintf( pErr, " -s num     : the limit on the number of states [default = %d].\n", nStatesLimit );
	fprintf( pErr, " -v         : toggles verbose output [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " -f         : toggles the output as FSM or automaton [default = %s]\n", (fUseFsm? "FSM": "automaton") );
	fprintf( pErr, " <file_out> : the output file with the STG as an automaton\n" );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Mvn_CommandNetSolve( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr, * pFile;
    Ntk_Network_t * pNetF, * pNetS;
    char * pNetNameF, * pNetNameS;
    char * pStringU, * pStringV;
    char * FileNameOut;
    int nStatesLimit, c;
    int fVerbose, fUseFsm;
    int fProgressive;
    int fUseLongNames;
    int fMoore;
    int fWriteAut;
    int fAlgorithm;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fAlgorithm = 0;
    fWriteAut = 0;
    fMoore = 0;
    fUseFsm = 0;
    fVerbose = 0;
    fProgressive = 1;
    fUseLongNames = 0;
    nStatesLimit = 100000;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "sapmlfvhz" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                nStatesLimit = atoi(argv[util_optind]);
                util_optind++;
                if ( nStatesLimit < 0 ) 
                    goto usage;
                break;
			case 'a':
				fAlgorithm ^= 1;
				break;
			case 'l':
				fUseLongNames ^= 1;
				break;
			case 'p':
				fProgressive ^= 1;
				break;
			case 'm':
				fMoore ^= 1;
				break;
			case 'f':
				fUseFsm ^= 1;
				break;
			case 'z':
				fWriteAut ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the file names
    if ( util_optind + 5 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    pNetNameF   = argv[util_optind + 0];
    pNetNameS   = argv[util_optind + 1];
    pStringU    = argv[util_optind + 2];
    pStringV    = argv[util_optind + 3];
    FileNameOut = argv[util_optind + 4];

    // check that the input files are okay
    if ( (pFile = fopen( pNetNameF, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", pNetNameF );
        goto usage;
    }
    fclose( pFile );
    if ( (pFile = fopen( pNetNameS, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", pNetNameS );
        goto usage;
    }
    fclose( pFile );

    // derive the input networks
    pNetF = Io_ReadNetwork( pMvsis, pNetNameF );
    if ( pNetF == NULL )
    {
        fprintf( pErr, "Cannot read in network from file \"%s\".\n", pNetNameF );
        goto usage;
    }
    pNetS = Io_ReadNetwork( pMvsis, pNetNameS );
    if ( pNetS == NULL )
    {
        fprintf( pErr, "Cannot read in network from file \"%s\".\n", pNetNameS );
        goto usage;
    }

    // solve the problem
    if ( Mvn_NetworkSolve( pNetF, pNetS, pStringU, pStringV, FileNameOut, nStatesLimit, 
                             fProgressive, fUseLongNames, fMoore, fVerbose, fWriteAut, fAlgorithm ) )
    {
        fprintf( pErr, "Solving language equation has failed.\n" );
        goto usage;
    }
    // delete the networks
    Ntk_NetworkDelete( pNetF );
    Ntk_NetworkDelete( pNetS );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: solve_fsm_equ [-s num] [-pmlvh] <net_F> <net_S> <U_vars> <V_vars> <file_out>\n");
	fprintf( pErr, "         solves language equation F * X = S when F and S are represented\n" );
	fprintf( pErr, "         as multi-level multi-valued sequential networks (.mv or .blif files);\n" );
	fprintf( pErr, "         components F and S have external inputs I and external outputs O;\n" );
	fprintf( pErr, "         F communicates with the unknown X through U (from F to X) and V (from X to F);\n" );
	fprintf( pErr, "         variables in U can be any variables of F;\n" );
	fprintf( pErr, "         variables in V should belong to the set of the PIs of F;\n" );
	fprintf( pErr, "         variables in U and V should not be duplicated or overlapping\n" );
    fprintf( pErr, "            . . . . . . . . .     \n" );
    fprintf( pErr, "            .               .     \n" );
    fprintf( pErr, "          I .  -----------  . O   \n" );
    fprintf( pErr, "          ---->|    F    |---->   \n" );
    fprintf( pErr, "            .  --|----A---  .     \n" );
    fprintf( pErr, "            .    |    |     .     \n" );
    fprintf( pErr, "            .  U |    | V   . S   \n" );
    fprintf( pErr, "            .  --V----|---  .     \n" );
    fprintf( pErr, "            .  |    X    |  .     \n" );
    fprintf( pErr, "            .  -----------  .     \n" );
    fprintf( pErr, "            . . . . . . . . .     \n" );
    fprintf( pErr, "                                  \n" );
    fprintf( pErr, " -s num     : the limit on the number of states [default = %d].\n", nStatesLimit );
	fprintf( pErr, " -p         : toggles making the result progressive [default = %s]\n", (fProgressive? "yes": "no") );
	fprintf( pErr, " -m         : toggles between Mealy and Moore solution [default = %s]\n", (fMoore? "Moore": "Mealy") );
//	fprintf( pErr, " -f         : toggles the output as FSM or automaton [default = %s]\n", (fUseFsm? "FSM": "automaton") );
	fprintf( pErr, " -l         : toggles the use of long state names [default = %s]\n", (fUseLongNames? "yes": "no") );
//	fprintf( pErr, " -z         : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " -v         : toggles verbose output [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " -h         : print the help message\n" );
	fprintf( pErr, " <net_F>    : the file for network representing component F\n" );
	fprintf( pErr, " <net_S>    : the file for network representing component S\n" );
	fprintf( pErr, " <U_vars>   : the list of U variables (comma-separated, no spaces, or @ when empty)\n" );
	fprintf( pErr, " <V_vars>   : the list of V variables (comma-separated, no spaces)\n" );
	fprintf( pErr, " <file_out> : the output file with resulting solution\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Mvn_CommandNetSolve2( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr, * pFile;
    Ntk_Network_t * pNetF, * pNetS;
    char * pNetNameF, * pNetNameS;
    char * pStringU, * pStringV;
    char * FileNameOut;
    int nStatesLimit, c;
    int fVerbose, fUseFsm;
    int fProgressive;
    int fUseLongNames;
    int fMoore;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fMoore = 0;
    fUseFsm = 0;
    fVerbose = 0;
    fProgressive = 1;
    fUseLongNames = 0;
    nStatesLimit = 100000;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "spmlfvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                nStatesLimit = atoi(argv[util_optind]);
                util_optind++;
                if ( nStatesLimit < 0 ) 
                    goto usage;
                break;
			case 'l':
				fUseLongNames ^= 1;
				break;
			case 'p':
				fProgressive ^= 1;
				break;
			case 'm':
				fMoore ^= 1;
				break;
			case 'f':
				fUseFsm ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    // get the file names
    if ( util_optind + 5 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    pNetNameF   = argv[util_optind + 0];
    pNetNameS   = argv[util_optind + 1];
    pStringU    = argv[util_optind + 2];
    pStringV    = argv[util_optind + 3];
    FileNameOut = argv[util_optind + 4];

    // check that the input files are okay
    if ( (pFile = fopen( pNetNameF, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", pNetNameF );
        goto usage;
    }
    fclose( pFile );
    if ( (pFile = fopen( pNetNameS, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", pNetNameS );
        goto usage;
    }
    fclose( pFile );

    // derive the input networks
    pNetF = Io_ReadNetwork( pMvsis, pNetNameF );
    if ( pNetF == NULL )
    {
        fprintf( pErr, "Cannot read in network from file \"%s\".\n", pNetNameF );
        goto usage;
    }
    pNetS = Io_ReadNetwork( pMvsis, pNetNameS );
    if ( pNetS == NULL )
    {
        fprintf( pErr, "Cannot read in network from file \"%s\".\n", pNetNameS );
        goto usage;
    }

    // solve the problem
    if ( Mvn_NetworkSolve2( pNetF, pNetS, pStringU, pStringV, FileNameOut, nStatesLimit, 
                             fProgressive, fUseLongNames, fMoore, fVerbose ) )
    {
        fprintf( pErr, "Solving language equation has failed.\n" );
        goto usage;
    }
    // delete the networks
    Ntk_NetworkDelete( pNetF );
    Ntk_NetworkDelete( pNetS );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: solve_fsm_equ [-s num] [-pmfvh] <net_F> <net_S> <U_vars> <V_vars> <file_out>\n");
	fprintf( pErr, "         solves language equation F * X = S when F and S are represented\n" );
	fprintf( pErr, "         as multi-level multi-valued sequential networks (.mv or .blif files);\n" );
	fprintf( pErr, "         components F and S have external inputs I and external outputs O;\n" );
	fprintf( pErr, "         F communicates with the unknown X through U (from F to X) and V (from X to F);\n" );
	fprintf( pErr, "         variables in U can be any variables of F;\n" );
	fprintf( pErr, "         variables in V should belong to the set of the PIs of F;\n" );
	fprintf( pErr, "         variables in U and V should not be duplicated or overlapping\n" );
    fprintf( pErr, "         \n" );
    fprintf( pErr, "            . . . . . . . . .     \n" );
    fprintf( pErr, "            .               .     \n" );
    fprintf( pErr, "          I .  -----------  . O   \n" );
    fprintf( pErr, "          ---->|    F    |---->   \n" );
    fprintf( pErr, "            .  --|----A---  .     \n" );
    fprintf( pErr, "            .    |    |     .     \n" );
    fprintf( pErr, "            .  U |    | V   . S   \n" );
    fprintf( pErr, "            .  --V----|---  .     \n" );
    fprintf( pErr, "            .  |    X    |  .     \n" );
    fprintf( pErr, "            .  -----------  .     \n" );
    fprintf( pErr, "            . . . . . . . . .     \n" );
    fprintf( pErr, "                                  \n" );
    fprintf( pErr, " -s num     : the limit on the number of states [default = %d].\n", nStatesLimit );
	fprintf( pErr, " -p         : toggles making the result progressive [default = %s]\n", (fProgressive? "yes": "no") );
	fprintf( pErr, " -m         : toggles between Mealy and Moore solution [default = %s]\n", (fMoore? "Moore": "Mealy") );
	fprintf( pErr, " -f         : toggles the output as FSM or automaton [default = %s]\n", (fUseFsm? "FSM": "automaton") );
	fprintf( pErr, " -l         : toggles the use of long state names [default = %s]\n", (fUseLongNames? "yes": "no") );
	fprintf( pErr, " -v         : toggles verbose output [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " -h         : print the help message\n" );
	fprintf( pErr, " <net_F>    : the BLIF/BLIF-MV file for network representing component F\n" );
	fprintf( pErr, " <net_S>    : the BLIF/BLIF-MV file for network representing component S\n" );
	fprintf( pErr, " <U_vars>   : the list of U variables (comma-separated, no spaces)\n" );
	fprintf( pErr, " <V_vars>   : the list of V variables (comma-separated, no spaces)\n" );
	fprintf( pErr, " <file_out> : the output file with resulting solution in KISS format\n" );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Mvn_CommandNetSplit( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    char * pNamesLatch, * pNamesOut;
    int fVerbose, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1; 
    }

    // get the latch list
    if ( util_optind + 1 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }
    pNamesLatch = argv[util_optind];



    // temporary
    if ( util_optind + 1 != argc )
    {
        fprintf( pErr, "Too many command line arguments.\n" );
        goto usage;
    }

    // get the output list
    if ( util_optind + 2 > argc )
        pNamesOut = argv[util_optind + 1];
    else
        pNamesOut = NULL;

    // check the number of latches
    if ( Ntk_NetworkReadLatchNum(pNet) < 1 )
    {
        fprintf( pErr, "The network has no latches.\n" );
        return 1;
    }

    // remove EXDC if present
    if ( pNet->pNetExdc )
    {
        Ntk_NetworkDelete( pNet->pNetExdc );
        pNet->pNetExdc = NULL;
        fprintf( pErr, "The EXDC network is removed.\n" );
    }

    // read the latch numbers
    if ( Mvn_NetworkSplit( pNet, pNamesLatch, pNamesOut, fVerbose ) )
    {
        fprintf( pErr, "Solving language equation has failed.\n" );
        goto usage;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
//	fprintf( pErr, "Usage: split <latch_list> <output_list>\n");
	fprintf( pErr, "Usage: split [-v] <latch_list>\n");
	fprintf( pErr, "       splits the current network S into two parts: F and X\n" );
	fprintf( pErr, "       generates the script to solve the equation F * X = S\n" );
    fprintf( pErr, "        -v    : toggles verbose [default = %s]\n", fVerbose? "yes" : "no" );
	fprintf( pErr, "<latch_list>  : the list of latches to be included in X\n" );
	fprintf( pErr, "                no spaces are allowed in the latch list\n" );
	fprintf( pErr, "                the numbers of latches are zero-based\n" );
	fprintf( pErr, "                for example: 0,3,5-7,9\n" );
//	fprintf( pErr, "<output_list> : the list of outputs to be produced by X\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Mvn_CommandNetSplit2( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    char * pNamesLatch, * pNamesOut;
    int fVerbose, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'v':
				fVerbose ^= 1;
				break;
			case 'h':
				goto usage;
			default:
				goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1; 
    }

    // get the latch list
    if ( util_optind + 1 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }
    pNamesLatch = argv[util_optind];



    // temporary
    if ( util_optind + 1 != argc )
    {
        fprintf( pErr, "Too many command line arguments.\n" );
        goto usage;
    }



    // get the output list
    if ( util_optind + 2 > argc )
        pNamesOut = argv[util_optind + 1];
    else
        pNamesOut = NULL;

    // check the number of latches
    if ( Ntk_NetworkReadLatchNum(pNet) < 1 )
    {
        fprintf( pErr, "The network has no latches.\n" );
        return 1;
    }

    // remove EXDC if present
    if ( pNet->pNetExdc )
    {
        Ntk_NetworkDelete( pNet->pNetExdc );
        pNet->pNetExdc = NULL;
        fprintf( pErr, "The EXDC network is removed.\n" );
    }

    // read the latch numbers
    if ( Mvn_NetworkSplit2( pNet, pNamesLatch, pNamesOut, fVerbose ) )
    {
        fprintf( pErr, "Solving language equation has failed.\n" );
        goto usage;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
//	fprintf( pErr, "Usage: split <latch_list> <output_list>\n");
	fprintf( pErr, "Usage: latch_split [-v] <latch_list>\n");
	fprintf( pErr, "       splits the current network S into two parts: F and X\n" );
    fprintf( pErr, "       generates the scripts to solve and check the solution\n" );
    fprintf( pErr, "       of the FSM equation F * X = S in two ways\n" );
    fprintf( pErr, "       1) nameS.script - using fsm_equation_solve (creates nameXS.aut)\n" );
    fprintf( pErr, "       2) nameL.script - using a script of automata commands (creates nameXL.aut)\n" );
    fprintf( pErr, "       3) nameSC.script - checks that nameXS.aut*F is contained in S\n" );
    fprintf( pErr, "       4) nameLC.script - checks that nameXL.aut*F is contained in S\n" );
    fprintf( pErr, "        -v    : toggles verbose [default = %s]\n", fVerbose? "yes" : "no" );
	fprintf( pErr, "<latch_list>  : the list of latches to be included in X\n" );
	fprintf( pErr, "                no spaces are allowed in the latch list\n" );
	fprintf( pErr, "                the numbers of latches are zero-based\n" );
	fprintf( pErr, "                for example: 0,3,5-7,9\n" );
//	fprintf( pErr, "<output_list> : the list of outputs to be produced by X\n" );
	return 1;					// error exit 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


