/**CFile****************************************************************

  FileName    [mvnSeqDc.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Sequential manipulation on the level of the MV network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnSeqDc.c,v 1.5 2004/02/19 03:11:18 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Network_t * Ntk_NetworkCreateExdcNet( Ntk_Network_t * pNet, DdNode * bUnreach );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes unreachable states and writes them as EXDC network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkExtractSequentialDc( Ntk_Network_t * pNet, int fVerbose )
{
    DdManager * dd;
    DdNode * bUnreach;

    // remove EXDC network if present
    if ( pNet->pNetExdc )
    {
        Ntk_NetworkDelete( pNet->pNetExdc );
        pNet->pNetExdc = NULL; 
    }

    // compute the set of unreachable states
    bUnreach = Ntk_NetworkComputeUnreach( pNet, fVerbose );          Cudd_Ref( bUnreach );

    // create the EXDC network representing the unreachable states
    pNet->pNetExdc = Ntk_NetworkCreateExdcNet( pNet, bUnreach );

    // deref the set of unreachable states
    dd = Ntk_NetworkReadDdGlo( pNet );
    Cudd_RecursiveDeref( dd, bUnreach );

    Ntk_NetworkSetDdGlo( pNet, NULL );
    return 0;
}


/**Function*************************************************************

  Synopsis    [Derives the EXDC network from the set of unreachable states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Ntk_NetworkCreateExdcNet( Ntk_Network_t * pNet, DdNode * bUnreach )
{
    DdManager * ddDc;
    char * pName;
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNodeNew, * pNodeCi, * pNodeCo, * pDriverCo;
    Ntk_Node_t ** ppFanins;
    int nFanins, i;

    // get the old manager
    ddDc = Ntk_NetworkReadDdGlo( pNet );

    // get the CI nodes
    nFanins = Ntk_NetworkReadCiNum( pNet );
    ppFanins = ALLOC( Ntk_Node_t *, nFanins );
    i = 0;
    Ntk_NetworkForEachCi( pNet, pNodeCi )
        ppFanins[i++] = pNodeCi;
    assert( nFanins == i );

    // create the node with the function equal to the MDD of unreachable states
    pNodeNew = Ntk_NodeCreateFromMdd( pNet, ppFanins, nFanins, ddDc, bUnreach );
    FREE( ppFanins );


    // create the network from this node
//    pNetNew = Ntk_NetworkCreateFromNode( pNet->pMvsis, pNodeNew );
    pNetNew = Ntk_NetworkCreateFromMdd( pNodeNew );
    Ntk_NodeDelete( pNodeNew );

    // remove the topmost node of the network
    pNodeCo = Ntk_NetworkReadCoNode( pNetNew, 0 );
    pDriverCo = Ntk_NodeReadFaninNode( pNodeCo, 0 );
    Ntk_NetworkDeleteNode( pNetNew, pNodeCo, 1, 1 );

    // create CO nodes for each CO of the old network
    Ntk_NetworkForEachCo( pNet, pNodeCo )
    {
        // create the buffer
        pNodeNew = Ntk_NodeCreateOneInputBinary( pNetNew, pDriverCo, 0 );
        // add the node to the network
        Ntk_NetworkAddNode( pNetNew, pNodeNew, 1 );
        // set the name
        pName = Ntk_NetworkRegisterNewName( pNetNew, pNodeCo->pName );
        Ntk_NodeAssignName( pNodeNew, pName );
        // create the CO node
        Ntk_NetworkAddNodeCo( pNetNew, pNodeNew, 1 );
    }

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );

    // check the network
    if ( !Ntk_NetworkCheck( pNetNew ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNetNew), "Ntk_NetworkCreateExdcNet(): Network check has failed.\n" );
    return pNetNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


