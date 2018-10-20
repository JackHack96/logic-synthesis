/**CFile****************************************************************

  FileName    [dual.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Automata synthesis from L language specifications. 
  The theory supporting this package is developed by Anatoly Chebotarev,
  Kiev, Ukraine, 1990 - 2003.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: dual.c,v 1.4 2005/06/02 03:34:21 alanmi Exp $]

***********************************************************************/

#include "dualInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Dual_CommandAutoFormula( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Dual_CommandAutoDual   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Dual_CommandAutoSynth  ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Dual_CommandAutoTest   ( Mv_Frame_t * pMvsis, int argc, char ** argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Synthesis from L language", "formula",  Dual_CommandAutoFormula,  0 ); 
    Cmd_CommandAdd( pMvsis, "Synthesis from L language", "dual",     Dual_CommandAutoDual,     0 ); 
    Cmd_CommandAdd( pMvsis, "Synthesis from L language", "synth",    Dual_CommandAutoSynth,    0 ); 
    Cmd_CommandAdd( pMvsis, "Synthesis from L language", "dtest",    Dual_CommandAutoTest,     0 ); 
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dual_End()
{
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dual_CommandAutoFormula( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Au_Auto_t * pAut;
    FILE * pOut, * pErr, * pFile;
    char * FileNameIn;
    char * FileNameOut;
    int fVerbose, c;
    int Rank;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    Rank = 5;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "rhv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'r':
                Rank = atoi(argv[util_optind]);
                util_optind++;
                if ( Rank < 0 ) 
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
    if ( argc < 3 )
        goto usage;

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

    Dual_ConvertAut2Formula( pAut, FileNameOut, Rank );
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: formula [-r num] <file_in> <file_out>\n");
	fprintf( pErr, "       converts the automaton into a Boolean formula\n" );
	fprintf( pErr, "    -r num  : the maximum rank of the automaton [default = %d]\n", Rank );
	fprintf( pErr, " <file_in>  : the input  file with the input automaton\n" );
	fprintf( pErr, " <file_out> : the output file with the formula in BLIF format\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dual_CommandAutoDual( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Dual_Spec_t * pSpec;
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
    if ( argc < 3 )
        goto usage;

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];

    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );

    pSpec = Dual_SpecRead( pMvsis, FileNameIn );
    if ( pSpec == NULL )
    {
        fprintf( pErr, "Reading L language file \"%s\" has failed.\n", FileNameIn );
        return 1;
    }

//PRB( pSpec->dd, pSpec->bSpec );

    Dual_SpecNormal( pSpec );
    pAut = Dual_AutoSynthesize( pSpec );
    Dual_SpecFree( pSpec );

    Au_AutoWriteKiss( pAut, FileNameOut );
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: dual <file_in> <file_out>\n");
	fprintf( pErr, "       synthesis of the automaton from the Boolean formula\n" );
	fprintf( pErr, " <file_in>  : the input  file with the input formula\n" );
	fprintf( pErr, " <file_out> : the output file with the output automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dual_CommandAutoSynth( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    Dual_Spec_t * pSpec;
    Au_Auto_t * pAut;
    FILE * pOut, * pErr;
    char * FileNameOut;
    int Rank;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    Rank = 1;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "rhv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'r':
                Rank = atoi(argv[util_optind]);
                util_optind++;
                if ( Rank < 0 ) 
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
    if ( argc < 2 )
        goto usage;

    FileNameOut = argv[util_optind];

    pSpec = Dual_DeriveSpecFromNetwork( pNet, Rank, 1 );
    if ( pSpec == NULL )
    {
        fprintf( pErr, "Computing has failed.\n" );
        return 1;
    }

//PRB( pSpec->dd, pSpec->bSpec );

    Dual_SpecNormal( pSpec );
    pAut = Dual_AutoSynthesize( pSpec );
    Dual_SpecFree( pSpec );

    Au_AutoWriteKiss( pAut, FileNameOut );
    Au_AutoFree( pAut );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: synth [-r num] <file_out>\n");
	fprintf( pErr, "       synthesizes the automaton starting from the current network\n" );
	fprintf( pErr, "    -r num  : the rank of the formula to be synthesize [default = %d]\n", Rank );
	fprintf( pErr, " <file_out> : the output file with the output automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dual_CommandAutoTest( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
//    Dual_Spec_t * pSpec;
    FILE * pOut, * pErr;
    char * FileNameIn;
    char * FileNameOut;
    int Rank;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    Rank = 1;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "rhv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'r':
                Rank = atoi(argv[util_optind]);
                util_optind++;
                if ( Rank < 0 ) 
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
    if ( argc < 3 )
        goto usage;

    FileNameIn  = argv[util_optind];
    FileNameOut = argv[util_optind + 1];
/*
    // check that the input file is okay
    if ( (pFile = fopen( FileNameIn, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\".\n", FileNameIn );
        goto usage;
    }
    fclose( pFile );
*/


    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: dtest <file_in> <file_out>\n");
	fprintf( pErr, "       testing some procedures\n" );
	fprintf( pErr, " <file_in>  : the input  file with the input  formula\n" );
	fprintf( pErr, " <file_out> : the output file with the output automaton\n" );
//    fprintf( pErr, "        -v  : toggles verbose [default = %s]\n", (fVerbose? "yes": "no") );
	return 1;					// error exit 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


