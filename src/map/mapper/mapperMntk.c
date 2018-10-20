/**CFile****************************************************************

  FileName    [mapperMntk.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mapperMntk.c,v 1.4 2005/07/08 01:01:25 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"
#include "mntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// macros to access the structurally hashed representation of the node
#define MTNK_NODE_READ0(pMapNode)      ((Mntk_Node_t *)pMapNode->pData0)
#define MTNK_NODE_READ1(pMapNode)      ((Mntk_Node_t *)pMapNode->pData1)
#define MTNK_NODE_WRITE0(pMapNode,p)   (pMapNode->pData0 = (char *)p)
#define MTNK_NODE_WRITE1(pMapNode,p)   (pMapNode->pData1 = (char *)p)

static void Map_ConvertMappingToMntkNode( Mntk_Man_t * p, Map_Node_t * pNodeMap, int Phase, Mio_Gate_t * pGateInv );
static Mntk_Node_t * Map_ConvertMappingToMntkPhase( Mntk_Man_t * p, Map_Node_t * pNodeMap, int Phase );
static Mntk_Node_t * Map_ConvertMappingToMntkSuper_rec( Mntk_Man_t * p, Map_Super_t * pSuper, Mntk_Node_t * pNodePis[], int nNodePis );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transforms the mapping currently defined into Mntk network.]

  Description [This procedure assumes that the reference counts are set
  corrently.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Man_t * Map_ConvertMappingToMntk( Map_Man_t * pMan )
{
    Mntk_Man_t * p;
    Mntk_Node_t * pNodeMntk;
    Map_Node_t * pNodeMap;
    Mio_Gate_t * pGateInv = Mio_LibraryReadInv(Map_ManReadGenLib(pMan));
    char ** ppOutputNames;
    float AreaTemp;
    int i;

    // set the correct reference counts
//    Map_MappingSetRefs( pMan );

    // start the new manager
    p = Mntk_ManStart( pMan->nInputs, pMan->nOutputs, pMan->vAnds->nSize, pMan->fVerbose );

    // prepare the data fields for writing
    Map_ManCleanData( pMan );

    // set the constant node
    MTNK_NODE_WRITE1( pMan->pConst1, p->pConst1 );
    // add the inverter if needed
    pNodeMntk = Mntk_NodeAlloc( p, &(p->pConst1), 1, pGateInv );
    Mntk_NodeRegister( pNodeMntk );
    MTNK_NODE_WRITE0( pMan->pConst1, pNodeMntk );
    // set the primary inputs
    for ( i = 0; i < pMan->nInputs; i++ )
    {
        MTNK_NODE_WRITE1( pMan->pInputs[i], p->pInputs[i] );
        // add the inverter if needed
        if ( pMan->pInputs[i]->nRefAct[0] > 0 )
        {
            pNodeMntk = Mntk_NodeAlloc( p, &(p->pInputs[i]), 1, pGateInv );
            Mntk_NodeRegister( pNodeMntk );
            MTNK_NODE_WRITE0( pMan->pInputs[i], pNodeMntk );
        }
    }

    // go through the internal nodes
    for ( i = 0; i < pMan->vAnds->nSize; i++ )
    {
        pNodeMap = pMan->vAnds->pArray[i];
        if ( pNodeMap->nRefAct[0] > 0 )
            Map_ConvertMappingToMntkNode( p, pNodeMap, 0, pGateInv );
        if ( pNodeMap->nRefAct[1] > 0 )
            Map_ConvertMappingToMntkNode( p, pNodeMap, 1, pGateInv );
    }

    // set the PO nodes
    for ( i = 0; i < pMan->nOutputs; i++ )
        if ( Map_IsComplement(pMan->pOutputs[i]) )
            p->pOutputs[i] = MTNK_NODE_READ0( Map_Regular(pMan->pOutputs[i]) );
        else
            p->pOutputs[i] = MTNK_NODE_READ1( Map_Regular(pMan->pOutputs[i]) );

    // set the output names
    ppOutputNames = ALLOC( char *, pMan->nOutputs );
    for ( i = 0; i < pMan->nOutputs; i++ )
        ppOutputNames[i] = pMan->ppOutputNames[i];
    Mntk_ManSetOutputNames( p, ppOutputNames );




    // compute the static area
    p->fAreaStatic = Mntk_ComputeStaticArea( p );

    // create the pseudo-node whose fanins are POs
    p->pPseudo = Mntk_NodeAlloc( p, p->pOutputs, p->nOutputs, Mio_GateCreatePseudo(p->nOutputs) );
    p->pPseudo->nRefs[1] = 1;

    // create fanouts and references (also computes area)
    p->fAreaGlo = Mntk_NodeInsertFaninFanouts( p->pPseudo, 1 );
    assert( Mntk_VerifyRefs( p ) );
    // compute area independently
    AreaTemp = Mntk_MappingArea( p );
    assert( p->fAreaGlo == AreaTemp );

    // set the input arrival times
    for ( i = 0; i < pMan->nInputs; i++ )
//        p->pInputs[i]->tArrival = pMan->pInputs[i]->pCutBest[1]->M[1].tArrive;   
        p->pInputs[i]->tArrival = pMan->pInputs[i]->tArrival[1];   

    // compute the arrival times for all the nodes
    Mntk_ComputeArrivalAll( p, 0 );
//    if ( pMan->fVerbose )
//        printf( "Initial area = %4.2f.  Static area = %4.2f.  Initial arrival time = %4.2f.\n", 
//            p->fAreaGlo, p->fAreaStatic, p->pPseudo->tArrival.Worst );
    return p;
}

/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ConvertMappingToMntkNode( Mntk_Man_t * p, Map_Node_t * pNodeMap, int fPhase, Mio_Gate_t * pGateInv )
{
    Mntk_Node_t * pNodeMntk;

    // implement the node if the best cut is assigned
    if ( pNodeMap->pCutBest[fPhase] != NULL )
    {
        Map_ConvertMappingToMntkPhase( p, pNodeMap, fPhase );
        return;
    }

    // if the cut is not assigned, implement the node
    assert( pNodeMap->pCutBest[!fPhase] != NULL );
    pNodeMntk = Map_ConvertMappingToMntkPhase( p, pNodeMap, !fPhase );

    // add the inverter
    pNodeMntk = Mntk_NodeAlloc( p, &pNodeMntk, 1, pGateInv );
    Mntk_NodeRegister( pNodeMntk );

    // set the inverter
    if ( fPhase )
        MTNK_NODE_WRITE1( pNodeMap, pNodeMntk );
    else
        MTNK_NODE_WRITE0( pNodeMap, pNodeMntk );
}

/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Map_ConvertMappingToMntkPhase( Mntk_Man_t * p, Map_Node_t * pNodeMap, int fPhase )
{
    Mntk_Node_t * pNodePIs[10];
    Mntk_Node_t * pNodeMntk;
    Map_Cut_t * pCut;
    int i, fInvPin;

    // make sure the node can be implemented in this phase
    assert( pNodeMap->pCutBest[fPhase] != NULL );

    // check if the phase is already implemented
    if ( fPhase )
        pNodeMntk = MTNK_NODE_READ1( pNodeMap );
    else
        pNodeMntk = MTNK_NODE_READ0( pNodeMap );
    if ( pNodeMntk )
        return pNodeMntk;

    // collect the PI nodes
    pCut = pNodeMap->pCutBest[fPhase];
    for ( i = 0; i < pCut->nLeaves; i++ )
    {
        fInvPin = ((pCut->M[fPhase].uPhaseBest & (1 << i)) > 0);
        if ( fInvPin )
            pNodePIs[i] = MTNK_NODE_READ0( pCut->ppLeaves[i] );
        else
            pNodePIs[i] = MTNK_NODE_READ1( pCut->ppLeaves[i] );
        assert( pNodePIs[i] != NULL );
    }

    // implement this supergate
    pNodeMntk = Map_ConvertMappingToMntkSuper_rec( p, pCut->M[fPhase].pSuperBest, pNodePIs, pCut->nLeaves );

    // set the node
    if ( fPhase )
        MTNK_NODE_WRITE1( pNodeMap, pNodeMntk );
    else
        MTNK_NODE_WRITE0( pNodeMap, pNodeMntk );
    return pNodeMntk;
}

/**Function*************************************************************

  Synopsis    [Constructs the nodes corrresponding to one supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mntk_Node_t * Map_ConvertMappingToMntkSuper_rec( Mntk_Man_t * p, Map_Super_t * pSuper, 
    Mntk_Node_t * pNodePis[], int nNodePis )
{
    Mio_Gate_t * pRoot;
    Map_Super_t ** ppFanins;
    Mntk_Node_t * pNodeNew, * pNodeFanins[10];
    int nFanins, Number, i;

    // get the parameters of the supergate
    pRoot = Map_SuperReadRoot(pSuper);
    if ( pRoot == NULL )
    {
        Number = Map_SuperReadNum(pSuper);
        if ( Number < nNodePis )  
        {
            return pNodePis[Number];
        }
        else
        {  
            /* It might happen that a super gate with 5 inputs is constructed that
             * actually depends only on the first four variables; i.e the fifth is a
             * don't care -- in that case we connect constant node for the fifth
             * (since the cut only has 4 variables). An interesting question is what
             * if the first variable (and not the fifth one is the redundant one;
             * can that happen?)  
             */
            return p->pConst1;
        }
    }

    // create or collect the node's fanins
    nFanins  = Map_SuperReadFaninNum( pSuper );
    ppFanins = Map_SuperReadFanins( pSuper );
    for ( i = 0; i < nFanins; i++ )
        pNodeFanins[i] = Map_ConvertMappingToMntkSuper_rec( p, ppFanins[i], pNodePis, nNodePis );

    // create the new node
    pNodeNew = Mntk_NodeAlloc( p, pNodeFanins, nFanins, pRoot );
    Mntk_NodeRegister( pNodeNew );
    return pNodeNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


