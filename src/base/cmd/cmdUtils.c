/**CFile****************************************************************

  FileName    [cmdUtils.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various utilities of the command package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cmdUtils.c,v 1.21 2005/06/02 03:34:08 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "mvInt.h"
#include "cmdInt.h"
#include <ctype.h>	// proper declaration of isspace

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int CmdCommandPrintCompare( Mv_Command ** ppC1, Mv_Command ** ppC2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int cmdCheckShellEscape( Mv_Frame_t * pMvsis, int argc, char ** argv)
{
	if (argv[0][0] == '!') 
	{
		const int size = 4096;
		int i;
		char buffer[4096];
		strncpy (buffer, &argv[0][1], size);
		for (i = 1; i < argc; ++i)
		{
				strncat (buffer, " ", size);
				strncat (buffer, argv[i], size);
		}
		if (buffer[0] == 0) 
			strncpy (buffer, "/bin/sh", size);
		system (buffer);

		// NOTE: Since we reconstruct the cmdline by concatenating
		// the parts, we lose information. So a command like
		// `!ls "file name"` will be sent to the system as
		// `ls file name` which is a BUG

    	return 1;
	}
	else
	{
		return 0;
	}
}

/**Function*************************************************************

  Synopsis    [Executes one command.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int CmdCommandDispatch( Mv_Frame_t * pMvsis, int argc, char **argv )
{
    Ntk_Network_t * pNetCopy;
    int (*pFunc) ( Mv_Frame_t *, int, char ** );
    Mv_Command * pCommand;
    char * value;
    int fError;
    int clk;

    if ( argc == 0 )
        return 0;

	if ( cmdCheckShellEscape( pMvsis, argc, argv ) == 1 )
		return 0;

    // get the command
    if ( !avl_lookup( pMvsis->tCommands, argv[0], (char **)&pCommand ) )
    {   // the command is not in the table
        fprintf( pMvsis->Err, "** cmd error: unknown command '%s'\n", argv[0] );
        return 1;
    }

    // get the backup network if the command is going to change the network
    if ( pCommand->fChange ) 
    {
        if ( pMvsis->pNetCur )
        {
            pNetCopy = Ntk_NetworkDup( pMvsis->pNetCur, Ntk_NetworkReadMan(pMvsis->pNetCur) );
            Mv_FrameSetCurrentNetwork( pMvsis, pNetCopy );
            // swap the current network and the backup network 
            // to prevent the effect of resetting the short names
            Mv_FrameSwapCurrentAndBackup( pMvsis );
        }
    }

    // execute the command
    clk = util_cpu_time();
    pFunc = ( int (*)( Mv_Frame_t *, int, char ** ) ) pCommand->pFunc;
    fError = (*pFunc)( pMvsis, argc, argv );
    pMvsis->TimeCommand += (util_cpu_time() - clk);

//    if ( !fError && pCommand->fChange && pMvsis->pNetCur ) 
//    {
//	Cmd_HistoryAddSnapshot(pMvsis, pMvsis->pNet);
//    }

    // automatic execution of arbitrary command after each command 
    // usually this is a passive command ... 
    if ( fError == 0 && !pMvsis->fAutoexac )
    {
        if ( avl_lookup( pMvsis->tFlags, "autoexec", &value ) )
        {
            pMvsis->fAutoexac = 1;
            fError = Cmd_CommandExecute( pMvsis, value );
            pMvsis->fAutoexac = 0;
        }
    }
    return fError;
}

/**Function*************************************************************

  Synopsis    [Splits the command line string into individual commands.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * CmdSplitLine( Mv_Frame_t * pMvsis, char *sCommand, int *argc, char ***argv )
{
    register char *p, *start, c;
    register int i, j;
    register char *new_arg;
    array_t *argv_array;
    int single_quote, double_quote;

    argv_array = array_alloc( char *, 5 );

    p = sCommand;
    for ( ;; )
    {
        // skip leading white space 
        while ( isspace( ( int ) *p ) )
        {
            p++;
        }

        // skip until end of this token 
        single_quote = double_quote = 0;
        for ( start = p; ( c = *p ) != '\0'; p++ )
        {
            if ( c == ';' || c == '#' || isspace( ( int ) c ) )
            {
                if ( !single_quote && !double_quote )
                {
                    break;
                }
            }
            if ( c == '\'' )
            {
                single_quote = !single_quote;
            }
            if ( c == '"' )
            {
                double_quote = !double_quote;
            }
        }
        if ( single_quote || double_quote )
        {
            ( void ) fprintf( pMvsis->Err, "** cmd warning: ignoring unbalanced quote ...\n" );
        }
        if ( start == p )
            break;

        new_arg = ALLOC( char, p - start + 1 );
        j = 0;
        for ( i = 0; i < p - start; i++ )
        {
            c = start[i];
            if ( ( c != '\'' ) && ( c != '\"' ) )
            {
                new_arg[j++] = isspace( ( int ) c ) ? ' ' : start[i];
            }
        }
        new_arg[j] = '\0';
        array_insert_last( char *, argv_array, new_arg );
    }

    *argc = array_n( argv_array );
    *argv = array_data( char *, argv_array );
    array_free( argv_array );
    if ( *p == ';' )
    {
        p++;
    }
    else if ( *p == '#' )
    {
        for ( ; *p != 0; p++ ); // skip to end of line 
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Replaces parts of the command line string by aliases if given.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int CmdApplyAlias( Mv_Frame_t * pMvsis, int *argcp, char ***argvp, int *loop )
{
    int i, argc, stopit, added, offset, did_subst, subst, fError, newc, j;
    char *arg, **argv, **newv;
    Mv_Alias *alias;

    argc = *argcp;
    argv = *argvp;
    stopit = 0;
    for ( ; *loop < 20; ( *loop )++ )
    {
        if ( argc == 0 )
            return 0;
        if ( stopit != 0 || 
            avl_lookup( pMvsis->tAliases, argv[0],  ( char ** ) &alias ) == 0 )
        {
            return 0;
        }
        if ( strcmp( argv[0], alias->argv[0] ) == 0 )
        {
            stopit = 1;
        }
        FREE( argv[0] );
        added = alias->argc - 1;

        /* shift all the arguments to the right */
        if ( added != 0 )
        {
            argv = REALLOC( char *, argv, argc + added );
            for ( i = argc - 1; i >= 1; i-- )
            {
                argv[i + added] = argv[i];
            }
            for ( i = 1; i <= added; i++ )
            {
                argv[i] = NIL( char );
            }
            argc += added;
        }
        subst = 0;
        for ( i = 0, offset = 0; i < alias->argc; i++, offset++ )
        {
            arg = CmdHistorySubstitution( pMvsis, alias->argv[i], &did_subst );
            if ( arg == NIL( char ) )
            {
                *argcp = argc;
                *argvp = argv;
                return ( 1 );
            }
            if ( did_subst != 0 )
            {
                subst = 1;
            }
            fError = 0;
            do
            {
                arg = CmdSplitLine( pMvsis, arg, &newc, &newv );
                /*
                 * If there's a complete `;' terminated command in `arg',
                 * when split_line() returns arg[0] != '\0'.
                 */
                if ( arg[0] == '\0' )
                { /* just a bunch of words */
                    break;
                }
                fError = CmdApplyAlias( pMvsis, &newc, &newv, loop );
                if ( fError == 0 )
                {
                   	fError = CmdCommandDispatch( pMvsis, newc, newv );
                }
                CmdFreeArgv( newc, newv );
            }
            while ( fError == 0 );
            if ( fError != 0 )
            {
                *argcp = argc;
                *argvp = argv;
                return ( 1 );
            }
            added = newc - 1;
            if ( added != 0 )
            {
                argv = REALLOC( char *, argv, argc + added );
                for ( j = argc - 1; j > offset; j-- )
                {
                    argv[j + added] = argv[j];
                }
                argc += added;
            }
            for ( j = 0; j <= added; j++ )
            {
                argv[j + offset] = newv[j];
            }
            FREE( newv );
            offset += added;
        }
        if ( subst == 1 )
        {
            for ( i = offset; i < argc; i++ )
            {
                FREE( argv[i] );
            }
            argc = offset;
        }
        *argcp = argc;
        *argvp = argv;
    }

    fprintf( pMvsis->Err, "** cmd warning: alias loop\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs history substitution (now, disabled).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * CmdHistorySubstitution( Mv_Frame_t * pMvsis, char *line, int *changed )
{
    // as of today, no history substitution 
    *changed = 0;
    return line;
}

/**Function*************************************************************

  Synopsis    [Opens the file with path (now, disabled).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
FILE * CmdFileOpen( Mv_Frame_t * pMvsis, char *sFileName, char *sMode, char **pFileNameReal, int silent )
{
    char * sRealName, * sPathUsr, * sPathLib, * sPathAll;
    FILE * pFile;
    
    if (strcmp(sFileName, "-") == 0) {
        if (strcmp(sMode, "w") == 0) {
            sRealName = util_strsav( "stdout" );
            pFile = stdout;
        }
        else {
            sRealName = util_strsav( "stdin" );
            pFile = stdin;
        }
    }
    else {
        sRealName = NULL;
        if (strcmp(sMode, "r") == 0) {
            
            /* combine both pathes if exist */
            sPathUsr = Cmd_FlagReadByName(pMvsis,"open_path");
            sPathLib = Cmd_FlagReadByName(pMvsis,"lib_path");
            
            if ( sPathUsr == NULL && sPathLib == NULL ) {
                sPathAll = NULL;
            }
            else if ( sPathUsr == NULL ) {
                sPathAll = util_strsav( sPathLib );
            }
            else if ( sPathLib == NULL ) {
                sPathAll = util_strsav( sPathUsr );
            }
            else {
                sPathAll = ALLOC( char, strlen(sPathLib)+strlen(sPathUsr)+5 );
                sprintf( sPathAll, "%s:%s",sPathUsr, sPathLib );
            }
            if ( sPathAll != NIL(char) ) {
                sRealName = util_file_search(sFileName, sPathAll, "r");
                FREE( sPathAll );
            }
        }
        if (sRealName == NIL(char)) {
            sRealName = util_tilde_expand(sFileName);
        }
        if ((pFile = fopen(sRealName, sMode)) == NIL(FILE)) {
            if (! silent) {
                perror(sRealName);
            }
        }
    }
    if ( pFileNameReal )
        *pFileNameReal = sRealName;
    else
        FREE(sRealName);
    
    return pFile;
}

/**Function*************************************************************

  Synopsis    [Frees the previously allocated argv array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdFreeArgv( int argc, char **argv )
{
    int i;
    for ( i = 0; i < argc; i++ )
        FREE( argv[i] );
    FREE( argv );
}

/**Function*************************************************************

  Synopsis    [Frees the previously allocated command.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdCommandFree( Mv_Command * pCommand )
{
    free( pCommand->sGroup );
    free( pCommand->sName );
    free( pCommand );
}


/**Function*************************************************************

  Synopsis    [Prints commands alphabetically by group.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdCommandPrint( Mv_Frame_t * pMvsis, bool fPrintAll )
{
    char *key, *value;
    avl_generator *gen;
    Mv_Command ** ppCommands;
    Mv_Command * pCommands;
    int nCommands, i;
    char * sGroupCur;
    int LenghtMax, nColumns, iCom = 0;

    // put all commands into one array
    nCommands = avl_count( pMvsis->tCommands );
    ppCommands = ALLOC( Mv_Command *, nCommands );
    i = 0;
    avl_foreach_item( pMvsis->tCommands, gen, AVL_FORWARD, &key, &value )
    {
        pCommands = (Mv_Command *)value;
        if ( fPrintAll || pCommands->sName[0] != '_' )
            ppCommands[i++] = pCommands;
    }
    nCommands = i;

    // sort command by group and then by name, alphabetically
    qsort( (void *)ppCommands, nCommands, sizeof(Mv_Command *), 
            (int (*)(const void *, const void *)) CmdCommandPrintCompare );
    assert( CmdCommandPrintCompare( ppCommands, ppCommands + nCommands - 1 ) <= 0 );

    // get the longest command name
    LenghtMax = 0;
    for ( i = 0; i < nCommands; i++ )
        if ( LenghtMax < (int)strlen(ppCommands[i]->sName) )
             LenghtMax = (int)strlen(ppCommands[i]->sName); 
    // get the number of columns
    nColumns = 79 / (LenghtMax + 1);

    // print the starting message 
#ifdef BALM
    fprintf( pMvsis->Out, "                        Welcome to BALM!" );
#else
    fprintf( pMvsis->Out, "                        Welcome to MVSIS!" );
#endif
    // print the command by group
    sGroupCur = NULL;
    for ( i = 0; i < nCommands; i++ )
        if ( sGroupCur && strcmp( sGroupCur, ppCommands[i]->sGroup ) == 0 )
        { // this command belongs to the same group as the previous one
            if ( iCom++ % nColumns == 0 )
                fprintf( pMvsis->Out, "\n" ); 
            // print this command
            fprintf( pMvsis->Out, " %-*s", LenghtMax, ppCommands[i]->sName );
        }
        else
        { // this command starts the new group of commands
            // start the new group
            fprintf( pMvsis->Out, "\n" );
            fprintf( pMvsis->Out, "\n" );
            fprintf( pMvsis->Out, "%s commands:\n", ppCommands[i]->sGroup );
            // print this command
            fprintf( pMvsis->Out, " %-*s", LenghtMax, ppCommands[i]->sName );
            // remember current command group
            sGroupCur = ppCommands[i]->sGroup;
            // reset the command counter
            iCom = 1;
        }
    fprintf( pMvsis->Out, "\n" );
    FREE( ppCommands );
}
 
/**Function*************************************************************

  Synopsis    [Comparision function used for sorting commands.]

  Description []
                
  SideEffects []

  SeeAlso     []

***********************************************************************/
int CmdCommandPrintCompare( Mv_Command ** ppC1, Mv_Command ** ppC2 )
{
    Mv_Command * pC1 = *ppC1;
    Mv_Command * pC2 = *ppC2;
    int RetValue;

    RetValue = strcmp( pC1->sGroup, pC2->sGroup );
    if ( RetValue < 0 )
        return -1;
    if ( RetValue > 0 )
        return 1;
    // the command belong to the same group

    // put commands with "_" at the end of the list
    if ( pC1->sName[0] != '_' && pC2->sName[0] == '_' )
        return -1;
    if ( pC1->sName[0] == '_' && pC2->sName[0] != '_' )
        return 1;

    RetValue = strcmp( pC1->sName, pC2->sName );
    if ( RetValue < 0 )
        return -1;
    if ( RetValue > 0 )
        return 1;
     // should not be two indentical commands
    assert( 0 );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
