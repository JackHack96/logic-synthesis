/**CFile****************************************************************

  FileName    [ioWriteMapped.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioWriteMapped.c,v 1.3 2005/04/10 23:27:01 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "ioInt.h"
#include "mio.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the network out as a mapped blif.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NetworkWriteMappedBlif( Ntk_Network_t * pNet, char * pFileNameOut )
{
    int fWriteLatches = 0;  // Mapping doesn't handle latches yet
    FILE * pFile;
    Ntk_Node_t * pNode, * pFanin, * pFanout;
    Ntk_Pin_t * pPin;
    Mio_Pin_t * pGatePin;
    Mio_Gate_t * pGate;
    Ntk_NodeBinding_t * pBinding;
    int i;
    Ntk_Node_t ** ppOrderedFanInArray;
    Ntk_Network_t * pNetExdc;

    pFile = fopen( pFileNameOut, "w" );
    if ( pFile == NULL )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Io_WriteNetwork(): Cannot open the output file %s\n",
                pFileNameOut );
        return;
    }

    fprintf( pFile, ".model %s\n", Ntk_NetworkReadName(pNet) );

    fprintf( pFile, ".inputs" );
    Io_WriteNetworkCis( pFile, pNet, fWriteLatches );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".outputs" );
    Io_WriteNetworkCos( pFile, pNet, fWriteLatches );
    fprintf( pFile, "\n" );

    assert ( fWriteLatches == 0 ); // Can't handle latches yet

    fprintf( pFile, ".default_input_arrival %g %g\n", 
            pNet->dDefaultArrTimeRise,
            pNet->dDefaultArrTimeFall );
    
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if (   (pNode->dArrTimeRise == pNet->dDefaultArrTimeRise)
            && (pNode->dArrTimeFall == pNet->dDefaultArrTimeFall) )
            continue;
        // Print only if this input has special arrival time
        fprintf( pFile, ".input_arrival %s %g %g\n", 
                Ntk_NodeGetNameLong(pNode),
                pNode->dArrTimeRise,
                pNode->dArrTimeFall );
    }

    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeIsCoFanin(pNode) && !Ntk_NodeHasCoName(pNode) )
        {
            // Write a buffer connecting node to each of it's Cos
            Ntk_NodeForEachFanout( pNode, pPin, pFanout )
            {
                if ( Ntk_NodeIsCo(pFanout) )
                {
                    fprintf ( pFile, ".names %s %s\n", Ntk_NodeGetNameLong(pNode), 
                        Ntk_NodeGetNameLong(pFanout) );
                    fprintf ( pFile, "1 1\n\n" );
                }
            }
        } 

        pBinding = (Ntk_NodeBinding_t *)Ntk_NodeReadMap( pNode );
        assert( pBinding != NULL );
        
        // The following is needed since the node may be a constant node
        if ( Ntk_NodeBindingReadGate( pBinding ) == NULL )   // Not a node generated directly by mapping
        {
            Io_WriteNetworkNode( pFile, pNode, IO_FILE_BLIF, 0 );
        }
        else
        {
            pGate = (Mio_Gate_t *)Ntk_NodeBindingReadGate( pBinding );
            ppOrderedFanInArray = Ntk_NodeBindingReadFanInArray( pBinding );

            fprintf( pFile, ".gate %s ", Mio_GateReadName( pGate ));

            for ( pGatePin = Mio_GateReadPins( pGate ), i = 0; pGatePin; pGatePin = Mio_PinReadNext( pGatePin ), ++i )
            {
                fprintf( pFile, "%s=%s ", Mio_PinReadName( pGatePin ), Ntk_NodeGetNameLong( ppOrderedFanInArray[i] ));
            }
            assert ( i == Mio_GateReadInputs( pGate ) );

            fprintf( pFile, "%s=%s\n", Mio_GateReadOutName( pGate ), Ntk_NodeGetNameLong( pNode ));
        }
    }

    Ntk_NetworkForEachCo( pNet, pNode )
    {
        // Direct connection from PI to PO, write buffer
        pFanin = Ntk_NodeReadFaninNode( pNode, 0 );
        if (Ntk_NodeIsCi(pFanin))
        {
            fprintf ( pFile, ".names %s %s\n", Ntk_NodeGetNameLong(pFanin),
                Ntk_NodeGetNameLong(pNode) );
            fprintf ( pFile, "1 1\n\n" );
        } 
    }

    pNetExdc = Ntk_NetworkReadNetExdc( pNet );
    if ( pNetExdc )
    {
        fprintf( pFile, "\n" );
        fprintf( pFile, ".exdc\n" );
        Io_WriteNetworkOne( pFile, pNetExdc, IO_FILE_BLIF, 0 );
    }

    fprintf( pFile, ".end\n" );
    fclose( pFile );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


