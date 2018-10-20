/**CFile****************************************************************

  FileName    [ioWritePla.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Writes the binary two-level circuits into a file as a PLA.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioWritePla.c,v 1.5 2003/09/01 04:55:53 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Io_WritePlaOne( FILE * pFile, Ntk_Network_t * pNet );
static void Io_WritePlaOneNode( FILE * pFile, Ntk_Node_t * pNode, int nIns, int nOuts, int iOut );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WritePla( Ntk_Network_t * pNet, char * FileName )
{
    FILE * pFile;
    pFile = fopen( FileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Io_WritePla(): Cannot open the output file.\n" );
        return;
    }
    // write the model name
    fprintf( pFile, "#.model %s\n", Ntk_NetworkReadName(pNet) );
    // write the network
    Io_WritePlaOne( pFile, pNet );
    // finalize the file
    fprintf( pFile, ".e\n" );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WritePlaOne( FILE * pFile, Ntk_Network_t * pNet )
{
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** ppIsets;
    Mvc_Cover_t * pMvc;
    Ntk_Node_t * pNode, * pDriver;
    int nInputs, nOutputs, i;
    int nCubes;

    // count the number of cubes
    nCubes = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        if ( Ntk_NodeIsCi(pDriver) )
        {
            nCubes++;
            continue;
        }
        pCvr = Ntk_NodeGetFuncCvr( pDriver );
        ppIsets = Cvr_CoverReadIsets( pCvr );
        pMvc = ppIsets[1];
        if ( pMvc == NULL )
        {
            nCubes = -1;
            break;
        }
        nCubes += Mvc_CoverReadCubeNum(pMvc);
    }

    nInputs = Ntk_NetworkReadCiNum(pNet);
    nOutputs = Ntk_NetworkReadCoNum(pNet);

    // write the header
    fprintf( pFile, ".i %d\n", nInputs );
    fprintf( pFile, ".o %d\n", nOutputs );
    // write the PIs
    fprintf( pFile, ".ilb" );
    // output the CI names in the order of increasing ID
//    Ntk_NetworkForEachCi( pNet, pNode )
    for ( i = 1; i <= nInputs; i++ )
    {
        pNode = Ntk_NetworkFindNodeById( pNet, i );
        assert( Ntk_NodeIsCi(pNode) );
        fprintf( pFile, " %s", Ntk_NodeReadName(pNode) );
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, ".ob" );
    Ntk_NetworkForEachCo( pNet, pNode )
        fprintf( pFile, " %s", Ntk_NodeReadName(pNode) );
    fprintf( pFile, "\n" );
    // write the number of cubes
    if ( nCubes != -1 )
        fprintf( pFile, ".p %d\n", nCubes );

    // write each internal node
    i = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
        Io_WritePlaOneNode( pFile, pDriver, nInputs, nOutputs, i++ );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WritePlaOneNode( FILE * pFile, Ntk_Node_t * pNode, int nIns, int nOuts, int iOut )
{
    Mvc_Data_t * pData; 
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** ppIsets;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pCube;
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    char * pPartIn, * pPartOut;
    int Value0, Value1;
    int nFanins, Id, i;
    bool fDerivedCover;

    pPartIn  = ALLOC( char, nIns  + 1 );
    pPartOut = ALLOC( char, nOuts + 1 );
    pPartIn[nIns]   = 0;
    pPartOut[nOuts] = 0;

    // print the output part
    for ( i = 0; i < nOuts; i++ )
        pPartOut[i] = '0';
    pPartOut[iOut] = '1';

    if ( Ntk_NodeIsCi(pNode) )
    {
        for ( i = 0; i < nIns; i++ )
            pPartIn[i] = '-';
        pPartIn[Ntk_NodeReadId(pNode)-1] = '1';
        fprintf( pFile, "%s %s\n", pPartIn, pPartOut );
    }
    else if ( Ntk_NodeReadFaninNum(pNode) == 0 )
    {
        pCvr = Ntk_NodeReadFuncCvr( pNode );
        ppIsets = Cvr_CoverReadIsets( pCvr );
        pMvc = ppIsets[1];
        if ( Mvc_CoverReadCubeNum(pMvc) == 1 )
        { // print the constant 1 function
            for ( i = 0; i < nIns; i++ )
                pPartIn[i] = '-';
            fprintf( pFile, "%s %s\n", pPartIn, pPartOut );
        }
        // constant 0 function need not to be printed
    }
    else
    {
        nFanins = Ntk_NodeReadFaninNum( pNode );
        pCvr = Ntk_NodeGetFuncCvr( pNode );
        ppIsets = Cvr_CoverReadIsets( pCvr );
        pMvc = ppIsets[1];

        fDerivedCover = 0;
        if ( pMvc == NULL )
        {
            pData = Mvc_CoverDataAlloc( Ntk_NodeReadFuncVm(pNode), ppIsets[0] ); 
            pMvc = Mvc_CoverComplement( pData, ppIsets[0] );
            Mvc_CoverDataFree( pData, ppIsets[0] );
            fDerivedCover = 1;
        }

        // go through the cubes
        Mvc_CoverForEachCube( pMvc, pCube )
        {
            /// fill in the don't-cares
            for ( i = 0; i < nIns; i++ )
                pPartIn[i] = '-';

            // fill in the cube
            Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
            {
                Id = Ntk_NodeReadId(pFanin);
                assert( Id <= nIns );
                Value0 = Mvc_CubeBitValue( pCube, 2 * i + 0 );
                Value1 = Mvc_CubeBitValue( pCube, 2 * i + 1 );
                if ( !Value0 && Value1 )
                    pPartIn[Id-1] = '1';
                else if ( Value0 && !Value1 )
                    pPartIn[Id-1] = '0';
                else if ( !Value0 && !Value1 )
                {
                    assert( 0 );
                }
            }

            // print the cube
            fprintf( pFile, "%s %s\n", pPartIn, pPartOut );
        }

        if ( fDerivedCover )
            Mvc_CoverFree( pMvc );
    }

    FREE( pPartIn );
    FREE( pPartOut );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


