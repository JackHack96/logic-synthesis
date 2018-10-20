/**CFile****************************************************************

  FileName    [fmbSweep.c]

  PackageName [Binary flexibility manager.]

  Synopsis    [Sweeps the nodes with functionality equal up to complementation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 3.0. Started - September 27, 2003.]

  Revision    [$Id: fmbSweep.c,v 1.1 2003/11/18 18:55:10 alanmi Exp $]

***********************************************************************/

#include "fmbInt.h"


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Fmb_EquivProcess( Ntk_Network_t * pNet, st_table * tClasses, int fVerbose );
static void Fmb_EquivProcess1( Ntk_Network_t * pNet, st_table * tClasses, int fVerbose );
static void Fmb_EquivProcess2( Ntk_Network_t * pNet, st_table * tClasses, int fVerbose );
static void Fmb_EquivPrint( Ntk_Network_t * pNet, st_table * tClasses );

////////////////////////////////////////////////////////////////////////
///                     EXPORTED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Merges the nodes with equivalent functionality.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
int Ntk_NetworkEquiv( Ntk_Network_t * pNet, int fVerbose )
{
    Fmb_Man_t * p;
    Fmb_Par_t * pPars;
    int RetValue;

    pPars = ALLOC( Fmb_Par_t, 1 );
    memset( pPars, 0, sizeof(Fmb_Par_t) );
    pPars->fUseExdc    = 0;        // set to 1 if the EXDC is to be used
    pPars->fDynEnable  = 1;        // enables dynmamic variable reordering
    pPars->fVerbose    = fVerbose; // to enable the output of puzzling messages

    p = Fmb_ManagerStart( net, pPars );
    RetValue = Fmb_NetworkMergeEquivalentNets( net, fVerbose );
    Fmb_ManagerStop( p );

    return RetValue;
}
*/


/**Function*************************************************************

  Synopsis    [Merges the nodes with equivalent functionality.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fmb_NetworkMergeEquivalentNets( Fmb_Manager_t * p )
{
    Ntk_Network_t * pNet = p->pNet;
    int fVerbose = p->pPars->fVerbose;
    st_generator * gen;
    Ntk_Node_t * pNode;
    Ntk_Node_t ** ppChain, * pChain;
    DdNode * bFunc;
    st_table * tClasses, * tClassesNew;
    int RetValue;
    int Counter;

    // create the equivalence classes of nets
    tClasses = st_init_table( st_ptrcmp, st_ptrhash );
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        bFunc = Ntk_NodeReadFuncBinGlo( pNode );
        // remove the complement attribute
        bFunc = Cudd_Regular(bFunc);
        // add the function to the list of classes
        if ( !st_find_or_add( tClasses, (char*)bFunc, (char***)&ppChain ) )
        {
            // this is the first node in the class
            Ntk_NodeSetOrder( pNode, NULL );
            *ppChain = pNode;
        }
        else
        {
            // there are already nodes in this class; add one more node
            Ntk_NodeSetOrder( pNode, *ppChain );
            *ppChain = pNode;
        }
    }

    // create the table of functions with more than one associated node
    tClassesNew = st_init_table( st_ptrcmp, st_ptrhash );
    st_foreach_item( tClasses, gen, (char**)&bFunc, (char**)&pChain )
    {
        // count the number of entries in the chain
        Counter = 0;
        Ntk_NetworkForEachNodeSpecialStart( pChain, pNode )
            Counter++;
        if ( Counter > 1 )
            st_insert( tClassesNew, (char *)bFunc, (char *)pChain );
    }
    st_free_table( tClasses );
    tClasses = tClassesNew;

    // merge equivalent nodes
    Fmb_EquivProcess( pNet, tClasses, fVerbose );

    // undo the classes
    RetValue = st_count(tClasses);
    st_free_table( tClasses );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Process equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_EquivProcess( Ntk_Network_t * pNet, st_table * tClasses, int fVerbose )
{
//    if ( !lib_network_is_mapped(net) )
    {
        if ( fVerbose )
            printf( "Performing technology-independent sweep.\n" );
        Fmb_EquivProcess1( pNet, tClasses, fVerbose );
    }
//    else
//    {
//        printf( "The timing-aware sweep is not implemented in this version.\n" );
//        return;

//        if ( fVerbose )
//            printf( "Performing timing-aware sweep.\n" );
//        delay_trace( net, DELAY_MODEL_LIBRARY );
//        Fmb_EquivProcess2( net, tClasses, fVerbose );
//    }
}

/**Function*************************************************************

  Synopsis    [Process equivalence classes.]

  Description [This function does not remove the nodes. It only switches 
  around the connections.  ]
               
  SideEffects [Changes the network structure. 
  If the original network is mapped, this function tries to map the
  newly added inverters using the inverter gates from the library.]

  SeeAlso     []

***********************************************************************/
void Fmb_EquivProcess1( Ntk_Network_t * pNet, st_table * tClasses, int fVerbose )
{
    st_generator * gen;
    Ntk_Node_t * pListDir, * pListInv;
    Ntk_Node_t * pNode, * pChain;
    Ntk_Node_t * pNodeMin, * pNodeMinInv;
    DdNode * bFunc, * bFuncMin;
    int Count0, Count1, nNodes;

    if ( fVerbose )
        printf( "There are %d classes of equivalent nets.\n",  st_count( tClasses ) );
    if ( st_count( tClasses ) == 0 )
        return;
//Fmb_EquivPrint( net, tClasses );

    // make sure the levels are assigned
    Ntk_NetworkGetNumLevels(pNet);

    // go through the entries
    Count0 = 0;
    Count1 = 0;
    st_foreach_item( tClasses, gen, (char**)&bFunc, (char**)&pChain )
    {
        // find the node with the smallest number of logic levels
        pNodeMin = pChain;
        nNodes = 0;
        Ntk_NetworkForEachNodeSpecialStart( Ntk_NodeReadOrder(pChain), pNode )
        {
            nNodes++;
            if (  Ntk_NodeReadLevel(pNodeMin) >  Ntk_NodeReadLevel(pNode) ) // || 
//                ( Ntk_NodeReadLevel(pNodeMin) == Ntk_NodeReadLevel(pNode) && 
//                  utilsNetworkDroppingCost(pNodeMin) < utilsNetworkDroppingCost(pNode) ) )
                pNodeMin = pNode;
        }
        assert( nNodes > 0 );

        // get the function of the node
        bFuncMin = Ntk_NodeReadFuncBinGlo(pNodeMin);

        // divide the nodes into two parts: 
        // those that need the invertor and those that don't need
        pListDir = pListInv = NULL;
        Ntk_NetworkForEachNodeSpecialStart( pChain, pNode )
        {
            if ( pNode == pNodeMin )
                continue;
            // check to which class the node belongs
            if ( bFuncMin == Ntk_NodeReadFuncBinGlo(pNode) ) // in general, phase assignment should be used
            {
                Ntk_NodeSetOrder( pNode, pListDir );
                pListDir = pNode;
                Count1++;
            }
            else
            {
                Ntk_NodeSetOrder( pNode, pListInv );
                pListInv = pNode;
                Count0++;
            }
        }

        // move the fanouts of the direct nodes
        Ntk_NetworkForEachNodeSpecialStart( pListDir, pNode )
            Ntk_NodeTransferFanout( pNode, pNodeMin );

        // skip if there are no inverted nodes
        if ( pListInv == NULL )
            continue;

        // add the invertor
        pNodeMinInv = Ntk_NodeCreateOneInputBinary( pNet, pNodeMin, 1);
        Ntk_NetworkAddNode( pNet, pNodeMinInv, 1 );

        // if the network is mapped, map the inverter gate
//        if ( lib_get_library() )
//        { // TO ADD
//        }

        // get the global BDD of the inverter node
        Ntk_NodeWriteFuncDd( pNodeMinInv, Ntk_NodeReadFuncDd(pNodeMin) );
        Ntk_NodeWriteFuncBinGlo( pNodeMinInv, Cudd_Not(bFuncMin) );   Cudd_Ref( bFuncMin );

        // move the fanouts of the inverted nodes
        Ntk_NetworkForEachNodeSpecialStart( pListInv, pNode )
            Ntk_NodeTransferFanout( pNode, pNodeMinInv );
    }
//printf( "Neg saved = %d. Pos saved = %d.\n", Count0, Count1 );
}



/**Function*************************************************************

  Synopsis    [Process equivalence classes.]

  Description [This function does not remove the nodes. It only switches 
  around the connections. 
  This version of the function does not introduce the inverters. 
  It only moves around the fanouts. It tries to determine what should be the
  remaining nodes using the arrival time information.]
               
  SideEffects [Changes the network structure. Adds nodes to tSlicePos.
  If the original network is mapped, this function tries to map the
  newly added inverters using the inverter gates from the library.]

  SeeAlso     []

***********************************************************************/
void Fmb_EquivProcess2( Ntk_Network_t * pNet, st_table * tClasses, int fVerbose )
{
}


/**Function*************************************************************

  Synopsis    [Print equivalence classes.]

  Description [This function prints the equivalence classes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fmb_EquivPrint( Ntk_Network_t * pNet, st_table * tClasses )
{
    st_generator * gen;
    Ntk_Node_t * pNode, * pChain;
    DdNode * bFunc;
    int Counter = 0;
    int fFirst;

    // go through the entries
    fprintf( stdout, "The following classes of equivalent nodes are detected:\n" );
    st_foreach_item( tClasses, gen, (char**)&bFunc, (char**)&pChain )
    {
        fFirst = 1;
        fprintf( stdout, "%02d: {", ++Counter );
        Ntk_NetworkForEachNodeSpecialStart( pChain, pNode )
        {
            if ( fFirst )
                fFirst = 0;
            else
                fprintf( stdout, "," );
            fprintf( stdout, " %s", Ntk_NodeGetNamePrintable(pNode) );
        }
        fprintf( stdout, "}\n" );
    }
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////


