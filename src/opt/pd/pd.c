/**CFile****************************************************************

  FileName    [pd.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: pd.c,v 1.8 2005/06/02 03:34:12 alanmi Exp $]

***********************************************************************/

#include "pdInt.h"
#include "cmd.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Pd_CommandPairDecode      ( Mv_Frame_t * pMvsis, int argc, char ** argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Initializes the pd package.]

  SideEffects []

  SeeAlso     [Pd_End]

******************************************************************************/
void Pd_Init( Mv_Frame_t * pMvsis )
{

#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "pair_decode",   Pd_CommandPairDecode,       1 );
#endif
}

/**Function********************************************************************

  Synopsis    [Ends the pd package.]

  SideEffects []

  SeeAlso     [Pd_Init]

******************************************************************************/
void Pd_End( Mv_Frame_t * pMvsis )
{
    return;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Pd_CommandPairDecode( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c, iNum, threshValue, changed, resubOption, iCost, Timelimit;
    char * sValue;
    double timeout;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    // set the defaults
    resubOption = RESUB_SIMP;
    fVerbose = 0;
    iNum = INFINITY;
    threshValue = 0;
    timeout = 3600;
    sValue = Cmd_FlagReadByName( pMvsis, "cost" );
    if ( sValue ) 
    {
        iCost = atoi( sValue );
    }
    else
    {
        iCost = 1;
    }
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "ithvmn")) != EOF ) 
    {
        switch (c) 
        {
            case 'i':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-n\" should be followed by an integer.\n" );
                    goto usage;
                }
                iNum = atoi(argv[util_optind]);
                util_optind++;
                if ( iNum <= 0 ) goto usage;
                break;
            case 't':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-n\" should be followed by an integer.\n" );
                    goto usage;
                }
                timeout = atof(argv[util_optind]);
                util_optind++;
                if ( timeout <= 0 ) goto usage;
            case 'v':
                fVerbose ^= 1;
                break;
            /*
            case 'm':
                if (strcmp(util_optarg, "simp") == 0) 
                {
                    resubOption = RESUB_SIMP; 
                }
                else if (strcmp(util_optarg, "alg") == 0) 
                {
                    resubOption = RESUB_ALGEBRAIC;
                }
                else 
                    goto usage;
                break;
            */
            case 'n':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-n\" should be followed by an integer.\n" );
                    goto usage;
                }
                threshValue = atoi(argv[util_optind]);
                util_optind++;
                break;
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }

    // read in threshold value
    /*
    if ( argc == util_optind+1 ) {
        threshValue = atoi(argv[util_optind]);
        if ( threshValue >= INFINITY || threshValue <= -INFINITY ) goto usage;
    }
    */

    Timelimit = (int)( timeout * (float)(CLOCKS_PER_SEC) ) + clock();
    changed = 0;
    do {
        if ( clock() > Timelimit )
            break;
        changed = Pd_PairDecode( pNet, threshValue, iCost, resubOption, Timelimit, fVerbose );
        iNum--;
    } while ( iNum > 0 && changed );

    Ntk_NetworkSweep( pNet, 1, 1, 1, 0); 
    return 0;

usage:
    fprintf( pErr, "usage: pair_decode <-i num> <-t timeout> <-m simp/alg> <thresh>\n");
    fprintf( pErr, "\t         pair nodes to reduce cost\n" );  
    fprintf( pErr, "\t-i     : the number of iterations\n");
    fprintf( pErr, "\t-t     : timeout limit in seconds\n");
    fprintf( pErr, "\t-m     : resubstitution method: simplify or algebraic\n");
    return 1;       /* error exit */
}
