/**CFile****************************************************************

  FileName    [mvInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Internal declarations of MVSIS main package.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvInt.h,v 1.14 2005/06/02 03:34:09 alanmi Exp $]

***********************************************************************/

#ifndef __MV_INT_H__
#define __MV_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// the current version of MVSIS
#ifdef BALM
#define MVSIS_VERSION "UC Berkeley, BALM 1.0"
#else
#define MVSIS_VERSION "UC Berkeley, MVSIS 2.0"
#endif

// the maximum length of an input line 
#define MAX_STR     32768

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct MvFrameStruct
{
    // general info about MVSIS
    char *          sVersion;    // the name of the current version of MVSIS
    // commands, aliases, etc
    avl_tree *      tCommands;   // the command table
    avl_tree *      tAliases;    // the alias table
    avl_tree *      tFlags;      // the flag table
    array_t *       aHistory;    // the command history
    // the functionality
    Ntk_Network_t * pNetCur;     // the current network
    int             nSteps;      // the counter of different network processed
    // when this flag is 1, the current command is executed in autoexec mode
    int             fAutoexac;  
    // output streams
    FILE *          Out;
    FILE *          Err;
    FILE *          Hst;
    // used for runtime measurement
    int             TimeCommand; // the runtime of the last command
    int             TimeTotal;   // the total runtime of all commands
	int				fBatchMode;	 // are we invoked in batch mode?
    // the functinality manager
    Fnc_Manager_t * pMan;      
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFITIONS                            ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/*=== mvMain.c ===========================================================*/
extern int             main( int argc, char * argv[] );
/*=== mvInit.c ===================================================*/
extern void            Mv_FrameInit( Mv_Frame_t * pMvsis );
extern void            Mv_FrameEnd( Mv_Frame_t * pMvsis );
/*=== mvFrame.c =====================================================*/
extern Mv_Frame_t *    Mv_FrameAllocate();
extern void            Mv_FrameDeallocate( Mv_Frame_t * p );
/*=== mvUtils.c =====================================================*/
extern char *          Mv_UtilsGetMvsisVersion( Mv_Frame_t * pMvsis );
extern char *          Mv_UtilsGetUsersInput( Mv_Frame_t * pMvsis );
extern void            Mv_UtilsPrintHello( Mv_Frame_t * pMvsis );
extern void            Mv_UtilsPrintUsage( Mv_Frame_t * pMvsis, char * ProgName );
extern void            Mv_UtilsSource( Mv_Frame_t * pMvsis );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

#endif
