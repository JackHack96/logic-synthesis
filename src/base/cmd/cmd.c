/**CFile****************************************************************

  FileName    [cmd.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Commands defined in the command package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cmd.c,v 1.25 2005/06/02 03:34:08 alanmi Exp $]

***********************************************************************/
 
#include "mv.h"
#include "mvInt.h"
#include "cmdInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int CmdCommandTime          ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandEcho          ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandQuit          ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandUsage         ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandWhich         ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandHistory       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandAlias         ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandUnalias       ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandHelp          ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandSource        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandSetVariable   ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandUnsetVariable ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandUndo          ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandRecall        ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandEmpty         ( Mv_Frame_t * pMvsis, int argc, char ** argv );
static int CmdCommandSis           ( Mv_Frame_t * pMvsis, int argc, char ** argv );
#ifdef WIN32
static int CmdCommandLs            ( Mv_Frame_t * pMvsis, int argc, char ** argv );
#endif

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Initializes the command package.]

  SideEffects [Commands are added to the command table.]

  SeeAlso     [Cmd_End]

******************************************************************************/
void Cmd_Init( Mv_Frame_t * pMvsis )
{
    pMvsis->tCommands = avl_init_table(strcmp);
    pMvsis->tAliases  = avl_init_table(strcmp);
    pMvsis->tFlags    = avl_init_table(strcmp);
    pMvsis->aHistory  = array_alloc( CmdHistory_t, 0 );

#ifdef BALM
    Cmd_CommandAdd( pMvsis, "Miscellaneous", "echo",      CmdCommandEcho,           0);
    Cmd_CommandAdd( pMvsis, "Miscellaneous", "time",      CmdCommandTime,           0);
    Cmd_CommandAdd( pMvsis, "Miscellaneous", "quit",      CmdCommandQuit,           0);
    Cmd_CommandAdd( pMvsis, "Miscellaneous", "history",   CmdCommandHistory,        0);
    Cmd_CommandAdd( pMvsis, "Miscellaneous", "alias",     CmdCommandAlias,          0);
    Cmd_CommandAdd( pMvsis, "Miscellaneous", "unalias",   CmdCommandUnalias,        0);
    Cmd_CommandAdd( pMvsis, "Miscellaneous", "help",      CmdCommandHelp,           0);
    Cmd_CommandAdd( pMvsis, "Miscellaneous", "source",    CmdCommandSource,         0);
/* UNIVR */
	Cmd_CommandAdd( pMvsis, "Basic", "set",       CmdCommandSetVariable,    0);
    Cmd_CommandAdd( pMvsis, "Basic", "unset",     CmdCommandUnsetVariable,  0);
/* UNIVR */
#ifdef WIN32
    Cmd_CommandAdd( pMvsis, "Miscellaneous", "ls",        CmdCommandLs,             0 );
#endif

#else
    Cmd_CommandAdd( pMvsis, "Basic", "time",      CmdCommandTime,           0);
    Cmd_CommandAdd( pMvsis, "Basic", "echo",      CmdCommandEcho,           0);
    Cmd_CommandAdd( pMvsis, "Basic", "quit",      CmdCommandQuit,           0);
    Cmd_CommandAdd( pMvsis, "Basic", "usage",     CmdCommandUsage,          0);
//    Cmd_CommandAdd( pMvsis, "Basic", "which",     CmdCommandWhich,          0);
    Cmd_CommandAdd( pMvsis, "Basic", "history",   CmdCommandHistory,        0);
    Cmd_CommandAdd( pMvsis, "Basic", "alias",     CmdCommandAlias,          0);
    Cmd_CommandAdd( pMvsis, "Basic", "unalias",   CmdCommandUnalias,        0);
    Cmd_CommandAdd( pMvsis, "Basic", "help",      CmdCommandHelp,           0);
    Cmd_CommandAdd( pMvsis, "Basic", "source",    CmdCommandSource,         0);
	Cmd_CommandAdd( pMvsis, "Basic", "set",       CmdCommandSetVariable,    0);
    Cmd_CommandAdd( pMvsis, "Basic", "unset",     CmdCommandUnsetVariable,  0);
	Cmd_CommandAdd( pMvsis, "Basic", "undo",      CmdCommandUndo,           0); 
    Cmd_CommandAdd( pMvsis, "Basic", "recall",    CmdCommandRecall,         0); 
    Cmd_CommandAdd( pMvsis, "Basic", "empty",     CmdCommandEmpty,          0); 
    Cmd_CommandAdd( pMvsis, "Various", "sis",     CmdCommandSis,            1); 
#ifdef WIN32
    Cmd_CommandAdd( pMvsis, "Basic", "ls",        CmdCommandLs,             0 );
#endif
#endif

}

/**Function********************************************************************

  Synopsis    [Ends the command package.]

  Description [Ends the command package. Tables are freed.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Cmd_End( Mv_Frame_t * pMvsis )
{
    int i;

    avl_free_table( pMvsis->tCommands, (void (*)()) 0, CmdCommandFree );
    avl_free_table( pMvsis->tAliases,  (void (*)()) 0, CmdCommandAliasFree );
    avl_free_table( pMvsis->tFlags,    free, free );

    for ( i = 0; i < array_n(pMvsis->aHistory); i++ )
    {
	CmdHistory_t cmd = array_fetch(CmdHistory_t, pMvsis->aHistory, i);
	if (cmd.fCommand) FREE(cmd.data.string);
    }
    array_free( pMvsis->aHistory );
}



/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandTime( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    int c;

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

    if ( argc != util_optind )
    {
        goto usage;
    }

    pMvsis->TimeTotal += pMvsis->TimeCommand;
    fprintf( pMvsis->Out, "elapse: %3.2f seconds, total: %3.2f seconds\n",
        (float)pMvsis->TimeCommand / CLOCKS_PER_SEC, (float)pMvsis->TimeTotal / CLOCKS_PER_SEC );
    pMvsis->TimeCommand = 0;
    return 0;

  usage:
    fprintf( pMvsis->Err, "usage: time [-h]\n" );
    fprintf( pMvsis->Err, "   -h \t\tprint the command usage\n" );
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandEcho( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    int i;
    int c;
	int n = 1;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hn" ) ) != EOF )
    {
        switch ( c )
        {
		case 'n':
			n = 0;
			break;
        case 'h':
            goto usage;
            break;
        default:
            goto usage;
        }
    }

    for ( i = util_optind; i < argc; i++ )
        fprintf( pMvsis->Out, "%s ", argv[i] );
	if ( n )
    	fprintf( pMvsis->Out, "\n" );
	else
		fflush ( pMvsis->Out );
    return 0;

  usage:
    fprintf( pMvsis->Err, "usage: echo [-h] string \n" );
    fprintf( pMvsis->Err, "   -n \t\tsuppress newline at the end\n" );
    fprintf( pMvsis->Err, "   -h \t\tprint the command usage\n" );
    return ( 1 );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandQuit( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    int c;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hs" ) ) != EOF )
    {
        switch ( c )
        {
        case 'h':
            goto usage;
            break;
        case 's':
            return -2;
            break;
        default:
            goto usage;
        }
    }

    if ( argc != util_optind )
        goto usage;
    return -1;

  usage:
    fprintf( pMvsis->Err, "usage: quit [-h] [-s]\n" );
    fprintf( pMvsis->Err, "   -h  print the command usage\n" );
    fprintf( pMvsis->Err,
                      "   -s  frees all the memory before quitting\n" );
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandUsage( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    int c;

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

    if ( argc != util_optind )
        goto usage;
    util_print_cpu_stats( pMvsis->Out );
    return 0;

  usage:
    fprintf( pMvsis->Err, "usage: usage [-h]\n" );
    fprintf( pMvsis->Err, "   -h \t\tprint the command usage\n" );
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandWhich( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    return 0;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandHistory( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    int i, num, lineno;
    int size;
    int c;
    num = 30;
    lineno = 1;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hn" ) ) != EOF )
    {
        switch ( c )
        {
        case 'h':
            goto usage;
            break;
	case 'n': 
	    lineno = 0; break;
        default:
            goto usage;
        }
    }

    if ( argc > 3 )
        goto usage;

    size = array_n( pMvsis->aHistory );
    num = ( num < size ) ? num : size;
    for ( i = size - num; i < size; i++ )
    {
	// TODO: move this into cmdHist.c
	CmdHistory_t cmd = array_fetch(CmdHistory_t, pMvsis->aHistory, i);
	if (cmd.fCommand) {
	    // command in history
	    fprintf( pMvsis->Out, "\t%s", cmd.data.string);
	} 
	else if (cmd.fSnapshot) {
	    // snapshot in history
	    fprintf( pMvsis->Out, "%d ** ", cmd.id);
	    Ntk_NetworkPrintStats(pMvsis->Out, cmd.data.snapshot, 0, 0, 1);
	}
    }
    return ( 0 );

  usage:
    fprintf( pMvsis->Err, "usage: history [-hn] [num]\n" );
    fprintf( pMvsis->Err, "   -h \t\tprint the command usage\n" );
    fprintf( pMvsis->Err, "   -n \t\tsuppress line numbers\n");
    fprintf( pMvsis->Err, "   num \t\tprint the last num commands\n" );
    return ( 1 );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandAlias( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char *key, *value;
    avl_generator *gen;
    int c;

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


    if ( argc == 1 )
    {
        avl_foreach_item( pMvsis->tAliases, gen, AVL_FORWARD, &key, &value )
        {
            CmdCommandAliasPrint( pMvsis, ( Mv_Alias * ) value );
        }
        return 0;

    }
    else if ( argc == 2 )
    {
        if ( avl_lookup( pMvsis->tAliases, argv[1], &value ) )
        {
            CmdCommandAliasPrint( pMvsis, ( Mv_Alias * ) value );
        }
        return 0;
    }

    /* delete any existing alias */
    key = argv[1];
    if ( avl_delete( pMvsis->tAliases, &key, &value ) )
    {
        CmdCommandAliasFree( ( Mv_Alias * ) value );
    }

    CmdCommandAliasAdd( pMvsis, argv[1], argc - 2, argv + 2 );
    return 0;

  usage:
    fprintf( pMvsis->Err, "usage: alias [-h] [command [string]]\n" );
    fprintf( pMvsis->Err, "   -h \t\tprint the command usage\n" );
    return ( 1 );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandUnalias( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    int i;
    char *key, *value;
    int c;

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

    if ( argc < 2 )
    {
        goto usage;
    }

    for ( i = 1; i < argc; i++ )
    {
        key = argv[i];
        if ( avl_delete( pMvsis->tAliases, &key, &value ) )
        {
            CmdCommandAliasFree( ( Mv_Alias * ) value );
        }
    }
    return 0;

  usage:
    fprintf( pMvsis->Err, "usage: unalias [-h] alias_names\n" );
    fprintf( pMvsis->Err, "   -h \t\tprint the command usage\n" );
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandHelp( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    bool fPrintAll;
    int c;

    fPrintAll = 0;
    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "ah" ) ) != EOF )
    {
        switch ( c )
        {
        case 'a':
            case 'v':
                fPrintAll ^= 1;
                break;
            break;
        case 'h':
            goto usage;
            break;
        default:
            goto usage;
        }
    }

    if ( argc != util_optind )
        goto usage;

    CmdCommandPrint( pMvsis, fPrintAll );
    return 0;

usage:
    fprintf( pMvsis->Err, "usage: help [-a] [-h]\n" );
    fprintf( pMvsis->Err, "       prints the list of available commands by group\n" );
    fprintf( pMvsis->Err, " -a       toggle printing hidden commands [default = %s]\n", fPrintAll? "yes": "no" );
    fprintf( pMvsis->Err, " -h       print the command usage\n" );
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandSource( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    int c, echo, prompt, silent, interactive, quit_count, lp_count;
    int status = 0;             /* initialize so that lint doesn't complain */
    int lp_file_index, did_subst;
    char *prompt_string, *real_filename, line[MAX_STR], *command;
    FILE *fp;

    interactive = silent = prompt = echo = 0;

    util_getopt_reset();
    while ( ( c = util_getopt( argc, argv, "hipsx" ) ) != EOF )
    {
        switch ( c )
        {
        case 'h':
            goto usage;
            break;
        case 'i':               /* a hack to distinguish EOF from stdin */
            interactive = 1;
            break;
        case 'p':
            prompt = 1;
            break;
        case 's':
            silent = 1;
            break;
        case 'x':
            echo ^= 1;
            break;
        default:
            goto usage;
        }
    }

    /* added to avoid core-dumping when no script file is specified */
    if ( argc == util_optind )
    {
        goto usage;
    }

    lp_file_index = util_optind;
    lp_count = 0;

    /*
     * FIX (Tom, 5/7/95):  I'm not sure what the purpose of this outer do loop
     * is. In particular, lp_file_index is never modified in the loop, so it
     * looks it would just read the same file over again.  Also, SIS had
     * lp_count initialized to -1, and hence, any file sourced by SIS (if -l or
     * -t options on "source" were used in SIS) would actually be executed
     * twice.
     */
    do
    {
        lp_count++;             /* increment the loop counter */

        fp = CmdFileOpen( pMvsis, argv[lp_file_index], "r", &real_filename, silent );
        if ( fp == NULL )
        {
            FREE( real_filename );
            return !silent;     /* error return if not silent */
        }

        quit_count = 0;
        do
        {
            if ( prompt )
            {
                prompt_string = Cmd_FlagReadByName( pMvsis, "prompt" );
                if ( prompt_string == NIL( char ) )
                {
#ifdef BALM
                    prompt_string = "balm> ";
#else
                    prompt_string = "mvsis> ";
#endif

                }

            }
            else
            {
                prompt_string = NIL( char );
            }

            /* clear errors -- e.g., EOF reached from stdin */
            clearerr( fp );

            /* read another command line */
//      if (CmdFgetsFilec(line, MAX_STR, fp, prompt_string) == NULL) {
//      Mv_UtilsPrintPrompt(prompt_string);
//      fflush(stdout);
            if ( fgets( line, MAX_STR, fp ) == NULL )
            {
                if ( interactive )
                {
                    if ( quit_count++ < 5 )
                    {
                        fprintf( pMvsis->Err,
                                          "\nUse \"quit\" to leave MVSIS.\n" );
                        continue;
                    }
                    status = -1;    /* fake a 'quit' */
                }
                else
                {
                    status = 0; /* successful end of 'source' ; loop? */
                }
                break;
            }
            quit_count = 0;

            if ( echo )
            {
#ifdef BALM
                fprintf( pMvsis->Out, "balm - > %s", line );
#else
                fprintf( pMvsis->Out, "mvsis - > %s", line );
#endif
				fflush( pMvsis->Out );
            }
            command = CmdHistorySubstitution( pMvsis, line, &did_subst );
            if ( command == NIL( char ) )
            {
                status = 1;
                break;
            }
            if ( did_subst )
            {
                if ( interactive )
                {
                    fprintf( pMvsis->Out, "%s\n", command );
                }
            }
            if ( command != line )
            {
                ( void ) strcpy( line, command );
            }
            if ( interactive && *line != '\0' )
            {
                array_insert_last( char *, pMvsis->aHistory,  util_strsav( line ) );
                if ( pMvsis->Hst != NIL( FILE ) )
                {
                    fprintf( pMvsis->Hst, "%s\n", line );
                    ( void ) fflush( pMvsis->Hst );
                }
            }

            status = Cmd_CommandExecute( pMvsis, line );
        }
        while ( status == 0 );

        if ( fp != stdin )
        {
            if ( status > 0 )
            {
                fprintf( pMvsis->Err,
                                  "** cmd error: aborting 'source %s'\n",
                                  real_filename );
            }
            ( void ) fclose( fp );
        }
        FREE( real_filename );

    }
    while ( ( status == 0 ) && ( lp_count <= 0 ) );

    return status;

  usage:
    fprintf( pMvsis->Err, "usage: source [-h] [-p] [-s] [-x] file_name\n" );
    fprintf( pMvsis->Err, "\t-h     print the command usage\n" );
    fprintf( pMvsis->Err, "\t-p     supply prompt before reading each line\n" );
    fprintf( pMvsis->Err, "\t-s     silently ignore nonexistant file\n" );
    fprintf( pMvsis->Err, "\t-x     echo each line as it is executed\n" );
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandSetVariable( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    char *flag_value, *key, *value;
    avl_generator *gen;
    int c;

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
    if ( argc == 0 || argc > 3 )
    {
        goto usage;
    }
    else if ( argc == 1 )
    {
        avl_foreach_item( pMvsis->tFlags, gen, AVL_FORWARD, &key, &value )
        {
            fprintf( pMvsis->Out, "%s\t%s\n", key, value );
        }
        return 0;
    }
    else
    {
        key = argv[1];
        if ( avl_delete( pMvsis->tFlags, &key, &value ) )
        {
            FREE( key );
            FREE( value );
        }

        flag_value = argc == 2 ? util_strsav( "" ) : util_strsav( argv[2] );

        ( void ) avl_insert( pMvsis->tFlags, util_strsav( argv[1] ),
                             flag_value );

        if ( strcmp( argv[1], "mvsisout" ) == 0 )
        {
            if ( pMvsis->Out != stdout )
            {
                ( void ) fclose( pMvsis->Out );
            }
            if ( strcmp( flag_value, "" ) == 0 )
            {
                flag_value = "-";
            }
            pMvsis->Out = CmdFileOpen( pMvsis, flag_value, "w", NIL( char * ), 0 );
            if ( pMvsis->Out == NULL )
            {
                pMvsis->Out = stdout;
            }
#if HAVE_SETVBUF
            setvbuf( pMvsis->Out, ( char * ) NULL, _IOLBF, 0 );
#endif
        }
        if ( strcmp( argv[1], "mvsiserr" ) == 0 )
        {
            if ( pMvsis->Err != stderr )
            {
                ( void ) fclose( pMvsis->Err );
            }
            if ( strcmp( flag_value, "" ) == 0 )
            {
                flag_value = "-";
            }
            pMvsis->Err = CmdFileOpen( pMvsis, flag_value, "w", NIL( char * ), 0 );
            if ( pMvsis->Err == NULL )
            {
                pMvsis->Err = stderr;
            }
#if HAVE_SETVBUF
            setvbuf( pMvsis->Err, ( char * ) NULL, _IOLBF, 0 );
#endif
        }
        if ( strcmp( argv[1], "history" ) == 0 )
        {
            if ( pMvsis->Hst != NIL( FILE ) )
            {
                ( void ) fclose( pMvsis->Hst );
            }
            if ( strcmp( flag_value, "" ) == 0 )
            {
                pMvsis->Hst = NIL( FILE );
            }
            else
            {
                pMvsis->Hst =
                    CmdFileOpen( pMvsis, flag_value, "w", NIL( char * ), 0 );
                if ( pMvsis->Hst == NULL )
                {
                    pMvsis->Hst = NIL( FILE );
                }
            }
        }
        return 0;
    }

  usage:
    fprintf( pMvsis->Err, "usage: set [-h] [name] [value]\n" );
    fprintf( pMvsis->Err, "\t        sets the value of parameter <name>\n" );
    fprintf( pMvsis->Err, "\t-h    : print the command usage\n" );
    return 1;

}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandUnsetVariable( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    int i;
    char *key, *value;
    int c;

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

    if ( argc < 2 )
    {
        goto usage;
    }

    for ( i = 1; i < argc; i++ )
    {
        key = argv[i];
        if ( avl_delete( pMvsis->tFlags, &key, &value ) )
        {
            FREE( key );
            FREE( value );
        }
    }
    return 0;


  usage:
    fprintf( pMvsis->Err, "usage: unset [-h] variables \n" );
    fprintf( pMvsis->Err, "   -h \t\tprint the command usage\n" );
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandUndo( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

    // if there are no arguments on the command line
    // set the current network to be the network from the previous step
    if ( argc == 1 ) 
        return CmdCommandRecall( pMvsis, argc, argv );

    fprintf( pMvsis->Err, "usage: undo\n" );
    fprintf( pMvsis->Err, "         sets the current network to be the previously saved network\n" );
    return 1;

}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandRecall( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNet;
    int iStep, iStepFound;
    int nNetsToSave, c;
    char * pValue;
    int iStepStart, iStepStop;

    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

    
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
 
    // get the number of networks to save
    pValue = Cmd_FlagReadByName( pMvsis, "savesteps" );
    // if the value of steps to save is not set, assume 1-level undo
    if ( pValue == NULL )
        nNetsToSave = 1;
    else 
        nNetsToSave = atoi(pValue);

    // if there are no arguments on the command line
    // set the current network to be the network from the previous step
    if ( argc == 1 ) 
    {
        // get the previously saved network
        pNet = Ntk_NetworkReadBackup(pMvsis->pNetCur);
        if ( pNet == NULL )
            fprintf( pMvsis->Out, "There is no previously saved network.\n" );
        else // set the current network to be the copy of the previous one
            Mv_FrameSetCurrentNetwork( pMvsis, Ntk_NetworkDup(pNet, Ntk_NetworkReadMan(pNet)) );
         return 0;
    }
    if ( argc == 2 ) // the second argument is the number of the step to return to
    {
        // read the number of the step to return to
        iStep = atoi(argv[1]);
        // check whether it is reasonable
        if ( iStep >= pMvsis->nSteps )
        {
            iStepStart = pMvsis->nSteps - nNetsToSave;
            if ( iStepStart <= 0 )
                iStepStart = 1;
            iStepStop  = pMvsis->nSteps;
            if ( iStepStop <= 0 )
                iStepStop = 1;
            if ( iStepStart == iStepStop )
                fprintf( pMvsis->Out, "Can only recall step %d.\n", iStepStop );
            else
                fprintf( pMvsis->Out, "Can only recall steps %d-%d.\n", iStepStart, iStepStop );
        }
        else if ( iStep < 0 )
            fprintf( pMvsis->Out, "Cannot recall step %d.\n", iStep );
        else if ( iStep == 0 )
            Mv_FrameDeleteAllNetworks( pMvsis );
        else 
        {
            // scroll backward through the list of networks
            // to determine if such a network exist
            iStepFound = 0;
            for ( pNet = pMvsis->pNetCur; pNet; pNet = Ntk_NetworkReadBackup(pNet) )
                if ( (iStepFound = Ntk_NetworkReadStep(pNet)) == iStep ) 
                    break;
            if ( pNet == NULL )
            {
                iStepStart = iStepFound;
                if ( iStepStart <= 0 )
                    iStepStart = 1;
                iStepStop  = pMvsis->nSteps;
                if ( iStepStop <= 0 )
                    iStepStop = 1;
                if ( iStepStart == iStepStop )
                    fprintf( pMvsis->Out, "Can only recall step %d.\n", iStepStop );
                else
                    fprintf( pMvsis->Out, "Can only recall steps %d-%d.\n", iStepStart, iStepStop );
            }
            else
                Mv_FrameSetCurrentNetwork( pMvsis, Ntk_NetworkDup(pNet, Ntk_NetworkReadMan(pNet)) );
        }
        return 0;
    }

usage:

    fprintf( pMvsis->Err, "usage: recall <num>\n" );
    fprintf( pMvsis->Err, "         set the current network to be one of the previous networks\n" );
    fprintf( pMvsis->Err, "<num> :  level to return to [default = previous]\n" );
    return 1;
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandEmpty( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    int c;

    if ( pMvsis->pNetCur == NULL )
    {
        fprintf( pMvsis->Out, "Empty network.\n" );
        return 0;
    }

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
 
    Mv_FrameDeleteAllNetworks( pMvsis );
    Mv_FrameRestart( pMvsis );
    return 0;
usage:

    fprintf( pMvsis->Err, "usage: empty [-h]\n" );
    fprintf( pMvsis->Err, "         removes all the currently stored networks\n" );
    fprintf( pMvsis->Err, "   -h :  print the command usage\n");
    return 1;
}


#if 0

/**Function********************************************************************

  Synopsis    [Donald's version.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandUndo( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNetTemp;
    int id, c;

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
    if (util_optind <= argc) {
	pNetTemp = pMvsis->pNet;
	pMvsis->pNet = pMvsis->pNetSaved;
	pMvsis->pNetSaved = pNetTemp;
    }
    id = atoi(argv[util_optind]);
    pNetTemp = Cmd_HistoryGetSnapshot(pMvsis, id);
    if (!pNetTemp) 
	fprintf( pMvsis->Err, "Snapshot %d does not exist\n", id);
    else
	pMvsis->pNet = Ntk_NetworkDup(pNetTemp, Ntk_NetworkReadMan(pNetTemp));

    return 0;
usage:
    fprintf( pMvsis->Err, "usage: undo\n" );
    fprintf( pMvsis->Err, "       swaps the current network and the backup network\n" );
    return 1;
}

#endif


#ifdef WIN32
/**Function*************************************************************

  Synopsis    [Command to print the contents of the current directory (Windows).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#include <io.h>

// these structures are defined in <io.h> but are for some reason invisible
typedef unsigned long _fsize_t; // Could be 64 bits for Win32

struct _finddata_t {
    unsigned    attrib;
    time_t      time_create;    // -1 for FAT file systems 
    time_t      time_access;    // -1 for FAT file systems 
    time_t      time_write;
    _fsize_t    size;
    char        name[260];
};

extern long _findfirst( char *filespec, struct _finddata_t *fileinfo );
extern int  _findnext( long handle, struct _finddata_t *fileinfo );
extern int  _findclose( long handle );

int CmdCommandLs( Mv_Frame_t * pMvsis, int argc, char **argv )
{
	struct _finddata_t c_file;
	long   hFile;
	int    fLong = 0;
	int    fOnlyBLIF = 0;
	char   Buffer[25];
	int    Counter = 0;
	int    fPrintedNewLine;
    char   c;

	util_getopt_reset();
	while ( (c = util_getopt(argc, argv, "lb") ) != EOF )
	{
		switch (c)
		{
			case 'l':
			  fLong = 1;
			  break;
			case 'b':
			  fOnlyBLIF = 1;
			  break;
			default:
			  goto usage;
		}
	}

	// find first .mv file in current directory
	if( (hFile = _findfirst( ((fOnlyBLIF)? "*.mv": "*.*"), &c_file )) == -1L )
	{
		if ( fOnlyBLIF )
			fprintf( pMvsis->Out, "No *.mv files in the current directory.\n" );
		else
			fprintf( pMvsis->Out, "No files in the current directory.\n" );
	}
	else
	{
		if ( fLong )
		{
			fprintf( pMvsis->Out, " File              Date           Size |  File             Date           Size \n" );
			fprintf( pMvsis->Out, " ----------------------------------------------------------------------------- \n" );
			do
			{
				strcpy( Buffer, ctime( &(c_file.time_write) ) );
				Buffer[16] = 0;
				fprintf( pMvsis->Out, " %-17s %.24s%7ld", c_file.name, Buffer+4, c_file.size );
				if ( ++Counter % 2 == 0 )
				{
					fprintf( pMvsis->Out, "\n" );
					fPrintedNewLine = 1;
				}
				else
				{
					fprintf( pMvsis->Out, " |" );
					fPrintedNewLine = 0;
				}
			}
			while( _findnext( hFile, &c_file ) == 0 );
		}
		else
		{
			do
			{
				fprintf( pMvsis->Out, " %-18s", c_file.name );
				if ( ++Counter % 4 == 0 )
				{
					fprintf( pMvsis->Out, "\n" );
					fPrintedNewLine = 1;
				}
				else
				{
					fprintf( pMvsis->Out, " " );
					fPrintedNewLine = 0;
				}
			}
			while( _findnext( hFile, &c_file ) == 0 );
		}
		if ( !fPrintedNewLine )
			fprintf( pMvsis->Out, "\n" );
		_findclose( hFile );
	}
	return 0;

usage:
	fprintf( pMvsis->Err, "Usage: ls [-l] [-b]\n" );
	fprintf( pMvsis->Err, "       print the file names in the current directory\n" );
	fprintf( pMvsis->Err, "        -l : print in the long format [default = short]\n" );
	fprintf( pMvsis->Err, "        -b : print only .mv files [default = all]\n" );
	return 1; 
}
#endif



#ifdef WIN32
#define unlink _unlink
#endif

/**Function********************************************************************

  Synopsis    [Calls SIS internally.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int CmdCommandSis( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    FILE * pFile;
    FILE * pOut, * pErr;
    Ntk_Network_t * pNet, * pNetNew;
    char Command[1000], Buffer[100];
    char * pSisName;
    int i;

    pNet = Mv_FrameReadNet(pMvsis);
    pOut = Mv_FrameReadOut(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);

    if ( pNet == NULL )
    {
        fprintf( pErr, "Empty network.\n" );
        goto usage;
    }

    if ( strcmp( argv[0], "sis" ) != 0 )
    {
        fprintf( pErr, "Wrong command: \"%s\".\n", argv[0] );
        goto usage;
    }

    if ( argc == 1 )
        goto usage;
    if ( strcmp( argv[1], "-h" ) == 0 )
        goto usage;
    if ( strcmp( argv[1], "-?" ) == 0 )
        goto usage;

    // cannot write a multi-valued network
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        fprintf( pErr, "Cannot execute the SIS command for the current MV network.\n" );
        goto usage;
    }

    // check if SIS is available
    if ( (pFile = fopen( "sis.exe", "r" )) )
        pSisName = "sis.exe";
    else if ( (pFile = fopen( "sis", "r" )) )
        pSisName = "sis";
    else if ( pFile == NULL )
    {
        fprintf( pErr, "Cannot find \"%s\" or \"%s\" in the current directory.\n", "sis.exe", "sis" );
        goto usage;
    }
    fclose( pFile );

    // write out the current network
    Io_WriteNetwork( pNet, "_sis_in.blif", IO_FILE_BLIF, 1 );

    // create the file for sis
    sprintf( Command, "%s -x -c ", pSisName );
    strcat ( Command, "\"" );
    strcat ( Command, "read_blif _sis_in.blif" );
    strcat ( Command, "; " );
    for ( i = 1; i < argc; i++ )
    {
        sprintf( Buffer, " %s", argv[i] );
        strcat( Command, Buffer );
    }
    strcat( Command, "; " );
    strcat( Command, "write_blif _sis_out.blif" );
    strcat( Command, "\"" );

    // call SIS
    if ( system( Command ) )
    {
        fprintf( pErr, "The following command has returned non-zero exit status:\n" );
        fprintf( pErr, "\"%s\"\n", Command );
        unlink( "_sis_in.blif" );
        goto usage;
    }

    // read in the SIS output
    if ( (pFile = fopen( "_sis_out.blif", "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open SIS output file \"%s\".\n", "_sis_out.blif" );
        unlink( "_sis_in.blif" );
        goto usage;
    }
    fclose( pFile );

    // set the new network
    pNetNew = Io_ReadNetwork( pMvsis, "_sis_out.blif" );
    // set the original spec of the new network
    if ( pNet->pSpec )
        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, pNet->pSpec );
    // replace the current network
    Mv_FrameReplaceCurrentNetwork( pMvsis, pNetNew );

    // remove temporary networks
    unlink( "_sis_in.blif" );
    unlink( "_sis_out.blif" );
    return 0;

usage:
	fprintf( pErr, "\n" );
	fprintf( pErr, "Usage: sis [-h] <com>\n");
	fprintf( pErr, "         invokes SIS command for the current MVSIS network\n" );
	fprintf( pErr, "         (the executable of SIS should be in the same directory)\n" );
    fprintf( pErr, "   -h  : print the command usage\n" );
	fprintf( pErr, " <com> : a SIS command (or a semicolon-separated list of commands in quotes)\n" );
    fprintf( pErr, "         Example 1: sis eliminate 0\n" );
    fprintf( pErr, "         Example 2: sis \"ps; rd; fx; ps\"\n" );
    fprintf( pErr, "         Example 3: sis source script.rugged\n" );
	return 1;					// error exit 
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


