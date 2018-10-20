/**CFile****************************************************************

  FileName    [mfsn.c]

  PackageName [Simplification of networks using complete flexiblity.]

  Synopsis    [Command file.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 13, 2003.]

  Revision    [$Id: mfsn.c,v 1.3 2005/06/02 03:34:11 alanmi Exp $]

***********************************************************************/
 
#include "mfsnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ntk_CommandNetworkMfsn( Mv_Frame_t * pMvsis, int argc, char ** argv );

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

void Mfsn_Init( Mv_Frame_t * pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "mfsn", Ntk_CommandNetworkMfsn,  1 ); 
#endif
}

void Mfsn_End( Mv_Frame_t * pMvsis )
{
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Gateway to mfsSimplify().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkMfsn( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    char * Names[4] = {"NONE", "SS", "NSC", "NS"};
	Mfsn_Params_t P, * p = &P;
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
	int nIter;
	int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    if ( pNet && Ntk_NetworkReadNodeIntNum(pNet) == 0 )
        return 0;

	// allocate the storage for info related to this simplification run
	memset( p, 0, sizeof(Mfsn_Params_t) );

	// assign the default parameters
    // general info
	p->nIters       = 1;
    p->AcceptType   = pNet? Ntk_NetworkGetAcceptType( pNet ): 0;
    p->fGlobal      = 0;
    // parameters of DCM
    p->TypeSpec     = DCMN_TYPE_SS; // "s"
    p->TypeFlex     = DCMN_TYPE_SS; // "f"
	p->nBddSizeMax  = 10000000;  // "b" the largest BDD size
	p->fSweep       = 0;
    p->fUseExdc     = 0;         // use EXDC
    p->fDynEnable   = 0;         // enable dynmamic variable reordering
	p->fVerbose     = 0;
	p->fUseProduct  = 0;
    // parameters of the network
	p->fDir         = DCMN_DIR_FROM_OUTPUTS;
    p->fOneNode     = 0;   // automatically remove the dangling nodes (nodes w/o POs in TFO)
	p->fResub       = 0;
	p->fEspresso    = 0;
	p->fPrint       = 0;

	util_getopt_reset();
	while ( (c = util_getopt(argc, argv, "isfxvdrepgy")) != EOF ) 
	{
		switch ( c ) 
		{
			case 'i':
				p->nIters = atoi(argv[util_optind]);
				util_optind++;
				if ( p->nIters < 0 ) 
					goto usage;
				break;
			case 's':
				if (strcmp(argv[util_optind], "ss") == 0 || strcmp(argv[util_optind], "SS") == 0) {
					p->TypeSpec = DCMN_TYPE_SS;
				} else if (strcmp(argv[util_optind], "nsc") == 0 || strcmp(argv[util_optind], "NSC") == 0) {
					p->TypeSpec = DCMN_TYPE_NSC;
				} else if (strcmp(argv[util_optind], "ns") == 0 || strcmp(argv[util_optind], "NS") == 0) {
					p->TypeSpec = DCMN_TYPE_NS;
				} else {
					goto usage;
				}
				util_optind++;
				break;
			case 'f':
				if (strcmp(argv[util_optind], "ss") == 0 || strcmp(argv[util_optind], "SS") == 0) {
					p->TypeFlex = DCMN_TYPE_SS;
				} else if (strcmp(argv[util_optind], "nsc") == 0 || strcmp(argv[util_optind], "NSC") == 0) {
					p->TypeFlex = DCMN_TYPE_NSC;
				} else if (strcmp(argv[util_optind], "ns") == 0 || strcmp(argv[util_optind], "NS") == 0) {
					p->TypeFlex = DCMN_TYPE_NS;
				} else {
					goto usage;
				}
				util_optind++;
				break;
/*
			case 'b':
				p->nBddSizeMax = atoi(argv[util_optind]);
				util_optind++;
				if ( p->nBddSizeMax < 0 ) 
					goto usage;
				break;
			case 'x':
				p->fUseExdc ^= 1;
				break;
*/
			case 'v':
				p->fVerbose ^= 1;
				break;
			case 'd':
				if (strcmp(argv[util_optind], "io") == 0) {
					p->fDir = DCMN_DIR_FROM_INPUTS;
				} else if (strcmp(argv[util_optind], "oi") == 0) {
					p->fDir = DCMN_DIR_FROM_OUTPUTS;
				} else if (strcmp(argv[util_optind], "random") == 0) {
					p->fDir = DCMN_DIR_RANDOM;
				} else {
					goto usage;
				}
				util_optind++;
				break;
			case 'r':
				p->fResub ^= 1;
				break;
			case 'e':
				p->fEspresso ^= 1;
				break;
			case 'p':
				p->fUseProduct ^= 1;
				break;
			case 'g':
				p->fGlobal ^= 1;
				break;
			case 'y':
				p->fDynEnable ^= 1;
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

    printf( "Using %s spec. ", Names[p->TypeSpec] );
    if ( !p->fGlobal )
        printf( "Computing %s flexibility.", Names[p->TypeFlex] );
    printf( "\n" );


    // get the node name
    pNode = NULL;
    if ( util_optind < argc )
    {
        pNode = Ntk_NetworkCollectNodeByName( pNet, argv[util_optind], 0 );
        if ( pNode == NULL )
            goto usage;
        p->fOneNode = 1;
        Mfsn_NetworkMfsOne( p, pNode );
        return 0;
    }

	// perform interative simplification
	for ( nIter = 0; nIter < p->nIters; nIter++ )
		if ( Mfsn_NetworkMfs( p, pNet ) == 0 )
			break;

	return 0;					// normal exit

usage:
//	fprintf( pErr, "\n" );
	fprintf( pErr, "usage: mfsn [-s spec] [-f flex] [-d dir] [-yvpg]\n" );
	fprintf( pErr, "       performs simplification of the network using complete don't-cares\n" );
//	fprintf( pErr, "        -i num  : the number of iterations over the network\n" );
	fprintf( pErr, "        -s spec : the specification type: SS [default] | NSC | NS\n" );
	fprintf( pErr, "        -f flex : the flexibility type:   SS [default] | NSC | NS\n" );
//	fprintf( pErr, "        -b num  : the max size of the global BDDs when construction is aborted\n" );
	fprintf( pErr, "        -d dir  : the node ordering: oi (outputs to inputs) [default] | io (vice versa) | random\n");
//	fprintf( pErr, "        -s      : toggles BDD sweep before and after optimization [default = %s]\n", p->fSweep? "yes": "no" );
    fprintf( pErr, "        -y      : toggles dynamic BDD variable reordering [default = %s]\n", p->fDynEnable? "yes": "no" );
//	fprintf( pErr, "        -x      : toggles the use of external don't-cares [default = %s]\n", p->fUseExdc? "yes": "no" );
	fprintf( pErr, "        -v      : toggles printout of the K-maps [default = %s]\n", p->fVerbose? "yes": "no" );
//	fprintf( pErr, "        -r      : toggles the use of resubstitution [default = %s]\n", p->fResub? "yes": "no" );
//	fprintf( pErr, "        -e      : toggles Espresso with/without reduced offset [default = %s]\n", p->fEspresso? "with": "without" );
//  fprintf( pErr, "        -p      : toggles printing the complete flexibility [default = %s]\n", p->fPrint? "yes": "no" );
    fprintf( pErr, "        -p      : toggles the use of product in the formula [default = %s]\n", p->fUseProduct? "new": "old" );
    fprintf( pErr, "        -g      : toggles computation of global functions only [default = %s]\n", p->fGlobal? "yes": "no" );
	return 1;					// error exit 
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


