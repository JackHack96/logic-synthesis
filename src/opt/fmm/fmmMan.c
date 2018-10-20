/**CFile****************************************************************

  FileName    [fmmMan.c]

  PackageName [Multi-valued flexibility manager.]

  Synopsis    [APIs of the flexibility manager.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmmMan.c,v 1.3 2003/12/22 02:03:30 alanmi Exp $]

***********************************************************************/

#include "fmmInt.h"

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
Fmm_Manager_t * Fmm_ManagerStart( Ntk_Network_t * pNet, Fmm_Params_t * pPars )
{
    Fmm_Manager_t * p;
    Ntk_Network_t * pNetSpec;
    Ntk_Node_t * pNode, * pNodeSpec;
    DdNode ** pbFuncs;
    bool fDropInternal = 0;  // need the intermediate functions
    bool fLatchOnly    = 0;  // need all network, not only latches
    bool fVerbose      = 0;  // no verbose output at this point
    int i;

    // allocate the manager
    p = ALLOC( Fmm_Manager_t, 1 );
    memset( p, 0, sizeof(Fmm_Manager_t) );
    p->timeStart = clock();

    // set the parameters
    p->pPars = pPars;

    // if the network has a spec, read get it first
    p->timeTemp = clock();
    if ( !p->pPars->fBinary && pNet->pSpec )
    {
        // derive the global BDDs in the spec network
        printf( "Using external spec \"%s\".\n", pNet->pSpec );
        pNetSpec = Io_ReadNetwork( Ntk_NetworkReadMvsis(pNet), pNet->pSpec );

        // free the old global manager if necessary
        Ntk_NetworkSetDdGlo( pNetSpec, NULL );
        // compute the global functions
        Ntk_NetworkGlobalCompute( pNetSpec, fDropInternal, p->pPars->fDynEnable, fLatchOnly, fVerbose );

        // update the global BDDs of the primary outputs of the spec network
        // using the EXDC of the spec network if present; the result is in GloS
        Fmm_UtilsPrepareSpec( pNetSpec );

        // copy the global functions of the spec network into the main network
        Ntk_NetworkForEachCo( pNetSpec, pNodeSpec )
        {
            // get the spec functionality
            pbFuncs = Ntk_NodeReadFuncGlobS( pNodeSpec );
            // get the corresponding node in the main network
            pNode = Ntk_NetworkFindNodeByName( pNet, pNodeSpec->pName );
            // set the spec functionality
            Ntk_NodeWriteFuncGlobS( pNode, pbFuncs );
        }

        // transfer the PI variables by name from spec to the network
        // this will also transfer the manager
        Ntk_NetworkGlobalSetCis( pNetSpec, pNet );

        // now the manager is saved in pNet, can safely clean it from pNetSpec
        Ntk_NetworkWriteDdGlo( pNetSpec, NULL );

        // compute the global functions of the primary network
        Ntk_NetworkGlobalComputeOne( pNet, 0, 0,  p->pPars->fDynEnable, 0, p->pPars->fVerbose );

        // delete the spec network (should also clean it)
        Ntk_NetworkDelete( pNetSpec );
    }
    else // the spec is not given or is not used
    {    
        // free the old global manager if necessary
        Ntk_NetworkSetDdGlo( pNet, NULL );
        // compute the global functions of the primary network
        Ntk_NetworkGlobalCompute( pNet, fDropInternal, p->pPars->fDynEnable, fLatchOnly, p->pPars->fVerbose );
        // update the global BDDs of the primary outputs of the network
        // using the EXDC of the spec network if present; the result is in GloS
        Fmm_UtilsPrepareSpec( pNet );
    }
    // free the EXDC if present
    if ( pNet->pNetExdc )
    {
        Ntk_NetworkGlobalClean( pNet->pNetExdc );
        Ntk_NetworkWriteDdGlo( pNet->pNetExdc, NULL );
    }

    // mark the runtime of global BDD computation
    p->timeGlo = clock() - p->timeTemp;
  
    // get the global variable maps
    p->pVmx  = Ntk_NetworkReadVmxGlo(pNet);
    p->pVm   = Vmx_VarMapReadVm(p->pVmx);

    // find the largest local space and the size of the global space
    p->nBitsLoc = Fmm_UtilsFaninMddSupportMax( pNet ) + FMM_MAX_FANIN_GROWTH;
    p->nBitsGlo = Vmx_VarMapReadBitsNum(p->pVmx); 

    // start the managers
//    p->ddLoc = Cudd_Init( p->nBitsLoc, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    p->ddGlo = Ntk_NetworkReadDdGlo(pNet);

	// data related to the network
    p->pNet        = pNet;
    p->nArrayAlloc = 1000;
    p->pbArray     = ALLOC( DdNode *, p->nArrayAlloc );

    // save elementary variables
    // save the currently allocated global variables
    assert( p->nBitsGlo == p->ddGlo->size );
    p->pbVars0 = ALLOC( DdNode *, p->nBitsGlo + FMM_ADD_BITS_NUM );
    for ( i = 0; i < p->nBitsGlo; i++ )
        p->pbVars0[i] = p->ddGlo->vars[i];
    // extend the global manager for the additional variables
    for ( i = 0; i < FMM_ADD_BITS_NUM; i++ )
        p->pbVars0[p->nBitsGlo+i] = Cudd_bddNewVar( p->ddGlo );
    // create the second set of variables
    p->pbVars1 = ALLOC( DdNode *, p->nBitsLoc );
    for ( i = 0; i < p->nBitsLoc; i++ )
        p->pbVars1[i] = Cudd_bddNewVar( p->ddGlo );
    return p;
}

/**Function*************************************************************

  Synopsis    [Resizes the don't-care manager to work with larger nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmm_ManagerResize( Fmm_Manager_t * p, int nFaninMax )
{
    int OverIncrease = 10;
    int i;
//    assert( p->ddLoc->size == p->nBitsLoc );
    assert( p->ddGlo->size == p->nBitsGlo + FMM_ADD_BITS_NUM + p->nBitsLoc );
    // check whether extension is necessary
    if ( p->nBitsLoc < nFaninMax )
    {
/*
        // extend the local manager
        for ( i = p->ddLoc->size; i < nFaninMax + OverIncrease; i++ )
            Cudd_bddNewVar( p->ddLoc );
        // recreate the ZDD variables
        Cudd_zddVarsFromBddVars( p->ddLoc, 2 );
*/
        // expand the global manager
        for ( i = p->ddGlo->size; i < p->nBitsGlo + FMM_ADD_BITS_NUM + nFaninMax + OverIncrease; i++ )
            Cudd_bddNewVar( p->ddGlo );
        // create a new variable array
        FREE( p->pbVars1 );
        p->pbVars1 = ALLOC( DdNode *, nFaninMax + OverIncrease );
        for ( i = 0; i < nFaninMax + OverIncrease; i++ )
            p->pbVars1[i] = p->ddGlo->vars[p->nBitsGlo + FMM_ADD_BITS_NUM + i];
        // update the manager size
        p->nBitsLoc = nFaninMax + OverIncrease;
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
void Fmm_ManagerStop( Fmm_Manager_t * p )
{
    // deref the global EXDCs
    if ( p->pbExdcs )
        Ntk_NodeGlobalDerefFuncs( p->ddGlo, p->pbExdcs, p->nExdcs );
    // clean the network
    Ntk_NetworkGlobalClean( p->pNet );
    // stop the managers
//printf( "Local manager: \n" );
//    Extra_StopManager( p->ddLoc );
//printf( "Global manager:\n" );
//    Extra_StopManager( p->ddGlo );
    Ntk_NetworkSetDdGlo( p->pNet, NULL ); // stops the global manager
    FREE( p->pPars );
    FREE( p->pbExdcs );
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
void Fmm_ManagerPrintTimeStats( Fmm_Manager_t * pMan )
{
    pMan->timeTotal = clock() - pMan->timeStart;
    pMan->timeOther = pMan->timeTotal - pMan->timeGlo - pMan->timeGloZ - 
        pMan->timeFlex - pMan->timeImage - pMan->timeSopMin - 
        pMan->timeResub - pMan->timeOther;
    PRT( "Global1 ",  pMan->timeGlo );
    PRT( "Global  ",  pMan->timeGloZ );
    PRT( "Flex    ",  pMan->timeFlex );
    PRT( "Image   ",  pMan->timeImage );
    PRT( "SOP min ",  pMan->timeSopMin );
    PRT( "Resub   ",  pMan->timeResub );
    PRT( "Other   ",  pMan->timeOther );
    PRT( "TOTAL   ",  pMan->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Sets the DCMN parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmm_ManagerSetParameters( Fmm_Manager_t * p, Fmm_Params_t * pPars )
{
    p->pPars = pPars;
}


/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Fmm_ManagerReadDdGlo( Fmm_Manager_t * p )
{
    return p->ddGlo;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmm_ManagerReadVerbose( Fmm_Manager_t * p )
{
    return p->pPars->fVerbose;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmm_ManagerAddToTimeSopMin( Fmm_Manager_t * p, int Time )
{
    p->timeSopMin += Time;
}

/**Function*************************************************************

  Synopsis    [Returns the global manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmm_ManagerAddToTimeResub( Fmm_Manager_t * p, int Time )
{
    p->timeResub += Time;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


