/**CFile****************************************************************

  FileName    [mvUtils.c]
 
  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Miscellaneous utilities.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvUtils.c,v 1.15 2005/06/02 03:34:09 alanmi Exp $]

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
static char * DateReadFromDateString(char * datestr);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Mv_UtilsGetMvsisVersion( Mv_Frame_t * pMvsis )
{
    static char Version[1000];

#ifdef WIN32
    (void) sprintf(Version, "%s (compiled %s %s)", MVSIS_VERSION, __DATE__, __TIME__);
#else
    (void) sprintf(Version, "%s (compiled %s)", MVSIS_VERSION, DateReadFromDateString(CUR_DATE));
#endif

    return Version;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Mv_UtilsGetUsersInput( Mv_Frame_t * pMvsis )
{
    static char Buffer[1000];
	static char Prompt[1000];

#if defined(HAVE_LIBREADLINE) && HAVE_LIBREADLINE==1
	char *line;
#endif

#ifdef BALM
    sprintf( Prompt, "balm %02d> ", pMvsis->nSteps );
#else
    sprintf( Prompt, "mvsis %02d> ", pMvsis->nSteps );
#endif

#if defined(HAVE_LIBREADLINE) && HAVE_LIBREADLINE==1
	
	line = readline(Prompt);
	if ((line != NULL) && (*line != 0))
	{
		add_history (line);
		strncpy (Buffer, line, 999);
		free(line);
	}
	else
	{
		Buffer[0] = 0;
	}

#else /* No readline */
	
    fprintf( pMvsis->Out, "%s", Prompt );
    fgets( Buffer, 999, stdin );

#endif

    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_UtilsPrintHello( Mv_Frame_t * pMvsis )
{
    fprintf( pMvsis->Out, "%s\n", pMvsis->sVersion );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_UtilsPrintUsage( Mv_Frame_t * pMvsis, char * ProgName )
{
    fprintf( pMvsis->Err, "\n" );
    fprintf( pMvsis->Err,
             "usage: %s [-c cmd] [-f script] [-h] [-o file] [-s] [-t type] [-T type] [-x] [file]\n", 
             ProgName);
    fprintf( pMvsis->Err,
             "    -c cmd\texecute MVSIS commands `cmd'\n");
    fprintf( pMvsis->Err,
             "    -F script\texecute MVSIS commands from a script file and echo commands\n");
    fprintf( pMvsis->Err,
             "    -f script\texecute MVSIS commands from a script file\n");
    fprintf( pMvsis->Err,
             "    -h\t\tprint the command usage\n");
    fprintf( pMvsis->Err,
             "    -o file\tspecify output filename to store the result\n");
    fprintf( pMvsis->Err,
             "    -s\t\tdo not read any initialization file\n");
    fprintf( pMvsis->Err,
             "    -t type\tspecify input type (blif_mv (default), blif_mvs, blif, or none)\n");
    fprintf( pMvsis->Err,
             "    -T type\tspecify output type (blif_mv (default), blif_mvs, blif, or none)\n");
    fprintf( pMvsis->Err,
             "    -x\t\tequivalent to '-t none -T none'\n");
    fprintf( pMvsis->Err, "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_UtilsSource( Mv_Frame_t * pMvsis )
{  
    /*
     * If .mvsisrc is present in both the home and current directories, then read
     * it from the home directory.  Otherwise, read it from wherever it's
     * located.
     */
#ifdef BALM
    
#ifdef WIN32
    if ( Cmd_CommandExecute(pMvsis, "source balm.rc") )
    {
        if ( Cmd_CommandExecute(pMvsis, "source ..\\balm.rc") == 0 )
            printf( "Loaded \"balm.rc\" from the parent directory.\n" );
        else if ( Cmd_CommandExecute(pMvsis, "source ..\\..\\balm.rc") == 0 )
            printf( "Loaded \"balm.rc\" from the grandparent directory.\n" );
    }
#else
    {
    char * sPath1, * sPath2;
    /*
     * Look in home directory and current directory for .mvsisrc.
     */
    sPath1 = util_file_search(".mvsisrc", "~/", "r");
    sPath2 = util_file_search(".mvsisrc", ".",  "r");
    
    if ( sPath1 && sPath2 ) {
        /* ~/.mvsisrc == .mvsisrc : Source the file only once */
        (void) Cmd_CommandExecute(pMvsis, "source -s ~/.rc");
    }
    else {
        if (sPath1) {
            (void) Cmd_CommandExecute(pMvsis, "source -s ~/.rc");
        }
        if (sPath2) {
            (void) Cmd_CommandExecute(pMvsis, "source -s .rc");
        }
    }
    if ( sPath1 ) FREE(sPath1);
    if ( sPath2 ) FREE(sPath2);
    
    /* execute the master script which can be open with the "open_path" */
    Cmd_CommandExecute( pMvsis, "source -s balm.rc" );
    }
#endif //WIN32
    

#else
    
#ifdef WIN32
    if ( Cmd_CommandExecute(pMvsis, "source master.mvsisrc") )
    {
        if ( Cmd_CommandExecute(pMvsis, "source ..\\master.mvsisrc") == 0 )
            printf( "Loaded \"master.mvsisrc\" from the parent directory.\n" );
        else if ( Cmd_CommandExecute(pMvsis, "source ..\\..\\master.mvsisrc") == 0 )
            printf( "Loaded \"master.mvsisrc\" from the grandparent directory.\n" );
    }
#else
    {
    char * sPath1, * sPath2;
    /*
     * Look in home directory and current directory for .mvsisrc.
     */
    sPath1 = util_file_search(".mvsisrc", "~/", "r");
    sPath2 = util_file_search(".mvsisrc", ".",  "r");
    
    if ( sPath1 && sPath2 ) {
        /* ~/.mvsisrc == .mvsisrc : Source the file only once */
        (void) Cmd_CommandExecute(pMvsis, "source -s ~/.mvsisrc");
    }
    else {
        if (sPath1) {
            (void) Cmd_CommandExecute(pMvsis, "source -s ~/.mvsisrc");
        }
        if (sPath2) {
            (void) Cmd_CommandExecute(pMvsis, "source -s .mvsisrc");
        }
    }
    if ( sPath1 ) FREE(sPath1);
    if ( sPath2 ) FREE(sPath2);
    
    /* execute the master script which can be open with the "open_path" */
    Cmd_CommandExecute( pMvsis, "source -s master.mvsisrc" );
    }
#endif //WIN32
    
#endif

    return;
}

/**Function********************************************************************

  Synopsis    [Returns the date in a brief format assuming its coming from
  the program `date'.]

  Description [optional]

  SideEffects []

******************************************************************************/
char *
DateReadFromDateString(
  char * datestr)
{
  static char result[25];
  char        day[10];
  char        month[10];
  char        zone[10];
  char       *at;
  int         date;
  int         hour;
  int         minute;
  int         second;
  int         year;

  if (sscanf(datestr, "%s %s %2d %2d:%2d:%2d %s %4d",
             day, month, &date, &hour, &minute, &second, zone, &year) == 8) {
    if (hour >= 12) {
      if (hour >= 13) hour -= 12;
      at = "PM";
    }
    else {
      if (hour == 0) hour = 12;
      at = "AM";
    }
    (void) sprintf(result, "%d-%3s-%02d at %d:%02d %s", 
                   date, month, year % 100, hour, minute, at);
    return result;
  }
  else {
    return datestr;
  }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


