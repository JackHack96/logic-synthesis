/**CFile****************************************************************

  FileName    [cmd.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [External declarations of the command package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cmd.h,v 1.12 2003/05/27 23:14:10 alanmi Exp $]

***********************************************************************/

#ifndef __CMD_H__
#define __CMD_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct MvCommand    Mv_Command;  // one command
typedef struct MvAlias      Mv_Alias;    // one alias

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== cmd.c ===========================================================*/
extern void     Cmd_Init();
extern void     Cmd_End();
/*=== cmdApi.c ========================================================*/
extern void     Cmd_CommandAdd( Mv_Frame_t * pMvsis, char * sGroup, char * sName, void * pFunc, int fChanges );
extern int      Cmd_CommandExecute( Mv_Frame_t * pMvsis, char * sCommand );
extern int      Cmd_CommandGetNodes( Mv_Frame_t * pMvsis, int argc, char ** argv, int fCollectCis );
/*=== cmdFlag.c ========================================================*/
extern char *   Cmd_FlagReadByName( Mv_Frame_t * pMvsis, char * flag );
extern void     Cmd_FlagDeleteByName( Mv_Frame_t * pMvsis, char * key );
extern void     Cmd_FlagUpdateValue( Mv_Frame_t * pMvsis, char * key, char * value );
/*=== cmdHist.c ========================================================*/
extern void	Cmd_HistoryAddCommand( Mv_Frame_t *p, char *command );
extern void	Cmd_HistoryAddSnapshot( Mv_Frame_t *p, Ntk_Network_t *net );
extern Ntk_Network_t* Cmd_HistoryGetSnapshot( Mv_Frame_t *p, int i);
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif

