/**CFile****************************************************************

  FileName    [mv.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Here everything starts.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mv.c,v 1.17 2004/06/07 19:11:02 satrajit Exp $]

***********************************************************************/

#if defined(HAVE_LIBREADLINE) && HAVE_LIBREADLINE==1
# include <stdio.h>
# include <readline/readline.h>
# include <readline/history.h>
#endif

#include "mv.h"
#include "mvInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
static int TypeCheck( Mv_Frame_t * pMvsis, char * s);

#if defined(HAVE_LIBREADLINE) && HAVE_LIBREADLINE==1

	char *command_generator (const char *text, int state);
	char **command_completion (const char *text, int start, int end);
	Mv_Frame_t * pMvsisCommandCompletion;

#endif

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [The main() procedure of MVSIS.]

  Description [Obviously, this main() procedure starts MVSIS. 
  This MVSIS framework is designed to be very simple and easy to 
  understand, compile, debug etc. The batch mode, tilde expand, 
  history substitution, and other fancy stuff from SIS/VIS will 
  be added later on demand.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int main( int argc, char * argv[] )
{
    Mv_Frame_t * pMvsis;
    char sCommandUsr[500], sCommandTmp[100], sReadCmd[20], sWriteCmd[20], c;
    char * sCommand, * sOutFile, * sInFile;
    int  fStatus = 0;
    bool fBatch, fInitSource, fInitRead, fFinalWrite;
    
    // added to detect memory leaks:
#ifdef _DEBUG
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	// get global frame (singleton pattern)
	// will be initialized on first call
	pMvsis = Mv_FrameGetGlobalFrame();

    // default options
    fBatch = 0;
    fInitSource = 1;
    fInitRead   = 0;
    fFinalWrite = 0;
    sInFile = sOutFile = NULL;
    sprintf( sReadCmd,  "read_blif_mv"  );
    sprintf( sWriteCmd, "write_blif_mv" );
    
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "c:hf:F:o:st:T:x")) != EOF) {
        switch(c) {
            case 'c':
                strcpy( sCommandUsr, util_optarg );
                fBatch = 1;
                break;
                
            case 'f':
                sprintf(sCommandUsr, "source %s", util_optarg);
                fBatch = 1;
                break;

            case 'F':
                sprintf(sCommandUsr, "source -x %s", util_optarg);
                fBatch = 1;
                break;
                
            case 'h':
                goto usage;
                break;
                
            case 'o':
                sOutFile = util_optarg;
                fFinalWrite = 1;
                break;
                
            case 's':
                fInitSource = 0;
                break;
                
            case 't':
                if ( TypeCheck( pMvsis, util_optarg ) )
                {
                    if ( !strcmp(util_optarg, "none") == 0 )
                    {
                        fInitRead = 1;
                        sprintf( sReadCmd, "read_%s", util_optarg );
                    }
                }
                else {
                    goto usage;
                }
                fBatch = 1;
                break;
                
            case 'T':
                if ( TypeCheck( pMvsis, util_optarg ) )
                {
                    if (!strcmp(util_optarg, "none") == 0)
                    {
                        fFinalWrite = 1;
                        sprintf( sWriteCmd, "write_%s", util_optarg);
                    }
                }
                else {
                    goto usage;
                }
                fBatch = 1;
                break;
                
            case 'x':
                fFinalWrite = 0;
                fInitRead   = 0;
                fBatch = 1;
                break;
                
            default:
                goto usage;
        }
    }
    
    if ( fBatch )
    {
		pMvsis->fBatchMode = 1;

        if (argc - util_optind == 0)
        {
            sInFile = NULL;
        }
        else if (argc - util_optind == 1)
        {
            fInitRead = 1;
            sInFile = argv[util_optind];
        }
        else
        {
            Mv_UtilsPrintUsage( pMvsis, argv[0] );
        }
        
        // source file "master.mvsisrc"
        if ( fInitSource )
        {
            Mv_UtilsSource( pMvsis );
        }
        
        fStatus = 0;
        if ( fInitRead && sInFile )
        {
            sprintf( sCommandTmp, "%s %s", sReadCmd, sInFile );
            fStatus = Cmd_CommandExecute( pMvsis, sCommandTmp );
        }
        
        if ( fStatus == 0 )
        {
            /* cmd line contains `source <file>' */
            fStatus = Cmd_CommandExecute( pMvsis, sCommandUsr );
            if ( (fStatus == 0 || fStatus == -1) && fFinalWrite && sOutFile )
            {
                sprintf( sCommandTmp, "%s %s", sWriteCmd, sOutFile );
                fStatus = Cmd_CommandExecute( pMvsis, sCommandTmp );
            }
        }
        
    }
    else
    {
        // start interactive mode
        // print the hello line
        Mv_UtilsPrintHello( pMvsis );
        
        // source file "master.mvsisrc"
        if ( fInitSource )
        {
            Mv_UtilsSource( pMvsis );
        }

	// Initialize readline completion for MVSIS commands

#if defined(HAVE_LIBREADLINE) && HAVE_LIBREADLINE==1

	pMvsisCommandCompletion = pMvsis;
	rl_attempted_completion_function = command_completion;

#endif
		        
        // execute commands given by the user
        while ( !feof(stdin) )
        {
            // print command line prompt and
            // get the command from the user
            sCommand = Mv_UtilsGetUsersInput( pMvsis );
            
            // execute the user's command
            fStatus = Cmd_CommandExecute( pMvsis, sCommand );
            
            // stop if the user quitted or an error occurred
            if ( fStatus == -1 || fStatus == -2 )
                break;
        }
    }
    
    // if the memory should be freed, quit packages
    if ( fStatus == -2 ) 
    {
        // perform uninitializations
        Mv_FrameEnd( pMvsis );
        // stop the MVSIS framework
        Mv_FrameDeallocate( pMvsis );
    }
    return 0;

usage:
    Mv_UtilsPrintHello( pMvsis );
    Mv_UtilsPrintUsage( pMvsis, argv[0] );
    return 1;
}


/**Function********************************************************************

  Synopsis    [Returns 1 if s is a file type recognized by MVSIS, else returns 0.]

  Description [Returns 1 if s is a file type recognized by VIS, else returns
  0. Recognized types are "blif", "blif_mv", "blif_mvs", and "none".]

  SideEffects []

******************************************************************************/
static int
TypeCheck(
    Mv_Frame_t * pMvsis,
    char       * s)
{
    if (strcmp(s, "blif") == 0) {
        return 1;
    }
    else if (strcmp(s, "blif_mv") == 0) {
        return 1;
    }
    else if (strcmp(s, "blif_mvs") == 0) {
        return 1;
    }
    else if (strcmp(s, "none") == 0) {
        return 1;
    }
    else {
        fprintf( pMvsis->Err, "unknown type %s\n", s );
        return 0;
    }
}

/**Functions*******************************************************************

  Synopsis    [Functions dealing with readline command completion in MVSIS]


******************************************************************************/

#if defined(HAVE_LIBREADLINE) && HAVE_LIBREADLINE==1

/* Attempt to complete on the contents of TEXT.  START and END
   bound the region of rl_line_buffer that contains the word to
   complete.  TEXT is the word to complete.  We can use the entire
   contents of rl_line_buffer in case we want to do some simple
   parsing.  Returnthe array of matches, or NULL if there aren't any. */
char **
command_completion (text, start, end)
     const char *text;
     int start, end;
{
  char **matches;

  matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
  {
    matches = completion_matches (text, command_generator);

    // READLINE docs say that I should use command below
    // but that function is not defined on present system
    // matches = rl_completion_matches (text, command_generator);
  }   

  return (matches);
}


/* Generator function for command completion.  STATE lets us
   know whether to start from scratch; without any state
   (i.e. STATE == 0), then we start at the top of the list. */
char *
command_generator (text, state)
     const char *text;
     int state;
{
  static int len;
  static avl_generator *gen;
  char *name;

  /* If this is a new word to complete, initialize now.  This
     includes saving the length of TEXT for efficiency, and
     initializing the index variable to 0. */
  if (!state)
    {
      len = strlen (text);
      if (gen) avl_free_gen(gen);
      gen = avl_init_gen(pMvsisCommandCompletion->tCommands,  AVL_FORWARD);
    }

  /* Return the next name which partially matches from the
     command list. */
  while (avl_gen(gen, &name, 0))
    {
      if (strncmp (name, text, len) == 0)
        return (util_strsav(name));
    }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


