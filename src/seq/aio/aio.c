/**CFile****************************************************************

  FileName    [aio.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Commands defined in the aio package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aio.c,v 1.3 2005/06/02 03:34:18 alanmi Exp $]

***********************************************************************/

#include "aioInt.h"
#include "mvInt.h"

static Aut_Auto_t * s_pAut;

////////////////////////////////////////////////////////////////////////
///                        DECLARATionS                              ///
////////////////////////////////////////////////////////////////////////

static int AioCommandReadAut ( Mv_Frame_t * pMvsis, int argc, char **argv );
static int AioCommandWriteAut( Mv_Frame_t * pMvsis, int argc, char **argv );

extern char * Io_ReadFileGetSimilar( char * pFileNameWrong, char * pS1, char * pS2, char * pS3, char * pS4, char * pS5 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTion DEFITionS                           ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Initializes the I/O package.]

  SideEffects []

  SeeAlso     [Aio_End]

******************************************************************************/
void Aio_Init( Mv_Frame_t * pMvsis )
{
#ifdef BALM
//    Cmd_CommandAdd( pMvsis, "I/O", "read_aut",      AioCommandReadAut,     0 );
//    Cmd_CommandAdd( pMvsis, "I/O", "write_aut",     AioCommandWriteAut,    0 );
#else
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "read_automaton",      AioCommandReadAut,     0 );
    Cmd_CommandAdd( pMvsis, "STG-based sequential synthesis", "write_automaton",     AioCommandWriteAut,    0 );
#endif
}

/**Function********************************************************************

  Synopsis    [Ends the I/O package.]

  SideEffects []

  SeeAlso     [Aio_Init]

******************************************************************************/
void Aio_End( Mv_Frame_t * pMvsis )
{
    Aut_AutoFree( s_pAut );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int AioCommandReadAut( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
//    Aut_Auto_t * pAut;
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
        if ( FileName = Io_ReadFileGetSimilar( FileName, ".mva", ".mv", ".auy", ".kiss", ".kiss2" ) )
            fprintf( pMvsis->Err, "Did you mean \"%s\"?", FileName );
        fprintf( pMvsis->Err, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    s_pAut = Aio_AutoRead( pMvsis, FileName, NULL );
    // replace the current network
//    Mv_FrameReplaceCurrentNetwork( pMvsis, pAut );
    return 0;

usage:
    fprintf( pMvsis->Err, "Usage: read_aut [-h] <file>\n" );
    fprintf( pMvsis->Err, "   -h     reads the automaton from file\n" );
    fprintf( pMvsis->Err, "   file   the name of a file in BLIF-MV/AUT/KISS2 format\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int AioCommandWriteAut( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char * FileName;
    bool fWriteFsm;
    bool fWriteBlifMv;
    int c;

    fWriteFsm = 1;
    fWriteBlifMv = 1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "fzh" ) ) != EOF )
    {
        switch ( c )
        {
			case 'f':
				fWriteFsm ^= 1;
				break;
			case 'b':
				fWriteBlifMv ^= 1;
				break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

//    if ( pMvsis->pAutCur == NULL )
//    {
//        fprintf( pMvsis->Out, "Empty network.\n" );
//        return 0;
//    }

    if ( argc != util_optind + 1 )
    {
        goto usage;
    }


    // get the input file name
    FileName = argv[util_optind];
    // check if the network is binary
    if ( !Aut_AutoCheckIsBinary(s_pAut) )
    {
        fprintf( pMvsis->Out, "The current network is not binary. Use \"write_blif_mv\" or \"write_blif_mvs\".\n" );
        return 0;
    }
    // write the file
    if ( fWriteBlifMv )
        Aio_AutoWriteBlifMv( s_pAut, FileName, fWriteFsm );
    else
        Aio_AutoWriteAut( s_pAut, FileName, fWriteFsm );
    return 0;

usage:
    fprintf( pMvsis->Err, "Usage: write_aut [-f] [-z] [-h] <file>\n" );
    fprintf( pMvsis->Err, "          write the automaton into a BLIF-MV file\n" );
    fprintf( pMvsis->Err, "     -f : toggle writing as FSM [default = %s]\n", fWriteFsm? "yes":"no" );
    fprintf( pMvsis->Err, "     -z : toggle writing as BLIF-MV/AUT [default = %s]\n", fWriteBlifMv? "yes":"no" );
    fprintf( pMvsis->Err, "     -h : print the help massage\n" );
    fprintf( pMvsis->Err, "   file : the name of the file to write\n" );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


