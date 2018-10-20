/**CFile****************************************************************

  FileName    [ntkCheck.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkCheck.c,v 1.27 2005/03/31 01:29:33 casem Exp $]

***********************************************************************/

#include "ntkInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Checks the integrity of a network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkCheck( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Ntk_Latch_t * pLatch;
    st_generator * gen;
    char * pName;
//  int clk;

    // check that the nodes in the table are also in the network
    st_foreach_item( pNet->tName2Node, gen, &pName, (char**)&pNode )
    {
        if ( pNode->pNet != pNet )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NetworkCheck: Node \"%s\" is in the table but not in the network.\n", pName );
            return 0;
        }
        if ( pNode->pName != pName )
        {
          fprintf( Ntk_NetworkReadMvsisOut(pNet), "NetworkCheck: Node \"%s\" has different name compared to the one in the table (%s).\n", pName, pNode->pName);
            return 0;
        }
    }
    // check that the nodes in the network are also in the table
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( pNode->pName )
        {
            if ( !st_lookup( pNet->tName2Node, pNode->pName, (char**)&pNode ) )
            {
                fprintf( Ntk_NetworkReadMvsisOut(pNet), "NetworkCheck: Node \"%s\" is in the network but not in the table.\n", pNode->pName );
                return 0;
            }
        }
        if ( Ntk_NetworkFindNodeById(pNet, pNode->Id) != pNode )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NetworkCheck: Node \"%s\" cannot be found in the ID table by its ID (%d).\n", pName, pNode->Id );
            return 0;
        }
    }
    // make sure that every PI/PO is in the network
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( !st_is_member( pNet->tName2Node, pNode->pName ) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NetworkCheck: Primary input \"%s\" is in the PI list but not in the table.\n", pNode->pName );
            return 0;
        }
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( !st_is_member( pNet->tName2Node, pNode->pName ) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NetworkCheck: Primary output \"%s\" is in the PO list but not in the table.\n", pNode->pName );
            return 0;
        }
    }

    // check the correctness of fanins/fanouts of each node
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( !Ntk_NetworkCheckNode( pNet, pNode ) )
            return 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        if ( !Ntk_NetworkCheckNode( pNet, pNode ) )
            return 0;
    Ntk_NetworkForEachCo( pNet, pNode )
        if ( !Ntk_NetworkCheckNode( pNet, pNode ) )
            return 0;

    // check latches
    Ntk_NetworkForEachLatch( pNet, pLatch )
        if ( !Ntk_NetworkCheckLatch( pNet, pLatch ) )
            return 0;

    // finally, check for combinational loops
//  clk = clock();
    if ( !Ntk_NetworkIsAcyclic( pNet ) )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "NetworkCheck: Network contains a combinational loop.\n" );
        return 0;
    }
//  PRT( "Acyclic  ", clock() - clk );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkCheckNode( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin, * pFaninPrev, * pFanout;
    Ntk_Pin_t * pPin, * pPin2;
    int RetValue, nValuesIn;
    int Value = 1;

    if ( pNode->pNet != pNet )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Node \"%s\" has the parent network different from the current network.\n", Ntk_NodeGetNamePrintable(pNode) );
        return Value;
    }

    if ( pNode->nValues < 0 || pNode->nValues > 32 )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Node \"%s\" has incorrect number of values (%d).\n", 
            Ntk_NodeGetNamePrintable(pNode), pNode->nValues );
        return Value;
    }

    if ( Ntk_NetworkFindNodeById(pNet, pNode->Id) != pNode )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Node \"%s\" cannot be found in the ID table by its ID (%d).\n", Ntk_NodeGetNamePrintable(pNode), pNode->Id );
        return 0;
    }

    if ( pNode->Type == MV_NODE_CI )
    {
        if ( Ntk_NodeReadFuncCvr(pNode) || Ntk_NodeReadFuncMvr(pNode) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Primary input \"%s\" has a local function.\n", Ntk_NodeGetNamePrintable(pNode) );
            return Value;
        }
        if ( Ntk_NodeReadFaninNum(pNode) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Primary input \"%s\" has fanin.\n", Ntk_NodeGetNamePrintable(pNode) );
            return Value;
        }
    }
    else if ( pNode->Type == MV_NODE_CO )
    {
        if ( Ntk_NodeReadFuncCvr(pNode) || Ntk_NodeReadFuncMvr(pNode) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Primary output \"%s\" has a local function.\n", Ntk_NodeGetNamePrintable(pNode) );
            return Value;
        }
        if ( Ntk_NodeReadFaninNum(pNode) != 1 )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Primary output \"%s\" has other than one fanin.\n", Ntk_NodeGetNamePrintable(pNode) );
            return Value;
        }
        if ( Ntk_NodeReadFanoutNum(pNode) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Primary output \"%s\" has fanout.\n", Ntk_NodeGetNamePrintable(pNode) );
            return Value;
        }
    }
    else if ( pNode->Type == MV_NODE_INT )
    {
//      if ( Ntk_NodeNumFanin(pNode) == 0 )
//          fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Internal node \"%s\" is not driven.\n", Ntk_NodeGetNameLong( pNode ) );
        // it is okay for a constant node to be not driven

//        if ( Ntk_NodeNumFanout(pNode) == 0 )
//            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Internal node \"%s\" does not fanout.\n", Ntk_NodeGetNameLong( pNode ) );
        // it is okay if an internal node does not fanout

        // make sure the fanins of the node are ordered and not duplicated
        pFaninPrev = NULL;
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        {
            if ( pFaninPrev )
            {
                RetValue = Ntk_NodeCompareByNameAndId( &pFaninPrev, &pFanin );
                if ( RetValue == 0 )
                {
                    fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Node \"%s\"", 
                        Ntk_NodeGetNamePrintable(pNode) );
                    fprintf( Ntk_NetworkReadMvsisOut(pNet), " has duplicated fanins (\"%s\"", 
                        Ntk_NodeGetNamePrintable(pFaninPrev) );
                    fprintf( Ntk_NetworkReadMvsisOut(pNet), " and \"%s\").\n", 
                        Ntk_NodeGetNamePrintable(pFanin) );
                }
                else if ( RetValue > 0 )
                {
                    fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Node \"%s\"", 
                        Ntk_NodeGetNamePrintable(pNode) );
                    fprintf( Ntk_NetworkReadMvsisOut(pNet), " has incorrectly ordered fanins (\"%s\"", 
                        Ntk_NodeGetNamePrintable(pFaninPrev) );
                    fprintf( Ntk_NetworkReadMvsisOut(pNet), " and \"%s\").\n", 
                        Ntk_NodeGetNamePrintable(pFanin) );
                }
            }
            pFaninPrev = pFanin;
        }

        // make sure the number of values of the node is okay
        nValuesIn = 0;
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
            nValuesIn += pFanin->nValues;
        if ( nValuesIn != Vm_VarMapReadValuesInNum(Ntk_NodeReadFuncVm(pNode)) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Node \"%s\" has var map with incorrect number of input values.\n", 
                Ntk_NodeGetNamePrintable(pNode) );
            return Value;
        }
        if ( pNode->nValues != Vm_VarMapReadValuesOutNum(Ntk_NodeReadFuncVm(pNode)) )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Node \"%s\" has var map with incorrect number of output values.\n", 
                Ntk_NodeGetNamePrintable(pNode) );
            return Value;
        }
    }
    else
    {
        assert( 0 );
    }

    // check the connectivity of fanins of the node
    Ntk_NodeForEachFanin( pNode, pPin, pFanin ) 
    {
        // go through the fanin's fanout and find the mirror pin
        Ntk_NodeForEachFanout( pFanin, pPin2, pFanout ) 
        {
            if ( pFanout == pNode && pPin2->pLink == pPin )
                break;
            // the second condition is added to treat correctly the case
            // when a fanin is duplicated in .names line of the node
            // for example, node "73(741)" of benchmark "c1908.blif"
            // in fact, there is nothing bad about such nodes
            // they should be fixed by "make_minimum_base"
        }
        if ( pFanout != pNode || pPin->pLink != pPin2 || pPin2->pLink != pPin )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Fanout list of fanin \"%s\" of node \"%s\" is not correct.\n", 
                Ntk_NodeGetNameLong( pFanin ), Ntk_NodeGetNameLong( pNode ) );
            return Value;
        }
    }
    // check the connectivity of fanouts of the node
    Ntk_NodeForEachFanout( pNode, pPin, pFanout ) 
    {
        // go through the fanin's fanout and find the mirror pin
        Ntk_NodeForEachFanin( pFanout, pPin2, pFanin ) 
        {
            if ( pFanin == pNode && pPin2->pLink == pPin )
                break;
            // the second condition is added to treat correctly the case
            // when a fanin is duplicated in .names line of the node
            // for example, node "73(741)" of benchmark "c1908.blif"
            // in fact, there is nothing bad about such nodes
            // they should be fixed by "make_minimum_base"
        }
        if ( pFanin != pNode || pPin->pLink != pPin2 || pPin2->pLink != pPin )
        {
            fprintf( Ntk_NetworkReadMvsisOut(pNet), "NodeCheck: Fanin list of fanout \"%s\" of node \"%s\" is not correct.\n", 
                Ntk_NodeGetNameLong( pFanout ), Ntk_NodeGetNameLong( pNode ) );
            return Value;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of a latch.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkCheckLatch( Ntk_Network_t * pNet, Ntk_Latch_t * pLatch )
{
    Ntk_Node_t * pNode;

    if ( pLatch->pNet != pNet )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "LatchCheck: Latch \"%s\" has the parent network different from the current network.\n", pLatch->pOutput->pName );
        return 0;
    }

    // make sure that latch input/output are PO/PI of the network
    if ( pLatch->pOutput->pName == NULL )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "LatchCheck: Latch output does not have a name.\n" );
        return 0;
    }
    if ( !st_lookup( pNet->tName2Node, pLatch->pOutput->pName, (char**)&pNode ) )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "LatchCheck: Latch \"%s\" has output not in the network.\n", pLatch->pOutput->pName );
        return 0;
    }
    if ( pNode->Type != MV_NODE_CI )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "LatchCheck: The latch output \"%s\" is not a primary input of the network.\n", Ntk_NodeGetNamePrintable(pNode) );
        return 0;
    }

    if ( pLatch->pInput->pName == NULL )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "LatchCheck: Latch input does not have a name.\n" );
        return 0;
    }
    if ( !st_lookup( pNet->tName2Node, pLatch->pInput->pName, (char**)&pNode ) )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "LatchCheck: Latch \"%s\" has input not in the network.\n", pLatch->pInput->pName );
        return 0;
    }
    if ( pNode->Type != MV_NODE_CO )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "LatchCheck: The latch input \"%s\" is not a primary output of the network.\n", Ntk_NodeGetNamePrintable(pNode) );
        return 0;
    }

    // make sure that the latch input/output belong to the latch
    if ( pLatch->pOutput->Subtype != MV_BELONGS_TO_LATCH )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "LatchCheck: Latch \"%s\" has output that does not belong to it.\n", pLatch->pOutput->pName );
        return 0;
    }
    if ( pLatch->pInput->Subtype != MV_BELONGS_TO_LATCH )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "LatchCheck: Latch \"%s\" has input that does not belong to it.\n", pLatch->pInput->pName );
        return 0;
    }

    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


