/**CFile****************************************************************

  FileName    [ntki.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]
 
  Synopsis    [Command file of the network package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntki.c,v 1.48 2005/07/08 01:01:22 alanmi Exp $]

***********************************************************************/
 
#include "ntkInt.h"
#include "ntkiInt.h"
#include "sh.h"
#include "wn.h"
#include "ver.h"
#include "mio.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ntk_CommandPrint            ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintIo          ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintLatch       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintStats       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintRange       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintValue       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintFactor      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintLevel       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintReconv      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintNd          ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintSpec        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintDsd         ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintMapStats    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandShow             ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintGml         ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintNpn         ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintSym         ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintSupp        ( Mv_Frame_t * pMvsis, int argc, char ** argv );

static int Ntk_CommandPrintClauses     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandPrintSat         ( Mv_Frame_t * pMvsis, int argc, char ** argv );

static int Ntk_CommandResetName        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandRename           ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandChangeNameMode   ( Mv_Frame_t * pMvsis, int argc, char ** argv );

static int Ntk_CommandNetworkPower     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkNDize     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetwork1Hotize   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkDize      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkReorder   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkCopy      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFree      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkDefault   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkWindow    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkWindowA   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFrames    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkMiter     ( Mv_Frame_t * pMvsis, int argc, char ** argv );

static int Ntk_CommandNetworkSweep     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFraigSweep( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFraig     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkChoice    ( Mv_Frame_t * pMvsis, int argc, char ** argv );

static int Ntk_CommandNetworkEliminate ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkCollapse  ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkCollapse2 ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkMerge     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFxu       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkLxu       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkDecomp    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkDsd       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkStrash    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkBalance   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkResetDef  ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFfResetDef( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkResub     ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkAddBuffers( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkEspresso  ( Mv_Frame_t * pMvsis, int argc, char ** argv );

static int Ntk_CommandNetworkVerify    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFraigVerify ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkAttach    ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkMap       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFpga      ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkFpgaRes   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkMapRes    ( Mv_Frame_t * pMvsis, int argc, char ** argv );

static int Ntk_CommandNetworkFraigRetime( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int Ntk_CommandNetworkRetime    ( Mv_Frame_t * pMvsis, int argc, char ** argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Initializes the I/O package.]

  SideEffects []

  SeeAlso     [Io_End]

******************************************************************************/
void Ntki_Init( Mv_Frame_t * pMvsis )
{
#ifdef BALM
    Cmd_CommandAdd( pMvsis, "Network viewing",  "print_stats",   Ntk_CommandPrintStats,       0 ); 
    Cmd_CommandAdd( pMvsis, "Network viewing",  "print",         Ntk_CommandPrint,            0 ); 
    Cmd_CommandAdd( pMvsis, "Network viewing",  "print_io",      Ntk_CommandPrintIo,          0 );
    Cmd_CommandAdd( pMvsis, "Network viewing",  "print_latch",   Ntk_CommandPrintLatch,       0 );
    Cmd_CommandAdd( pMvsis, "Network viewing",  "print_range",   Ntk_CommandPrintRange,       0 ); 
    Cmd_CommandAdd( pMvsis, "Network viewing",  "print_factor",  Ntk_CommandPrintFactor,      0 ); 
    Cmd_CommandAdd( pMvsis, "Network viewing",  "print_level",   Ntk_CommandPrintLevel,       0 ); 
    Cmd_CommandAdd( pMvsis, "Network viewing",  "print_nd",      Ntk_CommandPrintNd,          0 ); 
#else
    Cmd_CommandAdd( pMvsis, "Printing",  "print_stats",   Ntk_CommandPrintStats,       0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print",         Ntk_CommandPrint,            0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print_io",      Ntk_CommandPrintIo,          0 );
    Cmd_CommandAdd( pMvsis, "Printing",  "print_latch",   Ntk_CommandPrintLatch,       0 );
    Cmd_CommandAdd( pMvsis, "Printing",  "print_range",   Ntk_CommandPrintRange,       0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print_value",   Ntk_CommandPrintValue,       0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print_factor",  Ntk_CommandPrintFactor,      0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print_level",   Ntk_CommandPrintLevel,       0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print_reconv",  Ntk_CommandPrintReconv,      0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print_nd",      Ntk_CommandPrintNd,          0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print_spec",    Ntk_CommandPrintSpec,        0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print_dsd",     Ntk_CommandPrintDsd,         0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "print_map_stats", Ntk_CommandPrintMapStats,  0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "show_network",  Ntk_CommandShow,             0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "write_gml",     Ntk_CommandPrintGml,         0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "npn",           Ntk_CommandPrintNpn,         0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "sym",           Ntk_CommandPrintSym,         0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "supp",          Ntk_CommandPrintSupp,        0 ); 

    Cmd_CommandAdd( pMvsis, "Printing",  "_print_clauses", Ntk_CommandPrintClauses,     0 ); 
    Cmd_CommandAdd( pMvsis, "Printing",  "_print_sat",     Ntk_CommandPrintSat,         0 ); 

    Cmd_CommandAdd( pMvsis, "Naming",    "reset_name",    Ntk_CommandResetName,        0 ); 
    Cmd_CommandAdd( pMvsis, "Naming",    "rename",        Ntk_CommandRename,           0 ); 
    Cmd_CommandAdd( pMvsis, "Naming",    "chng_name",     Ntk_CommandChangeNameMode,   0 ); 

    Cmd_CommandAdd( pMvsis, "Various",   "_power",         Ntk_CommandNetworkPower,     1 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "_ndize",         Ntk_CommandNetworkNDize,     1 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "1hotize",       Ntk_CommandNetwork1Hotize,     1 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "dize",          Ntk_CommandNetworkDize,      1 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "reorder",       Ntk_CommandNetworkReorder,   1 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "_copy",          Ntk_CommandNetworkCopy,      1 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "free",          Ntk_CommandNetworkFree,      1 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "default",       Ntk_CommandNetworkDefault,   1 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "window",        Ntk_CommandNetworkWindow,    0 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "assert",        Ntk_CommandNetworkWindowA,   0 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "frames",        Ntk_CommandNetworkFrames,    1 ); 
    Cmd_CommandAdd( pMvsis, "Various",   "miter",         Ntk_CommandNetworkMiter,     1 ); 

    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "sweep",         Ntk_CommandNetworkSweep,     1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "fraig_sweep",   Ntk_CommandNetworkFraigSweep,1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "fraig",         Ntk_CommandNetworkFraig,     1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "choice",        Ntk_CommandNetworkChoice,    1 ); 

    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "eliminate",     Ntk_CommandNetworkEliminate, 1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "collapse",      Ntk_CommandNetworkCollapse,  1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "collapse2",     Ntk_CommandNetworkCollapse2, 1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "merge",         Ntk_CommandNetworkMerge,     1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "fxu",           Ntk_CommandNetworkFxu,       1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "lxu",           Ntk_CommandNetworkLxu,       1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "decomp",        Ntk_CommandNetworkDecomp,    1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "dsd",           Ntk_CommandNetworkDsd,       1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "strash",        Ntk_CommandNetworkStrash,    1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "balance",       Ntk_CommandNetworkBalance,   1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "reset_default", Ntk_CommandNetworkResetDef,  1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "_ffrsd",         Ntk_CommandNetworkFfResetDef,1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "resub",         Ntk_CommandNetworkResub,     1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "add_buffers",   Ntk_CommandNetworkAddBuffers,1 ); 
    Cmd_CommandAdd( pMvsis, "Combinational synthesis", "espresso",      Ntk_CommandNetworkEspresso,  1 ); 

    Cmd_CommandAdd( pMvsis, "Verification",  "verify",        Ntk_CommandNetworkVerify,         0 ); 
    Cmd_CommandAdd( pMvsis, "Verification",  "fraig_verify",  Ntk_CommandNetworkFraigVerify,    0 ); 

    Cmd_CommandAdd( pMvsis, "Mapping",       "attach",        Ntk_CommandNetworkAttach,         0 ); 
    Cmd_CommandAdd( pMvsis, "Mapping",       "map",           Ntk_CommandNetworkMap,            1 ); 
    Cmd_CommandAdd( pMvsis, "Mapping",       "fpga",          Ntk_CommandNetworkFpga,           1 ); 
    Cmd_CommandAdd( pMvsis, "Mapping",       "resf",          Ntk_CommandNetworkFpgaRes,        1 ); 
    Cmd_CommandAdd( pMvsis, "Mapping",       "resm",          Ntk_CommandNetworkMapRes,         1 ); 

    Cmd_CommandAdd( pMvsis, "Retiming",      "fraig_retime",  Ntk_CommandNetworkFraigRetime,    0 ); 
    Cmd_CommandAdd( pMvsis, "Retiming",      "retime",        Ntk_CommandNetworkRetime,         0 ); 
#endif
}

/**Function********************************************************************

  Synopsis    [Ends the I/O package.]

  SideEffects []

  SeeAlso     [Io_Init]

******************************************************************************/
void Ntki_End( Mv_Frame_t * pMvsis )
{
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintStats( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
    bool fComputeCvr;
    bool fComputeMvr;
    bool fShort;
    int nNodes, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fComputeCvr = 0;
    fComputeMvr = 0;
    fShort      = 1;
    nNodes = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "scmh" ) ) != EOF )
    {
        switch ( c )
        {
        case 's':
            fShort ^= 1;
            break;
        case 'c':
            fComputeCvr ^= 1;
            break;
        case 'm':
            fComputeMvr ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( pNet == NULL )
    {
        fprintf( Mv_FrameReadErr(pMvsis), "Empty network\n" );
        return 1;
    }

    // collect the nodes (if any)
    if ( argc - util_optind > 0 )
        nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
    if ( nNodes == 0 ) 
    {
        Ntk_NetworkPrintStats( pOut, pNet, fComputeCvr, fComputeMvr, fShort );
        return 0;
    }

    // print the nodes
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        Ntk_NodePrintStats( pOut, pNode, fComputeCvr, fComputeMvr );
    return 0;

usage:
    fprintf( pErr, "usage: print_stats [-c] [-m] [-h]\n" );
    fprintf( pErr, "\t           prints the network statistics and\n" );
    fprintf( pErr, "\t           reports the percentage of nodes having each representation\n" );
    fprintf( pErr, "\t-s       : output statistics in a shortened form [default = %s]\n", fShort? "yes": "no" );
    fprintf( pErr, "\t-c       : derives covers (MV SOPs) if not present [default = %s]\n", fComputeCvr? "yes": "no" );
    fprintf( pErr, "\t-m       : derives local MV relations if not present [default = %s]\n", fComputeMvr? "yes": "no" );
    fprintf( pErr, "\t-h       : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrint( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
    int nNodes;
    int fPrintDefault;
    int fPrintPositional;
    int fPrintKmap;
    int c; 

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fPrintDefault    = 0;
    fPrintPositional = 0;
    fPrintKmap       = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "dpmh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'd':
            fPrintDefault ^= 1;
            break;
        case 'p':
            fPrintPositional ^= 1;
            break;
        case 'm':
            fPrintKmap ^= 1;
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

    // collect the nodes
    nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
    if ( nNodes == 0 )
    {
        fprintf( Mv_FrameReadErr(pMvsis), "The list of nodes to print is empty.\n" );
        return 1;
    }

    // print the nodes
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( fPrintKmap )
            Ntk_NodePrintMvr( pOut, pNode );
        else
            Ntk_NodePrintCvr( pOut, pNode, fPrintDefault, fPrintPositional );
    return 0;

usage:
    fprintf( pErr, "usage: print [-d] [-p] [-m] <node_list>\n" );
    fprintf( pErr, "\t               print MV SOP representation of the nodes.\n" );
    fprintf( pErr, "\t-d           : print default i-set [default = %s]\n", fPrintDefault? "yes": "no" );
    fprintf( pErr, "\t-p           : print cubes in positional notation [default = %s]\n", fPrintPositional? "yes": "no" );
    fprintf( pErr, "\t-m           : print Karnaugh maps for the relation [default = %s]\n", fPrintKmap? "yes": "no" );
    fprintf( pErr, "\t<node_list>  : if empty, print all nodes in the network\n" );
    fprintf( pErr, "\t-h           : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintIo( Mv_Frame_t * pMvsis, int argc, char ** argv )
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

    // collect the nodes
    if ( argc == util_optind )
        nNodes = 0;
    else
        nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 1 );

    // print the nodes
    Ntk_NetworkPrintIo( pOut, pNet, nNodes );
    return 0;

usage:
    fprintf( pErr, "usage: print_io [-h] <node_list>\n" );
    fprintf( pErr, "\t               prints the fanins/fanouts of nodes in the list\n" );
    fprintf( pErr, "\t<node_list>  : if empty, print CIs/COs of the network\n" );
    fprintf( pErr, "\t-h           : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintLatch( Mv_Frame_t * pMvsis, int argc, char ** argv )
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

    // collect the nodes
    if ( argc == util_optind )
        nNodes = 0;
    else
        nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 1 );

    // print the nodes
    Ntk_NetworkPrintLatch( pOut, pNet, nNodes );
    return 0;

usage:
    fprintf( pErr, "usage: print_latch [-h]\n" );
    fprintf( pErr, "\t               prints the list of latches of the network\n" );
    fprintf( pErr, "\t-h           : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintRange( Mv_Frame_t * pMvsis, int argc, char **argv )
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

    // collect the nodes
    nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 1 );
    if ( nNodes == 0 )
    {
        fprintf( pErr, "The list of nodes to print is empty.\n" );
        return 1;
    }

    // print the nodes
    Ntk_NetworkPrintRange( pOut, pNet, nNodes );
    return 0;

usage:
    fprintf( pErr, "usage: print_range [-h] <node_list>\n" );
    fprintf( pErr, "\t               prints the number of values of nodes in the list\n" );
    fprintf( pErr, "\t <node_list> : if empty, prints values for all nodes in the network\n" );
    fprintf( pErr, "\t-h           : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintValue( Mv_Frame_t * pMvsis, int argc, char **argv )
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

    // collect the nodes
    nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
    if ( nNodes == 0 )
    {
        fprintf( pErr, "The list of nodes to print is empty.\n" );
        return 1;
    }

    // print the nodes
    Ntk_NetworkPrintValue( pOut, pNet, nNodes );
    return 0;

usage:
    fprintf( pErr, "usage: print_value [-h] <node_list>\n" );
    fprintf( pErr, "\t               prints the elimination value of nodes in the list\n" );
    fprintf( pErr, "\t <node_list> : if empty, prints values for all nodes in the network\n" );
    fprintf( pErr, "\t-h           : print the command usage\n");
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintFactor( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
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

    // collect the nodes
    nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
    if ( nNodes == 0 )
    {
        fprintf( pErr, "The list of nodes to print is empty.\n" );
        return 1;
    }

    // print the nodes
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        Ntk_NodePrintFactor( pOut, pNode );
    return 0;

usage:
    fprintf( pErr, "usage: print_factor [-h] <node_list>\n" );
    fprintf( pErr, "\t     print algebraic factored form of the nodes.\n" );
    fprintf( pErr, "\t <node_list> : if empty, print all nodes in the network\n" );
    fprintf( pErr, "\t-h           : print the command usage\n");
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintLevel( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fFromOutputs;
    int nNodes, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
   
    // set defaults
    fFromOutputs = 1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "oh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'o':
                fFromOutputs ^= 1;
                break;
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

    // collect the nodes
    if ( argc == util_optind )
        nNodes = 0;
    else
        nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
    
    // print nodes by level
    Ntk_NetworkPrintLevel( pOut, pNet, nNodes, fFromOutputs );
    return 0;

usage:
    fprintf( pErr, "usage: print_level [-o] [-h] <node_list>\n" );
    fprintf( pErr, "\t     prints nodes in the network by level.\n" );
    fprintf( pErr, "\t-o           : toggles DFS order used to levelize [default = %s]\n", fFromOutputs? "from COs": "from CIs");
    fprintf( pErr, "\t-h           : print the command usage\n");
    fprintf( pErr, "\t<node_list> : if empty, print all nodes in the network\n" );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Prints the recovergent nodes in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintReconv( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fFanout;
    int fVerbose;
    int nNodes, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
   
    // set defaults
    fFanout = 1;
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "dvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'd':
                fFanout ^= 1;
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    // collect the nodes
    nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
    
    // print nodes by level
    Ntk_NetworkPrintReconv( pOut, pNet, nNodes, fFanout, fVerbose );
    return 0;

usage:
    fprintf( pErr, "usage: print_reconv [-d] [-h] <node_list>\n" );
    fprintf( pErr, "\t     prints the reconvergence related to the nodes.\n" );
    fprintf( pErr, "\t-d          : toggles fanin/fanout reconvergence [default = %s]\n", fFanout? "fanout": "fanin");
    fprintf( pErr, "\t-v          : enable verbose output [default = %s].\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h          : print the command usage\n");
    fprintf( pErr, "\t<node_list> : if empty, print all nodes in the network\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintNd( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
   
    // set defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'h':
                goto usage;
            case 'v':
                fVerbose ^= 1;
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
    
    // print nodes by level
    Ntk_NetworkPrintNd( pOut, pNet, fVerbose );
    return 0;

usage:
    fprintf( pErr, "usage: print_nd [-vh]\n" );
    fprintf( pErr, "\t     prints the list of ND nodes of the current network\n" );
    fprintf( pErr, "\t-v          : enable verbose output [default = %s].\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h          : print the command usage\n");
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintSpec( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int c;

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
    
    fprintf( pOut, "External spec file name is \"%s\".\n", pNet->pSpec );
    return 0;

usage:
    fprintf( pErr, "usage: print_spec [-h]\n" );
    fprintf( pErr, "\t        prints the external spec file name for the current network\n" );
    fprintf( pErr, "\t-h    : print the command usage\n");
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintDsd( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int Output   = -1;
    int fShort   = 0;
    int fVerbose = 0;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
   
    // set defaults
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "o:sv" ) ) != EOF )
    {
        switch ( c )
        {
            case 'o':
                Output = atoi(util_optarg);
                break;
            case 's':
                fShort ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
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
    
    // print nodes by level
    Ntk_NetworkPrintDsd( pNet, Output, fShort );
    return 0;

usage:
    fprintf( pErr, "usage: print_dsd [-o num] [-s]\n" );
    fprintf( pErr, "       prints the DSD structure to the standard output\n" );
    fprintf( pErr, "\t-o num : selects one primary output to print [default = all]\n" );
    fprintf( pErr, "\t-s     : toggles the use of short/long names [default = %s]\n", (fShort? "short": "long") );
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintMapStats( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int c;

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
    
    Ntk_NetworkMapStats( pNet );
    return 0;

usage:
    fprintf( pErr, "usage: print_map_stats [-h]\n" );
    fprintf( pErr, "\t        prints the area and delay parameters of the mapping\n" );
    fprintf( pErr, "\t-h    : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandShow( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t ** ppNodes = NULL;
    int nNodes;
    Ntk_Node_t * pNode;
    int fShort   = 0;
    int fVerbose = 0;
    int fOldMode, c;
    int i;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
   
    // set defaults
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "sv" ) ) != EOF )
    {
        switch ( c )
        {
            case 's':
                fShort ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
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

    // collect the nodes
    nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
    if (nNodes > 0) {
      ppNodes = (Ntk_Node_t **) malloc(nNodes * sizeof(Ntk_Node_t*));
      i=0;
      Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        ppNodes[i++] = pNode;
    }
    
    // set the mode to short (0 stands for long; 1 stands for short)
    fOldMode = Mv_FrameReadMode( pMvsis );
    if ( fOldMode != fShort )
        Mv_FrameSetMode( pMvsis, !fOldMode );
    Ntk_NetworkShow( pNet, ppNodes, nNodes, 1 );
    if ( fOldMode != fShort )
        Mv_FrameSetMode( pMvsis, !fOldMode );
    free(ppNodes);
    return 0;

usage:
    fprintf( pErr, "usage: show_network [-s] <node_list>\n" );
    fprintf( pErr, "       shows the network topology using DOT software\n" );
    fprintf( pErr, "\t<node> : shows only the TFI cone rooted at the node [default = the whole network]\n" );
    fprintf( pErr, "\t-s     : toggles the use of short/long names [default = %s]\n", (fShort? "short": "long") );
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintGml( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
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
    if ( util_optind >= argc )
    {
        fprintf( pErr, "The second file name is not given.\n" );
        goto usage;
    }

    // read the file name
    pFileNameOutput = argv[util_optind];
    Ntk_NetworkPrintGml( pNet, NULL, 0, pFileNameOutput ); 
    return 0;

usage:
    fprintf( pErr, "usage: write_gml <output_file>\n" );
    fprintf( pErr, "       writes the network as a directed graph in GML format\n" );
    fprintf( pErr, "       <output_file>  the name of the output file.\n" );
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintNpn( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c;
//    extern void Ntk_NetworkNpn( Ntk_Network_t * pNet, int fVerbose );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    // read the file name
//    Ntk_NetworkNpn( pNet, fVerbose ); 
    return 0;

usage:
    fprintf( pErr, "usage: npn [-h]\n" );
    fprintf( pErr, "\t         prints stats about NPN equivalences of POs\n" );
    fprintf( pErr, "\t-v     : enable verbose output [default = %s].\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h     : prints the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintSym( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fUseBdds;
    int fNaive;
    int fVerbose;
    int nRoundLimit;
    int c;
    extern void Ntk_NetworkComputeSymmetries( Ntk_Network_t * pNet, int fUseBdds, int fNaive, int fVerbose, int nRoundLimit );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fUseBdds = 0;
    fNaive   = 0;
    fVerbose = 0;
    nRoundLimit = 10000;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "sbnvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 's':
            if ( util_optind >= argc )
            {
                fprintf( pErr, "Command line switch \"-s\" should be followed by an integer.\n" );
                goto usage;
            }
            nRoundLimit = atoi(argv[util_optind]);
            util_optind++;
            if ( nRoundLimit < 0 ) 
                goto usage;
            break;
        case 'b':
            fUseBdds ^= 1;
            break;
        case 'n':
            fNaive ^= 1;
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

    // read the file name
    Ntk_NetworkComputeSymmetries( pNet, fUseBdds, fNaive, fVerbose, nRoundLimit ); 
    return 0;

usage:
    fprintf( pErr, "usage: sym [-s num] [-bnvh]\n" );
    fprintf( pErr, "\t         computes symmetries of the PO functions [default = use S&S]\n" );
    fprintf( pErr, "\t-s num : the number of simulation rounds in S&S [default = %d].\n", nRoundLimit );  
    fprintf( pErr, "\t-n     : enable naive BDD-based computation [default = %s].\n", fNaive? "yes": "no" );  
    fprintf( pErr, "\t-b     : enable efficient BDD-based computation [default = %s].\n", fUseBdds? "yes": "no" );  
    fprintf( pErr, "\t-v     : enable verbose output [default = %s].\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h     : prints the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintSupp( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fExact;
    int fVerbose;
    int nSimLimit;
    int c;
    extern void Ntk_NetworkComputeSupports( Ntk_Network_t * pNet, int nSimLimit, int fExact, int fVerbose );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fExact = 0;
    fVerbose = 0;
    nSimLimit = 10;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "sevh" ) ) != EOF )
    {
        switch ( c )
        {
        case 's':
            if ( util_optind >= argc )
            {
                fprintf( pErr, "Command line switch \"-s\" should be followed by an integer.\n" );
                goto usage;
            }
            nSimLimit = atoi(argv[util_optind]);
            util_optind++;
            if ( nSimLimit < 0 ) 
                goto usage;
            break;
        case 'e':
            fExact ^= 1;
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

    // read the file name
    Ntk_NetworkComputeSupports( pNet, nSimLimit, fExact, fVerbose ); 
    return 0;

usage:
    fprintf( pErr, "usage: supp [-s num] [-evh]\n" );
    fprintf( pErr, "\t         computes functional support of the PO functions\n" );
    fprintf( pErr, "\t-s num : the number of simulation rounds [default = %d].\n", nSimLimit );  
    fprintf( pErr, "\t-e     : enable the exact computation using SAT [default = simulation].\n" );  
    fprintf( pErr, "\t-v     : enable verbose output [default = %s].\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h     : prints the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintClauses( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    char * pFileName;
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
    if ( util_optind < argc )
        pFileName = argv[util_optind];
    else
        pFileName = Extra_FileNameAppend(pNet->pName, ".cnf");

    // write the clauses
    Ntk_NetworkWriteClauses( pNet, pFileName ); 
    return 0;

usage:
    fprintf( pErr, "usage: _print_clauses <file_name>\n" );
    fprintf( pErr, "       generates MV SAT clauses for the network\n" );
    fprintf( pErr, "       <file_name>  (optional) the name of the output file.\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandPrintSat( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Network_t * pNet2;
    char * pFileNameInput;
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
    if ( util_optind >= argc )
    {
        fprintf( pErr, "The second file name is not given.\n" );
        goto usage;
    }

    // read the second network
    pFileNameInput  = argv[util_optind];
    pNet2 = Io_ReadNetwork( pMvsis, pFileNameInput );
   
    // get the output file name
    if ( util_optind+1 < argc )
        pFileNameOutput = argv[util_optind+1];
    else
    {
        char Buffer[100];
        sprintf( Buffer, "%s_%s", pNet->pName, pNet2->pName );
        pFileNameOutput = Extra_FileNameAppend(Buffer, ".mvsat");
    }

    Ntk_NetworkWriteSat( pNet, pNet2, pFileNameOutput ); 
    Ntk_NetworkDelete( pNet2 );
    return 0;

usage:
    fprintf( pErr, "usage: _print_sat <input_file> <output_file>\n" );
    fprintf( pErr, "       generates MV SAT clauses to express the non-equivalence\n" );
    fprintf( pErr, "       of the current network and the network found in <input_file>\n" );
    fprintf( pErr, "       (currently, this command works only for single-output networks)\n" );
    fprintf( pErr, "       <output_file>  (optional) the name of the output file.\n" );
    return 1;
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandResetName( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int nNodesOld, nNodesNew, c;

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

    // print nodes by level
    nNodesOld = pNet->nIds;
    nNodesNew = Ntk_NetworkCompactNodeIds( pNet );
    fprintf( pOut, "The number of node IDs is reduced from %d to %d.\n", nNodesOld, nNodesNew );
    return 0;

usage:
    fprintf( pErr, "usage: reset_name [-h]\n" );
    fprintf( pErr, "\t     compacts node IDs resulting in shorter short names\n" );
    fprintf( pErr, "\t-h           : print the command usage\n");
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandRename( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int c;

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

    // print nodes by level
    Ntk_NetworkRenameNodes( pNet );
    fprintf( pOut, "The names of all nodes have been changed.\n" );
    return 0;

usage:
    fprintf( pErr, "usage: rename [-h]\n" );
    fprintf( pErr, "\t     renames all nodes in the network\n" );
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Ntk_CommandChangeNameMode( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pOut, * pErr;

    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    if ( argc != 1 )
    {
        fprintf( pErr, "usage: chname\n" );
        fprintf( pErr, "       toggles between the short and long name modes\n" );
        return 1;
    }

    if ( Mv_FrameReadMode(pMvsis) )
    {
        Mv_FrameSetMode( pMvsis, 0 );
        fprintf( pOut, "Changing to long name mode.\n");
    }
    else 
    {
        Mv_FrameSetMode( pMvsis, 1 );
        fprintf( pOut, "Changing to short name mode.\n");
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkPower( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int Degree, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    Degree = 2;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "nh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'n':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-n\" should be followed by an integer.\n" );
                    goto usage;
                }
                Degree = atoi(argv[util_optind]);
                util_optind++;
                if ( Degree < 0 ) 
                    goto usage;
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

    // write the clauses
    Ntk_NetworkComputePower( pNet, Degree ); 
    return 0;

usage:
    fprintf( pErr, "usage: _power [-n num]\n" );
    fprintf( pErr, "\t           generates the \"power\" of the given network\n" );
    fprintf( pErr, "\t-n num   : the number of levels [default = %d].\n", Degree );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkNDize( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    double Prob;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    Prob = 0.1;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ph" ) ) != EOF )
    {
        switch ( c )
        {
        case 'p':
            Prob = atof(argv[util_optind]);
            util_optind++;
            if ( Prob < 0 ) 
                goto usage;
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

    // write the clauses
    Ntk_NetworkNonDeterminize( pNet, Prob ); 
    return 0;

usage:
    fprintf( pErr, "usage: _ndize [-p num] [-h]\n" );
    fprintf( pErr, "\t          non-determinizes the network by adding ND buffers\n" );
    fprintf( pErr, "\t-p num  : a floating point number (0 <= num <= 1) [default = %.2f].\n", Prob );
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetwork1Hotize( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
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

    // write the clauses
    Ntk_NetworkEncodeLatches( pNet ); 
    return 0;

usage:
    fprintf( pErr, "usage: 1hotize [-h]\n" );
    fprintf( pErr, "\t     encodes the latches in positional notation\n" );
    fprintf( pErr, "\t-h     : prints the command usage\n");
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkDize( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
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

    // write the clauses
    Ntk_NetworkForEachNode( pNet, pNode )
        Ntk_NodeDeterminize( pNode );

//    Ntk_NetworkEncode( pNet );
    return 0;

usage:
    fprintf( pErr, "usage: dize [-h]\n" );
    fprintf( pErr, "\t     determinizes ND nodes in the network\n" );
    fprintf( pErr, "\t-h     : prints the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkReorder( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int c, fVerbose = 0;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    // reorder the local function of each node
    Ntk_NetworkReorder( pNet, fVerbose );
    return 0;

usage:
    fprintf( pErr, "usage: reorder [-v]\n" );
    fprintf( pErr, "\t         reorders BDDs of the local MV relations of the nodes\n" );
    fprintf( pErr, "\t-v     : enable printing out statistics [default = %s].\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h     : prints the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkCopy( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
    {
        switch (c) 
        {
            case 'v':
                fVerbose ^= 1;
                break;
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

    // replace the current network without modifying the stack of saved networks
    Mv_FrameReplaceCurrentNetwork( pMvsis, Ntk_NetworkDup(pNet, pNet->pMan) );
    return 0;

usage:
    fprintf( pErr, "usage: copy [-h]\n");
    fprintf( pErr, "\t         replaces the current network by its copy\n" );  
    fprintf( pErr, "\t         does not modify the stack of the saved networks\n" );  
    fprintf( pErr, "\t-h     : prints the command usage\n");
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkFree( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
    {
        switch (c) 
        {
            case 'v':
                fVerbose ^= 1;
                break;
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

    // get the value
    if ( util_optind < argc )
    {
        Ntk_Node_t * pNode;
        if ( strcmp( argv[util_optind], "cvr" ) == 0 )
        {
            Ntk_NetworkForEachNode( pNet, pNode )
            {
                Ntk_NodeGetFuncMvr( pNode );
                Ntk_NodeFreeFuncCvr( pNode );
            }
            return 0;
        }
        else if ( strcmp( argv[util_optind], "mvr" ) == 0 )
        {
            Ntk_NetworkForEachNode( pNet, pNode )
            {
                Ntk_NodeGetFuncCvr( pNode );
                Ntk_NodeFreeFuncMvr( pNode );
            }
            return 0;
        }
    }

    fprintf( pOut, "The object to free is not specified\n" );  
    return 0;

usage:
    fprintf( pErr, "usage: free <object> [-h]\n");
    fprintf( pErr, "\t           frees MV SOPs (\"cvr\") or MV relations (\"mvr\") in all nodes\n" );  
    fprintf( pErr, "\t<object> : the representation to free (\"cvr\" or \"mvr\")\n" );
    fprintf( pErr, "\t-h       : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkDefault( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    bool fVerbose;
    bool fUseDefault;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    fUseDefault = 1;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "dhv")) != EOF ) 
    {
        switch (c) 
        {
            case 'd':
                fUseDefault ^= 1;
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

    // make sure the default is always present whenever possible
    if ( fUseDefault )
        Ntk_NetworkForceDefault( pNet );
    else
        Ntk_NetworkComputeDefault( pNet );
    return 0;

usage:
    fprintf( pErr, "usage: default [-d] [-h]\n");
    fprintf( pErr, "\t         adds/removes the default covers in determistic nodes\n" );  
    fprintf( pErr, "\t-d     : toggles adding/removing the default value [default = %s]\n", fUseDefault? "remove": "add" );
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkWindow( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    assert(0);

#if 0

    char pFileName[100];
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
    int nNodes;
    int nTfiLevels;
    int nTfoLevels;
    int nWndNodes;
    int fVerbose;
    int fPrintAll;
    int Window, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
 
    // set the defaults
    nTfiLevels = 2;
    nTfoLevels = 2;
    nWndNodes  = -1;
    fVerbose   = 0;
    fPrintAll  = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "wnahv")) != EOF ) 
    {
        switch (c) 
        {
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
                nTfiLevels = Window / 10;
                nTfoLevels = Window % 10;
                break;
            case 'n':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-n\" should be followed by an integer.\n" );
                    goto usage;
                }
                nWndNodes = atoi(argv[util_optind]);
                util_optind++;
                if ( nWndNodes < 0 ) 
                    goto usage;
                break;
            case 'a':
                fPrintAll ^= 1;
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

    if ( fPrintAll )
    {
        int nLeaves, nRoots, nNodes;
        int clk;

        extern int clock1;
        extern int clock2;
        extern int clock3;
        extern int clock4;
        extern int clock5;

        clock1 = 0;
        clock2 = 0;
        clock3 = 0;
        clock4 = 0;
        clock5 = 0;

        clk = clock();
        nLeaves = 0;
        nRoots = 0;
        nNodes = 0;
        Ntk_NetworkForEachNode( pNet, pNode )
        {
            Wn_Window_t * pWnd;            
            if ( nWndNodes > 0 )
                pWnd = Wn_WindowComputeForNode( pNode, nWndNodes );
            else
                pWnd = Wn_WindowDeriveForNode( pNode, nTfiLevels, nTfoLevels );
/*
            // print the stats
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "%5s  fi = %3d  fo = %3d :  lvs = %5d  rts = %5d  nds = %5d\n", 
                   Ntk_NodeGetNamePrintable(pNode), 
                   Ntk_NodeReadFaninNum(pNode),
                   Ntk_NodeReadFanoutNum(pNode),
                   Wn_WindowReadLeafNum(pWnd),
                   Wn_WindowReadRootNum(pWnd),
                   Wn_WindowReadNodeNum(pWnd) );
*/
            nLeaves += Wn_WindowReadLeafNum(pWnd);
            nRoots  += Wn_WindowReadRootNum(pWnd);
            nNodes  += Wn_WindowReadNodeNum(pWnd);

            // delete the window
            Wn_WindowDelete( pWnd );
        }
        printf( "Leaves = %8d.    Roots = %8d.    Nodes = %8d.\n", nLeaves, nRoots, nNodes );
        PRT( "Total time", clock() - clk );
/*
        PRT( "Time1", clock1 );
        PRT( "Time2", clock2 );
        PRT( "Time3", clock3 );
        PRT( "Time4", clock4 );
        PRT( "Time5", clock5 );
*/
        return 0;
    }

    if ( util_optind == argc )
    {
        fprintf( pErr, "The list of nodes for extracting is not given.\n" );
        return 1;
    }

    // collect the nodes
    nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
    if ( nNodes == 0 )
    {
        fprintf( pErr, "The list of nodes to be extracted is empty.\n" );
        return 1;
    }
    pNode = pNet->pOrder;

    // the nodes to be merged are linked into the special linked list
    if ( nTfiLevels > 0 || nTfoLevels > 0 || nWndNodes > 0 )
    {
        Wn_Window_t * pWnd;            
        if ( nWndNodes > 0 )
            pWnd = Wn_WindowComputeForNode( pNode, nWndNodes );
        else
        {
            pWnd = Wn_WindowDeriveForNode( pNode, nTfiLevels, nTfoLevels );
            // collect the internal nodes of the container window
            Wn_WindowCollectInternal( pWnd );
        }

        // print the stats
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "%5s  fi = %3d  fo = %3d :  lvs = %5d  rts = %5d  nds = %5d\n", 
               Ntk_NodeGetNamePrintable(pNode), 
               Ntk_NodeReadFaninNum(pNode),
               Ntk_NodeReadFanoutNum(pNode),
               Wn_WindowReadLeafNum(pWnd),
               Wn_WindowReadRootNum(pWnd),
               Wn_WindowReadNodeNum(pWnd) );

        // put the nodes into the list
        Ntk_NetworkCreateSpecialFromArray( pNet, Wn_WindowReadNodes(pWnd), Wn_WindowReadNodeNum(pWnd) );
        // delete the window
        Wn_WindowDelete( pWnd );
    }
    // create the name for the new file
    if ( Ntk_NetworkIsBinary(pNet) )
    {
        sprintf( pFileName, "%s_%d_%s.blif", pNet->pName, nNodes, 
            Ntk_NodeGetNamePrintable(pNode) );
        Ntk_SubnetworkWriteIntoFile( pNet, pFileName, 1 );
    }
    else
    {
        sprintf( pFileName, "%s_%d_%s.mv", pNet->pName, nNodes, 
            Ntk_NodeGetNamePrintable(pNode) );
        Ntk_SubnetworkWriteIntoFile( pNet, pFileName, 0 );
    }
    return 0;


usage:
    fprintf( pErr, "usage: window [-i num] [-o num] [-a] [-h] <node_list>\n");
    fprintf( pErr, "\t         carves out a window surrounding nodes in the list\n" );  
    fprintf( pErr, "\t-w NM  : the number of TFI (N) and TFO (M) logic levels [default = %d%d]\n", nTfiLevels, nTfoLevels );
    fprintf( pErr, "\t-a     : print statistics about all windows [default = %s]\n", fPrintAll? "yes": "no" );
    fprintf( pErr, "\t-h     : print the command usage\n");
#endif
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkWindowA( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    assert (0);

#if 0

//    char pFileName[100];
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
//    int nNodes;
    int nTfiLevels;
    int nTfoLevels;
    int nWndNodes;
    int fVerbose;
    int Window, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
 
    // set the defaults
    nTfiLevels =  2;
    nTfoLevels =  2;
    nWndNodes  = -1;
    fVerbose   =  0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "wh")) != EOF ) 
    {
        switch (c) 
        {
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
                nTfiLevels = Window / 10;
                nTfoLevels = Window % 10;
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


    if ( 1 )
    {
        Ntk_Node_t * pNode2;
        st_table * tNames;
        int TotalWins = 0;
        int TotalWinsA = 0;
        int TotalAsserts = 0;

        Ntk_Node_t ** ppLeaves;
        int nLeaves;
        int i;

        Ntk_NetworkForEachNode( pNet, pNode )
        {
            Wn_Window_t * pWnd;            
            int Counter;
            int fAssertionIsUseful;

            if ( nWndNodes > 0 )
                pWnd = Wn_WindowComputeForNode( pNode, nWndNodes );
            else
                pWnd = Wn_WindowDeriveForNode( pNode, nTfiLevels, nTfoLevels );

            // collect all the node names into a table
            tNames = st_init_table(strcmp, st_strhash);
            Ntk_NetworkForEachNodeSpecial( pNet, pNode2 )
                if ( pNode2->pName )
                    st_insert( tNames, pNode2->pName, NULL );

            ppLeaves = Wn_WindowReadLeaves( pWnd );
            nLeaves = Wn_WindowReadLeafNum( pWnd );
            for ( i = 0; i < nLeaves; i++ )
                if ( ppLeaves[i]->pName )
                    st_insert( tNames, ppLeaves[i]->pName, NULL );


            // go through assertions and count the number of those used in this window
            Counter = 0;
            Ntk_NetworkForEachNode2( pNet, pNode2 )
            {
                Ntk_Node_t * pFanin;
                Ntk_Pin_t * pPin;

                fAssertionIsUseful = 1;
                Ntk_NodeForEachFanin( pNode2, pPin, pFanin )
                {
                    char * pName = pFanin->pName;
                    if ( pName == NULL )
                    {
                         Ntk_Node_t * pFanoutCo;
                         pFanoutCo = Ntk_NodeHasCoName(pFanin);
                         pName = pFanoutCo->pName;
                    }

                    if ( pName )
                        if ( !st_is_member( tNames, pName ) )
                        {
                            fAssertionIsUseful = 0;
                            break;
                        }
                }
                Counter += fAssertionIsUseful;
            }
            printf( "Window for node %5s contains %3d nodes and uses %2d assertions.\n", 
                Ntk_NodeGetNamePrintable(pNode), st_count(tNames), Counter );

            st_free_table( tNames );
            // delete the window
            Wn_WindowDelete( pWnd );
            TotalWins++;
            TotalWinsA += (int)(Counter > 0);
            TotalAsserts += Counter;
        }

        printf( "\n" );
        printf( "The network has %d assertions.\n", pNet->lNodes2.nItems );
        printf( "The number of times an assertion was used in a window is %d.\n", TotalAsserts );
        printf( "\n" );
        printf( "This test has computed %d windows (for each internal node).\n", TotalWins );
        printf( "The number of windows containing at least one assertion is %d.\n", TotalWinsA );

//        printf( "Leaves = %8d.    Roots = %8d.    Nodes = %8d.\n", nLeaves, nRoots, nNodes );
        return 0;
    }

    return 0;

usage:
    fprintf( pErr, "usage: assert [-w NM] [-h]\n");
    fprintf( pErr, "\t         computes statistics related to assertions and windowing\n" );  
    fprintf( pErr, "\t-w NM  : the number of TFI (N) and TFO (M) logic levels [default = %d%d]\n", nTfiLevels, nTfoLevels );
    fprintf( pErr, "\t-h     : print the command usage\n");
#endif
    return 1;       /* error exit */
    
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkFrames( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    int nFrames;
    int fVerbose;
    int fMakeComb;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
 
    // set the defaults
    fVerbose  = 0;
    nFrames   = 5;
    fMakeComb = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "ichv")) != EOF ) 
    {
        switch (c) 
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
            case 'c':
                fMakeComb ^= 1;
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

    if ( Ntk_NetworkReadLatchNum(pNet) == 0 )
    {
        fprintf( pErr, "Cannot unroll combinational network.\n" );
        return 1;
    }

    // unroll the network given number of times
    if ( nFrames == 0 )
        pNetNew = Ntk_NetworkDup( pNet, Mv_FrameReadMan(pNet->pMvsis) );
    else
        pNetNew = Ntk_NetworkDeriveTimeFrames( pNet, nFrames );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Unrolling the network has failed!\n" );
        return 1;
    }

    // make the network combinational if requested
    if ( fMakeComb )
        Ntk_NetworkMakeCombinational( pNetNew );

    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    fprintf( pErr, "usage: frames [-i num] [-c] [-h]\n");
    fprintf( pErr, "\t         unrolls sequential network for a number of time frames\n" );  
    fprintf( pErr, "\t-i num : the number of time frames to unroll [default = %d]\n", nFrames );
    fprintf( pErr, "\t-c     : makes combinational by propagating initial state [default = %s]\n", fMakeComb? "yes": "no" );
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkMiter( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNet2, * pNetNew;
    int fVerbose;
    int c;
    extern Ntk_Network_t * Ntk_NetworkDeriveMiter( Ntk_Network_t * pNet, Ntk_Network_t * pNet2 );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
 
    // set the defaults
    fVerbose  = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }
    if ( Ntk_NetworkReadCoNum(pNet) == Ntk_NetworkReadLatchNum(pNet) )
    {
        fprintf( pErr, "The network does not have primary outputs.\n" );
        return 1;
    }

    if ( argc != util_optind + 1 ) 
    {
        fprintf( pErr, "The second network file should be specified on the command line.\n" );
        return 1;
    }

    // read the second network using the same functionality manager
    pNet2 = Io_ReadNetwork( pMvsis, argv[util_optind] );
    if ( pNet2 == NULL )
    {
        fprintf( pErr, "Reading the second network has failed.\n" );
        return 1;
    }

    // construct the miter
    pNetNew = Ntk_NetworkDeriveMiter( pNet, pNet2 );
    // delete the second network
    Ntk_NetworkDelete( pNet2 );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Constructing the miter has failed!\n" );
        return 1;
    }

    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    fprintf( pErr, "usage: miter [-h] <file>\n");
    fprintf( pErr, "\t         derives the miter of the POs of the networks\n" );  
    fprintf( pErr, "\t         if the networks are sequential, ignores latch inputs\n" );  
    fprintf( pErr, "\t<file> : the file with the second network of the miter\n");
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkSweep( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fSweepConsts;
    int fSweepBuffers;
    int fSweepIsets;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fSweepConsts  = 1;
    fSweepBuffers = 1;
    fSweepIsets   = 0;
    fVerbose      = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "cbivh")) != EOF ) 
    {
        switch (c) 
        {
            case 'c':
                fSweepConsts ^= 1;
                break;
            case 'b':
                fSweepBuffers ^= 1;
                break;
            case 'i':
                fSweepIsets ^= 1;
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

    Ntk_NetworkSweep( pNet, fSweepConsts, fSweepBuffers, fSweepIsets, fVerbose );
    return 0;

usage:
    fprintf( pErr, "usage: sweep [-b] [-i] [-v] [-h]\n");
    fprintf( pErr, "\t         simplifies the network by removing useless nodes\n");
    fprintf( pErr, "\t-c     : toggle removing the constant nodes [default = %s]\n", fSweepConsts? "yes": "no" );  
    fprintf( pErr, "\t-b     : toggle removing the single-input nodes [default = %s]\n", fSweepBuffers? "yes": "no" );  
    fprintf( pErr, "\t-i     : toggle sweeping i-sets of the internal nodes [default = %s]\n", fSweepIsets? "yes": "no" );  
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkFraigSweep( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c;
    int fMergeInvertedNodes;
    int fFuncRed;
    int fFeedBack;
    int fDoSparse;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fFuncRed  = 1;
    fFeedBack = 1;
    fDoSparse = 0;
    fVerbose  = 0;
    fMergeInvertedNodes = 0;

    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "rfsvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'r':
                fFuncRed ^= 1;
                break;
            case 'f':
                fFeedBack ^= 1;
                break;
            case 's':
                fDoSparse ^= 1;
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

    if ( Ntk_NetworkIsMapped( pNet ) )
    {
        Ntk_NetworkSweep( pNet, 0, 0, 0, 0 );
        Net_NetworkEquiv( pNet, fFuncRed, fFeedBack, fDoSparse, fVerbose );
        Ntk_NetworkSweep( pNet, 0, 0, 0, 0 );
    }
    else
    {
        Ntk_NetworkSweep( pNet, 1, 1, 0, 0 );
        Net_NetworkEquiv( pNet, fFuncRed, fFeedBack, fDoSparse, fVerbose );
        Ntk_NetworkSweep( pNet, 1, 1, 0, 0 );
    }
    return 0;

usage:
    fprintf( pErr, "usage: fraig_sweep [-rfsvh] \n");
    fprintf( pErr, "\t         sweeps functionally equivalent nodes using FRAIGs\n");
    fprintf( pErr, "\t-r     : toggle functional reduction [default = %s]\n", fFuncRed? "yes": "no" );  
    fprintf( pErr, "\t-f     : toggle the use of solver feedback [default = %s]\n", fFeedBack? "yes": "no" );  
    fprintf( pErr, "\t-s     : toggle doing the test for sparse functions [default = %s]\n", fDoSparse? "yes": "no" );  
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkFraig( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    int fVerbose;
    int fMulti;
    int fFuncRed;
    int fDoSparse;
    int fAreaDup;
    int fWriteAnds;
    int fTryProve;
    int fBalance;
    int c;

    extern Ntk_Network_t * Ntk_NetworkDeriveFraig( Mv_Frame_t * pMvsis, Ntk_Network_t * pNet, int fMulti, int fAreaDup, int fWriteAnds, int fFuncRed, int fBalance, int fDoSparse, int fTryProve, int fVerbose );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fMulti    = 0;
    fAreaDup  = 0;
    fWriteAnds= 0;
    fFuncRed  = 1;
    fBalance  = 1;
    fDoSparse = 0;
    fTryProve = 0;
    fVerbose  = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "amdrbspvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'a':
                fWriteAnds ^= 1;
                break;
            case 'm':
                fMulti ^= 1;
                break;
            case 'd':
                fAreaDup ^= 1;
                break;
            case 'r':
                fFuncRed ^= 1;
                break;
            case 'b':
                fBalance ^= 1;
                break;
            case 's':
                fDoSparse ^= 1;
                break;
            case 'p':
                fTryProve ^= 1;
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

    pNetNew =  Ntk_NetworkDeriveFraig( pMvsis, pNet, fMulti, fAreaDup, fWriteAnds, fFuncRed, fBalance, fDoSparse, fTryProve, fVerbose );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Network fraiging has failed!\n" );
        return 1;
    }
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    fprintf( pErr, "usage: fraig [-mdrbsapvh]\n");
    fprintf( pErr, "\t          transforms the current network into AND-network by FRAIGing\n");
    fprintf( pErr, "\t-m      : toggle AND2/ANDN gates [default = %s]\n", fMulti? "multi-input": "two-input" );  
    fprintf( pErr, "\t-d      : toggle node duplication when using ANDN gates [default = %s]\n", fAreaDup? "yes": "no" );  
    fprintf( pErr, "\t-r      : toggle functional reduction [default = %s]\n", fFuncRed? "yes": "no" );  
    fprintf( pErr, "\t-b      : toggles AIG balancing [default = %s]\n", fBalance? "yes": "no" );  
    fprintf( pErr, "\t-s      : toggle doing the test for sparse functions [default = %s]\n", fDoSparse? "yes": "no" );  
    fprintf( pErr, "\t-a      : toggle AND2s/INVs and AND2s w/compl. inputs [default = %s]\n", fWriteAnds? "AND2s/INVs" : "AND2s" );  
    fprintf( pErr, "\t-p      : tries to prove the final miter [default = %s]\n", fTryProve? "yes": "no" ); 
    fprintf( pErr, "\t-v      : print verbose information [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkChoice( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    int fVerbose;
    int c;
    int fFuncRed;
    int fDoSparse;
    int fPickOne;
    int fBalance;

    extern Ntk_Network_t * Ntk_NetworkChoice( Mv_Frame_t * pMvsis, 
        Ntk_Network_t * pNet, char * pNames[], int nNames, 
        int fFuncRed, int fBalance, int fDoSparse, int fPickOne, int fVerbose );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fFuncRed  = 1;
    fDoSparse = 1;
    fBalance  = 1;
    fVerbose  = 0;
    fPickOne  = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "prbsvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'p':
                fPickOne ^= 1;
                break;
            case 'r':
                fFuncRed ^= 1;
                break;
            case 'b':
                fBalance ^= 1;
                break;
            case 's':
                fDoSparse ^= 1;
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

    pNetNew =  Ntk_NetworkChoice( pMvsis, pNet, argv + util_optind, argc - util_optind, 
        fFuncRed, fBalance, fDoSparse, fPickOne, fVerbose );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Network choicing has failed!\n" );
        return 1;
    }
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    fprintf( pErr, "usage: choice [-rbspvh] [<net1.blif> <net2.blif> ... <netK.blif>]\n");
    fprintf( pErr, "\t         creates choice graph from the variants of a network\n");
    fprintf( pErr, "\t[files] : optional list of functionally equivalent networks to choice [default = current]\n" );  
    fprintf( pErr, "\t-r      : toggle functional reduction [default = %s]\n", fFuncRed? "yes": "no" );  
    fprintf( pErr, "\t-b      : toggles AIG balancing [default = %s]\n", fBalance? "yes": "no" );  
    fprintf( pErr, "\t-s      : toggle doing the test for sparse functions [default = %s]\n", fDoSparse? "yes": "no" );  
    fprintf( pErr, "\t-p      : toggle picking one best choice [default = %s]\n", fPickOne? "yes": "no" );  
    fprintf( pErr, "\t-v      : print verbose information [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkEliminate( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    bool fVerbose;
    bool fUseCostBdd;
    int Value, nNodesElim, c;
    int nCubeLimit;
    int nFaninLimit;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    nNodesElim  = 100000;
    nCubeLimit  = 500;
    nFaninLimit = 15;
    Value       = -1;
    fUseCostBdd = 0;
    fVerbose    = 0;

    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "cflbvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'c':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-c\" should be followed by an integer.\n" );
                    goto usage;
                }
                nCubeLimit = atoi(argv[util_optind]);
                util_optind++;
                if ( nCubeLimit < 0 ) 
                    goto usage;
                break;
            case 'f':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-f\" should be followed by an integer.\n" );
                    goto usage;
                }
                nFaninLimit = atoi(argv[util_optind]);
                util_optind++;
                if ( nFaninLimit < 0 ) 
                    goto usage;
                break;
            case 'l':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-l\" should be followed by an integer.\n" );
                    goto usage;
                }
                nNodesElim = atoi(argv[util_optind]);
                util_optind++;
                if ( nNodesElim < 0 ) 
                    goto usage;
                break;
            case 'b':
                fUseCostBdd ^= 1;
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

    if ( util_optind == argc )
    {
        // fprintf( pErr, "Elimination value is not given; %d is assumed.\n", Value );
    }
    else if ( util_optind == argc - 1 )
    {
        Value = atoi(argv[util_optind]);
        if ( Value < -1 )
        {
            fprintf( pErr, "Elimination value is incorrect (%d).\n", Value );
            goto usage;
        }
    }
    else
        goto usage;

    // call the elimination procedure
    Ntk_NetworkEliminate( pNet, nNodesElim, nCubeLimit, nFaninLimit, -Value, fUseCostBdd, fVerbose );
    return 0;

usage:
    fprintf( pErr, "usage: eliminate [-c num] [-f num] [-l num] [-b] [-v] [-h] <value>\n" );
    fprintf( pErr, "\t         selectively collapses the nodes into their fanouts\n" );  
    fprintf( pErr, "\t-c num : the cube limit (if more, skip the node) [default = %d]\n", nCubeLimit );
    fprintf( pErr, "\t-f num : the fanin limit (if more, skip the node) [default = %d]\n", nFaninLimit );
    fprintf( pErr, "\t-l num : the maximum number of nodes to eliminate [default = %d]\n", nNodesElim );
    fprintf( pErr, "\t-b     : use BDDs nodes instead of FF literals as a cost [default = %s]\n", fUseCostBdd? "yes": "no" );  
    fprintf( pErr, "\t-v     : prints verbose information about elimination [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h     : print the command usage\n");
    fprintf( pErr, "\t<value>: the elimination value [default = %d]\n", Value );
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkCollapse( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    Ntk_Node_t * pNode1, * pNode2;
    int c;
    int fVerbose;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
    {
        switch (c) 
        {
            case 'h':
                goto usage;
                break;
            case 'v':
                fVerbose ^= 1;
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

    if ( argc == util_optind + 2 ) 
    {
        pNode1  = Ntk_NetworkCollectNodeByName( pNet, argv[util_optind],   0 );
        pNode2  = Ntk_NetworkCollectNodeByName( pNet, argv[util_optind+1], 0 );
        if ( !pNode1 || !pNode2 )
            return 1;

        if ( Ntk_NodeReadFaninIndex( pNode1, pNode2 ) != -1 )
            Ntk_NetworkCollapseNodes( pNode1, pNode2 );
        else if ( Ntk_NodeReadFaninIndex( pNode2, pNode1 ) != -1 )
            Ntk_NetworkCollapseNodes( pNode2, pNode1 );
        else
        {
            fprintf( pErr, "Nodes \"%s\" and \"%s\" are not fanin/fanout of each other.\n", 
                Ntk_NodeGetNamePrintable(pNode1), Ntk_NodeGetNamePrintable(pNode2) );
            return 1;
        }
    }
    else if ( argc == util_optind ) 
    {
        // single level networks cannot be collapsed any further
        if ( Ntk_NetworkGetNumLevels(pNet) < 2 )
        {
/*
            // experiment with the new minimization procedure
            Mvc_Data_t * pData;
            Vm_VarMap_t * pVm;
            Mvc_Cover_t * pMvc;
            Cvr_Cover_t * pCvr;
            Ntk_Node_t * pNode;

            pNode = Ntk_NetworkReadFirstCo( pNet );
            if ( pNode->Type != MV_NODE_CO )
            {
                pVm = Ntk_NodeReadFuncVm(pNode);
                pCvr = Ntk_NodeReadFuncCvr(pNode);
                pMvc = Cvr_CoverReadIsets(pCvr)[1];
                pData = Mvc_CoverDataAlloc( pVm, pMvc );
                Mvc_CoverMinimizeByReshape( pData, pMvc );
                Mvc_CoverDataFree( pData, pMvc );
            }
*/
            return 0;
        }

        // perform collapsing of the network
        pNetNew = Ntk_NetworkCollapse( pNet, fVerbose );
        if ( pNetNew == NULL )
        {
            fprintf( pErr, "Network collapse has failed!\n" );
            return 1;
        }
        // replace the current network
        Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    }
    else if ( argc == util_optind + 1 ) 
    {

        pNode1  = Ntk_NetworkCollectNodeByName( pNet, argv[util_optind],   0 );
        if ( !pNode1  )
            return 1;
        Ntk_NetworkCollapseNode( pNet, pNode1 );
/*
 //       Ntk_NetworkEncodeNode( pNode1, 2, 3 );
        {
            Mvr_Relation_t * pRel;
            int Test;
            int * pValues;
            pRel = Ntk_NodeReadFuncMvr(pNode1);
            Test = Mvr_RelationMinimumValueSet( pRel, Mvr_RelationReadRel(pRel), &pValues );
            Test = 1;
        }
*/
    }
    else
    {
        fprintf( pErr, "Incorrect number of command line arguments.\n" );
        return 1;
    }

    return 0;

usage:
    fprintf( pErr, "usage: collapse [-v] [-h] <node1> <node2>\n");
    fprintf( pErr, "\t          collapses the nodes or the network (default)\n" );  
    fprintf( pErr, "\t<node1> : the node to collapse into its fanouts\n");
    fprintf( pErr, "\t<node2> : if given, the node to collapse into <node1>\n");
    fprintf( pErr, "\t-v      : toggles verbose output [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkCollapse2( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    Ntk_Node_t * pNode1, * pNode2;
    int c;
    int fVerbose;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
    {
        switch (c) 
        {
            case 'h':
                goto usage;
                break;
            case 'v':
                fVerbose ^= 1;
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

    if ( argc == util_optind + 2 ) 
    {
        pNode1  = Ntk_NetworkCollectNodeByName( pNet, argv[util_optind],   0 );
        pNode2  = Ntk_NetworkCollectNodeByName( pNet, argv[util_optind+1], 0 );
        if ( !pNode1 || !pNode2 )
            return 1;

        if ( Ntk_NodeReadFaninIndex( pNode1, pNode2 ) != -1 )
            Ntk_NetworkCollapseNodes( pNode1, pNode2 );
        else if ( Ntk_NodeReadFaninIndex( pNode2, pNode1 ) != -1 )
            Ntk_NetworkCollapseNodes( pNode2, pNode1 );
        else
        {
            fprintf( pErr, "Nodes \"%s\" and \"%s\" are not fanin/fanout of each other.\n", 
                Ntk_NodeGetNamePrintable(pNode1), Ntk_NodeGetNamePrintable(pNode2) );
            return 1;
        }
    }
    else if ( argc == util_optind ) 
    {
        // single level networks cannot be collapsed any further
        if ( Ntk_NetworkGetNumLevels(pNet) < 2 )
        {
/*
            // experiment with the new minimization procedure
            Mvc_Data_t * pData;
            Vm_VarMap_t * pVm;
            Mvc_Cover_t * pMvc;
            Cvr_Cover_t * pCvr;
            Ntk_Node_t * pNode;

            pNode = Ntk_NetworkReadFirstCo( pNet );
            if ( pNode->Type != MV_NODE_CO )
            {
                pVm = Ntk_NodeReadFuncVm(pNode);
                pCvr = Ntk_NodeReadFuncCvr(pNode);
                pMvc = Cvr_CoverReadIsets(pCvr)[1];
                pData = Mvc_CoverDataAlloc( pVm, pMvc );
                Mvc_CoverMinimizeByReshape( pData, pMvc );
                Mvc_CoverDataFree( pData, pMvc );
            }
*/
            return 0;
        }

        // perform collapsing of the network
        pNetNew = Ntk_NetworkCollapseNew( pNet, fVerbose );
        if ( pNetNew == NULL )
        {
            fprintf( pErr, "Network collapse has failed!\n" );
            return 1;
        }
        // replace the current network
        Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    }
    else if ( argc == util_optind + 1 ) 
    {

        pNode1  = Ntk_NetworkCollectNodeByName( pNet, argv[util_optind],   0 );
        if ( !pNode1  )
            return 1;
//        Ntk_NetworkCollapseNode( pNet, pNode1 );

 //       Ntk_NetworkEncodeNode( pNode1, 2, 3 );

        {
            Mvr_Relation_t * pRel;
            int Test;
            int * pValues;
            
            pRel = Ntk_NodeReadFuncMvr(pNode1);

            Test = Mvr_RelationMinimumValueSet( pRel, Mvr_RelationReadRel(pRel), &pValues );

            Test = 1;


        }
    }
    else
    {
        fprintf( pErr, "Incorrect number of command line arguments.\n" );
        return 1;
    }

    return 0;

usage:
    fprintf( pErr, "usage: collapse2 [-v] [-h] <node1> <node2>\n");
    fprintf( pErr, "\t          collapses the nodes or the network (default)\n" );  
    fprintf( pErr, "\t<node1> : the node to collapse into its fanouts\n");
    fprintf( pErr, "\t<node2> : if given, the node to collapse into <node1>\n");
    fprintf( pErr, "\t-v      : toggles verbose output [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h      : print the command usage\n");
    return 1;       /* error exit */
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkMerge( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int nNodes;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    if ( util_optind == argc )
    {
        fprintf( pErr, "The list of nodes for merging is not given.\n" );
        return 1;
    }

    // collect the nodes
    nNodes = Cmd_CommandGetNodes( pMvsis, argc-util_optind+1, argv+util_optind-1, 0 );
    if ( nNodes < 2 )
    {
        fprintf( pErr, "The list of nodes contains less then two internal nodes.\n" );
        return 1;
    }

    // the nodes to be merged are linked into the special linked list
    Ntk_NetworkMerge( pNet, nNodes );
    return 0;

usage:
    fprintf( pErr, "usage: merge <node_list>\n");
    fprintf( pErr, "\t         merges all the nodes in the list\n" );  
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkFxu( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    Fxu_Data_t * p = NULL;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    if ( pNet && Ntk_NetworkReadNodeIntNum(pNet) == 0 )
        return 0;


    // allocate the structure
    p = ALLOC( Fxu_Data_t, 1 );
    memset( p, 0, sizeof(Fxu_Data_t) );
    // set the defaults
    p->nPairsMax = 30000;
    p->nNodesExt = 10000;
    p->fOnlyS    = 0;
    p->fOnlyD    = 0;
    p->fUse0     = 0;
    p->fUseCompl = 1;
    p->fVerbose  = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "lnsdzcvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'l':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-l\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nPairsMax = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nPairsMax < 0 ) 
                    goto usage;
                break;
            case 'n':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-n\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nNodesExt = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nNodesExt < 0 ) 
                    goto usage;
                break;
            case 's':
                p->fOnlyS ^= 1;
                break;
            case 'd':
                p->fOnlyD ^= 1;
                break;
            case 'z':
                p->fUse0 ^= 1;
                break;
            case 'c':
                p->fUseCompl ^= 1;
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
        Ntk_NetworkFxFreeInfo( p );
        return 1;
    }

    // the nodes to be merged are linked into the special linked list
    Ntk_NetworkFastExtract( pNet, p );
    Ntk_NetworkFxFreeInfo( p );
    return 0;

usage:
    fprintf( pErr, "usage: fxu [-n num] [-l num] [-s] [-d] [-z] [-c] [-v] [-h]\n");
    fprintf( pErr, "\t         performs unate fast extract on the current network\n");
    fprintf( pErr, "\t-n num : the maximum number of divisors to extract [default = %d]\n", p->nNodesExt );  
    fprintf( pErr, "\t-l num : the maximum number of cube pairs to consider [default = %d]\n", p->nPairsMax );  
    fprintf( pErr, "\t-s     : use only single-cube divisors [default = %s]\n", p->fOnlyS? "yes": "no" );  
    fprintf( pErr, "\t-d     : use only double-cube divisors [default = %s]\n", p->fOnlyD? "yes": "no" );  
    fprintf( pErr, "\t-z     : use zero-weight divisors [default = %s]\n", p->fUse0? "yes": "no" );  
    fprintf( pErr, "\t-c     : use complement in the binary case [default = %s]\n", p->fUseCompl? "yes": "no" );  
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", p->fVerbose? "yes": "no" ); 
    fprintf( pErr, "\t-h     : print the command usage\n");
    Ntk_NetworkFxFreeInfo( p );
    return 1;       
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkLxu( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Ntk_Network_t * pNet;
    FILE * pOut, * pErr;
    Lxu_Data_t * p = NULL;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    if ( pNet && Ntk_NetworkReadNodeIntNum(pNet) == 0 )
        return 0;


    // allocate the structure
    p = ALLOC( Lxu_Data_t, 1 );
    memset( p, 0, sizeof(Lxu_Data_t) );
    // set the defaults
    p->nPairsMax = 30000;
    p->nNodesExt = 10000;
    p->fOnlyS    = 0;
    p->fOnlyD    = 0;
    p->fUse0     = 0;
    p->fUseCompl = 1;
    p->fVerbose  = 0;

    p->alpha     = 1;
    p->beta      = 0;
    p->interval  = -1;
    p->plot      = 0;

    p->extPlacerCmd = "ext-placer";
    
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "pabeilnsdzcvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'p':
                p->plot ^= 1;
                break;

            case 'a':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-a\" should be followed by an float.\n" );
                    goto usage;
                }
                p->alpha = atof(argv[util_optind]);
                util_optind++;
                break;

            case 'b':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-b\" should be followed by an float.\n" );
                    goto usage;
                }
                p->beta = atof(argv[util_optind]);
                util_optind++;
                break;

            case 'i':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-i\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->interval = atoi(argv[util_optind]);
                util_optind++;
                break;

            case 'l':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-l\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nPairsMax = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nPairsMax < 0 ) 
                    goto usage;
                break;
            case 'n':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-n\" should be followed by an integer.\n" );
                    goto usage;
                }
                p->nNodesExt = atoi(argv[util_optind]);
                util_optind++;
                if ( p->nNodesExt < 0 ) 
                    goto usage;
                break;
            case 's':
                p->fOnlyS ^= 1;
                break;
            case 'd':
                p->fOnlyD ^= 1;
                break;
            case 'z':
                p->fUse0 ^= 1;
                break;
            case 'c':
                p->fUseCompl ^= 1;
                break;
case 'e':
    if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-e\" should be followed by a string.\n" );
                    goto usage;
                }
        p->extPlacerCmd = util_strsav(argv[util_optind]);
        util_optind++;
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
        Ntk_NetworkLxFreeInfo( p );
        return 1;
    }

    // the nodes to be merged are linked into the special linked list
    Ntk_NetworkLayoutFastExtract( pNet, p );
    Ntk_NetworkLxFreeInfo( p );
    return 0;

usage:
    fprintf( pErr, "usage: lxu [-n num] [-l num] [-s] [-d] [-z] [-c] [-v] [-h]\n");
    fprintf( pErr, "\t         performs layout-aware unate fast extract on the current network\n");
    fprintf( pErr, "\t-n num : the maximum number of divisors to extract [default = %d]\n", p->nNodesExt );  
    fprintf( pErr, "\t-l num : the maximum number of cube pairs to consider [default = %d]\n", p->nPairsMax );  
    fprintf( pErr, "\t-s     : use only single-cube divisors [default = %s]\n", p->fOnlyS? "yes": "no" );  
    fprintf( pErr, "\t-d     : use only double-cube divisors [default = %s]\n", p->fOnlyD? "yes": "no" );  
    fprintf( pErr, "\t-z     : use zero-weight divisors [default = %s]\n", p->fUse0? "yes": "no" );  
    fprintf( pErr, "\t-c     : use complement in the binary case [default = %s]\n", p->fUseCompl? "yes": "no" );  
    fprintf( pErr, "\t-a num : the relative weight of the logical weight (i.e. value) [default = %g]\n", p->alpha );
    fprintf( pErr, "\t-b num : the relative weight of the physical weight (i.e. wirevalue) [default = %g]\n", p->beta );
    fprintf( pErr, "\t-i num : interval between calls to the external placer [default = %d]\n", p->interval );
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", p->fVerbose? "yes": "no" ); 
    fprintf( pErr, "\t-p     : plot intermediate point placements [default = %d]\n", p->plot );
    fprintf( pErr, "\t-e     : command name for the external placer [default = %s]\n", p->extPlacerCmd );
    fprintf( pErr, "\t-h     : print the command usage\n");
    Ntk_NetworkLxFreeInfo( p );
    return 1;       
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkDecomp( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    Ntk_NetworkDecomp( pNet );
    return 0;

usage:
    fprintf( pErr, "usage: decomp [-h]\n");
    fprintf( pErr, "\t         decomposes the network using the factored forms\n" );  
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkDsd( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    int fVerbose, fUseNand, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose  = 0;
    fUseNand  = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "nvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'n':
                fUseNand ^= 1;
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

    // check if the network is binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    { 
        // what if the network is binary but not completely specified???
        fprintf( pOut, "The current network is not binary. Decomposition is not performed.\n" );
        return 0;
    }

    // create the structurally hashed network
    pNetNew = Ntk_NetworkDsd( pNet, fUseNand, fVerbose );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Disjoint support decomposition has failed!\n" );
        return 1;
    }

    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    fprintf( pErr, "usage: dsd [-nvh]\n" );
    fprintf( pErr, "\t     collapses the network and decomposes it using\n" );
    fprintf( pErr, "\t     disjoint-support decomposition (binary networks only)\n" );
    fprintf( pErr, "\t-n     : use NAND gates instead of OR gates [default = %s]\n", fUseNand? "yes": "no" );  
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", fVerbose? "yes": "no" ); 
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkStrash( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
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

    // create the structurally hashed network
    pNetNew = Ntk_NetworkStrash( pNet );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Network strashing has failed!\n" );
        return 1;
    }

    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    fprintf( pErr, "usage: strash\n" );
    fprintf( pErr, "\t     replaces the current network by SS-equivalent\n" );
    fprintf( pErr, "\t     structurally hashed network of AND/INV gates\n" );
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_CommandNetworkBalance( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    extern Mio_Library_t * s_pLib;
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    int c;
    double dAndGateDelay;

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

    if ( s_pLib == NULL )
    {
        printf( "A genlib library is not given. Technology independent delay model\n" );
        printf( "will be used to balance AIGs.\n" );
        dAndGateDelay = 1.0;
    }
    else
        dAndGateDelay = Mio_LibraryReadDelayNand2Max( s_pLib );

    // create the structurally hashed network
    pNetNew = Ntk_NetworkBalance( pNet, dAndGateDelay );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Network balancing has failed!\n" );
        return 1;
    }

    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    fprintf( pErr, "usage: balance [-h]\n" );
    fprintf( pErr, "\t     replaces the current network by a well-balanced netlist of AND2s\n" );
    fprintf( pErr, "\t     the delay of AND2 is estimated using NAND2 from the current library\n" );
    fprintf( pErr, "\t-h        : print the command usage\n");
    return 1;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkResetDef( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
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
        Ntk_NodeResetDefault( pNode, Ntk_NetworkGetAcceptType(pNet) );
        return 0;
    }

    Ntk_NetworkResetDefault( pNet );
    return 0;

usage:
    fprintf( pErr, "usage: reset_default [-h] <node>\n");
    fprintf( pErr, "\t         assigns the default values by minimizing MV relations\n" );  
    fprintf( pErr, "\t-h     : print the command usage\n");
    fprintf( pErr, "\t<node> : the name of one node to reset default\n");
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkFfResetDef( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
    int fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
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
        Ntk_NodeFfResetDefault( pNode, 0, fVerbose );
        return 0;
    }


    Ntk_NetworkFfResetDefault( pNet, fVerbose );
    return 0;

usage:
    fprintf( pErr, "usage: _ffrsd [-h] <node>\n");
    fprintf( pErr, "\t         phase-assigns binary covers using FF complement\n" );  
    fprintf( pErr, "\t-h     : print the command usage\n");
    fprintf( pErr, "\t<node> : the name of one node to reset default\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkResub( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode; 
    int nCands;
    bool fVerbose;
    int nFaninMax;
   
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    nCands   = 8;
    nFaninMax = 20;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "fcvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'f':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-f\" should be followed by an integer.\n" );
                    goto usage;
                }
                nFaninMax = atoi(argv[util_optind]);
                util_optind++;
                if ( nFaninMax < 0 ) 
                    goto usage;
                break;
            case 'c':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-c\" should be followed by an integer.\n" );
                    goto usage;
                }
                nCands = atoi(argv[util_optind]);
                util_optind++;
                if ( nCands < 0 ) 
                    goto usage;
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

    // get the node name
    if ( util_optind < argc )
    {
        pNode = Ntk_NetworkCollectNodeByName( pNet, argv[util_optind], 0 );
        if ( pNode == NULL )
            goto usage;
        Ntk_NetworkResubNode( pNode, Ntk_NodeGetFuncMvr(pNode), Ntk_NetworkGetAcceptType(pNet), nCands, fVerbose );
        return 0;
    }

    Ntk_NetworkResub( pNet, nCands, nFaninMax, fVerbose );
    return 0;

usage:
    fprintf( pErr, "usage: resub [-c num] [-f num] [-v] [-h] <name>\n");
    fprintf( pErr, "\t         performs boolean resubstitution on the network\n" );  
    fprintf( pErr, "\t-c num : the max number of resubstitution candidates [default = %d]\n", nCands ); 
    fprintf( pErr, "\t-f num : the fanin limit (if more, skip the node) [default = %d]\n", nFaninMax );
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", fVerbose? "yes": "no" ); 
    fprintf( pErr, "\t-h     : print the command usage\n");
    fprintf( pErr, "\t<name> : the name of one node to resubstitute into\n");
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkAddBuffers( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode, * pDriver; 
    Ntk_Node_t * pNodeBuf;
    bool fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "fcvh")) != EOF ) 
    {
        switch (c) 
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

    Ntk_NetworkForEachCi( pNet, pNode )
    {
        // create the node
        pNodeBuf = Ntk_NodeCreateOneInputBinary( pNet, pNode, 0 );
        // move the fanout
        Ntk_NodeTransferFanout( pNode, pNodeBuf );
        // add the node
        Ntk_NetworkAddNode( pNet, pNodeBuf, 1 );
    }
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        // create the node
        pNodeBuf = Ntk_NodeCreateOneInputBinary( pNet, pDriver, 0 );
        // move the fanout
        Ntk_NodeTransferFanout( pDriver, pNodeBuf );
        // add the node
        Ntk_NetworkAddNode( pNet, pNodeBuf, 1 );
    }
    Ntk_NetworkOrderFanins( pNet );
    assert( Ntk_NetworkCheck( pNet ) );
    return 0;

usage:
    fprintf( pErr, "usage: add_buffers [-v] [-h]\n");
    fprintf( pErr, "\t         adds buffers after each CI and before each CO\n" );  
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", fVerbose? "yes": "no" ); 
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkEspresso( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode;
    Mvc_Cover_t ** pIsets;
    Cvr_Cover_t * pCvr;
    Vm_VarMap_t * pVm;
    bool fVerbose;
    int i, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "fcvh")) != EOF ) 
    {
        switch (c) 
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

    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pCvr = Ntk_NodeReadFuncCvr( pNode );
        if ( pCvr == NULL )
            continue;
        Cvr_CoverFreeFactor( pCvr );
        pVm = Ntk_NodeReadFuncVm( pNode );
        pIsets = Cvr_CoverReadIsets( pCvr );
        for ( i = 0; i < pNode->nValues; i++ )
            if ( pIsets[i] )
            {
                pIsets[i] = Cvr_IsetEspresso( pVm, pIsets[i], NULL, 0, 0, 0 );
            }
    }
    return 0;

usage:
    fprintf( pErr, "usage: espresso [-v] [-h]\n");
    fprintf( pErr, "\t         minimizes each node's covers with Espresso\n" );  
    fprintf( pErr, "\t-v     : print verbose information [default = %s]\n", fVerbose? "yes": "no" ); 
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkVerify( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Ntk_Network_t * pNet, * pNet1, * pNet2;
    FILE * pOut, * pErr, * pFile;
    int fVerbose, c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "vh")) != EOF) 
    {
        switch(c) 
        {
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
        Ntk_NetworkVerify( pNet1, pNet2, fVerbose );
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
        Ntk_NetworkVerify( pNet1, pNet2, fVerbose );
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
        Ntk_NetworkVerify( pNet1, pNet2, fVerbose );
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
    fprintf( pErr, "usage: verify [-v] [-h] <file1> <file2>\n");
    fprintf( pErr, "\t          verifies the containment of two networks\n" );  
    fprintf( pErr, "\t-v      : print debug information [default = %s]\n", fVerbose? "yes": "no" );  
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
Ntk_CommandNetworkFraigVerify( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    Ntk_Network_t * pNet, * pNet1, * pNet2;
    FILE * pOut, * pErr, * pFile;
    int fVerbose, c;
    int fFuncRed;
    int fFeedBack;
    int fDoSparse;

    extern void Ntk_NetworkVerifySat( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, 
        int fFuncRed, int fFeedBack, int fDoSparse, int fVerbose );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fFuncRed  = 1;
    fFeedBack = 1;
    fDoSparse = 0;
    fVerbose  = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "rfsvh")) != EOF) 
    {
        switch(c) 
        {
            case 'r':
                fFuncRed ^= 1;
                break;
            case 'f':
                fFeedBack ^= 1;
                break;
            case 's':
                fDoSparse ^= 1;
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
        Ntk_NetworkVerifySat( pNet1, pNet2, fFuncRed, fFeedBack, fDoSparse, fVerbose );
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
        Ntk_NetworkVerifySat( pNet1, pNet2, fFuncRed, fFeedBack, fDoSparse, fVerbose );
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
        Ntk_NetworkVerifySat( pNet1, pNet2, fFuncRed, fFeedBack, fDoSparse, fVerbose );
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
    fprintf( pErr, "usage: fraig_verify [-rfsvh] <file1> <file2>\n");
    fprintf( pErr, "\t          verifies the equivalence of two networks using FRAIGs\n" );  
    fprintf( pErr, "\t-r      : toggle functional reduction [default = %s]\n", fFuncRed? "yes": "no" );  
    fprintf( pErr, "\t-f      : toggle the use of solver feedback [default = %s]\n", fFeedBack? "yes": "no" );  
    fprintf( pErr, "\t-s      : toggle doing the test for sparse functions [default = %s]\n", fDoSparse? "yes": "no" );  
    fprintf( pErr, "\t-v      : print debug information [default = %s]\n", fVerbose? "yes": "no" );  
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
Ntk_CommandNetworkAttach( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    bool fVerbose;
    int c;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "hv")) != EOF ) 
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    // make sure the default is always present whenever possible
    Ntk_NetworkAttach( pNet );
    return 0;

usage:
    fprintf( pErr, "usage: attach [-h]\n");
    fprintf( pErr, "\t         attaches the gates from the current library to the nodes\n" );  
    fprintf( pErr, "\t-h     : print the command usage\n");
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkMap( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    char Buffer[100];
    char * FileNameOut = 0;
    int fWrite;
    int fVerbose;
    int fTrust;
    int fSweep;
    int fEnableRefactoring;
    int nIterations;
    int fFuncRed;
    int fChoicing;
    int fAreaRecovery;
    int fEnableADT;
    float DelayTarget;
    int fUseSops;
    int c;
    extern Ntk_Network_t * Ntk_NetworkMap( Ntk_Network_t * pNet, char * pFileNameOut, int fTrust, int fFuncRed, 
        int fChoicing, int fAreaRecovery, bool fVerbose, bool fSweep, bool fEnableRefactoring, 
        int nIterations, bool fEnableADT, float DelayTarget, int fUseSops );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fTrust            = 0;
    fWrite            = 0;
    fSweep            = 1;
    fEnableRefactoring= 0;
    fFuncRed          = 0;
    fChoicing         = 0;
    fAreaRecovery     = 1;
    nIterations       = 1;
    fEnableADT        = 0;
    DelayTarget       =-1;
    fUseSops          = 0;
    fVerbose          = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "rctaswfdilxvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'r':
                fFuncRed ^= 1;
                break;
            case 'c':
                fChoicing ^= 1;
                break;
            case 't':
                fTrust ^= 1;
                break;
            case 'a':
                fAreaRecovery ^= 1;
                break;
            case 'w':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-w\" should be followed by the output file name.\n" );
                    goto usage;
                }
                FileNameOut = argv[util_optind];
                util_optind++;
                break;
            case 's':
                fSweep ^= 1;
                break;
            case 'f':
                fUseSops ^= 1;
                break;
            case 'l':
                fEnableADT ^= 1;
                break;
            case 'x':
                fEnableRefactoring ^= 1;
                break;
            case 'i':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-i\" should be followed by an integer.\n" );
                    goto usage;
                }
                nIterations = atoi(argv[util_optind]);
                util_optind++;
                if ( nIterations < 0 ) 
                    goto usage;
                break;
			case 'd':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-d\" should be followed by a floating point number.\n" );
                    goto usage;
                }
				DelayTarget = (float)atof(argv[util_optind]);
				util_optind++;
				if ( DelayTarget <= 0.0 ) 
					goto usage;
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

    if ( argc != util_optind )
    {
        fprintf( pErr, "Incorrect number of command line arguments.\n" );
        return 1;
    }

    pNetNew = Ntk_NetworkMap( pNet, FileNameOut, fTrust, fFuncRed, fChoicing, fAreaRecovery, 
        fVerbose, fSweep, fEnableRefactoring, nIterations, fEnableADT, DelayTarget, fUseSops );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Network mapping has failed!\n" );
        return 1;
    }
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    if ( DelayTarget == -1 ) 
        sprintf( Buffer, "not used" );
    else
        sprintf( Buffer, "%.3f", DelayTarget );
    fprintf( pErr, "usage: map [-rctafslxv] [-i num] [-w outfile] [-d float] [-h]\n");
    fprintf( pErr, "\t          performs technology mapping using a standard cell library\n" );  
    fprintf( pErr, "\t          (use \"read_super\" to read the supergate library before mapping)\n" );  
    fprintf( pErr, "\t-r          : toggles functional reduction [default = %s]\n", fFuncRed? "yes": "no" );  
    fprintf( pErr, "\t-c          : toggles choice nodes [default = %s]\n", fChoicing? "yes": "no" );  
    fprintf( pErr, "\t-t          : toggles the trust mode when the input is an AIG [default = %s]\n", fTrust? "yes": "no" );  
    fprintf( pErr, "\t-a          : toggles area recovery [default = %s]\n", fAreaRecovery? "yes": "no" );  
    fprintf( pErr, "\t-f          : toggles SOPs/FFs to derive AIGs [default = %s]\n", fUseSops? "SOPs": "factored forms" );    
    fprintf( pErr, "\t-s          : toggles sweeping after mapping for area-recovery [default = %s]\n", fSweep? "yes": "no" );  
    fprintf( pErr, "\t-l          : toggles algebraic-type choices [default = %s]\n", fEnableADT? "yes": "no" );  
    fprintf( pErr, "\t-x          : toggles refactoring choices [default = %s]\n", fEnableRefactoring? "yes": "no" );  
    fprintf( pErr, "\t-v          : toggles verbose output [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-i num      : the number of area/fanout recovery iterations [default = %d]\n", nIterations );  
    fprintf( pErr, "\t-w outfile  : writes network to outfile as a mapped BLIF\n");  
    fprintf( pErr, "\t-d          : sets the required time for the mapping [default = %s]\n", Buffer );  
    fprintf( pErr, "\t-h          : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkFpga( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    char * FileNameOut = 0;
    int fTrust;
    int fPower;
    int fVerbose;
    int fFuncRed;
    int fChoicing;
    int fAreaRecovery;
    int fResynthesis;
    int fSequential;
    int c;
    float DelayLimit, AreaLimit, TimeLimit;
    extern Ntk_Network_t * Ntk_NetworkFpga( Ntk_Network_t * pNet, 
        bool fTrust, int fFuncRed, int fChoicing, int fPower, int fAreaRecovery, bool fVerbose, 
        int fResynthesis, float DelayLimit, float AreaLimit, float TimeLimit, int fGlobalCones, int fSequential );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fTrust   = 0;
    fPower   = 0;
    fVerbose = 0;
    fFuncRed = 0;
    fChoicing = 0;
    fAreaRecovery = 1;
    fResynthesis = 0;
    DelayLimit = 0;
    AreaLimit = 0;
    TimeLimit = 0;
    fSequential = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "arRDATctpsvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'a':
                fAreaRecovery ^= 1;
                break;
            case 'r':
                fFuncRed ^= 1;
                break;
            case 'R':
                fResynthesis ^= 1;
                break;
            case 'c':
                fChoicing ^= 1;
                break;
            case 't':
                fTrust ^= 1;
                break;
            case 'p':
                fPower ^= 1;
                break;
			case 'D':
				DelayLimit = (float)atof(argv[util_optind]);
				util_optind++;
				if ( DelayLimit <= 0.0 ) 
					goto usage;
				break;
			case 'A':
				AreaLimit = (float)atof(argv[util_optind]);
				util_optind++;
				if ( AreaLimit <= 0.0 ) 
					goto usage;
				break;
			case 'T':
				TimeLimit = (float)atof(argv[util_optind]);
				util_optind++;
				if ( TimeLimit <= 0.0 ) 
					goto usage;
				break;
            case 's':
                fSequential ^= 1;
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

    if ( argc != util_optind )
    {
        fprintf( pErr, "Incorrect number of command line arguments.\n" );
        return 1;
    }

    pNetNew = Ntk_NetworkFpga( pNet, fTrust, fFuncRed, fChoicing, fPower, fAreaRecovery, fVerbose, fResynthesis, DelayLimit, AreaLimit, TimeLimit, 0, fSequential );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Network FPGA mapping has failed!\n" );
//        return 1;
        return 0;
    }
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    fprintf( pErr, "usage: fpga [-arctpsRDATvh]\n");
    fprintf( pErr, "\t          performs DAG mapping into variable-sized LUTS\n" );  
    fprintf( pErr, "\t-a          : toggles the use of area recovery [default = %s]\n", fAreaRecovery? "yes": "no" );  
    fprintf( pErr, "\t-r          : toggles the use of functional reduction [default = %s]\n", fFuncRed? "yes": "no" );  
    fprintf( pErr, "\t-c          : toggles the use of choice nodes [default = %s]\n", fChoicing? "yes": "no" );  
    fprintf( pErr, "\t-t          : toggles the trust mode when the input is an AIG [default = %s]\n", fTrust? "yes": "no" );  
    fprintf( pErr, "\t-p          : toggles power optimization [default = %s]\n", fPower? "yes": "no" );  
    fprintf( pErr, "\t-s          : toggles retiming during mapping (experimental) [default = %s]\n", fSequential? "yes": "no" );  
    fprintf( pErr, "\t-R          : toggles resynthesis [default = %s]\n", fResynthesis? "yes": "no" );  
    fprintf( pErr, "\t-D num      : delay threshold for resynthesis [default = not given]\n" );  
    fprintf( pErr, "\t-A num      : area threshold for resynthesis [default = not given]\n" );  
    fprintf( pErr, "\t-T num      : time threshold for resynthesis [default = %d sec]\n", (int)TimeLimit );  
    fprintf( pErr, "\t-v          : toggles verbose output [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h          : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkFpgaRes( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    char * FileNameOut = 0;
    float DelayLimit, AreaLimit, TimeLimit;
    int fVerbose, fGlobalCones;
    int c;
    extern Ntk_Network_t * Ntk_NetworkFpga( Ntk_Network_t * pNet, 
        bool fTrust, int fFuncRed, int fChoicing, int fPower, int fAreaRecovery, bool fVerbose, 
        int fResynthesis, float DelayLimit, float AreaLimit, float TimeLimit, int fGlobalCones, int fSequential );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    DelayLimit = 0;
    AreaLimit  = 0;
    TimeLimit  = 60; // one minute
    fVerbose   = 0;
    fGlobalCones = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "DATgvh")) != EOF ) 
    {
        switch (c) 
        {
			case 'D':
				DelayLimit = (float)atof(argv[util_optind]);
				util_optind++;
				if ( DelayLimit <= 0.0 ) 
					goto usage;
				break;
			case 'A':
				AreaLimit = (float)atof(argv[util_optind]);
				util_optind++;
				if ( AreaLimit <= 0.0 ) 
					goto usage;
				break;
			case 'T':
				TimeLimit = (float)atof(argv[util_optind]);
				util_optind++;
				if ( TimeLimit <= 0.0 ) 
					goto usage;
				break;
            case 'g':
                fGlobalCones ^= 1;
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

    if ( argc != util_optind )
    {
        fprintf( pErr, "Incorrect number of command line arguments.\n" );
        return 1;
    }

    pNetNew = Ntk_NetworkFpga( pNet, 0, 0, 0, 0, 0, fVerbose, 1, DelayLimit, AreaLimit, TimeLimit, fGlobalCones, 0 );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Network resynthesis mapping has failed!\n" );
        return 1;
    }
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );
    return 0;

usage:
    fprintf( pErr, "usage: resf [-D num] [-A num] [-T num] [-gvh]\n");
    fprintf( pErr, "\t          performs resynthesis using the current LUT library\n" );  
    fprintf( pErr, "\t-D num      : delay threshold for resynthesis [default = not given]\n" );  
    fprintf( pErr, "\t-A num      : area threshold for resynthesis [default = not given]\n" );  
    fprintf( pErr, "\t-T num      : time threshold for resynthesis [default = %d sec]\n", (int)TimeLimit );  
    fprintf( pErr, "\t-g          : toggles global logic cones [default = %s]\n", fGlobalCones? "yes": "no" );  
    fprintf( pErr, "\t-v          : toggles verbose output [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h          : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkMapRes( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    char * FileNameOut = 0;
    float DelayLimit, AreaLimit, TimeLimit;
    int fVerbose;
    int fGlobalCones;
    int fUseThree;
    int CritWindow;
    int ConeDepth;
    int ConeSize;
    int c;
    extern Ntk_Network_t * Ntk_NetworkResm( Ntk_Network_t * pNet, char * pFileNameOut, 
        float DelayLimit, float AreaLimit, float TimeLimit, int fGlobalCones, 
        int fUseThree, int CritWindow, int ConeDepth, int ConeSize, int fVerbose );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    DelayLimit   = 0;
    AreaLimit    = 0;
    TimeLimit    = 180; // three minutes
    fVerbose     = 0;
    fGlobalCones = 0;
    fUseThree    = 0;
    CritWindow   = 10; // 10% of max delay of the circuit
    ConeDepth    = 6;  // the number of inv delays to consider
    ConeSize     = 40; // the number of nodes in the cone
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "DATwgtcdnvh")) != EOF ) 
    {
        switch (c) 
        {
			case 'D':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-D\" should be followed by a number.\n" );
                    goto usage;
                }
				DelayLimit = (float)atof(argv[util_optind]);
				util_optind++;
				if ( DelayLimit <= 0.0 ) 
					goto usage;
				break;
			case 'A':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-A\" should be followed by a number.\n" );
                    goto usage;
                }
				AreaLimit = (float)atof(argv[util_optind]);
				util_optind++;
				if ( AreaLimit <= 0.0 ) 
					goto usage;
				break;
			case 'T':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-T\" should be followed by a number.\n" );
                    goto usage;
                }
				TimeLimit = (float)atof(argv[util_optind]);
				util_optind++;
				if ( TimeLimit <= 0.0 ) 
					goto usage;
				break;
            case 'w':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-w\" should be followed by the output file name.\n" );
                    goto usage;
                }
                FileNameOut = argv[util_optind];
                util_optind++;
                break;
            case 'g':
                fGlobalCones ^= 1;
                break;
            case 't':
                fUseThree ^= 1;
                break;
            case 'c':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-c\" should be followed by an integer.\n" );
                    goto usage;
                }
                CritWindow = atoi(argv[util_optind]);
                util_optind++;
                if ( CritWindow < 0 ) 
                    goto usage;
                break;
            case 'd':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-d\" should be followed by an integer.\n" );
                    goto usage;
                }
                ConeDepth = atoi(argv[util_optind]);
                util_optind++;
                if ( ConeDepth < 0 ) 
                    goto usage;
                break;
            case 'n':
                if ( util_optind >= argc )
                {
                    fprintf( pErr, "Command line switch \"-n\" should be followed by an integer.\n" );
                    goto usage;
                }
                ConeSize = atoi(argv[util_optind]);
                util_optind++;
                if ( ConeSize < 0 ) 
                    goto usage;
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

    if ( argc != util_optind )
    {
        fprintf( pErr, "Incorrect number of command line arguments.\n" );
        return 1;
    }

    pNetNew = Ntk_NetworkResm( pNet, FileNameOut, DelayLimit, AreaLimit, TimeLimit, 
        fGlobalCones, fUseThree, CritWindow, ConeDepth, ConeSize, fVerbose );
    if ( pNetNew == NULL )
    {
        fprintf( pErr, "Network resynthesis has failed!\n" );
        return 1;
    }
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );

    // perform verification
    Cmd_CommandExecute( pMvsis, "fraig_verify" );
    return 0;

usage:
    fprintf( pErr, "usage: resm [-D num] [-A num] [-T num] [-c num] [-d num] [-n num] [-w outfile] [-gtvh]\n");
    fprintf( pErr, "\t          performs resynthesis using the current standard cell library\n" );  
    fprintf( pErr, "\t-D num      : delay threshold for resynthesis [default = not given]\n" );  
    fprintf( pErr, "\t-A num      : area threshold for resynthesis [default = not given]\n" );  
    fprintf( pErr, "\t-T num      : time threshold for resynthesis [default = %d sec]\n", (int)TimeLimit );  
    fprintf( pErr, "\t-c num      : the critical path window in percents of max delay [default = %d]\n", CritWindow );  
    fprintf( pErr, "\t-d num      : the max depth of logic cone (in inverter delays) [default = %d]\n", ConeDepth );  
    fprintf( pErr, "\t-n num      : the max number of nodes in the logic cone [default = %d]\n", ConeSize );  
    fprintf( pErr, "\t-w outfile  : writes network to outfile as a mapped BLIF\n");  
    fprintf( pErr, "\t-g          : toggles global logic cones [default = %s]\n", fGlobalCones? "yes": "no" );  
    fprintf( pErr, "\t-t          : toggles the use of three-node groups [default = %s]\n", fUseThree? "yes": "no" );  
    fprintf( pErr, "\t-v          : toggles verbose output [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h          : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkFraigRetime( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fFuncRed;
    int fBalance;
    int fChoicing;
    int fVerbose;
    int c;

    extern Ntk_Network_t * Ntk_NetworkFraigRetime( Ntk_Network_t * pNet, int fFuncRed, int fBalance, int fChoicing, int fVerbose );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fFuncRed     = 1;
    fBalance     = 0;
    fChoicing    = 1;
    fVerbose     = 0;
    util_getopt_reset();
    while ( (c = util_getopt(argc, argv, "rbcvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'r':
                fFuncRed ^= 1;
                break;
            case 'b':
                fBalance ^= 1;
                break;
            case 'c':
                fChoicing ^= 1;
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

    Ntk_NetworkFraigRetime( pNet, fFuncRed, fBalance, fChoicing, fVerbose );
    return 0;

usage:
    fprintf( pErr, "usage: fraig_retime [-rbcvh]\n");
    fprintf( pErr, "\t          converts the network into AIG with choices and retimes it\n" );  
    fprintf( pErr, "\t-r          : toggles functional reduction [default = %s]\n", fFuncRed? "yes": "no" );  
    fprintf( pErr, "\t-b          : toggles AIG balancing [default = %s]\n", fBalance? "yes": "no" );  
    fprintf( pErr, "\t-c          : toggles choice nodes [default = %s]\n", fChoicing? "yes": "no" );  
    fprintf( pErr, "\t-v          : toggles verbose output [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h          : print the command usage\n");
    return 1;       /* error exit */
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_CommandNetworkRetime( Mv_Frame_t * pMvsis, int argc, char ** argv )
{
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet;
    int fVerbose;
    int c;

    extern Ntk_Network_t * Ntk_NetworkRetime( Ntk_Network_t * pNet, int fVerbose );

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    // set the defaults
    fVerbose     = 0;
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

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    Ntk_NetworkRetime( pNet, fVerbose );
    return 0;

usage:
    fprintf( pErr, "usage: retime [-vh]\n");
    fprintf( pErr, "\t          retimes the current network using the unit delay model\n" );  
    fprintf( pErr, "\t-v          : toggles verbose output [default = %s]\n", fVerbose? "yes": "no" );  
    fprintf( pErr, "\t-h          : print the command usage\n");
    return 1;       /* error exit */
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


