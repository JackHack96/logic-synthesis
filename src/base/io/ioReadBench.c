/**CFile****************************************************************

  FileName    [ioReadBench.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Reads the BENCH format.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioReadBench.c,v 1.1 2005/06/02 03:34:08 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define BENCH_BUFF_SIZE  1000

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadNetworkBench( Mv_Frame_t * pMvsis, char * pFileName )
{
    FILE * pFile;
    Ntk_Network_t * pNet;
    Ntk_Latch_t * pLatch;
    Ntk_NodeVec_t * vNamesPO, * vNamesLI, * vNamesLO, * vFanins;
    Ntk_Node_t * pNode, * pFanin, * pNodePres;
    char Buffer[BENCH_BUFF_SIZE];
    char * pTemp, * pToken, * pType, * pNodeName;
    int * pCompl, fCompl, nLines, i;
    
    // allocate the empty network
    pNet = Ntk_NetworkAlloc( pMvsis );
    // set the specs
    pNet->pName = util_strsav( pFileName );
    pNet->pSpec = util_strsav( pFileName );

    // go through the lines of the file
    vNamesPO = Ntk_NodeVecAlloc( 100 );
    vNamesLI = Ntk_NodeVecAlloc( 100 );
    vNamesLO = Ntk_NodeVecAlloc( 100 );
    vFanins  = Ntk_NodeVecAlloc( 100 );
    pCompl = ALLOC( int, BENCH_BUFF_SIZE );
    nLines = 0;
    pFile = fopen( pFileName, "r" );
    while ( pTemp = fgets(Buffer, BENCH_BUFF_SIZE-1, pFile) )
    {
        nLines++;
        // skip empty lines and comment lines
        while ( *pTemp && (*pTemp==' ' || *pTemp=='\n' || *pTemp=='\r' || *pTemp=='\t') )
            pTemp++;
        if ( pTemp[0] == 0 || pTemp[0] == '#' )
            continue;
        // get the type of the line
        if ( strncmp( pTemp, "INPUT", 5 ) == 0 )
        {
            pToken = strtok( pTemp + 5, "() \n\r" );
            pNode = Ntk_NetworkFindOrAddNodeByName( pNet, pToken, MV_NODE_CI );
            Ntk_NodeSetValueNum( pNode, 2 );
        }
        else if ( strncmp( pTemp, "OUTPUT", 5 ) == 0 )
        {
            pToken = strtok( pTemp + 6, "() \n\r" );
            Ntk_NodeVecPush( vNamesPO, (Ntk_Node_t *)util_strsav(pToken) );
        }
        else 
        {
            // get the node name and the node type
            pToken = strtok( pTemp, ",()= \n\r" );
            pType  = strtok( NULL, ",()= \n\r" );
            fCompl = !strcmp(pType,"NAND") || !strcmp(pType,"NOR");
            if ( strcmp(pType, "DFF") == 0 )
            {
                // create the LO (PI)
                pNode = Ntk_NetworkFindOrAddNodeByName( pNet, pToken, MV_NODE_CI );
                Ntk_NodeSetValueNum( pNode, 2 );
                Ntk_NodeVecPush( vNamesLO, (Ntk_Node_t *)util_strsav(pToken) );
                // save the LI (PO)
                pToken = strtok( NULL, ",()= \n\r" );
                Ntk_NodeVecPush( vNamesLI, (Ntk_Node_t *)util_strsav(pToken) );
            }
            else
            {
                // save the name of this node
                pNodeName = pToken;
                // process the fanins
                Ntk_NodeVecClear( vFanins );
                while ( pToken = strtok( NULL, ",()= \n\r" ) )
                {
                    pCompl[ vFanins->nSize ] = fCompl;
                    pFanin = Ntk_NetworkFindOrAddNodeByName( pNet, pToken, MV_NODE_INT );
                    Ntk_NodeSetValueNum( pFanin, 2 );
                    Ntk_NodeVecPush( vFanins, pFanin );
                }
                // create the internal node
                if ( strncmp(pType, "BUF", 3) == 0 )
                    pNode = Ntk_NodeCreateOneInputBinary( pNet, vFanins->pArray[0], 0 );
                else if ( strcmp(pType, "NOT") == 0 )
                    pNode = Ntk_NodeCreateOneInputBinary( pNet, vFanins->pArray[0], 1 );
                else if ( strcmp(pType, "AND") == 0 )
                    pNode = Ntk_NodeCreateSimpleBinary( pNet, vFanins->pArray, pCompl, vFanins->nSize, 0 );
                else if ( strcmp(pType, "NOR") == 0 )
                    pNode = Ntk_NodeCreateSimpleBinary( pNet, vFanins->pArray, pCompl, vFanins->nSize, 0 );
                else if ( strcmp(pType, "OR") == 0 )
                    pNode = Ntk_NodeCreateSimpleBinary( pNet, vFanins->pArray, pCompl, vFanins->nSize, 1 );
                else if ( strcmp(pType, "NAND") == 0 )
                    pNode = Ntk_NodeCreateSimpleBinary( pNet, vFanins->pArray, pCompl, vFanins->nSize, 1 );
                else if ( strcmp(pType, "XOR") == 0 )
                    pNode = Ntk_NodeCreateTwoInputBinary( pNet, vFanins->pArray, 6 );
                else if ( strcmp(pType, "NXOR") == 0 )
                    pNode = Ntk_NodeCreateTwoInputBinary( pNet, vFanins->pArray, 9 );
                else
                {
                    printf( "Cannot determine gate type \"%s\" in line %d.\n", pType, nLines );
                    Ntk_NetworkDelete( pNet );
                    return NULL;
                }
                // assign the name to this node
                if ( pNodePres = Ntk_NetworkFindNodeByName( pNet, pNodeName ) )
                    Ntk_NodeReplace( pNodePres, pNode );
                else
                {
                    Ntk_NodeSetName( pNode, Ntk_NetworkRegisterNewName(pNet, pNodeName) );
                    Ntk_NetworkAddNode( pNet, pNode, 1 );
                }
            }
        }
    }
    free( pCompl );

    // create latches
    for ( i = 0; i < vNamesLI->nSize; i++ )
    {
        // make latch LI another CO
        pNode = Ntk_NetworkFindNodeByName( pNet, (char *)vNamesLI->pArray[i] );
        if ( Ntk_NodeIsInternal(pNode) )
            Ntk_NetworkAddNodeCo( pNet, pNode, 1 );
        else
        {
            assert( 0 );
        }
        // create latch
        pLatch = Ntk_LatchCreate( pNet, NULL, 0, (char *)vNamesLI->pArray[i], (char *)vNamesLO->pArray[i] );
        Ntk_NodeSetSubtype( Ntk_LatchReadInput(pLatch), MV_BELONGS_TO_LATCH );
        Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_LATCH );
        Ntk_NetworkAddLatch( pNet, pLatch );
        free( vNamesLI->pArray[i] );
        free( vNamesLO->pArray[i] );
    }
    Ntk_NodeVecFree( vNamesLI );
    Ntk_NodeVecFree( vNamesLO );

    // create the POs
    for ( i = 0; i < vNamesPO->nSize; i++ )
    {
        pNode = Ntk_NetworkFindNodeByName( pNet, (char *)vNamesPO->pArray[i] );
        if ( Ntk_NodeIsInternal(pNode) )
            Ntk_NetworkAddNodeCo( pNet, pNode, 1 );
        else
        {
            assert( Ntk_NodeIsCi(pNode) );
            fprintf( Ntk_NodeReadMvsisOut(pNode), "Warning: input and output named \"%s\": ", Ntk_NodeReadName(pNode) );
            Ntk_NetworkTransformCiToCo( pNet, pNode );
            fprintf( Ntk_NodeReadMvsisOut(pNode), "renaming input \"%s\".\n", Ntk_NodeReadName(pNode) );
        }
        free( vNamesPO->pArray[i] );
    }
    Ntk_NodeVecFree( vNamesPO );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNet );
    Ntk_NetworkOrderFanins( pNet );

    // make sure that everything is okay with the network structure
    if ( !Ntk_NetworkCheck( pNet ) )
    {
        printf( "The netowrk check has failed.\n" );
        Ntk_NetworkDelete( pNet );
        return NULL;
    }
    return pNet;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


