/**CFile****************************************************************

  FileName    [ver.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ver.c,v 1.8 2005/06/02 03:34:18 alanmi Exp $]

***********************************************************************/

#include "verInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ntk_CommandNetworkSweepSat  ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkVerifySat ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkReach     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFuncDep   ( Mv_Frame_t * pMvsis, int argc, char ** argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function********************************************************************

  Synopsis    [Initializes the I/O package.]

  SideEffects []

  SeeAlso     [Io_End]

******************************************************************************/
void Ver_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "sat_sweep",  Ntk_CommandNetworkSweepSat,  1 ); 
    Cmd_CommandAdd( pMvsis, "Verification",            "sat_verify", Ntk_CommandNetworkVerifySat, 0 ); 
    Cmd_CommandAdd( pMvsis, "Verification",            "reach",      Ntk_CommandNetworkReach,     0 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "fd",         Ntk_CommandNetworkFuncDep,   0 ); 
#endif
}

/**Function********************************************************************

  Synopsis    [Ends the I/O package.]

  SideEffects []

  SeeAlso     [Io_Init]

******************************************************************************/
void Ver_End( Mv_Frame_t * pMvsis )
{
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkSweepSat( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c;
    int fMergeInvertedNodes;
    int fSplitLimit;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose      = 0;
    fMergeInvertedNodes = 1;
    fSplitLimit   = 5;

    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "ilvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'l':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-l\" should be followed by a number.\n" );
                    goto usage;
                }
                fSplitLimit = atoi( argv[util_optind] );
                util_optind++;
                break;
            case 'i':
                fMergeInvertedNodes ^= 1;
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    Ntk_NetworkSweep( pNet, 1, 1, 0, 0 );
    Ver_NetworkSweep( pNet, fVerbose, fMergeInvertedNodes, fSplitLimit );
    Ntk_NetworkSweep( pNet, 1, 1, 0, 0 );
    return 0;

usage:
    fprintf( pErr, "usage: sat_sweep [-h] [-i] [-l limit] [-v] \n");
    fprintf( pErr, "\t         sweeps functionally equivalent nodes using SAT\n");
    fprintf( pErr, "\t-h     : print the command usage\n");
    fprintf( pErr, "\t-i     : merge nodes that are equivalent upto an inversion [default = %s]\n", fMergeInvertedNodes? "yes": "no" );  
    fprintf( pErr, "\t-l     : if an equiv class contains more members than limit, it is not considered. -1 means no limit. [default = %d]\n", fSplitLimit );  
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", fVerbose? "yes": "no" );  
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkVerifySat( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Ntk_Network_t * pNet, * pNet1, * pNet2;
    FILE * pOut, * pErr, * pFile;
    int fVerbose, c;
    int nFrames;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    nFrames  = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "ivh")) != EOF) 
    {
        switch(c) 
        {
            case 'i':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-i\" should be followed by an integer.\n" );
                    goto usage;
                }
                nFrames = atoi(argv[util_optind]);
                util_optind++;
                if ( nFrames < 0 ) 
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

    if ( argc == util_optind ) 
    { // use the spec
        if ( pNet == NULL )
        {
            fprintf( pErr, "Empty current network.\n" );
            return 1;
        }
        if ( pNet->pSpec == NULL )
        {
            fprintf( pErr, "The external spec is not given; cannot perform verification.\n" );
            return 1;
        }

        pFile = fopen( pNet->pSpec, "r" );
        if ( pFile == NULL )
        {
            fprintf( pErr, "Cannot open the external spec file \"%s\"; cannot perform verification.\n", pNet->pSpec );
            return 1;
        }
        else
            fclose( pFile );

        // the first network is the current network
        pNet1 = pNet;
        // read the spec network using the same functionality manager
        pNet2 = Io_ReadNetwork( pMvsis, pNet->pSpec );
        if ( pNet2 == NULL )
            return 1;
        // verify
        if ( nFrames > 0 )
            Ver_NetworkVerifySatSeq( pNet1, pNet2, nFrames, fVerbose );
        else
            Ver_NetworkVerifySatComb( pNet1, pNet2, fVerbose );
        // delete the second network
        Ntk_NetworkDelete( pNet2 );
        return 0;
    }
    if ( argc == util_optind + 1 ) 
    {
        if ( pNet == NULL )
        {
            fprintf( pErr, "Empty current network.\n" );
            return 1;
        }
        // the first network is the current network
        pNet1 = pNet;
        // read the second network using the same functionality manager
        pNet2 = Io_ReadNetwork( pMvsis, argv[util_optind] );
        if ( pNet2 == NULL )
            return 1;
        // verify
        if ( nFrames > 0 )
            Ver_NetworkVerifySatSeq( pNet1, pNet2, nFrames, fVerbose );
        else
            Ver_NetworkVerifySatComb( pNet1, pNet2, fVerbose );
        // delete the second network
        Ntk_NetworkDelete( pNet2 );
        return 0;
    }
    if ( argc == util_optind + 2 ) 
    {
        Fnc_Manager_t * pManOld, * pManNew;

        // perform verification in the new functionality manager
        pManNew = Fnc_ManagerAllocate();
        pManOld = Mv_FrameSetMan( pMvsis, pManNew );

        // read the first network with the new functionality manager
        pNet1 = Io_ReadNetwork( pMvsis,  argv[util_optind] );
        if ( pNet1 == NULL )
            return 1;
        // read the second network with the same functionality manager
        pNet2 = Io_ReadNetwork( pMvsis, argv[util_optind+1] );
        if ( pNet2 == NULL )
        {
            Ntk_NetworkDelete( pNet1 );
            return 1;
        }
        // verify
        if ( nFrames > 0 )
            Ver_NetworkVerifySatSeq( pNet1, pNet2, nFrames, fVerbose );
        else
            Ver_NetworkVerifySatComb( pNet1, pNet2, fVerbose );
        // delete the network (should also delete the new functionality manager)
        Ntk_NetworkDelete( pNet1 );
        Ntk_NetworkDelete( pNet2 );

        // set the old manager
        pManNew = Mv_FrameSetMan( pMvsis, pManOld );
        // deallocate the new manager
        Fnc_ManagerDeallocate( pManNew );
        return 0;
    }

usage:
    fprintf( pErr, "usage: sat_verify [-i num] [-v] [-h] <file1> <file2>\n");
    fprintf( pErr, "\t          verifies the equivalence of two binary circuits;\n" );  
    fprintf( pErr, "\t          performs combinational verification by default;\n" );  
    fprintf( pErr, "\t          to verify sequentially, specify the number of time frames\n" );  
	fprintf( pErr, "\t-i num  : the number of time frames to use for seq verification [default = %d]\n", nFrames );
    fprintf( pErr, "\t-v      : print detailed information [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h      : print the command usage\n");
    fprintf( pErr, "\t<file1> : (if given) the network to verify against the current network\n" );  
    fprintf( pErr, "\t<file2> : (if given) the network to verify against <file1>\n" );  
    fprintf( pErr, "\t        : if no file name is given, verifies against the spec\n" );  
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkReach( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int fAlgorithm;
    int nIters;
    int nSquar;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    nIters     = 5;
    nSquar     = 0;
    fAlgorithm = 0;
    fVerbose   = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "isahv")) != EOF ) 
    {
        switch (c) 
        {
            case 'i':
                nIters = atoi(argv[util_optind]);
                util_optind++;
                if ( nIters < 0 ) 
                    goto usage;
                break;
            case 's':
                nSquar = atoi(argv[util_optind]);
                util_optind++;
                if ( nSquar < 0 ) 
                    goto usage;
                break;
            case 'a':
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }
    Ver_NetworkReachability( pNet, nIters, nSquar, fAlgorithm );
    return 0;

usage:
    fprintf( pErr, "usage: reach [-i num] [-s num] [-h]\n");
    fprintf( pErr, "\t         performes reachability on the network\n" );  
    fprintf( pErr, "\t-i num : the number of iterations to perform [default = %d]\n", nIters );
    fprintf( pErr, "\t-s num : the number of times to square the relation [default = %d]\n", nSquar );
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkFuncDep( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int fAlgorithm;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fAlgorithm = 0;
    fVerbose   = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "ahv")) != EOF ) 
    {
        switch (c) 
        {
            case 'a':
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }
    Ver_FunctionalDependency( pNet );
    return 0;

usage:
    fprintf( pErr, "usage: fd [-h]\n");
    fprintf( pErr, "\t         explores latch dependency of the current network using SAT\n" );  
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


