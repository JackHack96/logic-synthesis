/**CFile****************************************************************

  FileName    [cmdInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of the command package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cmdInt.h,v 1.11 2003/05/27 23:14:12 alanmi Exp $]

***********************************************************************/

#ifndef __CMD_INT_H__
#define __CMD_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "cmd.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct MvCommand
{
    char *        sName;       // the command name  
    char *        sGroup;      // the group name  
    void *        pFunc;       // the function to execute the command 
    int           fChange;     // set to 1 to mark that the network is changed
};

struct MvAlias
{
    char *        sName;       // the alias name
    int           argc;        // the number of alias parts
    char **       argv;        // the alias parts
};

typedef struct {
    unsigned	fSnapshot	:1;
    unsigned	fCommand	:1;
    unsigned	id		:30;
    union {
	Ntk_Network_t	*snapshot;
	char		*string;
    } data;
} CmdHistory_t;

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== cmdAlias.c =============-========================================*/
extern void       CmdCommandAliasAdd( Mv_Frame_t * pMvsis, char * sName, int argc, char ** argv );
extern void       CmdCommandAliasPrint( Mv_Frame_t * pMvsis, Mv_Alias * pAlias );
extern char *     CmdCommandAliasLookup( Mv_Frame_t * pMvsis, char * sCommand );
extern void       CmdCommandAliasFree( Mv_Alias * p );
/*=== cmdUtils.c =======================================================*/
extern int        CmdCommandDispatch( Mv_Frame_t * pMvsis, int  argc, char ** argv );
extern char *     CmdSplitLine( Mv_Frame_t * pMvsis, char * sCommand, int * argc, char *** argv );
extern int        CmdApplyAlias( Mv_Frame_t * pMvsis, int * argc, char *** argv, int * loop );
extern char *     CmdHistorySubstitution( Mv_Frame_t * pMvsis, char * line, int * changed );
extern FILE *     CmdFileOpen( Mv_Frame_t * pMvsis, char * sFileName, char * sMode, char ** pFileNameReal, int silent );
extern void       CmdFreeArgv( int argc, char ** argv );
extern void       CmdCommandFree( Mv_Command * pCommand );
extern void       CmdCommandPrint( Mv_Frame_t * pMvsis, bool fPrintAll );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

