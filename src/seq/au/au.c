/**CFile****************************************************************

  FileNameIn  [au.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: au.c,v 1.4 2005/07/08 01:24:34 alanmi Exp $]

***********************************************************************/

#include "auInt.h" 

#ifndef WIN32
#define _unlink unlink
#endif

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Au_CommandAutoReadFsm    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoReadFsmMv  ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoReadPara   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoWriteFsm   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoGenerate   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoLanguage   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoSupport    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoComplete   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoComplement ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoDeterminize( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoMinimize   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoDcMin      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoProduct    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoReverse    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoPrefixClose( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoProgressive( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoMoore      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoReduce     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoComb       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoCheck      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoCheckNd    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoDot        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoTest       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoRemove     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoPsa        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Au_CommandAutoShow       ( Mv_Frame_t * pMvsis, int argc, char ** argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM

    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_read_fsm",    Au_CommandAutoReadFsm,     0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_read_fsm_mv", Au_CommandAutoReadFsmMv,   0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_read_para",   Au_CommandAutoReadPara,    0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_write_fsm",   Au_CommandAutoWriteFsm,    0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_generate",    Au_CommandAutoGenerate,    0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_language",    Au_CommandAutoLanguage,    0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_support",     Au_CommandAutoSupport,     0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_complete",    Au_CommandAutoComplete,    0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_complement",  Au_CommandAutoComplement,  0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_determinize", Au_CommandAutoDeterminize, 0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_minimize",    Au_CommandAutoMinimize,    0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_dcmin",       Au_CommandAutoDcMin,       0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_product",     Au_CommandAutoProduct,     0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_reverse",     Au_CommandAutoReverse,     0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_prefix",      Au_CommandAutoPrefixClose, 0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_progressive", Au_CommandAutoProgressive, 0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_moore",       Au_CommandAutoMoore,       0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_reduce",      Au_CommandAutoReduce,      0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_comb",        Au_CommandAutoComb,        0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_check",       Au_CommandAutoCheck,       0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_check_nd",    Au_CommandAutoCheckNd,     0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_dot",         Au_CommandAutoDot,         0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_remove",      Au_CommandAutoRemove,      0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_psa",         Au_CommandAutoPsa,         0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_test",        Au_CommandAutoTest,        0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis (old)", "_show",        Au_CommandAutoShow,        0 ); 
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_End()
{
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoReadFsm( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    if ( !Au_FsmReadKiss( pMvsis, FileNameIn, FileNameOut ) )
    {
        fprintf( pErr, "Converting FSM KISS file into an automaton KISS file has failed.\n" );
        return 1;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _read_fsm <file_in> <file_out>\n");
	fprintf( pErr, "       reads FSM from a KISS file and converts into an automaton\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input FSM\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoReadFsmMv( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    if ( Au_AutoEncodeSymbolicFsm( pMvsis, FileNameIn, FileNameOut ) )
    {
        fprintf( pErr, "Encoding FSM with MV inputs/output from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _read_fsm_mv <file_in> <file_out>\n");
	fprintf( pErr, "       encodes the FSM with MV inputs/outputs\n" );
	fprintf( pErr, " <file_in>  : the input  BLIF-MV file with the input FSM\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting FSM\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoReadPara( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr, * pFile;
    char * pInputString;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 3 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    pInputString = argv[util_optind];
    FileNameIn   = argv[util_optind + 1];
    FileNameOut  = argv[util_optind + 2];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    if ( !Au_AutoReadPara( pMvsis, pInputString, FileNameIn, FileNameOut ) )
    {
        fprintf( pErr, "Reading parallel automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _read_para <channels> <file_in> <file_out>\n");
	fprintf( pErr, "       encodes the FSM with MV inputs/outputs\n" );
	fprintf( pErr, " <channels> : the description of I/O channels (comma-separated, no spaces,\n" );
	fprintf( pErr, "              vertical bars separate channels, input channels follow first)\n" );
	fprintf( pErr, " <file_in>  : the input  BLIF-MV file with the input FSM\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting FSM\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoWriteFsm( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
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

    if ( nInputs == -1 )
    {
        fprintf( pErr, "The number of FSM inputs should be given on the command line.\n" );
        goto usage;
    }

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );


    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    if ( nInputs == -1 )
    {
//        nInputs = pAut->nInputs - pAut->nOutputs;
//        fprintf( pErr, "Warning: Assuming the number of FSM inputs to be %d.\n", nInputs );
    }

    Au_AutoWriteKissFsm( pAut, FileNameOut, nInputs );
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _write_fsm [-i num] <file_in> <file_out>\n");
	fprintf( pErr, "       converts the automaton into an FSM\n" );
	fprintf( pErr, "    -i num  : the number of inputs in the resulting FSM [default = guess]\n" );
	fprintf( pErr, "              the first \"num\" inputs of the automaton become FSM inputs\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting FSM\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoGenerate( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutR;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, nOutputs, c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    nInputs = -1;
    nOutputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "iohv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
                break;
            case 'o':
                nOutputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nOutputs < 0 ) 
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

    if ( nInputs == -1 )
    {
        fprintf( pErr, "The number of inputs to remove should be given on the command line.\n" );
        goto usage;
    }

    if ( nOutputs == -1 )
    {
        fprintf( pErr, "The number of outputs to remove should be given on the command line.\n" );
        goto usage;
    }

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );


    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    pAutR = Auto_AutoBenchmarkGenerate( pAut, nInputs, nOutputs );
    if ( pAutR == NULL )
    {
        fprintf( pErr, "Generating benchmark for the automaton \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    Au_AutoWriteKiss( pAutR, FileNameOut );
    Au_AutoFree( pAut );
    Au_AutoFree( pAutR );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _generate [-i num] [-o num] <file_in> <file_out>\n");
	fprintf( pErr, "       generates benchmark by randomly removing inputs/outputs\n" );
	fprintf( pErr, "       and transitions to achieve non-containment\n" );
	fprintf( pErr, "    -i num  : the number of inputs to remove [default = %d]\n", nInputs );
	fprintf( pErr, "    -o num  : the number of inputs to remove [default = %d]\n", nOutputs );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoLanguage( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
//    char * FileNameOut;
    int Length, c;
    int fVerbose;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    Length = 3;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "lhv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'l':
                Length = atoi(argv[util_optind]);
                util_optind++;
                if ( Length < 0 ) 
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

    // get the file names
    if ( util_optind + 1 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
//    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );


    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    Au_AutoLanguage( pOut, pAut, Length );
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _language [-l num] <file_in>\n");
	fprintf( pErr, "       prints out the language of the automaton\n" );
	fprintf( pErr, "    -l num  : the length of words to be printed [default = %d]\n", Length );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoSupport( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    char * pInputString;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 3 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    pInputString = argv[util_optind];
    FileNameIn   = argv[util_optind + 1];
    FileNameOut  = argv[util_optind + 2];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    Au_AutoSupport( pAut, pInputString );
    Au_AutoWriteKiss( pAut, FileNameOut );
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _support <input_order> <file_in> <file_out>\n");
	fprintf( pErr, "       changes the input variables of the automaton\n" );
	fprintf( pErr, " <input_order> : the resulting list of inputs (comma-separated, no spaces)\n" );
	fprintf( pErr, " <file_in>     : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out>    : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoComplete( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int nInputs, c;
    int fVerbose, fAccepting;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    nInputs = -1;
    fAccepting = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "iahv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
                break;
			case 'a':
				fAccepting ^= 1;
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
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    // if the number of inputs is not given, use all inputs of the automaton
    if ( nInputs == -1 )
        nInputs = pAut->nInputs;

    if ( Au_AutoComplete( pAut, nInputs, fAccepting, 0 ) == 0 )
        fprintf( pErr, "The automaton is already complete and is left unchanged.\n", FileNameIn );

    Au_AutoWriteKiss( pAut, FileNameOut );
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _complete [-i num] [-a] <file_in> <file_out>\n");
	fprintf( pErr, "       completes the automaton by adding the don't-care state\n" );
	fprintf( pErr, "    -i num  : the number of inputs to consider for completion [default = all].\n" );
	fprintf( pErr, "              (if the number of inputs is less than all inputs,\n" );
	fprintf( pErr, "              the completion is performed in the FSM sense)\n" );
    fprintf( pErr, "    -a      : toggles the don't-care state type [default = %s]\n", (fAccepting? "accepting": "non-accepting") );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoComplement( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    Au_AutoComplement( pAut );
    Au_AutoWriteKiss( pAut, FileNameOut );
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _complement <file_in> <file_out>\n");
	fprintf( pErr, "       complements the automaton\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoDeterminize( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutD;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fLongNames, fAllAccepting;
    int fVerbose;
    int fComplete;
    int fComplement;
    int fTransFull;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    fComplete = 0;
    fComplement = 0;
    fLongNames = 0;
    fAllAccepting = 0;
    fTransFull = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "flciahv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'f':
				fTransFull ^= 1;
				break;
			case 'l':
				fLongNames ^= 1;
				break;
			case 'c':
				fComplete ^= 1;
				break;
			case 'i':
				fComplement ^= 1;
				break;
			case 'a':
				fAllAccepting ^= 1;
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
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );
 
    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    if ( fTransFull )
        pAutD = Au_AutoDeterminizeFull( pAut, fAllAccepting, fLongNames );
    else
        pAutD = Au_AutoDeterminizePart( pAut, fAllAccepting, fLongNames );
//        pAutD = Au_AutoDeterminizeExp( pAut, fAllAccepting, fLongNames );
    if ( pAutD != pAut )
        Au_AutoFree( pAut );

    if ( pAutD == NULL )
    {
        fprintf( pErr, "Determinization has failed.\n" );
        return 1;
    }

    // complete if asked
    if ( fComplete )
        Au_AutoComplete( pAutD, pAutD->nInputs, 0, 0 );
    // complement if asked
    if ( fComplement )
        Au_AutoComplement( pAutD );

    Au_AutoWriteKiss( pAutD, FileNameOut );
    Au_AutoFree( pAutD );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _determinize [-f] [-c] [-i] [-a] [-l] <file_in> <file_out>\n");
	fprintf( pErr, "       determinizes (and complements) the automaton\n" );
    fprintf( pErr, "        -f  : toggle using full/partitioned TR [default = %s]\n", (fTransFull? "full": "partitioned") );
    fprintf( pErr, "        -c  : complete the automaton after determinization [default = %s]\n", (fComplete? "yes": "no") );
    fprintf( pErr, "        -i  : complement the automaton after determinization [default = %s]\n", (fComplement? "yes": "no") );
    fprintf( pErr, "        -a  : require all states in a subset to be accepting [default = %s]\n", (fAllAccepting? "yes": "no") );
    fprintf( pErr, "        -l  : use long names for the subsets of states [default = %s]\n", (fLongNames? "yes": "no") );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoMinimize( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutM;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;
    int fUseTable;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fUseTable = 1;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ahv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'a':
				fUseTable ^= 1;
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
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    pAutM = Au_AutoStateMinimize( pAut, fUseTable );
    Au_AutoFree( pAut );
    if ( pAutM == NULL )
    {
        fprintf( pErr, "State minimization has failed.\n" );
        return 1;
    }
    Au_AutoWriteKiss( pAutM, FileNameOut );
    Au_AutoFree( pAutM );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _minimize [-a] <file_in> <file_out>\n");
	fprintf( pErr, "       minimizes the number of states using a simple algorithm\n" );
    fprintf( pErr, "        -a  : toggles minimization using table/other [default = %s]\n", (fUseTable? "table": "other") );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoDcMin( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutM;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    pAutM = Au_AutoStateDcMinimize( pAut, fVerbose );
    Au_AutoFree( pAut );
    if ( pAutM == NULL )
    {
        fprintf( pErr, "State minimization has failed.\n" );
        return 1;
    }
    Au_AutoWriteKiss( pAutM, FileNameOut );
    Au_AutoFree( pAutM );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _dcmin [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       minimizes the number of states by collapsing states\n" );
	fprintf( pErr, "       whose transitions into the non-DC states are compatible\n" );
	fprintf( pErr, "       (the input automaton should be complete with an accepting DC\n" );
	fprintf( pErr, "       state, or incomplete, then the accepting DC state is assumed)\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoProduct( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut1, * pAut2, * pAutP;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn1, * FileNameIn2;
    char * FileNameOut;
    int fLongNames;
    int fVerbose;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    fLongNames = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "lhv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'l':
				fLongNames ^= 1;
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
    if ( util_optind + 3 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn1 = argv[util_optind];
    FileNameIn2 = argv[util_optind + 1];
    FileNameOut = argv[util_optind + 2];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn1, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn1 );
        goto usage;
    }
    fclose( pFile );
    if ( (pFile = fopen( FileNameIn2, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn2 );
        goto usage;
    }
    fclose( pFile );

    pAut1 = Au_AutoReadKiss( pMvsis, FileNameIn1 );
    if ( pAut1 == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn1 );
        return 1;
    }
    pAut2 = Au_AutoReadKiss( pMvsis, FileNameIn2 );
    if ( pAut2 == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn2 );
        return 1;
    }
    pAutP = Au_AutoProduct( pAut1, pAut2, fLongNames );
    Au_AutoFree( pAut1 );
    Au_AutoFree( pAut2 );
    if ( pAutP == NULL )
    {
        fprintf( pErr, "Constructing the product has failed.\n" );
        return 1;
    }
    Au_AutoWriteKiss( pAutP, FileNameOut );
    Au_AutoFree( pAutP );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _product [-l] <file_in1> <file_in2> <file_out>\n");
	fprintf( pErr, "       builds the product of the two automata\n" );
    fprintf( pErr, "        -l  : use long names for the subsets of states [default = %s]\n", (fLongNames? "yes": "no") );
	fprintf( pErr, " <file_in1> : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_in2> : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoReverse( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutM;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    pAutM = Au_AutoReverse( pAut );
    Au_AutoFree( pAut );
    if ( pAutM == NULL )
    {
        fprintf( pErr, "Reversing the automaton has failed.\n" );
        return 1;
    }
    Au_AutoWriteKiss( pAutM, FileNameOut );
    Au_AutoFree( pAutM );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _reverse <file_in> <file_out>\n");
	fprintf( pErr, "       creates the automaton with the reversed transitions\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoPrefixClose( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutF;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    pAutF = Au_AutoPrefixClose( pAut );
    Au_AutoFree( pAut );
    if ( pAutF == NULL )
    {
        fprintf( pErr, "Computation of the prefix closed automaton has failed.\n" );
        return 1;
    }
    Au_AutoWriteKiss( pAutF, FileNameOut );
    Au_AutoFree( pAutF );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _prefix <file_in> <file_out>\n");
	fprintf( pErr, "       leaves only accepting states that are reachable\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoProgressive( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutF;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
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

    if ( nInputs == -1 )
    {
        fprintf( pErr, "The number of FSM inputs should be given on the command line.\n" );
        goto usage;
    }

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }


    if ( nInputs == -1 )
    {
//        nInputs = pAut->nInputs - pAut->nOutputs;
//        fprintf( pErr, "Warning: Assuming the number of FSM inputs to be %d.\n", nInputs );
    }


    pAutF = Au_AutoProgressive( pAut, nInputs, 0, fVerbose );
    Au_AutoFree( pAut );
    if ( pAutF == NULL )
    {
        fprintf( pErr, "Computation of progressive automaton has failed.\n" );
        return 1;
    }
    Au_AutoWriteKiss( pAutF, FileNameOut );
    Au_AutoFree( pAutF );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _progressive [-i num] [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       leaves only accepting and complete states that are reachable\n" );
	fprintf( pErr, "    -i num  : the number of inputs to consider for completion [default = guess].\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoMoore( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutF;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
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

    if ( nInputs == -1 )
    {
        fprintf( pErr, "The number of FSM inputs should be given on the command line.\n" );
        goto usage;
    }

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }


    if ( nInputs == -1 )
    {
//        nInputs = pAut->nInputs - pAut->nOutputs;
//        fprintf( pErr, "Warning: Assuming the number of FSM inputs to be %d.\n", nInputs );
    }

    
    pAutF = Au_AutoProgressive( pAut, nInputs, 1, fVerbose );
    Au_AutoFree( pAut );
    if ( pAutF == NULL )
    {
        fprintf( pErr, "Computation of the Moore automaton has failed.\n" );
        return 1;
    }
    Au_AutoWriteKiss( pAutF, FileNameOut );
    Au_AutoFree( pAutF );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _moore [-i num] [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       trims the automaton to contain Moore states only\n" );
	fprintf( pErr, "    -i num  : the number of inputs to consider [default = guess].\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoReduce( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutF;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
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

    if ( nInputs == -1 )
    {
        fprintf( pErr, "The number of FSM inputs should be given on the command line.\n" );
        goto usage;
    }

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }


    if ( nInputs == -1 )
    {
//        nInputs = pAut->nInputs - pAut->nOutputs;
//        fprintf( pErr, "Warning: Assuming the number of FSM inputs to be %d.\n", nInputs );
    }


    pAutF = Au_AutoReductionSat( pAut, nInputs, fVerbose );
    Au_AutoFree( pAut );
    if ( pAutF == NULL )
    {
        fprintf( pErr, "Computation of the reduced automaton has failed.\n" );
        return 1;
    }
    Au_AutoWriteKiss( pAutF, FileNameOut );
    Au_AutoFree( pAutF );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _reduce [-i num] [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       derives a complete sub-automaton\n" );
	fprintf( pErr, "    -i num  : the number of FSM inputs [default = guess].\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoComb( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut, * pAutF;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
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

    if ( nInputs == -1 )
    {
        fprintf( pErr, "The number of FSM inputs should be given on the command line.\n" );
        goto usage;
    }

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( "zchaff.exe", "r" )) == NULL && 
         (pFile = fopen( "..\\zchaff.exe", "r" )) == NULL )
    {
        fprintf( pErr, "Cannot find the binary of zChaff (\"%s\") in the current directory or in the parent directory.\n", "zchaff.exe" );
        goto usage;
    }
    fclose( pFile );

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }


    if ( nInputs == -1 )
    {
//        nInputs = pAut->nInputs - pAut->nOutputs;
//        fprintf( pErr, "Warning: Assuming the number of FSM inputs to be %d.\n", nInputs );
    }


    pAutF = Au_AutoCombinationalSat( pAut, nInputs, fVerbose );
    Au_AutoFree( pAut );
    if ( pAutF == NULL )
        return 0;
//    {
//        fprintf( pErr, "Computation of the reduced automaton has failed.\n" );
//        return 1;
//    }
    Au_AutoWriteKiss( pAutF, FileNameOut );
    Au_AutoFree( pAutF );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _comb [-i num] [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       derives a combinatinal solution if present\n" );
	fprintf( pErr, "       (the binary \"%s\" should be in the current directory)\n", "zchaff.exe" );
	fprintf( pErr, "    -i num  : the number of FSM inputs [default = guess].\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoCheck( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut1, * pAut2;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn1, * FileNameIn2;
    int fVerbose;
    int fError;
    int fCheckFsm;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fCheckFsm = 0;
    fVerbose = 0;
    fError = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "efhv" ) ) != EOF )
    {
        switch ( c )
        {
			case 'e':
				fError ^= 1;
				break;
			case 'f':
				fCheckFsm ^= 1;
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
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn1 = argv[util_optind];
    FileNameIn2 = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn1, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn1 );
        goto usage;
    }
    fclose( pFile );
    if ( (pFile = fopen( FileNameIn2, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn2 );
        goto usage;
    }
    fclose( pFile );

    // read the first automaton/FSM
    if ( !fCheckFsm )
        pAut1 = Au_AutoReadKiss( pMvsis, FileNameIn1 );
    else
    {
        if ( !Au_FsmReadKiss( pMvsis, FileNameIn1, "_temp_.aut" ) )
        {
            fprintf( pErr, "Converting FSM KISS file 1 into an automaton KISS file has failed.\n" );
            return 1;
        }
        pAut1 = Au_AutoReadKiss( pMvsis, "_temp_.aut" );
        _unlink( "_temp_.aut" );
        if ( pAut1 )
        {
            FREE( pAut1->pName );
            pAut1->pName = util_strsav( FileNameIn1 );
            Au_AutoComplete( pAut1, pAut1->nInputs, 0, 0 );
        }
    }
    if ( pAut1 == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn1 );
        return 1;
    }

    // read the second automaton/FSM
    if ( !fCheckFsm )
        pAut2 = Au_AutoReadKiss( pMvsis, FileNameIn2 );
    else
    {
        if ( !Au_FsmReadKiss( pMvsis, FileNameIn2, "_temp_.aut" ) )
        {
            fprintf( pErr, "Converting FSM KISS file 2 into an automaton KISS file has failed.\n" );
            return 1;
        }
        pAut2 = Au_AutoReadKiss( pMvsis, "_temp_.aut" );
        _unlink( "_temp_.aut" );
        if ( pAut2 )
        {
            FREE( pAut2->pName );
            pAut2->pName = util_strsav( FileNameIn2 );
            Au_AutoComplete( pAut2, pAut2->nInputs, 0, 0 );
        }
    }
    if ( pAut2 == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn2 );
        return 1;
    }

    Au_AutoCheck( pOut, pAut1, pAut2, fError );
    Au_AutoFree( pAut1 );
    Au_AutoFree( pAut2 );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _check [-e] [-f] <file_in1> <file_in2>\n");
	fprintf( pErr, "       checks equivalence or containment of the automata behaviors\n" );
    fprintf( pErr, "        -e  : toggle printing error trace [default = %s]\n", (fError? "yes": "no") );
    fprintf( pErr, "        -f  : toggle checking FSMs/automata [default = %s]\n", (fCheckFsm? "FSMs": "automata") );
	fprintf( pErr, " <file_in1> : the input KISS file with the input automaton\n" );
	fprintf( pErr, " <file_in2> : the input KISS file with the input automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoCheckNd( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    int fVerbose;
    int fError;
    int nInputs, c;
    int nStatesNd;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    nInputs = -1;
    fVerbose = 0;
    fError = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
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

    // get the file names
    if ( util_optind + 1 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn = argv[util_optind];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    
    // if the number of inputs is not given, assume all
    if ( nInputs == -1 )
        nInputs = pAut->nInputs;

    nStatesNd = Au_AutoCheckNd( pOut, pAut, nInputs, fVerbose );
    fprintf( pErr, "The automaton has %d non-deterministic states.\n", nStatesNd );
    
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _check_nd [-i num] [-v] <file>\n");
	fprintf( pErr, "       checks whether the given automaton is non-deterministic\n" );
	fprintf( pErr, "   -i num   : the number of inputs to consider for the check [default = all].\n" );
    fprintf( pErr, "   -v       : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, "   <file>   : the input KISS file with the input automaton\n" );
	return 1;					// error exit 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoDot( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;
    int nStatesMax;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    nStatesMax = 100;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "shv" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                nStatesMax = atoi(argv[util_optind]);
                util_optind++;
                if ( nStatesMax < 0 ) 
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

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    if ( pAut->nStates == 0 )
    {
        fprintf( pErr, "Attempting to create DOT file for an automaton with no states!\n" );
        Au_AutoFree( pAut );
        return 0;
    }

    Au_AutoWriteDot( pAut, FileNameOut, nStatesMax, 1 );
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _dot [-s num] <file_in> <file_out>\n");
	fprintf( pErr, "       generates DOT file to visualize the STG of the automaton\n" );
	fprintf( pErr, "    -s num  : the limit on the number of states [default = %d].\n", nStatesMax );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output DOT file with the drawing\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoRemove( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 1 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = NULL;

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    _unlink( FileNameIn );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _remove <file>\n");
	fprintf( pErr, "       removes the given file\n" );
	fprintf( pErr, " <file>  : the file to be removed from the hard drive\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoPsa( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    char Buffer1[20], Buffer2[20];
    int nStatesIncomp, nStatesNonDet;
    int fVerbose, nInputs, c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    nInputs = -1;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
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

    // get the file names
    if ( util_optind + 1 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = NULL;

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    if ( pAut->nStates == 0 )
    {
        fprintf( pErr, "%s: The automaton has no states.\n", FileNameIn );
        Au_AutoFree( pAut );
        return 0;
    }

    if ( nInputs == -1 )
        nInputs = pAut->nInputs;

    // the first line
    nStatesIncomp = Au_AutoComplete( pAut, nInputs, 0, 1 );
    nStatesNonDet = Au_AutoCheckNd( stdout, pAut, nInputs, 0 );
    sprintf( Buffer1, " (%d states)", nStatesIncomp );
    sprintf( Buffer2, " (%d states)", nStatesNonDet );

	fprintf( pOut, "%s: The automaton is %s%s and %s%s.\n", FileNameIn,  
        (nStatesIncomp? "incomplete" : "complete"),
        (nStatesIncomp? Buffer1 : ""),
        (nStatesNonDet? "non-deterministic": "deterministic"),
        (nStatesNonDet? Buffer2 : "")     );

    // the third line
    fprintf( pOut, "%d inputs  ",   pAut->nInputs );
	fprintf( pOut, "%d states  ",    pAut->nStates );
	fprintf( pOut, "%d transitions  ",     Au_AutoCountTransitions(pAut) );
	fprintf( pOut, "%d products\n",  Au_AutoCountProducts(pAut) );

    // the second line
    fprintf( pOut, "Inputs = { ",   pAut->nInputs );
    for ( c = 0; c < pAut->nInputs; c++ )
        fprintf( pOut, "%s%s", ((c==0)? "": ","),  pAut->ppNamesInput[c] );
    fprintf( pOut, " }\n" );

    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _psa [-i num] <file>\n");
	fprintf( pErr, "       prints statistics about the automaton\n" );
	fprintf( pErr, "   -i num  : the number of FSM inputs [default = all].\n" );
	fprintf( pErr, "   <file>  : the KISS file with the automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoTest( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;

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

    // get the file names
    if ( util_optind + 2 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }


//    Au_AutoWriteKiss( pAut, FileNameOut );
    {
//        Au_Rel_t * pTR;
//        pTR = Au_AutoRelCreate( pAut );
//        PRB( pTR->dd, pTR->bRel );
    }

    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _test <file_in> <file_out>\n");
	fprintf( pErr, "       testing some procedures\n" );
	fprintf( pErr, " <file_in>  : the input  KISS file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output KISS file with the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


#ifdef WIN32
#include "process.h" 
#endif

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_CommandAutoShow( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    char * FileGeneric;
    char FileNameDot[200];
    char FileNamePs[200];
    char CommandDot[1000];
#ifndef WIN32
    char CommandPs[1000];
#endif
    int fShowCond;
    char * pProgDotName;
    char * pProgGsViewName;
    int fVerbose;
    int c;
    int nStatesMax;

#ifdef WIN32
    pProgDotName = "dot.exe";
    pProgGsViewName = NULL;
#else
    pProgDotName = "dot";
    pProgGsViewName = "gv";
#endif

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
    nStatesMax = 100;
    fVerbose = 0;
    fShowCond = 1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "schv" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                nStatesMax = atoi(argv[util_optind]);
                util_optind++;
                if ( nStatesMax < 0 ) 
                    goto usage;
                break;
			case 'c':
				fShowCond ^= 1;
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
    if ( util_optind + 1 > argc )
    {
        fprintf( pErr, "Not enough command line arguments.\n" );
        goto usage;
    }

    FileNameIn  = argv[util_optind];
    FileNameOut = NULL;

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );
    
    pAut = Au_AutoReadKiss( pMvsis, FileNameIn );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    if ( pAut->nStates == 0 )
    {
        fprintf( pErr, "Attempting to create DOT file for an automaton with no states!\n" );
        Au_AutoFree( pAut );
        return 0;
    }

    // get the generic file name
    FileGeneric = Extra_FileNameGeneric( FileNameIn );
    sprintf( FileNameDot, "%s.dot", FileGeneric ); 
    sprintf( FileNamePs,  "%s.ps",  FileGeneric ); 
    FREE( FileGeneric );

    Au_AutoWriteDot( pAut, FileNameDot, 1000, fShowCond );
    Au_AutoFree( pAut );

    // check that the input file is okay
    if ( (pFile = fopen( FileNameDot, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        goto usage;
    }
    fclose( pFile );

    // generate the DOT file
    sprintf( CommandDot,  "%s -Tps -o %s %s", pProgDotName, FileNamePs, FileNameDot ); 
    if ( system( CommandDot ) == -1 )
    {
        _unlink( FileNameDot );
        fprintf( pErr, "Cannot find \"%s\".\n", pProgDotName );
        goto usage;
    }
    _unlink( FileNameDot );

    // check that the input file is okay
    if ( (pFile = fopen( FileNamePs, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open intermediate file \"%s\".\n", FileNamePs );
        goto usage;
    }
    fclose( pFile ); 

#ifdef WIN32
    if ( _spawnl( _P_NOWAIT, "gsview32.exe", "gsview32.exe", FileNamePs, NULL ) == -1 )
        if ( _spawnl( _P_NOWAIT, "C:\\Program Files\\Ghostgum\\gsview\\gsview32.exe", 
            "C:\\Program Files\\Ghostgum\\gsview\\gsview32.exe", FileNamePs, NULL ) == -1 )
        {
            fprintf( pErr, "Cannot find \"%s\".\n", "gsview32.exe" );
            goto usage;
        }
#else
    sprintf( CommandPs,  "%s %s &", pProgGsViewName, FileNamePs ); 
    if ( system( CommandPs ) == -1 )
    {
        fprintf( pErr, "Cannot execute \"%s\".\n", FileNamePs );
        goto usage;
    }
#endif

    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: _show [-c] <file>\n");
	fprintf( pErr, "       visualizes the automaton using DOT and GSVIEW\n" );
	fprintf( pErr, "       \"dot.exe\" and \"gsview32.exe\" should be set in the paths\n" );
	fprintf( pErr, "       (\"gsview32.exe\" may be in \"C:\\Program Files\\Ghostgum\\gsview\\\")\n" );
	fprintf( pErr, " -c      : toggles showing the conditions on edges [default = %s]\n", (fShowCond? "yes": "no") );
	fprintf( pErr, " <file>  : the input  KISS file with the input automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


