/**CFile****************************************************************

  FileName    [io.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Commands defined in the IO package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: io.c,v 1.28 2005/07/08 01:01:21 alanmi Exp $]

***********************************************************************/

#include "ioInt.h"
#include "mvInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int IoCommandReadBlif    ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandReadBlifMv  ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandReadBlifMvs ( Mv_Frame_t * pMvsis, int argc, char **argv );

static int IoCommandReadPla     ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandReadPlaMv   ( Mv_Frame_t * pMvsis, int argc, char **argv );

static int IoCommandReadKiss    ( Mv_Frame_t * pMvsis, int argc, char **argv );

static int IoCommandReadBlif    ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandReadBench   ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandWriteBlif   ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandWriteBlifMv ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandWriteBlifMvs( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandWriteBench  ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandWriteGate   ( Mv_Frame_t * pMvsis, int argc, char **argv );

static int IoCommandWritePla    ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int IoCommandWritePlaMv  ( Mv_Frame_t * pMvsis, int argc, char **argv );


static int IoCommandSplit       ( Mv_Frame_t * pMvsis, int argc, char **argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Initializes the I/O package.]

  SideEffects []

  SeeAlso     [Io_End]

******************************************************************************/
void Io_Init( Mv_Frame_t * pMvsis )
{
#ifdef BALM
    Cmd_CommandAdd( pMvsis, "I/O", "read_blif",     IoCommandReadBlif,     1 );
    Cmd_CommandAdd( pMvsis, "I/O", "read_blif_mv",  IoCommandReadBlifMv,   1 );
    Cmd_CommandAdd( pMvsis, "Writing", "write_blif_mv", IoCommandWriteBlifMv,  0 );
    Cmd_CommandAdd( pMvsis, "Reading", "read_kiss",     IoCommandReadKiss,     1 );
#else
    Cmd_CommandAdd( pMvsis, "Reading", "read_blif",     IoCommandReadBlif,     1 );
    Cmd_CommandAdd( pMvsis, "Reading", "read_blif_mv",  IoCommandReadBlifMv,   1 );
    Cmd_CommandAdd( pMvsis, "Reading", "read_blif_mvs", IoCommandReadBlifMvs,  1 );
    Cmd_CommandAdd( pMvsis, "Reading", "read_bench",    IoCommandReadBench,    1 );

    Cmd_CommandAdd( pMvsis, "Reading", "read_pla",      IoCommandReadPla,      1 );
    Cmd_CommandAdd( pMvsis, "Reading", "read_pla_mv",   IoCommandReadPlaMv,    1 );

    Cmd_CommandAdd( pMvsis, "Reading", "read_kiss",     IoCommandReadKiss,     1 );

    Cmd_CommandAdd( pMvsis, "Writing", "write_blif",    IoCommandWriteBlif,    0 );
    Cmd_CommandAdd( pMvsis, "Writing", "write_blif_mv", IoCommandWriteBlifMv,  0 );
    Cmd_CommandAdd( pMvsis, "Writing", "write_blif_mvs",IoCommandWriteBlifMvs, 0 );
    Cmd_CommandAdd( pMvsis, "Writing", "write_bench",   IoCommandWriteBench,   0 );
    Cmd_CommandAdd( pMvsis, "Writing", "write_gate",    IoCommandWriteGate,    0 );

    Cmd_CommandAdd( pMvsis, "Writing", "write_pla",     IoCommandWritePla,     0 );
    Cmd_CommandAdd( pMvsis, "Writing", "write_pla_mv",  IoCommandWritePlaMv,   0 );

    Cmd_CommandAdd( pMvsis, "Various", "split_outputs", IoCommandSplit,        0 );
#endif

}

/**Function********************************************************************

  Synopsis    [Ends the I/O package.]

  SideEffects []

  SeeAlso     [Io_Init]

******************************************************************************/
void Io_End( Mv_Frame_t * pMvsis )
{
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadBlif( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Ntk_Network_t * pNet;
    char * FileName;
    FILE * pFile;
    int c;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    if ( (pFile = fopen( FileName, "r" )) == NULL )
    {
        fprintf( pMvsis->Err, "Cannot open input file \"%s\". ", FileName );
        if ( FileName = Io_ReadFileGetSimilar( FileName, ".mv", ".blif", ".pla", ".mvpla", NULL ) )
            fprintf( pMvsis->Err, "Did you mean \"%s\"?", FileName );
        fprintf( pMvsis->Err, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pNet = Io_ReadNetwork( pMvsis, FileName );
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNet );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: read_blif [-h] <file>\n" );
    fprintf( pMvsis->Err, "   -h     reads the network from file\n" );
    fprintf( pMvsis->Err, "   file   the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadBlifMv( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    return IoCommandReadBlif( pMvsis, argc, argv );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadBlifMvs( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    return IoCommandReadBlif( pMvsis, argc, argv );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadPla( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    return IoCommandReadBlif( pMvsis, argc, argv );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadBench( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Ntk_Network_t * pNet;
    char * FileName;
    FILE * pFile;
    int c;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    if ( (pFile = fopen( FileName, "r" )) == NULL )
    {
        fprintf( pMvsis->Err, "Cannot open input file \"%s\". ", FileName );
        if ( FileName = Io_ReadFileGetSimilar( FileName, ".mv", ".blif", ".pla", ".mvpla", NULL ) )
            fprintf( pMvsis->Err, "Did you mean \"%s\"?", FileName );
        fprintf( pMvsis->Err, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pNet = Io_ReadNetworkBench( pMvsis, FileName );
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNet );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: read_bench [-h] <file>\n" );
    fprintf( pMvsis->Err, "   -h     reads the network in BENCH format\n" );
    fprintf( pMvsis->Err, "   file   the name of a file to read\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadPlaMv( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Ntk_Network_t * pNet;
    char * FileName;
    FILE * pFile;
    int c;
    bool fBinaryOutput;


    fprintf( stdout, "Implementation of this command is not finished.\n" );
    return 0;



    fBinaryOutput = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "bh" ) ) != EOF )
    {
        switch ( c )
        {
			case 'b':
				fBinaryOutput ^= 1;
				break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    if ( (pFile = fopen( FileName, "r" )) == NULL )
    {
        fprintf( pMvsis->Err, "Cannot open input file \"%s\". ", FileName );
        if ( FileName = Io_ReadFileGetSimilar( FileName, "pla", ".mvpla", ".pla", NULL, NULL ) )
            fprintf( pMvsis->Err, "Did you mean \"%s\"?", FileName );
        fprintf( pMvsis->Err, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pNet = Io_ReadPlaMv( pMvsis, FileName, fBinaryOutput );
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNet );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: read_pla_mv [-h] <file>\n" );
    fprintf( pMvsis->Err, "          creates the network with one node\n" );
    fprintf( pMvsis->Err, "   -b     adds the output to the inputs [default = %s]\n", fBinaryOutput? "yes": "no" );
    fprintf( pMvsis->Err, "   -h     prints the help message\n" );
    fprintf( pMvsis->Err, "   file   the name of a file in Espresso PLA-MV format\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandReadKiss( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Ntk_Network_t * pNet;
    char * FileName;
    FILE * pFile;
    int c;
    bool fReadAut;

    fReadAut = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ah" ) ) != EOF )
    {
        switch ( c )
        {
			case 'a':
				fReadAut ^= 1;
				break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    if ( (pFile = fopen( FileName, "r" )) == NULL )
    {
        fprintf( pMvsis->Err, "Cannot open input file \"%s\". ", FileName );
        if ( FileName = Io_ReadFileGetSimilar( FileName, ".kiss", ".kiss2", ".aut", ".stg", NULL ) )
            fprintf( pMvsis->Err, "Did you mean \"%s\"?", FileName );
        fprintf( pMvsis->Err, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pNet = Io_ReadKiss( pMvsis, FileName, fReadAut );
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNet );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: read_kiss [-ah] <file>\n" );
    fprintf( pMvsis->Err, "          reads an KISS FSM file and creates an MV network\n" );
    fprintf( pMvsis->Err, "     -a   reads the file as an I/O automaton [default = %s]\n", fReadAut? "yes": "no" );
    fprintf( pMvsis->Err, "     -h   prints the help message\n" );
    fprintf( pMvsis->Err, "   file   the name of a file in KISS2 format\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteBlif( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char * FileName;
    int c;
    bool fWriteLatches;

    fWriteLatches = 1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "lh" ) ) != EOF )
    {
        switch ( c )
        {
			case 'l':
				fWriteLatches ^= 1;
				break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    // check if the network is binary
    if ( !Ntk_NetworkIsBinary(pMvsis->pNetCur) )
    {
        fprintf( pMvsis->Out, "The current network is not binary. Use \"write_blif_mv\" or \"write_blif_mvs\".\n" );
        return 0;
    }
    // write the file
    Io_WriteNetwork( pMvsis->pNetCur, FileName, IO_FILE_BLIF, fWriteLatches );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: write_blif [-l] [-h] <file>\n" );
    fprintf( pMvsis->Err, "          write the network into a BLIF file\n" );
    fprintf( pMvsis->Err, "     -l : toggle writing latches [default = %s]\n", fWriteLatches? "yes":"no" );
    fprintf( pMvsis->Err, "     -h : print the help massage\n" );
    fprintf( pMvsis->Err, "   file : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteBlifMv( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char * FileName;
    int c;
    bool fWriteLatches;

    fWriteLatches = 1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "lh" ) ) != EOF )
    {
        switch ( c )
        {
			case 'l':
				fWriteLatches ^= 1;
				break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    // write the file
    Io_WriteNetwork( pMvsis->pNetCur, FileName, IO_FILE_BLIF_MV, fWriteLatches );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: write_blif_mv [-l] [-h] <file>\n" );
    fprintf( pMvsis->Err, "          write the network into a BLIF-MV file\n" );
    fprintf( pMvsis->Err, "     -l : toggle writing latches [default = %s]\n", fWriteLatches? "yes":"no" );
    fprintf( pMvsis->Err, "     -h : print the help massage\n" );
    fprintf( pMvsis->Err, "   file : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteBlifMvs( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char * FileName;
    int c;
    bool fWriteLatches;

    fWriteLatches = 1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "lh" ) ) != EOF )
    {
        switch ( c )
        {
			case 'l':
				fWriteLatches ^= 1;
				break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    // write the file
    Io_WriteNetwork( pMvsis->pNetCur, FileName, IO_FILE_BLIF_MVS, fWriteLatches );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: write_blif_mvs [-l] [-h] <file>\n" );
    fprintf( pMvsis->Err, "          write the network into a BLIF-MVS file\n" );
    fprintf( pMvsis->Err, "     -l : toggle writing latches [default = %s]\n", fWriteLatches? "yes":"no" );
    fprintf( pMvsis->Err, "     -h : print the help massage\n" );
    fprintf( pMvsis->Err, "   file : the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWritePla( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char * FileName;
    int c;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    // check if the network is binary
    if ( !Ntk_NetworkIsBinary(pMvsis->pNetCur) )
    {
        fprintf( pMvsis->Out, "Cannot write binary PLA for the MV network.\n" );
        return 0;
    }
    if ( Ntk_NetworkGetNumLevels(pMvsis->pNetCur) > 1 )
    {
        fprintf( pMvsis->Out, "Cannot write PLA for a multi-level network.\n" );
        return 0;
    }
    // write the file
    Io_WritePla( pMvsis->pNetCur, FileName );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: write_pla [-h] <file>\n" );
    fprintf( pMvsis->Err, "   -h     write the binary two-level network into a PLA file\n" );
    fprintf( pMvsis->Err, "   file   the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWritePlaMv( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char * NodeName;
    char * FileName;
    int c;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

    if ( argc != util_optind + 2 )
    {
        goto usage;
    }

    // get the input file name
    NodeName = argv[util_optind];
    FileName = argv[util_optind+1];
    // write the file
    Io_WritePlaMv( pMvsis->pNetCur, NodeName, FileName );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: write_pla_mv [-h] [node] <file>\n" );
    fprintf( pMvsis->Err, "   -h     write the MV PLA file for one node\n" );
    fprintf( pMvsis->Err, "   node   the name of the node to write into a file\n" );
    fprintf( pMvsis->Err, "   file   the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteBench( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char * FileName;
    int c;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    // write the file
    Io_WriteNetworkBench( pMvsis->pNetCur, FileName );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: write_bench [-h] <file>\n" );
    fprintf( pMvsis->Err, "   -h     write the newtork of AND2's and NOT's into .bench file\n" );
    fprintf( pMvsis->Err, "   file   the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandWriteGate( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char * FileName;
    int c;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary( pMvsis->pNetCur ) )
    {
        fprintf( pMvsis->Out, "Currently can only write mapped binary networks.\n" );
        return 0;
    }

    if ( !Ntk_NetworkIsMapped( pMvsis->pNetCur ) )
    {
        fprintf( pMvsis->Out, "The network is not mapped using a standard cell library.\n" );
        fprintf( pMvsis->Out, "Use commands \"map\" or \"attach\" and try again.\n" );
        return 0;
    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }

    // get the input file name
    FileName = argv[util_optind];
    // write the file
    Io_NetworkWriteMappedBlif( pMvsis->pNetCur, FileName );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: write_gate [-h] <file>\n" );
    fprintf( pMvsis->Err, "   -h     write the mapped network using .gate's\n" );
    fprintf( pMvsis->Err, "   file   the name of the file to write\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Splits the multi-output function into many single output ones.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IoCommandSplit( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
	int Output     = -1;
	int fWriteBlif = 1;
	int fAllInputs = 0;
	int fVerbose   = 0;
	int OutSuppMin = 0;
	int NodeFanMax = 1000;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "o:s:f:ibv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'o':
				Output = atoi(util_optarg);
				break;
			case 's':
				OutSuppMin = atoi(util_optarg);
				break;
			case 'f':
				NodeFanMax = atoi(util_optarg);
				break;
			case 'i':
				fAllInputs ^= 1;
				break;
			case 'b':
				fWriteBlif ^= 1;
				break;
			case 'v':
				fVerbose ^= 1;
				break;
			default:
				goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( pOut, "Empty network.\n" );
        return 0;
    }

    IoNetworkSplit( pNet, Output, OutSuppMin, NodeFanMax, fAllInputs, fWriteBlif, fVerbose );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: split_outputs [-o num] [-s num] [-f num] [-v]\n");
	fprintf( pErr, "       writes primary output cone(s) into separate BLIF-MV file(s)\n" );
	fprintf( pErr, "        -o num : the zero-based output number to write [default = all]\n" );
	fprintf( pErr, "        -s num : the lower limit on the output support size [default = %d]\n", OutSuppMin );
	fprintf( pErr, "        -f num : the upper limit on the nodes' fanin count [default = %d]\n", NodeFanMax );
	fprintf( pErr, "        -i     : toggles between all/support inputs [default = %s]\n", (fAllInputs? "all": "support") );
	fprintf( pErr, "        -b     : toggles between BLIF and BLIF-MV [default = %s]\n", (fWriteBlif? "BLIF": "BLIF-MV") );
	fprintf( pErr, "        -v     : toggles verbose [default = disabled]\n" );
	return 1;					// error exit 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


