/**CFile****************************************************************

  FileName    [mfs.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Simplification of MV networks with complete flexibility.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mfs.c,v 1.8 2005/06/02 03:34:11 alanmi Exp $]

***********************************************************************/

#include "ntkInt.h"
#include "mfsInt.h"
#include "sh.h"
#include "wn.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
static int Ntk_CommandNetworkMfs       ( Mv_Frame_t * pMvsis, int argc, char ** argv );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Initializes the I/O package.]

  SideEffects []

  SeeAlso     [Io_End]

******************************************************************************/
void Mfs_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "_mfs", Ntk_CommandNetworkMfs,  1 ); 
#endif
}

/**Function********************************************************************

  Synopsis    [Ends the I/O package.]

  SideEffects []

  SeeAlso     [Io_Init]

******************************************************************************/
void Mfs_End( Mv_Frame_t * pMvsis )
{
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkMfs( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Mfs_Params_t P, * p = &P;
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;  
    int c, Window;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    if ( pNet && Ntk_NetworkReadNodeIntNum(pNet) == 0 )
        return 0;

    // set the default parameters
    // modifiable
    p->fUseMv      =  0;   // switch "-m"
    p->fUseSpec    =  0;   // switch "-s"
    p->nLevelsTfi  =  2;   // switch "-i"
    p->nLevelsTfo  =  2;   // switch "-o"
    p->nTermsLimit = 30;   // switch "-l" 
    p->fUseWindow  =  0;   // switch "-i" or "-o"
    p->fVerbose    =  0;   // switch "-v"
    p->fShowKMap   =  0;   // switch "-k"
    p->fOrderItoO  =  0;   // switch "-d"
    p->nCands      =  8;   // switch "-c"
    // unmodifiable
    p->fUseSS      =  1;
    p->fBinary     = -1;
    p->fNonDet     = -1;
    p->fSetInputs  =  0;
    p->fSetOutputs =  0;
    p->AcceptType  =  0;
    p->fOneNode    =  0;

    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "msdwlckvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'm':
                p->fUseMv ^= 1;
                break;
            case 's':
                p->fUseSpec ^= 1;
                break;
            case 'd':
                p->fOrderItoO ^= 1;
                break;
			case 'w':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-w\" should be followed by an integer.\n" );
                    goto usage;
                }
                if ( strlen(argv[util_optind]) != 2 )
					goto usage;
				Window = atoi(argv[util_optind]);
				util_optind++;
				if ( Window < 0 || Window > 99 ) 
					goto usage;
                p->nLevelsTfi = Window / 10;
                p->nLevelsTfo = Window % 10;

                p->fUseWindow = 1;
				break;
/*
            case 'i':
                p->fUseWindow = 1;
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-i\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nLevelsTfi = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nLevelsTfi < 0 ) 
                    goto usage;
                break;
            case 'o':
                p->fUseWindow = 1;
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-o\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nLevelsTfo = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nLevelsTfo < 0 ) 
                    goto usage;
                break;
*/
            case 'l':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-l\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nTermsLimit = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nTermsLimit < 0 ) 
                    goto usage;
                break;
            case 'c':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-c\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nCands = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nCands < 0 ) 
                    goto usage;
                break;
            case 'k':
                p->fShowKMap ^= 1;
                break;
            case 'v':
                p->fVerbose ^= 1;
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

    // get the node name
    pNode = NULL;
    if ( util_optind < argc )
    {
        pNode = Ntk_NetworkCollectNodeByName( pNet, argv[util_optind], 0 );
        if ( pNode == NULL )
            goto usage;
        p->fOneNode = 1;
        Mfs_NetworkMfsOne( p, pNode );
        return 0;
    }

    if ( !p->fUseWindow )
        Mfs_NetworkMfsSpec( p, pNet );
    else
        Mfs_NetworkMfs( p, pNet );
    return 0;

usage:
    fprintf( pErr, "usage: _mfs [-w NM] [-m] [-s] [-k] [-v] [-h] <node>\n");
    fprintf( pErr, "\t         simplifies the network using complete flexibility\n" );  
	fprintf( pErr, "\t-w NM  : the number of TFI (N) and TFO (M) logic levels [default = total network]\n" );
    fprintf( pErr, "\t-m     : forces MV processing [default = %s]\n", p->fUseMv? "yes": "no" ); 
    fprintf( pErr, "\t-s     : toggles use of the external spec if available [default = %s]\n", p->fUseSpec? "yes": "no" ); 
    fprintf( pErr, "\t-d     : toggles the direction of simplification [default = %s]\n", p->fOrderItoO? "CI to CO": "CO to CI" ); 
//    fprintf( pErr, "\t-i num : the number of TFI levels for windowing [default = %d]\n", p->nLevelsTfi ); 
//    fprintf( pErr, "\t-o num : the number of TFO levels for windowing [default = %d]\n", p->nLevelsTfo ); 
    fprintf( pErr, "\t-l num : the max number of window leaves/roots  [default = %d]\n", p->nTermsLimit ); 
    fprintf( pErr, "\t-c num : the max number of resubstitution candidates [default = %d]\n", p->nCands ); 
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", p->fVerbose? "yes": "no" ); 
    fprintf( pErr, "\t-k     : print K-map for CF of <node> or all nodes in the network [default = %s]\n", p->fShowKMap? "yes": "no" ); 
    fprintf( pErr, "\t-h     : print the command usage\n");
    fprintf( pErr, "\t<node> : the name of one node to simplify\n");
    return 1;       /* error exit */
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


