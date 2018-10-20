/**CFile****************************************************************

  FileName    [mvnEncode.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Sequential manipulation on the level of the MV network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnEncode.c,v 1.5 2004/05/12 04:30:16 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Encodes the PIs/POs of the network using logarithmic encoding.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkIOEncode( Ntk_Network_t * pNet, bool fEncodeLatches )
{
    Ntk_Node_t * pNodePi, * pNodePo, * pNodeDec;
    Ntk_Node_t * pNode2, * pNodeBuf, * pNodeIn;
    int i, nBits, nValues, nTotalValues;
    Ntk_Node_t * ppNodesBits[32];
    char Buffer[1000], Buffer1[1000], Buffer2[1000], * pNameRegistered;
    int pValueAssign[32], k;
    unsigned uPols[32];
//    int nUnused, iValue;
    char ** ppLatchIns = NULL;
    char ** ppLatchOuts = NULL;
    int * pInitValues = NULL;
    int * pTotalValues = NULL;
    int * pLatchInSubtypes = NULL;
    int * pLatchOutSubtypes = NULL;
    Ntk_Latch_t * pLatch, * pLatch2;
    int nLatches, nLatchesBin, nInitValue;

    // if the encoding of latches is requested, save their names
    if ( fEncodeLatches )
    {
        nLatches = Ntk_NetworkReadLatchNum( pNet );
        ppLatchIns  = ALLOC( char *, nLatches );
        ppLatchOuts = ALLOC( char *, nLatches );
        pInitValues = ALLOC( int, nLatches );
        pTotalValues = ALLOC( int, nLatches );
        pLatchInSubtypes = ALLOC( int, nLatches );
        pLatchOutSubtypes = ALLOC( int, nLatches );
        i = 0;
        Ntk_NetworkForEachLatchSafe( pNet, pLatch, pLatch2 )
        {
            ppLatchIns[i]        = Ntk_NodeReadName( pLatch->pInput );
            assert( Ntk_NodeReadType( pLatch->pInput ) == MV_NODE_CO );
            ppLatchOuts[i]       = Ntk_NodeReadName( pLatch->pOutput );
            assert( Ntk_NodeReadType( pLatch->pOutput ) == MV_NODE_CI );
            pInitValues[i]       = pLatch->Reset;
            pTotalValues[i]      = pLatch->pInput->nValues;
            pLatchInSubtypes[i]  = Ntk_NodeReadSubtype( pLatch->pInput );
            pLatchOutSubtypes[i] = Ntk_NodeReadSubtype( pLatch->pOutput );
            i++;
            Ntk_NodeSetSubtype( pLatch->pInput, MV_BELONGS_TO_NET );
            Ntk_NodeSetSubtype( pLatch->pOutput, MV_BELONGS_TO_NET );
            Ntk_NetworkDeleteLatch( pLatch );
        }
    }


    for ( k = 0; k < 32; k++ )
        uPols[k] = (1<<k);

    // put all the PI nodes into the linked list
    Ntk_NetworkStartSpecial( pNet );
    Ntk_NetworkForEachCi( pNet, pNodePi )
    {
        if ( Ntk_NodeReadSubtype(pNodePi) == MV_BELONGS_TO_LATCH )
                continue;
        Ntk_NetworkAddToSpecial( pNodePi );
    }
    Ntk_NetworkStopSpecial( pNet );

    // go through these nodes and transform them
    Ntk_NetworkForEachNodeSpecialSafe( pNet, pNodePi, pNode2 )
    {
        // figure out how many nodes to add instead of this PI
        nValues = Ntk_NodeReadValueNum(pNodePi);
        assert( nValues <= 32 );
        nBits = Extra_Base2Log( nValues );
        nTotalValues = (1 << nBits);
        // create the nodes
        for ( k = 0; k < nBits; k++ )
        {
            sprintf( Buffer, "%s_%d", Ntk_NodeReadName(pNodePi), k );
            ppNodesBits[k] = Ntk_NodeCreate( pNet, Buffer, MV_NODE_CI, 2 );
            Ntk_NetworkAddNode( pNet, ppNodesBits[k], 1 );
        }

        // derive very simple natural encoding
        for ( i = 0; i < nValues; i++ )
            pValueAssign[i] = i;
        for (      ; i < nTotalValues; i++ )
            pValueAssign[i] = -1;

/*
        // assign two minterms to the first "nUnused" values
        nUnused = nTotalValues - nValues;
        iValue = 0;
        for ( i = 0; i < nUnused; i++ )
        {
            pValueAssign[2*i+0] = iValue;
            pValueAssign[2*i+1] = iValue;
            iValue++;
        }
        // assign the remaining minterms to each of the remaining values
        for ( i = 2 * i; i < nTotalValues; i++ )
            pValueAssign[i] = iValue++;
*/

        // create the decoder
        pNodeDec = Ntk_NodeCreateDecoderGeneral( ppNodesBits, nBits, 
            pValueAssign, nTotalValues, nValues );
//Ntk_NodePrintCvr( stdout, pNodeDec, 1, 1 );
        // add the decode to the network
        Ntk_NetworkAddNode( pNet, pNodeDec, 1 );
        // transfer the fanouts
        Ntk_NodeTransferFanout( pNodePi, pNodeDec );
        // remove the original node
        Ntk_NetworkDeleteNode( pNet, pNodePi, 1, 1 );
    }

    

    // put all the PO nodes into the linked list
    Ntk_NetworkStartSpecial( pNet );
    Ntk_NetworkForEachCo( pNet, pNodePo )
    {
        if ( Ntk_NodeReadSubtype(pNodePo) == MV_BELONGS_TO_LATCH )
                continue;
        Ntk_NetworkAddToSpecial( pNodePo );
    }
    Ntk_NetworkStopSpecial( pNet );

    // go through these nodes and transform them
    Ntk_NetworkForEachNodeSpecialSafe( pNet, pNodePo, pNode2 )
    {
        // figure out how many nodes to add instead
        nValues = Ntk_NodeReadValueNum(pNodePo);
        assert( nValues <= 32 );
        nBits = Extra_Base2Log( nValues );
        nTotalValues = (1 << nBits);
        // create the buffer with the same number of values
        pNodeIn = Ntk_NodeReadFaninNode( pNodePo, 0 );
        pNodeBuf = Ntk_NodeCreateOneInputNode( pNet, pNodeIn, nValues, nValues, uPols );

        // derive very simple natural encoding
        for ( i = 0; i < nValues; i++ )
            pValueAssign[i] = i;
        for (      ; i < nTotalValues; i++ )
            pValueAssign[i] = -1;
/*
        // assign two minterms to the first "nUnused" values
        nUnused = nTotalValues - nValues;
        iValue = 0;
        for ( i = 0; i < nUnused; i++ )
        {
            pValueAssign[2*i+0] = iValue;
            pValueAssign[2*i+1] = iValue;
            iValue++;
        }
        // assign the remaining minterms to each of the remaining values
        for ( i = 2 * i; i < nTotalValues; i++ )
            pValueAssign[i] = iValue++;
*/
        // encode this node
        Ntk_NodeCreateEncoded( pNodeBuf, ppNodesBits, 
            nBits, pValueAssign, nTotalValues, nValues );
        Ntk_NodeDelete( pNodeBuf );
        // create the names
        for ( k = 0; k < nBits; k++ )
        {
            Ntk_NetworkAddNode( pNet, ppNodesBits[k], 1 );
            // create the name for this node
            sprintf( Buffer, "%s_%d", Ntk_NodeReadName(pNodePo), k );
            pNameRegistered = Ntk_NetworkRegisterNewName( pNet, Buffer );
            Ntk_NodeAssignName( ppNodesBits[k], pNameRegistered );
            // create the CO node
            Ntk_NetworkAddNodeCo( pNet, ppNodesBits[k], 1 );
        }
        // remove the original node
        Ntk_NetworkDeleteNode( pNet, pNodePo, 1, 1 );
    }


    // create the encoded latches
    if ( fEncodeLatches )
    {
        for ( i = 0; i < nLatches; i++ )
        {
            nLatchesBin = Extra_Base2Log(pTotalValues[i]);
            for ( k = 0; k < nLatchesBin; k++ )
            {
                sprintf( Buffer1, "%s_%d", ppLatchIns[i], k );
                pNodePo = Ntk_NetworkFindNodeByName( pNet, Buffer1 );
                assert( pNodePo );
                Ntk_NodeSetSubtype( pNodePo, pLatchInSubtypes[i] );

                sprintf( Buffer2, "%s_%d", ppLatchOuts[i], k );
                pNodePi = Ntk_NetworkFindNodeByName( pNet, Buffer2 );
                assert( pNodePi );
                Ntk_NodeSetSubtype( pNodePi, pLatchOutSubtypes[i] );

                nInitValue = ( (pInitValues[i] & (1<<k)) > 0 );
                pLatch = Ntk_LatchCreate( pNet, NULL, nInitValue, Buffer1, Buffer2 );
                Ntk_NetworkAddLatch( pNet, pLatch );
            }
        }
        FREE( ppLatchIns );
        FREE( ppLatchOuts );
        FREE( pInitValues );
        FREE( pTotalValues );
        FREE( pLatchInSubtypes );
        FREE( pLatchOutSubtypes );
    }



    // put IDs into a proper order
    Ntk_NetworkOrderFanins( pNet );

    if ( !Ntk_NetworkCheck( pNet ) )
       fprintf( Ntk_NetworkReadMvsisOut(pNet), "Mvn_NetworkIOEncode(): Network check has failed.\n" );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


