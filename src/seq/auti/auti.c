/**CFile****************************************************************

  FileNameIn  [auti.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auti.c,v 1.6 2005/06/02 03:34:20 alanmi Exp $]

***********************************************************************/

#include "autiInt.h" 

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

//static int Auti_CommandAutoReadFsm    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
//static int Auti_CommandAutoReadFsmMv  ( Mv_Frame_t * pMvsis, int argc, char ** argv );
//static int Auti_CommandAutoReadPara   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
//static int Auti_CommandAutoWriteFsm   ( Mv_Frame_t * pMvsis, int argc, char ** argv );

//static int Auti_CommandAutoGenerate   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
//static int Auti_CommandAutoLanguage   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoSupport    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoComplete   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoComplement ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoDeterminize( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoMinimize   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoDcMin      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoTrim       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoProduct    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
//static int Auti_CommandAutoReverse    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoPrefixClose( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoProgressive( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoMoore      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
//static int Auti_CommandAutoReduce     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
//static int Auti_CommandAutoComb       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoCheck      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoCheckNd    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoDot        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoBinary     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoTest       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
//static int Auti_CommandAutoRemove     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoSupp       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoPsa        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoShow       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoVolume     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Auti_CommandAutoRemoveDc   ( Mv_Frame_t * pMvsis, int argc, char ** argv );

//Aut_Auto_t * Auti_AutoStateMinimize( Aut_Auto_t * pAut, int fUseTable ) { return NULL; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_Init( Mv_Frame_t * pMvsis )
{
#ifdef BALM
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "contain",     Auti_CommandAutoCheck,       0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "complete",    Auti_CommandAutoComplete,    0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "complement",  Auti_CommandAutoComplement,  0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "dcmin",       Auti_CommandAutoDcMin,       0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "determinize", Auti_CommandAutoDeterminize, 0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "minimize",    Auti_CommandAutoMinimize,    0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "moore",       Auti_CommandAutoMoore,       0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "product",     Auti_CommandAutoProduct,     0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "prefix",      Auti_CommandAutoPrefixClose, 0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "progressive", Auti_CommandAutoProgressive, 0 ); 
    Cmd_CommandAdd( pMvsis, "Automata manipulation", "support",     Auti_CommandAutoSupport,     0 ); 

    Cmd_CommandAdd( pMvsis, "Automata viewing",      "print_support",   Auti_CommandAutoSupp,        0 ); 
    Cmd_CommandAdd( pMvsis, "Automata viewing",      "print_stats_aut", Auti_CommandAutoPsa,         0 ); 
    Cmd_CommandAdd( pMvsis, "Automata viewing",      "plot_aut",        Auti_CommandAutoShow,        0 ); 
    Cmd_CommandAdd( pMvsis, "Automata viewing",      "print_nd_states", Auti_CommandAutoCheckNd,     0 ); 
    Cmd_CommandAdd( pMvsis, "Automata viewing",      "print_lang_size", Auti_CommandAutoVolume,      0 ); 
#else
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "read_fsm",    Auti_CommandAutoReadFsm,     0 ); 
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "read_fsm_mv", Auti_CommandAutoReadFsmMv,   0 ); 
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "read_para",   Auti_CommandAutoReadPara,    0 ); 
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "write_fsm",   Auti_CommandAutoWriteFsm,    0 ); 
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "generate",    Auti_CommandAutoGenerate,    0 ); 
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "language",    Auti_CommandAutoLanguage,    0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "support",     Auti_CommandAutoSupport,     0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "complete",    Auti_CommandAutoComplete,    0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "complement",  Auti_CommandAutoComplement,  0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "determinize", Auti_CommandAutoDeterminize, 0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "minimize",    Auti_CommandAutoMinimize,    0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "dcmin",       Auti_CommandAutoDcMin,       0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "trim",        Auti_CommandAutoTrim,        0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "product",     Auti_CommandAutoProduct,     0 ); 
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "reverse",     Auti_CommandAutoReverse,     0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "prefix",      Auti_CommandAutoPrefixClose, 0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "progressive", Auti_CommandAutoProgressive, 0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "moore",       Auti_CommandAutoMoore,       0 ); 
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "reduce",      Auti_CommandAutoReduce,      0 ); 
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "comb",        Auti_CommandAutoComb,        0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "check",       Auti_CommandAutoCheck,       0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "check_nd",    Auti_CommandAutoCheckNd,     0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "dot",         Auti_CommandAutoDot,         0 ); 
//    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "remove",      Auti_CommandAutoRemove,      0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "print_support", Auti_CommandAutoSupp,         0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "psa",         Auti_CommandAutoPsa,         0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "binary",      Auti_CommandAutoBinary,      0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "test",        Auti_CommandAutoTest,        0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "show",        Auti_CommandAutoShow,        0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "volume",      Auti_CommandAutoVolume,      0 ); 
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "remove_dc",   Auti_CommandAutoRemoveDc,    0 ); 
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Auti_End()
{
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoLanguage( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
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


    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

//    Auti_AutoLanguage( pOut, pAut, Length );

    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: language [-l num] <file_in>\n");
	fprintf( pErr, "       prints out the language of the automaton\n" );
	fprintf( pErr, "    -l num  : the length of words to be printed [default = %d]\n", Length );
	fprintf( pErr, " <file_in>  : the input automaton file\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoSupport( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    char * pInputString;
    int fVerbose;
    int fWriteAut;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    fWriteAut = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hvz" ) ) != EOF )
    {
        switch ( c )
        {
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    if ( !Auti_AutoSupport( pAut, pInputString ) )
        fprintf( pErr, "Support computation has failed.\n" );
    else
        Aio_AutoWrite( pAut, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: support <supp_var> <file_in> <file_out>\n");
	fprintf( pErr, "       changes the support and order of variables of the automaton\n" );
	//fprintf( pErr, "    -z         : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " <input_order> : an ordered list of variables to be in the resulting automaton (comma-separated, no spaces)\n" );
	fprintf( pErr, " <file_in>     : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out>    : the output file for the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoComplete( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int nInputs, c;
    int fVerbose, fAccepting;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    fWriteAut = 0;
    nInputs = -1;
    fAccepting = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "iahvz" ) ) != EOF )
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    // if the number of inputs is not given, use all inputs of the automaton
    if ( nInputs == -1 )
        nInputs = Aut_AutoReadVarNum(pAut);

    if ( Auti_AutoComplete( pAut, nInputs, fAccepting ) == 0 )
        fprintf( pErr, "The automaton is already complete and is left unchanged.\n", FileNameIn );

    Aio_AutoWrite( pAut, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: complete [-i num] [-a] <file_in> <file_out>\n");
	fprintf( pErr, "       completes the automaton by adding the don't-care state\n" );
	fprintf( pErr, "    -i num  : the number of inputs to consider for completion [default = all].\n" );
	fprintf( pErr, "              (if the number of inputs is less than all inputs,\n" );
	fprintf( pErr, "              the completion is performed in the FSM sense)\n" );
    fprintf( pErr, "    -a      : toggles the don't-care state type [default = %s]\n", (fAccepting? "accepting": "non-accepting") );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoComplement( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut, * pAutD;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    fWriteAut = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hvz" ) ) != EOF )
    {
        switch ( c )
        {
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    if ( Auti_AutoCheckIsNd( stdout, pAut, Aut_AutoReadVarNum(pAut), 0 ) )
    {
        fprintf( pOut, "The automaton in \"%s\" is non-deterministic. Determinization will be performed.\n", FileNameIn );
        pAutD = Auti_AutoDeterminizePart( pAut, 0, 0 );
        if ( pAutD != pAut )
            Aut_AutoFree( pAut );
        pAut = pAutD;
    }
    if ( Auti_AutoComplete( pAut, Aut_AutoReadVarNum(pAut), 0 ) )
    {
        fprintf( pOut, "The automaton in \"%s\" is completed before complementing.\n", FileNameIn );
    }

    Auti_AutoComplement( pAut );
    Aio_AutoWrite( pAut, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: complement <file_in> <file_out>\n");
	fprintf( pErr, "       complements the automaton (a non-deterministic automaton is determinized)\n" );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoDeterminize( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut, * pAutD;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fLongNames, fAllAccepting;
    int fVerbose;
    int fComplete;
    int fComplement;
    int fTransFull;
    int c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    fComplete = 0;
    fComplement = 0;
    fLongNames = 0;
    fAllAccepting = 0;
    fTransFull = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "flciahvz" ) ) != EOF )
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
 
    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

//    if ( fTransFull )
//        pAutD = Auti_AutoDeterminizeFull( pAut, fAllAccepting, fLongNames );
//    else
//        pAutD = Auti_AutoDeterminizePart( pAut, fAllAccepting, fLongNames );
    pAutD = Auti_AutoDeterminizePart( pAut, fAllAccepting, fLongNames );

    if ( pAutD != pAut )
        Aut_AutoFree( pAut );

    if ( pAutD == NULL )
    {
        fprintf( pErr, "Determinization has failed.\n" );
        return 1;
    }

    // complete if asked
    if ( fComplete )
        Auti_AutoComplete( pAutD, Aut_AutoReadVarNum(pAutD), 0 );
    // complement if asked
    if ( fComplement )
        Auti_AutoComplement( pAutD );

    Aio_AutoWrite( pAutD, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAutD );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: determinize [-c] [-i] [-a] [-l] <file_in> <file_out>\n");
	fprintf( pErr, "       determinizes (and complements) the automaton\n" );
//    fprintf( pErr, "        -f  : toggle using full/partitioned TR [default = %s]\n", (fTransFull? "full": "partitioned") );
    fprintf( pErr, "        -c  : complete the automaton after determinization [default = %s]\n", (fComplete? "yes": "no") );
    fprintf( pErr, "        -i  : complement the automaton after determinization [default = %s]\n", (fComplement? "yes": "no") );
    fprintf( pErr, "        -a  : require all states in a subset to be accepting [default = %s]\n", (fAllAccepting? "yes": "no") );
    fprintf( pErr, "        -l  : use long names for the subsets of states [default = %s]\n", (fLongNames? "yes": "no") );
	//fprintf( pErr, "        -z  : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoMinimize( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut, * pAutM;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hvz" ) ) != EOF )
    {
        switch ( c )
        {
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    pAutM = Auti_AutoStateMinimize( pAut );
    if ( pAutM != pAut )
        Aut_AutoFree( pAut );
    if ( pAutM == NULL )
    {
        fprintf( pErr, "State minimization has failed.\n" );
        return 1;
    }
    Aio_AutoWrite( pAutM, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAutM );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: minimize <file_in> <file_out>\n");
	fprintf( pErr, "       minimizes the number of states using a simple algorithm\n" );
//	fprintf( pErr, "        -z  : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoDcMin( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut, * pAutM;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hvz" ) ) != EOF )
    {
        switch ( c )
        {
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    pAutM = Auti_AutoStateDcMin( pAut, fVerbose );
    Aut_AutoFree( pAut );
    if ( pAutM == NULL )
    {
        fprintf( pErr, "DC minimization has failed.\n" );
        return 1;
    }
    Aio_AutoWrite( pAutM, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAutM );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: dcmin [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       minimizes the number of states by collapsing states\n" );
	fprintf( pErr, "       whose transitions into the non-DC states are compatible\n" );
	fprintf( pErr, "       (the input automaton should be complete with an accepting DC\n" );
	fprintf( pErr, "       state, or incomplete, then the accepting DC state is assumed)\n" );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
    fprintf( pErr, "    -v      : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoTrim( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAutX, * pAutA, * pAutT;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn1;
    char * FileNameIn2;
    char * FileNameOut;
    int fVerbose;
    int c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hvz" ) ) != EOF )
    {
        switch ( c )
        {
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

    pAutX = Aio_AutoRead( pMvsis, FileNameIn1, NULL );
    if ( pAutX == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn1 );
        return 1;
    }


    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn2, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn2 );
        goto usage;
    }
    fclose( pFile );

    pAutA = Aio_AutoRead( pMvsis, FileNameIn2, Aut_AutoReadMan(pAutX) );
    if ( pAutA == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn2 );
        return 1;
    }

    pAutT = Auti_AutoTrim( pAutX, pAutA );
    Aut_AutoFree( pAutA );
    if ( pAutT == NULL )
    {
        fprintf( pErr, "Trimming has failed.\n" );
        return 1;
    }
    Aio_AutoWrite( pAutT, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAutT );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: trim [-v] <file_inX> <file_inA> <file_out>\n");
	fprintf( pErr, "       trims the MGS (X) using the known solution (A)\n" );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
    fprintf( pErr, "    -v      : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " <file_inX> : the input file with the MGS\n" );
	fprintf( pErr, " <file_inA> : the input file with the known solution\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoProduct( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut1, * pAut2, * pAutP;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn1, * FileNameIn2;
    char * FileNameOut;
    int fLongNames;
    int fVerbose;
    int c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    fLongNames = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "lhvz" ) ) != EOF )
    {
        switch ( c )
        {
			case 'l':
				fLongNames ^= 1;
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

    pAut1 = Aio_AutoRead( pMvsis, FileNameIn1, NULL );
    if ( pAut1 == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn1 );
        return 1;
    }
    if ( pAut1->nStates >= (1<<16) )
    {
        printf( "The first automaton has more than %d states.\n", (1<<16) );
        return 0;
    }

    pAut2 = Aio_AutoRead( pMvsis, FileNameIn2, Aut_AutoReadMan(pAut1) );
    if ( pAut2 == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn2 );
        return 1;
    }
    if ( pAut2->nStates >= (1<<16) )
    {
        printf( "The second automaton has more than %d states.\n", (1<<16) );
        return 0;
    }
    if ( !Auti_AutoProductCommonSupp( pAut1, pAut2, FileNameIn1, FileNameIn2, &pAut1, &pAut2 ) )
    {
        printf( "Constructing the product has failed.\n" );
        return 0;
    }

    pAutP = Auti_AutoProduct( pAut1, pAut2, fLongNames );
    Aut_AutoFree( pAut1 );
    Aut_AutoFree( pAut2 );
    if ( pAutP == NULL )
    {
        fprintf( pErr, "Constructing the product has failed.\n" );
        return 1;
    }
    Aio_AutoWrite( pAutP, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAutP );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: product [-l] <file_in1> <file_in2> <file_out>\n");
	fprintf( pErr, "       builds the product of the two automata\n" );
    fprintf( pErr, "        -l  : use long names for the subsets of states [default = %s]\n", (fLongNames? "yes": "no") );
//	fprintf( pErr, "        -z  : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " <file_in1> : the input file with the given automaton\n" );
	fprintf( pErr, " <file_in2> : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoPrefixClose( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hvz" ) ) != EOF )
    {
        switch ( c )
        {
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    Auti_AutoPrefixClose( pAut );
    if ( pAut->nStates == 0 )
        printf( "The result of prefix close is empty.\n" );
    Aio_AutoWrite( pAut, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: prefix <file_in> <file_out>\n");
	fprintf( pErr, "       leaves only accepting states that are reachable\n" );
//	fprintf( pErr, "        -z  : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoProgressive( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;
    int fWriteAut;
    int fPrefix;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fPrefix = 1;
    fWriteAut = 0;
    fVerbose = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihvpz" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
                break;
			case 'z':
				fWriteAut ^= 1;
				break;
			case 'p':
				fPrefix ^= 1;
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }


    Auti_AutoProgressive( pAut, nInputs, fVerbose, fPrefix );
    if ( pAut->nStates == 0 )
        printf( "The result of progressive is empty.\n" );
    Aio_AutoWrite( pAut, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: progressive [-i num] [-p] [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       leaves only accepting and complete (for every input there is a next state) states that are reachable\n" );
	fprintf( pErr, "    -i num  : the number of inputs to consider for completion [default = guess]\n" );
    fprintf( pErr, "              (assumes that the inputs precede the outputs in the support variables)\n" );
	fprintf( pErr, "    -p      : use prefix close before progressive [default = %s]\n", (fPrefix? "yes": "no") );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
    fprintf( pErr, "    -v      : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoMoore( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihvz" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    
    Auti_AutoMoore( pAut, nInputs, fVerbose );
    if ( pAut->nStates == 0 )
        printf( "The result of Moore reduction is empty.\n" );
    Aio_AutoWrite( pAut, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: moore [-i num] [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       trims the automaton to contain Moore states only\n" );
	fprintf( pErr, "    -i num  : the number of inputs to consider [default = guess].\n" );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
    fprintf( pErr, "    -v      : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoReduce( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut, * pAutF;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihvz" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }


    if ( nInputs == -1 )
    {
//        nInputs = Aut_AutoReadVarNum(pAut) - pAut->nOutputs;
//        fprintf( pErr, "Warning: Assuming the number of FSM inputs to be %d.\n", nInputs );
    }


//    pAutF = Auti_AutoReductionSat( pAut, nInputs, fVerbose );
    pAutF = NULL;

    Aut_AutoFree( pAut );
    if ( pAutF == NULL )
    {
        fprintf( pErr, "Computation of the reduced automaton has failed.\n" );
        return 1;
    }
    Aio_AutoWrite( pAutF, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAutF );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: reduce [-i num] [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       derives a complete sub-automaton\n" );
	fprintf( pErr, "    -i num  : the number of FSM inputs [default = guess].\n" );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
    fprintf( pErr, "    -v      : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoComb( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut, * pAutF;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    nInputs = -1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ihvz" ) ) != EOF )
    {
        switch ( c )
        {
            case 'i':
                nInputs = atoi(argv[util_optind]);
                util_optind++;
                if ( nInputs < 0 ) 
                    goto usage;
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }


    if ( nInputs == -1 )
    {
//        nInputs = Aut_AutoReadVarNum(pAut) - pAut->nOutputs;
//        fprintf( pErr, "Warning: Assuming the number of FSM inputs to be %d.\n", nInputs );
    }


//    pAutF = Auti_AutoCombinationalSat( pAut, nInputs, fVerbose );
    pAutF = NULL;

    Aut_AutoFree( pAut );
    if ( pAutF == NULL )
        return 0;
//    {
//        fprintf( pErr, "Computation of the reduced automaton has failed.\n" );
//        return 1;
//    }
    Aio_AutoWrite( pAutF, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAutF );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: comb [-i num] [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       derives a combinatinal solution if present\n" );
	fprintf( pErr, "       (the binary \"%s\" should be in the current directory)\n", "zchaff.exe" );
	fprintf( pErr, "    -i num  : the number of FSM inputs [default = guess].\n" );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
    fprintf( pErr, "    -v      : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoCheck( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut1, * pAut2;
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
    pAut1 = Aio_AutoRead( pMvsis, FileNameIn1, NULL );
    if ( pAut1 == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn1 );
        return 1;
    }

    // read the second automaton/FSM
    pAut2 = Aio_AutoRead( pMvsis, FileNameIn2, Aut_AutoReadMan(pAut1) );
    if ( pAut2 == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn2 );
        return 1;
    }
    if ( !Auti_AutoProductCommonSupp( pAut1, pAut2, FileNameIn1, FileNameIn2, &pAut1, &pAut2 ) )
    {
        printf( "Constructing the product has failed.\n" );
        return 0;
    }

    Auti_AutoCheck( pOut, pAut1, pAut2, fError );
    Aut_AutoFree( pAut1 );
    Aut_AutoFree( pAut2 );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: contain [-e] <file_in1> <file_in2>\n");
	fprintf( pErr, "       checks language containment of the two automata\n" );
    fprintf( pErr, "        -e  : toggle printing error trace [default = %s]\n", (fError? "yes": "no") );
//    fprintf( pErr, "        -f  : toggle checking FSMs/automata [default = %s]\n", (fCheckFsm? "FSMs": "automata") );
	fprintf( pErr, " <file_in1> : the input file with automaton 1\n" );
	fprintf( pErr, " <file_in2> : the input file with automaton 2\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoCheckNd( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    
    // if the number of inputs is not given, assume all
    if ( nInputs == -1 )
        nInputs = Aut_AutoReadVarNum(pAut);

    nStatesNd = Auti_AutoCheckIsNd( pOut, pAut, nInputs, fVerbose );
    fprintf( pErr, "The automaton has %d non-deterministic states.\n", nStatesNd );
    
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: print_nd_states [-i num] [-v] <file>\n");
	fprintf( pErr, "       prints information about non-deteminimistic states of the automaton\n" );
	fprintf( pErr, "   -i num   : the number of FSM inputs [default = all].\n" );
    fprintf( pErr, "   -v       : toggles printing state names [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, "   <file>   : the input file with the automaton\n" );
	return 1;					// error exit 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoDot( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    if ( Aut_AutoReadStateNum(pAut) == 0 )
    {
        fprintf( pErr, "Attempting to create DOT file for an automaton with no states!\n" );
        Aut_AutoFree( pAut );
        return 0;
    }

    Auti_AutoWriteDot( pAut, FileNameOut, nStatesMax, 1 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: print_dot [-s num] <file_in> <file_out>\n");
	fprintf( pErr, "       generates DOT file to visualize the STG of the automaton\n" );
	fprintf( pErr, "    -s num  : the limit on the number of states [default = %d].\n", nStatesMax );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
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
int Auti_CommandAutoRemove( Mv_Frame_t * pMvsis, int argc, char **argv )
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
	fprintf( pErr, "Usage: remove <file>\n");
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
int Auti_CommandAutoPsa( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int nInputs, c;

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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    if ( Aut_AutoReadStateNum(pAut) == 0 )
    {
        fprintf( pErr, "%s: The automaton has no states.\n", FileNameIn );
        Aut_AutoFree( pAut );
        return 0;
    }

    if ( nInputs == -1 )
        nInputs = Aut_AutoReadVarNum(pAut);
    Auti_AutoPrintStats( pOut, pAut, nInputs, fVerbose );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: print_stats_aut [-i num] <file>\n");
	fprintf( pErr, "       prints statistics about the automaton\n" );
	fprintf( pErr, "   -i num  : the number of FSM inputs [default = all].\n" );
	fprintf( pErr, "   <file>  : the file with the automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoSupp( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    {
        Aut_Var_t ** pVars;
        int nVars, c;
        // the second line
        fprintf( stdout, "",   Aut_AutoReadVarNum(pAut) );
        nVars = Aut_AutoReadVarNum(pAut);
        pVars = Aut_AutoReadVars(pAut);
        for ( c = 0; c < nVars; c++ )
        {
            fprintf( stdout, "%s%s", ((c==0)? "": ","), Aut_VarReadName(pVars[c]) );
            if ( Aut_VarReadValueNum(pVars[c]) > 2 )
                fprintf( stdout, "(%d)", Aut_VarReadValueNum(pVars[c]) );
        }
        fprintf( stdout, "\n" );
    }
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: print_support <file>\n");
	fprintf( pErr, "       prints the list of support variables of the automaton\n" );
	fprintf( pErr, "   <file>  : the file with the automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoBinary( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 1;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hvz" ) ) != EOF )
    {
        switch ( c )
        {
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    Aio_AutoWrite( pAut, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: binary <file_in> <file_out>\n");
	fprintf( pErr, "       encodes the MV automaton and writes it into an AUT file\n" );
//	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoTest( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int c;
    int fWriteAut;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hvz" ) ) != EOF )
    {
        switch ( c )
        {
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    Aio_AutoWrite( pAut, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: test <file_in> <file_out>\n");
	fprintf( pErr, "       testing some procedures\n" );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file for the resulting automaton\n" );
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
int Auti_CommandAutoShow( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
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
    
    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }
    if ( Aut_AutoReadStateNum(pAut) == 0 )
    {
        fprintf( pErr, "Attempting to create DOT file for an automaton with no states!\n" );
        Aut_AutoFree( pAut );
        return 0;
    }
    if ( Aut_AutoReadStateNum(pAut) >= nStatesMax )
    {
        fprintf( pErr, "The automaton has more than %d states.\n", nStatesMax );
        Aut_AutoFree( pAut );
        return 0;
    }

    // get the generic file name
    FileGeneric = Extra_FileNameGeneric( FileNameIn );
    sprintf( FileNameDot, "%s.dot", FileGeneric ); 
    sprintf( FileNamePs,  "%s.ps",  FileGeneric ); 
    FREE( FileGeneric );

    Auti_AutoWriteDot( pAut, FileNameDot, nStatesMax, fShowCond );
    Aut_AutoFree( pAut );

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
	fprintf( pErr, "Usage: plot_aut [-c] <file>\n");
	fprintf( pErr, "       visualizes the automaton using DOT and GSVIEW\n" );
#ifdef WIN32
	fprintf( pErr, "       \"dot.exe\" and \"gsview32.exe\" should be set in the paths\n" );
	fprintf( pErr, "       (\"gsview32.exe\" may be in \"C:\\Program Files\\Ghostgum\\gsview\\\")\n" );
#endif
	fprintf( pErr, " -c      : toggles showing the conditions on edges [default = %s]\n", (fShowCond? "yes": "no") );
	fprintf( pErr, " <file>  : the input file with the given automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoVolume( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    int fVerbose, fWriteAut;
    int Length, c;
    double dRes;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    Length    = 5;
    fWriteAut = 0;
    fVerbose  = 0;
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

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    dRes = Auti_AutoComputeVolume( pAut, Length );
    printf( "%40.0f accepted strings up to length %d.\n", dRes, Length );

    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: print_lang_size [-l num] [-v] <file_in>\n");
	fprintf( pErr, "       computes the number of I/O strings accepted by the maximum\n" );
	fprintf( pErr, "       prefix-closed sub-automaton of the given automaton\n" );
	fprintf( pErr, "    -l num  : the max length of the strings counted [default = %d].\n", Length );
    fprintf( pErr, "    -v      : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Auti_CommandAutoRemoveDc( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Aut_Auto_t * pAut;
    Aut_State_t * pState;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose;
    int fAccepting;
    int fWriteAut;
    int c;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fWriteAut = 0;
    fAccepting = 0;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "zavh" ) ) != EOF )
    {
        switch ( c )
        {
			case 'z':
				fWriteAut ^= 1;
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

    pAut = Aio_AutoRead( pMvsis, FileNameIn, NULL );
    if ( pAut == NULL )
    {
        fprintf( pErr, "Reading the automaton from file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

    while ( pState = Auti_AutoFindDcStateAccepting( pAut ) )
    {
        Aut_AutoStateDelete( pState );
        Aut_StateFree( pState );
    }
    if ( !fAccepting )
    while ( pState = Auti_AutoFindDcStateNonAccepting( pAut ) )
    {
        Aut_AutoStateDelete( pState );
        Aut_StateFree( pState );
    }

    Aio_AutoWrite( pAut, FileNameOut, fWriteAut, 0 );
    Aut_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: remove_dc [-a] [-v] <file_in> <file_out>\n");
	fprintf( pErr, "       removes the don't-care states of the automaton\n" );
	fprintf( pErr, "       (a don't-care state is defined as a state with\n" );
	fprintf( pErr, "       the universal self-loop and no outgoing transitions)\n" );
	fprintf( pErr, "    -a      : only remove accepting don't-care state [default = %s]\n", (fAccepting? "yes": "no") );
	//fprintf( pErr, "    -z      : toggles the output in BLIF-MV or AUT [default = %s]\n", (fWriteAut? "AUT": "BLIF-MV") );
    fprintf( pErr, "    -v      : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	fprintf( pErr, " <file_in>  : the input file with the given automaton\n" );
	fprintf( pErr, " <file_out> : the output file with the given automaton\n" );
	return 1;					// error exit 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


