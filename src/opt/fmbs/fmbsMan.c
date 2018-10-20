/**CFile****************************************************************

  FileName    [fmbsMan.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [Starting and stopping the manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbsMan.c,v 1.2 2004/10/18 01:39:23 alanmi Exp $]

***********************************************************************/

#include "fmbsInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fmbs_Manager_t * Fmbs_ManagerStart( Ntk_Network_t * pNet, Fmbs_Params_t * pPars )
{
    Fmbs_Manager_t * p;
    // allocate the manager
    p = ALLOC( Fmbs_Manager_t, 1 );
    memset( p, 0, sizeof(Fmbs_Manager_t) );
    p->timeStart = clock();
    // set the parameters
    p->pPars = pPars;
    // start the managers
    p->pShMan = Sh_ManagerCreate( 200, 100000, 0 );
    p->pSat   = Sat_SolverAlloc( pPars->nNodesMax+1, 1, 1, 1, 1, 0 );
	// data related to the network
    p->pNet          = pNet;
    p->nCareSetAlloc = 1000;
    p->pCareSet      = ALLOC( char, p->nCareSetAlloc );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmbs_ManagerStop( Fmbs_Manager_t * p )
{
//    assert( p->pShNet );
    Sh_ManagerFree( p->pShMan, 1 );
    Sat_SolverFree( p->pSat );
    FREE( p->pPars );
    FREE( p->pCareSet );
    FREE( p );
}



/**Function*************************************************************

  Synopsis    [Prints the timing stats of network minimization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmbs_ManagerPrintTimeStats( Fmbs_Manager_t * pMan )
{
    pMan->timeTotal = clock() - pMan->timeStart;
    pMan->timeOther = pMan->timeTotal - pMan->timeWnd - pMan->timeStr - 
        pMan->timeSim - pMan->timeSat - pMan->timeSopMin - pMan->timeResub;
    PRT( "Window ",  pMan->timeWnd );
    PRT( "Strash ",  pMan->timeStr );
    PRT( "Simul  ",  pMan->timeSim );
    PRT( "Sat    ",  pMan->timeSat );
    PRT( "SOP min",  pMan->timeSopMin );
    PRT( "Resub  ",  pMan->timeResub );
    PRT( "Other  ",  pMan->timeOther );
    PRT( "TOTAL  ",  pMan->timeTotal );
    PRT( "CDC    ",  pMan->timeCdc );
    PRT( "AIG    ",  pMan->timeAig );
}

/**Function*************************************************************

  Synopsis    [Sets the DCMN parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmbs_ManagerSetParameters( Fmbs_Manager_t * p, Fmbs_Params_t * pPars )
{
    p->pPars = pPars;
}

/**Function*************************************************************

  Synopsis    [Sets the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmbs_ManagerSetNetwork( Fmbs_Manager_t * p, Ntk_Network_t * pNet )
{
    p->pNet = pNet;
}


/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmbs_ManagerReadVerbose( Fmbs_Manager_t * p )
{
    return p->pPars->fVerbose;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmbs_ManagerReadNodesMax( Fmbs_Manager_t * p )
{
    return p->pPars->nNodesMax;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmbs_ManagerAddToTimeSopMin( Fmbs_Manager_t * p, int Time )
{
    p->timeSopMin += Time;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmbs_ManagerAddToTimeResub( Fmbs_Manager_t * p, int Time )
{
    p->timeResub += Time;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmbs_ManagerAddToTimeCdc( Fmbs_Manager_t * p, int Time )
{
    p->timeCdc += Time;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


