/**CFile****************************************************************

  FileName    [fpga.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Command file for the FPGA package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2004.]

  Revision    [$Id: fpga.c,v 1.7 2005/05/18 04:14:38 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Fpga_CommandReadLibrary( Mv_Frame_t * pMvsis, int argc, char **argv );
static int Fpga_CommandPrintLibrary( Mv_Frame_t * pMvsis, int argc, char **argv );

// storage for the current LUT library
Fpga_LutLib_t * s_pLutLib = NULL;


// the default LUT library
//Fpga_LutLib_t    s_LutLib = { "lutlib", 6, {0,1,2,4,8,16,32}, {0,1,2,3,4,5,6} };
Fpga_LutLib_t    s_LutLib = { "lutlib", 5, {0,1,1,1,1,1}, {0,1,1,1,1,1} };

// the library file format should be as follows:
/*
# The area/delay of k-variable LUTs:
# k  area   delay
1      1      1
2      2      2
3      4      3
4      8      4
5     16      5
6     32      6
*/

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Package initialization procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Mapping", "read_lut",   Fpga_CommandReadLibrary,  0 ); 
    Cmd_CommandAdd( pMvsis, "Mapping", "print_lut",  Fpga_CommandPrintLibrary, 0 ); 
#endif
}

/**Function*************************************************************

  Synopsis    [Package ending procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_End()
{
    Fpga_LutLibFree( s_pLutLib );
}


/**Function*************************************************************

  Synopsis    [Command procedure to read LUT libraries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CommandReadLibrary( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pFile;
    FILE * pOut, * pErr;
    Fpga_LutLib_t * pLib;
    Ntk_Network_t * pNet;
    char * FileName;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 1;
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
        if ( FileName = Io_ReadFileGetSimilar( FileName, ".genlib", ".lib", ".gen", ".g", NULL ) )
            fprintf( pErr, "Did you mean \"%s\"?", FileName );
        fprintf( pErr, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pLib = Fpga_LutLibCreate( FileName, fVerbose );
    if ( pLib == NULL )
    {
        fprintf( pErr, "Reading LUT library has failed.\n" );
        goto usage;
    }
    // replace the current library
    Fpga_LutLibFree( s_pLutLib );
    s_pLutLib = pLib;
    return 0;

usage:
    fprintf( pErr, "\nusage: read_lut [-vh]\n");
    fprintf( pErr, "\t          read the LUT library from the file\n" );  
    fprintf( pErr, "\t-v      : toggles enabling of verbose output [default = %s]\n", (fVerbose? "yes" : "no") );
    fprintf( pErr, "\t-h      : print the command usage\n");
    fprintf( pErr, "\t                                        \n");
    fprintf( pErr, "\t          File format for a LUT library:\n");
    fprintf( pErr, "\t          (the default library is shown)\n");
    fprintf( pErr, "\t                                        \n");
    fprintf( pErr, "\t          # The area/delay of k-variable LUTs:\n");
    fprintf( pErr, "\t          # k  area   delay\n");
    fprintf( pErr, "\t          1      1      1\n");
    fprintf( pErr, "\t          2      2      2\n");
    fprintf( pErr, "\t          3      4      3\n");
    fprintf( pErr, "\t          4      8      4\n");
    fprintf( pErr, "\t          5     16      5\n");
    fprintf( pErr, "\t          6     32      6\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    [Command procedure to read LUT libraries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CommandPrintLibrary( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 1;
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


    if ( argc != util_optind )
    {
        goto usage;
    }

    // set the new network
    Fpga_LutLibPrint( s_pLutLib );
    return 0;

usage:
    fprintf( pErr, "\nusage: read_print [-vh]\n");
    fprintf( pErr, "\t          print the current LUT library\n" );  
    fprintf( pErr, "\t-v      : toggles enabling of verbose output [default = %s]\n", (fVerbose? "yes" : "no") );
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       /* error exit */
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


