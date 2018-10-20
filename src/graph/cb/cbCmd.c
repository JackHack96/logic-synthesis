/*****************************************************************************

  FileName    [cbCmd.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [clubbing commands]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cbCmd.c,v 1.6 2003/11/18 18:55:01 alanmi Exp $]

******************************************************************************/

#include "cb.h"
#include "cmd.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static int  Cb_CommandClub(Mv_Frame_t * pMvsis, int argc, char ** argv);
static int  Cb_CommandPrintDomi( Mv_Frame_t * pMvsis, int argc, char **argv );
static int  Cb_CommandPrintDot ( Mv_Frame_t * pMvsis, int argc, char **argv );
static bool Cb_NetworkCheckNonFanout( Ntk_Network_t *pNet );


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Cb_Init( Mv_Frame_t *pMvsis )
{
    Cmd_CommandAdd(pMvsis, "Combinational synthesis", "club",       Cb_CommandClub, 1);
    Cmd_CommandAdd(pMvsis, "Printing",  "print_domi", Cb_CommandPrintDomi, 0);
    Cmd_CommandAdd(pMvsis, "Writing",   "_write_dot", Cb_CommandPrintDot, 0);
}

void
Cb_End( void )
{
    return;
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Group nodes into clusters and collapse them]
  
  CommandName [club]
  
  CommandSynopsis [Group nodes into clusters and collapse them]
  
  CommandArguments []
  
  CommandDescription [Group nodes into a set of disjoint clusters and collapse
  all nodes in the same cluster. The default approach is to compute the prime
  dominators for each node (a dominator that is not dominated by any other
  node), and collapse all nodes into its prime dominators. This tends to give
  results that lie between those of eliminate and collapse. An alternative
  approach visits all nodes from primary inputs to primary outputs in a
  levelized order, and heuristically merge nodes into a cluster such that the
  sharing of fanin and fanouts are maximized within a cluster. The size of a
  cluster is limited by parameters settable at the command line (number of
  input/output bits and total number of literals).
  ]
  
  SideEffects []

******************************************************************************/
static int
Cb_CommandClub(
    Mv_Frame_t *pMvsis,
    int     argc,
    char ** argv)
{
    char          c;
    FILE          *pErr;
    Ntk_Network_t *pNet;
    Cb_Option_t   *pOpt;
    
    pNet = Mv_FrameReadNet( pMvsis );
    pErr = Mv_FrameReadErr( pMvsis );
    pOpt = Cb_OptionInit( pMvsis );
    
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "c:i:o:m:v")) != EOF) {
        switch(c) {
            case 'v':
                pOpt->fVerb = TRUE;
                break;
                
            case 'c':
                pOpt->nCost = atoi(util_optarg);
                if ( pOpt->nCost < 1 )
                    goto usage;
                break;
                
            case 'i':
                pOpt->nMaxIn = atoi(util_optarg);
                if ( pOpt->nMaxIn < 1 )
                    goto usage;
                break;
                
            case 'o':
                pOpt->nMaxOut = atoi(util_optarg);
                if ( pOpt->nMaxOut < 0 )
                    goto usage;
                break;
                
            case 'm':
                if (strcmp(util_optarg, "dominator") == 0) {
                    pOpt->iMethod = 1;
                }
                else if (strcmp(util_optarg, "greedy") == 0) {
                    pOpt->iMethod = 0;
                }
                else {
                    goto usage;
                }
                break;
                
            default:
                goto usage;
        }
    }
    
    /* safty checks */
    if ( pNet == NULL ) {
        fprintf( Mv_FrameReadErr(pMvsis), "Empty network\n" );
        FREE( pOpt );
        return 1;
    }
    if (!Ntk_NetworkCheck(pNet)) {
        printf("club aborted\n");
        FREE( pOpt );
        return 1;
    }
    
    Ntk_NetworkSweep( pNet, 1,1,1,0 );
    Cb_NetworkClub( pOpt, pNet );
    
    FREE( pOpt );
    return 0;
    
  usage:
    fprintf(pErr, "Usage: club [-v][-m method][-c cost][-i num][-o num]\n");
    fprintf(pErr, "       group nodes into clusters and collapse them\n");
    fprintf(pErr, "\t-v        : verbose for debugging\n");
    fprintf(pErr, "\t-c cost   : maximum cost for each club [default=200] (for greedy)\n");
    fprintf(pErr, "\t-i num    : maximum number of inputs for each club [default=20] (for greedy)\n");
    fprintf(pErr, "\t-o num    : maximum number of outputs for each club [default=2] (for greedy)\n");
    fprintf(pErr, "\t-m method : clubbing algorithm to use\n");
    fprintf(pErr, "\t            dominator (default) | greedy \n");
    FREE( pOpt );
    
    return 1;
}

/**Function********************************************************************

  Synopsis    [print the immediate dominators]
  
  CommandName [print_domi]
  
  CommandSynopsis [print the immediate dominators]
  
  CommandArguments []
  
  CommandDescription [Compute print the immediate dominator of requested nodes
  in the network. It uses the algorithm proposed by Lengauer and Tarjan in
  1979.
  ]
  
  SideEffects []

******************************************************************************/
int
Cb_CommandPrintDomi( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int nNodes, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set defaults
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    /* non-fanout nodes may skew the result */
    if ( Cb_NetworkCheckNonFanout( pNet ) )
    {
        fprintf( pErr, "There are non-fanout nodes. Do sweep first.\n" );
        return 1;
    }
    
    // collect the nodes
    nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 1 );
    if ( nNodes == 0 )
    {
        fprintf( pErr, "The list of nodes to print is empty.\n" );
        return 1;
    }

    Cb_NetworkPrintDominators( pOut, pNet, nNodes );
    return 0;

usage:
    fprintf( pErr, "usage: print_domi [-h] <node_list>\n" );
    fprintf( pErr, "\t     print the immediate dominators of nodes in the list\n" );
    fprintf( pErr, "\t <node_list> : if empty, print all nodes in the network\n" );
    fprintf( pErr, "\t-h           : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    [print dominators]
  CommandName []
  CommandSynopsis []
  CommandArguments []
  CommandDescription []
  SideEffects []

******************************************************************************/
int
Cb_CommandPrintDot( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr, * pFileOutput;
    Ntk_Network_t * pNet;
    char * pFileNameOutput;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
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

    // get the file name
    if ( util_optind > (argc-1) )
    {
        fprintf( pErr, "The output file name is not given.\n" );
        goto usage;
    }

    // read the second network
    pFileNameOutput = argv[util_optind];
    pFileOutput = fopen( pFileNameOutput, "w" );
    if ( pFileOutput == NULL ) {
        
        fprintf( pErr, "Cannot open file %s to write.\n", pFileNameOutput );
        goto usage;
    }
    
    Cb_NetworkPrintDot( pFileOutput, pNet );
    
    fclose( pFileOutput );
    return 0;

usage:
    fprintf( pErr, "usage: _write_dot <output_file>\n" );
    fprintf( pErr, "                     write out the netlist as a DOT format\n" );
    fprintf( pErr, "       <output_file> the name of the output file.\n" );
    return 1;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
Cb_Option_t *
Cb_OptionInit( Mv_Frame_t *pMvsis ) 
{
    Cb_Option_t *pOpt;
    
    pOpt = ALLOC( Cb_Option_t, 1 );
    memset( pOpt, 0, sizeof( Cb_Option_t ) );
    
    pOpt->nMaxOut = 2;
    pOpt->nMaxIn  = 20;
    pOpt->nCost   = 200;
    pOpt->iMethod = 1;   /* dominator-based clubbing */
    
    return pOpt;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
Cb_NetworkCheckNonFanout( Ntk_Network_t *pNet ) 
{
    Ntk_Node_t * pNode;
    Ntk_NetworkForEachNode( pNet, pNode ) {
        
        if ( Ntk_NodeReadFanoutNum( pNode ) == 0 &&
             !Ntk_NodeIsCo( pNode ) )
            return TRUE;
        if ( Ntk_NodeReadFaninNum( pNode ) == 0 &&
             !Ntk_NodeIsCi( pNode ) )
            return TRUE;
    }
    return FALSE;
}

