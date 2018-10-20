
/**CFile****************************************************************

  FileName    [mapperSweep.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Sweeping the functionally equivalent nodes of the mapped network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mapperSweep.c,v 1.3 2004/08/09 22:16:31 satrajit Exp $]

***********************************************************************/

#include "mapperInt.h"
#include "sh.h"
#include "ver.h"
#include "ntk.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Map_SweepSplitClasses( Ntk_Network_t * pNet, st_table * tEquiv );
static void Map_SweepChooseEquivRep( Ntk_Network_t * pNet, st_table * tEquiv );
static void Map_SweepMergeClasses( Ntk_Network_t * pNet, st_table * tEquiv );
static void Map_SweepMergeClass( Ntk_Network_t * pNet, Ntk_Node_t * pClass, Ntk_Node_t * pRep );
static void Map_SweepFixFanoutBinding( Ntk_Node_t * pNode, Ntk_Node_t * pRep );
static int  Map_NetworkCheckAllBound( Ntk_Network_t * pNet );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************
 
  Synopsis    [Checks that all nodes the network are bound]
 
  Description []
 
  SideEffects [Changes the network structure.]
 
  SeeAlso     []

***********************************************************************/
int Map_NetworkCheckAllBound( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNodeNew;
    int unmapped = 0;
    Ntk_NetworkForEachNode( pNet, pNodeNew )
    {
        if ( pNodeNew->pData == 0)
        {
            fprintf (stderr, "Unmapped node: %s\n", Ntk_NodeGetNameLong(pNodeNew));
            unmapped++;
        }
    }
    if (unmapped > 0 )
        fprintf (stderr, "Unmapped TOTAL: %d\n", unmapped);

    return unmapped;
}

/**Function*************************************************************
 
  Synopsis    [Find functionally equivalent points in the mapped netlist and sweep]
 
  Description []
 
  SideEffects [Changes the network structure.]
 
  SeeAlso     []

***********************************************************************/
void Map_NetworkSweep( Ntk_Network_t * pNet )
{
    Sh_Manager_t * pShMan;
    Sh_Network_t * pShNet;
    st_table *     tEquiv;

    printf ("map_sweep: Starting\n");

    // This is now done always, whether or not Sat-Sweep is requested
    // Map_NetworkCheckAllBound( pNet );
    // Ntk_NetworkSweep ( pNet, 0, 0, 0, 0 );
    // Map_NetworkCheckAllBound( pNet );

    pShMan = Sh_ManagerCreate( Ntk_NetworkReadCiNum(pNet), 100000, 1 );
    Sh_ManagerTwoLevelEnable( pShMan );
    pShNet = Ver_NetworkStrashInt( pShMan, pNet );

    tEquiv = Ver_NetworkDetectEquiv( pNet, pShMan, pShNet, 0, 5 );

    Map_SweepSplitClasses( pNet, tEquiv );
    Map_SweepChooseEquivRep ( pNet, tEquiv );
    Map_SweepMergeClasses( pNet, tEquiv );

    // reorder the fanins of the nodes
    Ntk_NetworkOrderFanins( pNet );
    assert ( Ntk_NetworkCheck( pNet ) );

    Ntk_NetworkSweep ( pNet, 0, 0, 0, 0 );

    st_free_table( tEquiv );
    Sh_NetworkFree( pShNet );
    Sh_ManagerFree( pShMan, 1 );

    printf ("map_sweep: Done\n");
}

/**Function*************************************************************
 
  Synopsis    [Split each equivalence class into two parts: oen of nodes 
               of same polarity as the first node; the other of opposite
               polarity]
 
  Description []
 
  SideEffects [Changes tEquiv]
 
  SeeAlso     []

***********************************************************************/
void Map_SweepSplitClasses( Ntk_Network_t * pNet, st_table * tEquiv )
{
    st_generator * gen;
    Ntk_Node_t   * pList, * pNode, * pNext;
    Ntk_Node_t   * pListDir, * pListInv;

    printf ("map_sweep: Before inverter split: %5d\n", st_count( tEquiv ));

    st_foreach_item( tEquiv, gen, (char **)&pList, NULL )
    {
        // equiv class with only one member
        if ( pList->pOrder == 0 )
            continue;
        
        // divide the nodes into two parts: 
        // those that need the invertor and those that don't need
        pListDir = pListInv = 0;
        for ( pNode = pList->pOrder, pNext = pList->pOrder->pOrder; pNode; pNode = pNext, pNext = pNode? pNode->pOrder : 0 )
        {
            if ( pNode == pList )
                continue;

            // check to which class the node belongs
            if ( pList->fNdTfi == pNode->fNdTfi )
            {
                pNode->pOrder = pListDir;
                pListDir = pNode;
            }
            else
            {
                pNode->pOrder = pListInv;
                pListInv = pNode;
            }
        }

        st_delete( tEquiv, (char **)&pList, (char **)0 );

        if ( pListInv != 0 && pListInv->pOrder != 0 )
            st_insert( tEquiv, (char *)pListInv, (char *)0 );

        if ( pListDir != 0 && pListDir->pOrder != 0 )
            st_insert( tEquiv, (char *)pListDir, (char *)0 );
    }

    printf ("map_sweep: After  inverter split: %5d\n", st_count( tEquiv ));
}

/**Function*************************************************************
 
  Synopsis    [Pick the representative from each class]
 
  Description []
 
  SideEffects [Changes tEquiv]
 
  SeeAlso     []

***********************************************************************/
void Map_SweepChooseEquivRep( Ntk_Network_t * pNet, st_table * tEquiv )
{
    st_generator * gen;
    Ntk_Node_t   * pList;

    st_foreach_item( tEquiv, gen, (char **)&pList, NULL )
    {
        st_insert( tEquiv, (char *)pList, (char *)pList );
    }
}

/**Function*************************************************************
 
  Synopsis    [Substitutes each class with its chosen rep]
 
  Description []
 
  SideEffects [Changes pNet]
 
  SeeAlso     []

***********************************************************************/
void Map_SweepMergeClasses( Ntk_Network_t * pNet, st_table * tEquiv )
{
    st_generator * gen;
    Ntk_Node_t   * pClass, * pRep;

    st_foreach_item( tEquiv, gen, (char **)&pClass, (char **)&pRep )
    {
        Map_SweepMergeClass( pNet, pClass, pRep); 
    }
}

void Map_SweepMergeClass( Ntk_Network_t * pNet, Ntk_Node_t * pClass, Ntk_Node_t * pRep )
{
    Ntk_Node_t   * pNode;

    for ( pNode = pClass; pNode; pNode = pNode->pOrder )
    {
        if ( pNode != pRep )
        {
            // First fix bindings so that we can use the 
            // NTK fanout info and then we change NTK fanout info
            Map_SweepFixFanoutBinding( pNode, pRep );
            Ntk_NodeTransferFanout( pNode, pRep );
        }
    }
}

void Map_SweepFixFanoutBinding( Ntk_Node_t * pNode, Ntk_Node_t * pRep )
{
    Ntk_Pin_t *  pPin;
    Ntk_Node_t * pFanout;
    Map_Binding_t * pBinding;
    Mio_Gate_t * pGate;
    Ntk_Node_t ** ppFanInArray;
    int i;

    Ntk_NodeForEachFanout( pNode, pPin, pFanout )
    {
        // no binding to update for a CO node
        if ( Ntk_NodeReadType( pFanout ) == MV_NODE_CO )
            continue;
        
        pBinding = (Map_Binding_t *)Ntk_NodeReadData( pFanout );
        pGate = Map_BindingReadGate( pBinding );
        ppFanInArray = Map_BindingReadFanInArray( pBinding );
        for (i = 0; i < Mio_GateReadInputs( pGate ); ++i)
        {
            if ( ppFanInArray[i] == pNode )
                ppFanInArray[i] = pRep;
        }
    }
}

