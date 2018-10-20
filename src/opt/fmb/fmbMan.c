/**CFile****************************************************************

  FileName    [fmbApi.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [APIs of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbMan.c,v 1.3 2004/05/12 04:30:12 alanmi Exp $]

***********************************************************************/

#include "fmbInt.h"

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
Fmb_Manager_t * Fmb_ManagerStart( Ntk_Network_t * pNet, Fmb_Params_t * pPars )
{
    Fmb_Manager_t * p;

    // allocate the manager
    p = ALLOC( Fmb_Manager_t, 1 );
    memset( p, 0, sizeof(Fmb_Manager_t) );
    p->timeStart = clock();

    // set the parameters
    p->pPars = pPars;

    // start the managers
    p->ddGlo = Cudd_Init( 102, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

	// data related to the network
    p->pNet        = pNet;
    p->nArrayAlloc = 1000;
    p->pbArray     = ALLOC( DdNode *, p->nArrayAlloc );

    // save elementary variables
    Fmb_ManagerResize( p, 50, 50, FMB_ADD_BITS_NUM );
    return p;
}

/**Function*************************************************************

  Synopsis    [Resizes the don't-care manager.]

  Description [nGlo is the number of variables in the global space.
  nLoc is the number of variables in the largest local space. nExt is
  the number of variables in the output of the relation if the flexibility
  to be computed is a multi-output flexibility.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ManagerResize( Fmb_Manager_t * p, int nGlo, int nLoc, int nExt )
{
    int nTotal, i;

    if ( p->nBitsGlo < nGlo || p->nBitsLoc < nLoc || p->nBitsExt < nExt )
    {
        // resize the manager if necessary
        nTotal = nGlo + 2 * nExt + nLoc;
        if ( p->ddGlo->size < nTotal )
        {
            for ( i = p->ddGlo->size; i < nTotal; i++ )
                Cudd_bddNewVar( p->ddGlo );
        }

        // assign global variables    
        FREE( p->pbVars0 );
        p->pbVars0 = ALLOC( DdNode *, nGlo + nExt );
        for ( i = 0; i < nGlo + nExt; i++ )
            p->pbVars0[i] = p->ddGlo->vars[i];

        // assign local variables    
        FREE( p->pbVars1 );
        p->pbVars1 = ALLOC( DdNode *, nLoc + nExt );
        for ( i = 0; i < nLoc + nExt; i++ )
            p->pbVars1[i] = p->ddGlo->vars[nGlo + nExt + i];

        // set the new parameters
        p->nBitsGlo = nGlo;
        p->nBitsLoc = nLoc;
        p->nBitsExt = nExt;
    }
    // realloc array if necessary
    if ( p->nArrayAlloc < p->nBitsLoc )
    {
        p->pbArray = REALLOC( DdNode *, p->pbArray, 2 * p->nBitsLoc );
        p->nArrayAlloc = 2 * p->nBitsLoc;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ManagerStop( Fmb_Manager_t * p )
{
//    Fmb_ManagerPrintTimeStats( p );
    Extra_StopManager( p->ddGlo );
    FREE( p->pPars );
    FREE( p->pbArray );
    FREE( p->pbVars0 );
    FREE( p->pbVars1 );
    FREE( p );
}


/**Function*************************************************************

  Synopsis    [Prints the timing stats of network minimization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ManagerPrintTimeStats( Fmb_Manager_t * pMan )
{
    pMan->timeTotal = clock() - pMan->timeStart;
    pMan->timeOther = pMan->timeTotal - pMan->timeWnd - pMan->timeGlo - pMan->timeGloZ -
        pMan->timeFlex - pMan->timeImage - pMan->timeSopMin - pMan->timeResub - pMan->timeOther;

    PRT( "Window ",  pMan->timeWnd );
    PRT( "Global1",  pMan->timeGlo );
    PRT( "Global ",  pMan->timeGloZ );
    PRT( "Flex   ",  pMan->timeFlex );
    PRT( "Image  ",  pMan->timeImage );
    PRT( "SOP min",  pMan->timeSopMin );
    PRT( "Resub  ",  pMan->timeResub );
    PRT( "Other  ",  pMan->timeOther );
    PRT( "TOTAL  ",  pMan->timeTotal );
    PRT( "CDC    ",  pMan->timeCdc );
}

/**Function*************************************************************

  Synopsis    [Updates the global functions of the nodes in the network.]

  Description [This function should be called each time a node's local function
  is changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ManagerCleanCurrentWindow( Fmb_Manager_t * p )
{
    Ntk_Node_t * pNode;
    Ntk_Node_t ** ppLeaves;
    int nLeaves, i;

    nLeaves = Wn_WindowReadLeafNum( p->pWnd );
    ppLeaves = Wn_WindowReadLeaves( p->pWnd );
    for ( i = 0; i < nLeaves; i++ )
        Ntk_NodeGlobalClean( ppLeaves[i] );

    Wn_WindowDfs( p->pWnd );
    Ntk_NetworkForEachNodeSpecial( p->pNet, pNode )
        Ntk_NodeGlobalClean( pNode );

    Wn_WindowDelete( p->pWnd );
    p->pWnd = NULL;
}

/**Function*************************************************************

  Synopsis    [Sets the DCMN parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ManagerSetParameters( Fmb_Manager_t * p, Fmb_Params_t * pPars )
{
    p->pPars = pPars;
}


/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Fmb_ManagerReadDdGlo( Fmb_Manager_t * p )
{
    return p->ddGlo;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmb_ManagerReadVerbose( Fmb_Manager_t * p )
{
    return p->pPars->fVerbose;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ManagerAddToTimeSopMin( Fmb_Manager_t * p, int Time )
{
    p->timeSopMin += Time;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ManagerAddToTimeResub( Fmb_Manager_t * p, int Time )
{
    p->timeResub += Time;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_ManagerAddToTimeCdc( Fmb_Manager_t * p, int Time )
{
    p->timeCdc += Time;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


