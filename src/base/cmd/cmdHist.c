/**CFile****************************************************************

  FileName    [cmdHist.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Command history.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: cmdHist.c,v 1.4 2003/11/18 18:54:51 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "mvInt.h"
#include "cmd.h"
#include "cmdInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void
Cmd_HistoryAddCommand(
	Mv_Frame_t *p,
	char *command)
{
    CmdHistory_t cmd;
    cmd.fSnapshot   = 0;
    cmd.fCommand    = 1;
    cmd.data.string = util_strsav(command);
    array_insert_last(CmdHistory_t, p->aHistory, cmd);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void
Cmd_HistoryAddSnapshot(
	Mv_Frame_t *p,
	Ntk_Network_t *net)
{
    int i;
    CmdHistory_t cmd, prev;
    cmd.fSnapshot   = 1;
    cmd.fCommand    = 0;
    cmd.id	    = 1;
    /* find the previous id if it exists */ {
	for (i=array_n(p->aHistory)-1; i>=0; i--) {
	    prev = array_fetch(CmdHistory_t, p->aHistory, i);
	    if (!prev.fSnapshot) continue;
	    cmd.id = prev.id+1;
	    break;
	}
    }
    cmd.data.snapshot = Ntk_NetworkDup(p->pNetCur, Ntk_NetworkReadMan(p->pNetCur));
    array_insert_last(CmdHistory_t, p->aHistory, cmd);

    /* delete extra snapshots if desired */ 
    if (Cmd_FlagReadByName(p, "maxundo")) {
	int snapmax = atoi(Cmd_FlagReadByName(p, "maxundo"));
	for (i=array_n(p->aHistory)-1; i>=0; i--) {
	    prev = array_fetch(CmdHistory_t, p->aHistory, i);
	    if (!prev.fSnapshot) continue;
	    snapmax--;
	    if (snapmax >= 0) continue;
	    prev.fSnapshot = 0;
	    Ntk_NetworkDelete(prev.data.snapshot);
	    array_insert(CmdHistory_t, p->aHistory, i, prev);
	}
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t*
Cmd_HistoryGetSnapshot(
	Mv_Frame_t *p,
	int i)
{
    int j;
    for (j=0; j<array_n(p->aHistory); j++) {
	CmdHistory_t snap = array_fetch(CmdHistory_t, p->aHistory, j);
	if (snap.fSnapshot && snap.id==(unsigned)i)
	    return snap.data.snapshot;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#if 0
void
Cmd_HistoryPrintItem(
	Mv_Frame_t *p,
	int i)
{
    CmdHistory_t cmd = array_fetch(CmdHistory_t, p->aHistory, i);
}
#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
