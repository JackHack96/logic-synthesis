/**CFile****************************************************************

  FileName    [ioWriteBench.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Writes the strashed network in .bench format.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioWriteBench.c,v 1.2 2005/01/23 07:11:41 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the network into a BLIF file with the given name.]

  Description [This procedure is really stupid. It can write only single
  input and two-input nodes. Each single input node is written as NOT().
  Each two-input node is written as AND().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteNetworkBench( Ntk_Network_t * pNet, char * FileName )
{
    Ntk_Node_t * pNode, * pFanin, * pFanout;
    Ntk_Pin_t * pPin;
    Ntk_Latch_t * pLatch;
    int nSingles, nDoubles, nFanins;
    FILE * pFile;

    // count the number of single- and double-input nodes
    nSingles = nDoubles = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        nFanins = Ntk_NodeReadFaninNum(pNode);
        if ( nFanins == 1 )
            nSingles++;
        else if ( nFanins == 2 )
            nDoubles++;
        else if ( nFanins == 0 )
        {
        }
        else
        {
            printf( "The network %s contains nodes with %d fanins.\n", pNet->pName, nFanins );
            printf( "Such nodes cannot be currently written into BENCH files.\n" );
            return;
        }
    }

//# 8 inputs
//# 19 outputs
//# 6 D-type flipflops
//# 103 inverters
//# 550 gates (350 ANDs + 0 NANDs + 200 ORs + 0 NORs)

    pFile = fopen( FileName, "w" );
    fprintf( pFile, "# %d inputs\n", Ntk_NetworkReadCiNum(pNet) - Ntk_NetworkReadLatchNum(pNet) );
    fprintf( pFile, "# %d outputs\n", Ntk_NetworkReadCoNum(pNet) - Ntk_NetworkReadLatchNum(pNet) );
    fprintf( pFile, "# %d D-type flipflops\n", Ntk_NetworkReadLatchNum(pNet) );
    fprintf( pFile, "# %d inverters\n", nSingles );
    fprintf( pFile, "# %d gates (%d ANDs + 0 NANDs + 0 ORs + 0 NORs)\n", nDoubles, nDoubles );

    Ntk_NetworkForEachCi( pNet, pNode )
        if ( pNode->Subtype == MV_BELONGS_TO_NET )
            fprintf( pFile, "INPUT(%s)\n", pNode->pName );

    Ntk_NetworkForEachCo( pNet, pNode )
        if ( pNode->Subtype == MV_BELONGS_TO_NET )
            fprintf( pFile, "OUTPUT(%s)\n", pNode->pName );

    Ntk_NetworkForEachLatch( pNet, pLatch )
        fprintf( pFile, "%s = DFF(%s)\n", pLatch->pOutput->pName, pLatch->pInput->pName );


    // some PO nodes may have a fanin, which is a PI
    // such PO nodes are written here, because they are not written
    // in the above loop, which interates through the internal nodes
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        pFanin = Ntk_NodeReadFaninNode( pNode, 0 );
        if ( Ntk_NodeIsCi(pFanin) )
            fprintf( pFile, "%s = BUF(%s)\n", pNode->pName, pFanin->pName );
    }

    Ntk_NetworkForEachNode( pNet, pNode )
    {        
        // however, if an internal node has more than one CO fanout
        // it is written as an intenal node with its own name
        // in this case, the COs should be written as MV buffers
        // driven by the given internal node
        if ( Ntk_NodeIsCoFanin(pNode) && !Ntk_NodeHasCoName(pNode) )
        {
            Ntk_NodeForEachFanout( pNode, pPin, pFanout ) 
                if ( Ntk_NodeIsCo(pFanout) )
                    fprintf( pFile, "%s = BUF(%s)\n", pFanout->pName, Ntk_NodeGetNameLong(pNode) );
        }
    } 

    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) == 0 )
        {
            Mvr_Relation_t * pMvr = Ntk_NodeGetFuncMvr( pNode );
            unsigned iPos = Mvr_RelationGetConstant( pMvr );

            fprintf( pFile, "%s", Ntk_NodeGetNameLong(pNode) );
            fprintf( pFile, " = %s\n", (iPos==2)? "vcc" : "gnd" );
        }
    }

    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) == 1 )
        {
            fprintf( pFile, "%s", Ntk_NodeGetNameLong(pNode) );
            fprintf( pFile, " = NOT(%s)\n", Ntk_NodeGetNameLong( Ntk_NodeReadFaninNode(pNode,0) ) );
        }
    }

    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) == 2 )
        {
            fprintf( pFile, "%s", Ntk_NodeGetNameLong(pNode) );
            fprintf( pFile, " = AND(%s, ", Ntk_NodeGetNameLong( Ntk_NodeReadFaninNode(pNode,0) ) );
            fprintf( pFile, "%s)\n", Ntk_NodeGetNameLong( Ntk_NodeReadFaninNode(pNode,1) ) );
        }
    }
    fprintf( pFile, "\n" );
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


