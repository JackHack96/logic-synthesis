/**CFile****************************************************************

  FileName    [mfsm.c]

  PackageName [Simplification of networks using complete flexiblity.]

  Synopsis    [Command file.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: mfsm.c,v 1.5 2005/06/02 03:34:11 alanmi Exp $]

***********************************************************************/
 
#include "mfsmInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ntk_CommandNetworkMfsm( Mv_Frame_t * pMvsis, int argc, char ** argv );

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfsm_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "mfs", Ntk_CommandNetworkMfsm,  1 ); 
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfsm_End( Mv_Frame_t * pMvsis )
{
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Gateway to "mfs".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkMfsm( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
	Mfsm_Params_t P, * p = &P;
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t ** ppNodes;
	int nIter, nNodes, Window, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    if ( pNet && Ntk_NetworkReadNodeIntNum(pNet) == 0 )
        return 0;

	// allocate the storage for info related to this simplification run
	memset( p, 0, sizeof(Mfsm_Params_t) );

	// assign the default parameters
    p->nNodesMax    = 300;// "-n" the maximum number of AND-gates
    p->nFaninMax    = 30; // "f" the largest fanin count
	p->nIters       = 1;  // "-i"
    p->nCands       = 8;  // "-c"
	p->fDir         = MFSM_DIR_FROM_OUTPUTS;  // "-d"
	p->fSweep       = 1;  // "-q"
	p->fResub       = 1;  // "-r"
	p->fEspresso    = 1;  // "-e"
	p->fPrint       = 0;  // "-k"
    // parameters of DCM
    p->fUseExdc     = 1;  // "-x"
    p->fDynEnable   = 1;  // "-y"
	p->fVerbose     = 0;  // "-v"
    p->nLevelTfi    = -1; // "-w"
    p->nLevelTfo    = -1; // "-w"
    p->AcceptType   = pNet? Ntk_NetworkGetAcceptType( pNet ): 0;
    p->fBinary      = 0;
    p->fUseSat      = 0;

	util_getopt_reset();
	while ( (c = util_getopt(argc, argv, "snfwcdqrekxyv")) != EOF ) 
	{
		switch ( c ) 
		{
			case 's':
				p->fUseSat ^= 1;
				break;
            case 'n':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-n\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nNodesMax = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nNodesMax < 0 ) 
                    goto usage;
                break;
            case 'f':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-f\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nFaninMax = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nFaninMax < 0 ) 
                    goto usage;
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
                p->nLevelTfi = Window / 10;
                p->nLevelTfo = Window % 10;
				break;
			case 'i':
				p->nIters = atoi(argv[util_optind]);
				util_optind++;
				if ( p->nIters < 0 ) 
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
			case 'd':
				if (strcmp(argv[util_optind], "io") == 0) {
					p->fDir = MFSM_DIR_FROM_INPUTS;
				} else if (strcmp(argv[util_optind], "oi") == 0) {
					p->fDir = MFSM_DIR_FROM_OUTPUTS;
				} else if (strcmp(argv[util_optind], "random") == 0) {
					p->fDir = MFSM_DIR_RANDOM;
				} else {
					goto usage;
				}
				util_optind++;
				break;
			case 'q':
				p->fSweep ^= 1;
				break;
			case 'r':
				p->fResub ^= 1;
				break;
			case 'e':
				p->fEspresso ^= 1;
				break;
			case 'k':
				p->fPrint ^= 1;
				break;
			case 'x':
				p->fUseExdc ^= 1;
				break;
			case 'y':
				p->fDynEnable ^= 1;
				break;
			case 'v':
				p->fVerbose ^= 1;
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

    // collect the nodes if any
    ppNodes = NULL;
    nNodes = 0;
    if ( util_optind < argc )
    {
        Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
        nNodes = Ntk_NetworkCountSpecial(pNet);
        ppNodes = ALLOC( Ntk_Node_t *, nNodes );
        Ntk_NetworkCreateArrayFromSpecial( pNet, ppNodes );
    }

    // check if the network is binary
    p->fBinary = Ntk_NetworkIsBinary( pNet );
	// perform interative simplification
    if ( p->fBinary && p->nLevelTfi != -1 && p->nLevelTfo != -1 )
    { // the network is pure binary and windowing is requested
//printf( "Running binary.\n" );
	    for ( nIter = 0; nIter < p->nIters; nIter++ )
		    if ( Mfsb_NetworkMfs( p, pNet, ppNodes, nNodes ) == 0 )
			    break;
    }
    else
    { // the network has MV nodes, or is binary with ISF nodes,
      // or the network is pure binary and no windowing is requested
//printf( "Running MV.\n" );
	    for ( nIter = 0; nIter < p->nIters; nIter++ )
		    if ( Mfsm_NetworkMfs( p, pNet, ppNodes, nNodes ) == 0 )
			    break;
    }

	return 0;					// normal exit

usage:
//	fprintf( pErr, "\n" );
	fprintf( pErr, "usage: mfs [-s] [-w NM] [-i num] [-c num] [-d dir] [-qrekxyv]\n" );
	fprintf( pErr, "       performs simplification of the network using complete don't-cares\n" );
	fprintf( pErr, "\t-s      : toggles the SAT based don't-care computation [default = %s]\n", p->fVerbose? "yes": "no" );
	fprintf( pErr, "\t-w NM   : the number of TFI (N) and TFO (M) logic levels [default = total network]\n" );
	fprintf( pErr, "\t-n num  : the limit on the number of AND-nodes for SAT [default = %d]\n", p->nNodesMax );
	fprintf( pErr, "\t-i num  : the number of iterations over the network [default = %d]\n", p->nIters );
    fprintf( pErr, "\t-c num  : the max number of resubstitution candidates [default = %d]\n", p->nCands ); 
	fprintf( pErr, "\t-d dir  : the node ordering: oi (outputs to inputs) [default] | io (vice versa) | random\n");
    fprintf( pErr, "\t-f num  : the fanin limit (if more, skip the node) [default = %d]\n", p->nFaninMax );
	fprintf( pErr, "\t-q      : toggles removing equivalent nets [default = %s]\n", p->fSweep? "yes": "no" );
	fprintf( pErr, "\t-r      : toggles the use of resubstitution [default = %s]\n", p->fResub? "yes": "no" );
	fprintf( pErr, "\t-e      : toggles Espresso with/without reduced offset [default = %s]\n", p->fEspresso? "with": "without" );
    fprintf( pErr, "\t-k      : toggles printout of the K-maps [default = %s]\n", p->fPrint? "yes": "no" );
	fprintf( pErr, "\t-x      : toggles the use of external don't-cares [default = %s]\n", p->fUseExdc? "yes": "no" );
    fprintf( pErr, "\t-y      : toggles dynamic BDD variable reordering [default = %s]\n", p->fDynEnable? "yes": "no" );
	fprintf( pErr, "\t-v      : toggles verbose output [default = %s]\n", p->fVerbose? "yes": "no" );
	return 1;					// error exit 
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


