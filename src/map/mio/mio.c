/**CFile****************************************************************

  FileName    [mio.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: mio.c,v 1.6 2005/06/02 03:34:10 alanmi Exp $]

***********************************************************************/

#include "mioInt.h"
#include "ioInt.h"
#include "mapper.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Mio_CommandReadLibrary( Mv_Frame_t * pMvsis, int argc, char **argv );

Mio_Library_t * s_pLib = NULL;

extern Map_SuperLib_t * s_pSuperLib;
extern void Map_SuperLibFree( Map_SuperLib_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Mapping", "read_library",  Mio_CommandReadLibrary, 0 ); 
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_End()
{
    Mio_LibraryDelete( s_pLib );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_CommandReadLibrary( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pFile;
    FILE * pOut, * pErr;
    Mio_Library_t * pLib;
    Ntk_Network_t * pNet;
    char * FileName;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "vh")) != EOF ) 
    {
        switch (c) 
        {
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
    if ( (pFile = fopen( FileName, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\". ", FileName );
        if ( (FileName = Io_ReadFileGetSimilar( FileName, ".genlib", ".lib", ".gen", ".g", NULL )) )
            fprintf( pErr, "Did you mean \"%s\"?", FileName );
        fprintf( pErr, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pLib = Mio_LibraryRead( pMvsis, FileName, 0 );  
    if ( pLib == NULL )
    {
        fprintf( pErr, "Reading GENLIB library has failed.\n" );
        goto usage;
    }
    // free the current superlib because it depends on the old Mio library
    if ( s_pSuperLib )
    {
        Map_SuperLibFree( s_pSuperLib );
        s_pSuperLib = NULL;
    }
    // replace the current library
    Mio_LibraryDelete( s_pLib );
    s_pLib = pLib;
    return 0;

usage:
    fprintf( pErr, "usage: read_library [-v] [-h]\n");
    fprintf( pErr, "\t         read the library from a genlib file\n" );  
    fprintf( pErr, "\t-h     : enable verbose output\n");
    return 1;       /* error exit */
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


