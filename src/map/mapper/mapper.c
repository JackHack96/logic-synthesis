/**CFile****************************************************************

  FileName    [mapper.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Command file for the mapper package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: mapper.c,v 1.9 2005/07/08 01:01:23 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Map_CommandReadLibrary( Mv_Frame_t * pMvsis, int argc, char **argv );

Map_SuperLib_t * s_pSuperLib = NULL;
extern Mio_Library_t * s_pLib;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Mapping", "read_super",  Map_CommandReadLibrary, 0 ); 
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_End()
{
    Map_SuperLibFree( s_pSuperLib );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_CommandReadLibrary( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pFile;
    FILE * pOut, * pErr;
    Map_SuperLib_t * pLib;
    Ntk_Network_t * pNet;
    char * FileName, * ExcludeFile;
    int fVerbose;
    int fAlgorithm;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 1;
    fAlgorithm = 1;
    ExcludeFile = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "eovh")) != EOF ) 
    {
        switch (c) 
        {
            case 'e':
                ExcludeFile = argv[util_optind];
                if ( ExcludeFile == 0 )
                    goto usage;
                util_optind++;
                break;
            case 'o':
                fAlgorithm ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
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
    if ( (pFile = Io_FileOpen( FileName, "open_path", "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\". ", FileName );
        if ( FileName = Io_ReadFileGetSimilar( FileName, ".genlib", ".lib", ".gen", ".g", NULL ) )
            fprintf( pErr, "Did you mean \"%s\"?", FileName );
        fprintf( pErr, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pLib = Map_SuperLibCreate( FileName, ExcludeFile, fAlgorithm, fVerbose );
    if ( pLib == NULL )
    {
        fprintf( pErr, "Reading supergate library has failed.\n" );
        goto usage;
    }
    // replace the current library
    Map_SuperLibFree( s_pSuperLib );
    s_pSuperLib = pLib;
    // replace the current genlib library
    if ( s_pLib )
        Mio_LibraryDelete( s_pLib );
    s_pLib = s_pSuperLib->pGenlib;
    return 0;

usage:
    fprintf( pErr, "\nusage: read_super [-ovh]\n");
    fprintf( pErr, "\t         read the supergate library from the file\n" );  
    fprintf( pErr, "\t-e file : file contains list of genlib gates to exclude\n" );
    fprintf( pErr, "\t-o      : toggles the use of old file format [default = %s]\n", (fAlgorithm? "new" : "old") );
    fprintf( pErr, "\t-v      : toggles enabling of verbose output [default = %s]\n", (fVerbose? "yes" : "no") );
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       /* error exit */
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


