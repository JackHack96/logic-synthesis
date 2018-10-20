/**CFile****************************************************************

  FileName    [mvFrame.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvFrame.c,v 1.19 2005/01/23 07:11:41 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "mvInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Mv_Frame_t * Mv_FrameGlobalFrame = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mv_Frame_t * Mv_FrameAllocate()
{
    Mv_Frame_t * p;

    // allocate and clean
    p = ALLOC( Mv_Frame_t, 1 );
    memset( p, 0, sizeof(Mv_Frame_t) );
    // get version
    p->sVersion = Mv_UtilsGetMvsisVersion( p );
    // set streams
    p->Err = stderr;
    p->Out = stdout;
    p->Hst = NULL;
    // start the functinality manager
    p->pMan = Fnc_ManagerAllocate();
    // set the starting step
    p->nSteps = 1;
	p->fBatchMode = 0;
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameDeallocate( Mv_Frame_t * p )
{
    Mv_FrameDeleteAllNetworks( p );
    // stop the functinality manager
    Fnc_ManagerDeallocate( p->pMan );
    free( p );
    p = NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameRestart( Mv_Frame_t * p )
{
    Fnc_ManagerDeallocate( p->pMan );
    p->pMan = Fnc_ManagerAllocate();
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Mv_FrameReadNet( Mv_Frame_t * p )
{
    return p->pNetCur;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
FILE * Mv_FrameReadOut( Mv_Frame_t * p )
{
    return p->Out;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
FILE * Mv_FrameReadErr( Mv_Frame_t * p )
{
    return p->Err;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mv_FrameReadMode( Mv_Frame_t * p )
{
    int fShortNames;
    char * pValue;
    pValue = Cmd_FlagReadByName( p, "namemode" );
    if ( pValue == NULL )
        fShortNames = 0;
    else 
        fShortNames = atoi(pValue);
    return fShortNames;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fnc_Manager_t * Mv_FrameReadMan( Mv_Frame_t * p )
{
    return p->pMan;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Mv_FrameSetMode( Mv_Frame_t * p, bool fNameMode )
{
    char Buffer[2];
    bool fNameModeOld;
    fNameModeOld = Mv_FrameReadMode( p );
    Buffer[0] = '0' + fNameMode;
    Buffer[1] = 0;
    Cmd_FlagUpdateValue( p, "namemode", (char *)Buffer );
    return fNameModeOld;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fnc_Manager_t * Mv_FrameSetMan( Mv_Frame_t * p, Fnc_Manager_t * pMan )
{
    Fnc_Manager_t * pManOld;
    pManOld = p->pMan;
    p->pMan = pMan;
    return pManOld;
}



/**Function*************************************************************

  Synopsis    [Sets the given network to be the current one.]

  Description [Takes the network and makes it the current MVSIS network.
  The previous current network is attached to the given network as 
  a backup copy. In the stack of backup networks contains too many
  networks (defined by the paramater "savesteps"), the bottom
  most network is deleted.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameSetCurrentNetwork( Mv_Frame_t * p, Ntk_Network_t * pNetNew )
{
    Ntk_Network_t * pNet, * pNet2, * pNet3;
    int nNetsPresent;
    int nNetsToSave;
    char * pValue;

    // link it to the previous network
    Ntk_NetworkSetBackup( pNetNew, p->pNetCur );
    // set the step of this network
    Ntk_NetworkSetStep( pNetNew, ++p->nSteps );
    // set this network to be the current network
    p->pNetCur = pNetNew;

    // remove any extra network that may happen to be in the stack
    pValue = Cmd_FlagReadByName( p, "savesteps" );
    // if the value of steps to save is not set, assume 1-level undo
    if ( pValue == NULL )
        nNetsToSave = 1;
    else 
        nNetsToSave = atoi(pValue);
    
    // count the network, remember the last one, and the one before the last one
    nNetsPresent = 0;
    pNet2 = pNet3 = NULL;
    for ( pNet = p->pNetCur; pNet; pNet = Ntk_NetworkReadBackup(pNet2) )
    {
        nNetsPresent++;
        pNet3 = pNet2;
        pNet2 = pNet;
    }

    // remove the earliest backup network if it is more steps away than we store
    if ( nNetsPresent - 1 > nNetsToSave )
    { // delete the last network
        Ntk_NetworkDelete( pNet2 );
        // clean the pointer of the network before the last one
        Ntk_NetworkSetBackup( pNet3, NULL );
    }
}

/**Function*************************************************************

  Synopsis    [This procedure swaps the current and the backup network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameSwapCurrentAndBackup( Mv_Frame_t * p )
{
    Ntk_Network_t * pNetCur, * pNetBack, * pNetBack2;
    int iStepCur, iStepBack;

    pNetCur  = p->pNetCur;
    pNetBack = Ntk_NetworkReadBackup( pNetCur );
    iStepCur = Ntk_NetworkReadStep  ( pNetCur );

    // if there is no backup nothing to reset
    if ( pNetBack == NULL )
        return;

    // remember the backup of the backup
    pNetBack2 = Ntk_NetworkReadBackup( pNetBack );
    iStepBack = Ntk_NetworkReadStep  ( pNetBack );

    // set pNetCur to be the next after the backup's backup
    Ntk_NetworkSetBackup( pNetCur, pNetBack2 );
    Ntk_NetworkSetStep  ( pNetCur, iStepBack );

    // set pNetCur to be the next after the backup
    Ntk_NetworkSetBackup( pNetBack, pNetCur );
    Ntk_NetworkSetStep  ( pNetBack, iStepCur );

    // set the current network
    p->pNetCur = pNetBack;
}


/**Function*************************************************************

  Synopsis    [Replaces the current network by the given one.]

  Description [This procedure does not modify the stack of saved
  networks.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameReplaceCurrentNetwork( Mv_Frame_t * p, Ntk_Network_t * pNet )
{
    if ( pNet == NULL )
        return;

    // transfer the parameters to the new network
    if ( p->pNetCur )
    {
        Ntk_NetworkSetBackup( pNet, Ntk_NetworkReadBackup(p->pNetCur) );
        Ntk_NetworkSetStep( pNet, Ntk_NetworkReadStep(p->pNetCur) );
        // delete the current network
        Ntk_NetworkDelete( p->pNetCur );
    }
    else
    {
        Ntk_NetworkSetBackup( pNet, NULL );
        Ntk_NetworkSetStep( pNet, ++p->nSteps );
    }
    // set the new current network
    p->pNetCur = pNet;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameDeleteAllNetworks( Mv_Frame_t * p )
{
    Ntk_Network_t * pNet, * pNet2;
    // delete all the currently saved networks
    for ( pNet  = p->pNetCur, 
          pNet2 = pNet? Ntk_NetworkReadBackup(pNet): NULL; 
          pNet; 
          pNet  = pNet2, 
          pNet2 = pNet? Ntk_NetworkReadBackup(pNet): NULL )
        Ntk_NetworkDelete( pNet );
    // set the current network empty
    p->pNetCur = NULL;
    fprintf( p->Out, "All networks have been deleted.\n" );
}

/**Function*************************************************************

  Synopsis    [Removes library binding of all currently stored networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameFreeNetworkBindings( Mv_Frame_t * p )
{
    Ntk_Network_t * pNet;
    for ( pNet = p->pNetCur; pNet; pNet = Ntk_NetworkReadBackup(pNet) )
        Ntk_NetworkFreeBinding( pNet );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mv_FrameSetGlobalFrame( Mv_Frame_t * p )
{
	Mv_FrameGlobalFrame = p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mv_Frame_t * Mv_FrameGetGlobalFrame()
{
	if ( Mv_FrameGlobalFrame == 0 )
	{
		// start the MVSIS framework
		Mv_FrameGlobalFrame = Mv_FrameAllocate();
		// perform initializations
		Mv_FrameInit( Mv_FrameGlobalFrame );
	}
	return Mv_FrameGlobalFrame;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


