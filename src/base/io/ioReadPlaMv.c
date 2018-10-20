/**CFile****************************************************************

  FileName    [ioReadPlaMv.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to read files in Espresso MV PLA notation.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioReadPlaMv.c,v 1.7 2003/09/01 04:55:53 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MV_PLA_BUFFER_SIZE  2000

static Vm_VarMap_t * Io_ReadPlaMvGetVarMap( FILE * pFile, FILE * pOutput, Vm_Manager_t * pVmMan );
static char ** Io_ReadPlaMvGetNames( FILE * pFile, FILE * pOutput, Vm_VarMap_t * pVm, bool fInputs );
static Mvc_Cover_t * Io_ReadPlaMvGetMvc( FILE * pFile, FILE * pOutput, Vm_VarMap_t * pVm, Mvc_Manager_t * pMvcMan );
static Ntk_Network_t * Io_ReadPlaMvConstructNetwork( Vm_VarMap_t * pVm, 
    char ** ppNamesInput, char ** ppNamesOutput, Mvc_Cover_t * pMvc,
    Mv_Frame_t * pMvsis, char * FileName, bool fBinaryOutput );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads Espresso MV-PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadPlaMv( Mv_Frame_t * pMvsis, char * FileName, bool fBinaryOutput )
{
    Ntk_Network_t * pNet;
    char ** ppNamesInput, ** ppNamesOutput;
    FILE * pFile, * pOutput;
    Vm_VarMap_t * pVm;
    Mvc_Cover_t * pMvc;
    Vm_Manager_t * pVmMan;
    Mvc_Manager_t * pMvcMan;
    int i;

    // get the managers
    pVmMan  = Fnc_ManagerReadManVm(Mv_FrameReadMan(pMvsis));
    pMvcMan = Fnc_ManagerReadManMvc(Mv_FrameReadMan(pMvsis));

    // open the file
    pOutput = Mv_FrameReadErr(pMvsis);
    pFile   = fopen( FileName, "r" );
    if ( pFile == NULL )
    {
        fprintf( pOutput, "Cannot open the input file \"%s\".\n", FileName );
        return NULL;
    }

    // read the header to determine VarMap parameters
    pVm = Io_ReadPlaMvGetVarMap( pFile, pOutput, pVmMan );
    if ( pVm == NULL )
    {
        fprintf( pOutput, "Cannot read the values of inputs/outputs.\n" );
        return NULL;
    }


    // read the input/output names
    ppNamesInput  = Io_ReadPlaMvGetNames( pFile, pOutput, pVm, 0 );
    ppNamesOutput = Io_ReadPlaMvGetNames( pFile, pOutput, pVm, 1 );

    // read the body to determine the Mvc cover
    pMvc = Io_ReadPlaMvGetMvc( pFile, pOutput, pVm, pMvcMan );
    if ( pMvc == NULL )
    {
        fprintf( pOutput, "Cannot read the cube table.\n" );
        return NULL;
    }

    // create the network composed of one node with this cover
    pNet = Io_ReadPlaMvConstructNetwork( pVm, ppNamesInput, ppNamesOutput, pMvc, 
        pMvsis, FileName, fBinaryOutput );
    // deallocate the input/output names
    for ( i = 0; i < Vm_VarMapReadVarsInNum(pVm); i++ )
        FREE( ppNamesInput[i] );
    for ( i = 0; i < Vm_VarMapReadVarsOutNum(pVm); i++ )
        FREE( ppNamesOutput[i] );
    FREE( ppNamesInput );
    FREE( ppNamesOutput );
    assert( pNet );
    return pNet;
}

/**Function*************************************************************

  Synopsis    [Reads the variable map form the input MV-PLA file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vm_VarMap_t * Io_ReadPlaMvGetVarMap( FILE * pFile, FILE * pOutput, Vm_Manager_t * pVmMan )
{
    char Buffer[MV_PLA_BUFFER_SIZE];
    char * pTemp;
    int nVarsTotal, nVarsBin, i;
    int * pValues;
    Vm_VarMap_t * pVm;

    // find .mv line
    rewind( pFile );
    while ( pTemp = fgets( Buffer, MV_PLA_BUFFER_SIZE, pFile ) )
        if ( strcmp( Buffer, ".mv" ) == 0 )
            break;
    if ( pTemp == NULL )
        return NULL;    

    // read .mv line
    pTemp = strtok( Buffer, " \n\r\t" );
    assert( strcmp( pTemp, ".mv" ) == 0 );
    // get the total number of variables
    pTemp = strtok( NULL, " \n\r\t" );
    if ( pTemp == NULL )
    {
        fprintf( pOutput, "Not enough entries on the .mv line.\n" );
        return NULL;
    }
    nVarsTotal = atoi( pTemp );
    // get the number of binary variables
    pTemp = strtok( NULL, " \n\r\t" );
    if ( pTemp == NULL )
    {
        fprintf( pOutput, "Not enough entries on the .mv line.\n" );
        return NULL;
    }
    nVarsBin = atoi( pTemp );
    // read the values of MV variables
    pValues = ALLOC( int, nVarsTotal );
    for ( i = 0; i < nVarsBin; i++ )
        pValues[i] = 2;
    for (      ; i < nVarsTotal; i++ )
    {
        pTemp = strtok( NULL, " \n\r\t" );
        if ( pTemp == NULL )
        {
            fprintf( pOutput, "Not enough entries on the .mv line.\n" );
            return NULL;
        }
        pValues[i] = atoi( pTemp );
    }
    // check the remaining chars on the line
    pTemp = strtok( NULL, " \n\r\t" );
    if ( pTemp != NULL )
    {
        fprintf( pOutput, "Trailing symbols on .mv line.\n" );
        return NULL;
    }
    // create the variable map
    pVm = Vm_VarMapLookup( pVmMan, nVarsTotal - 1, 1, pValues );
    FREE( pValues );
    return pVm;
}

/**Function*************************************************************

  Synopsis    [Reads the array of variable names from the input MV-PLA file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char ** Io_ReadPlaMvGetNames( FILE * pFile, FILE * pOutput, Vm_VarMap_t * pVm, bool fInputs )
{
    char Buffer[MV_PLA_BUFFER_SIZE];
    char * pTemp;
    char ** ppNames;
    int nNames, i;

    nNames = fInputs? Vm_VarMapReadVarsInNum(pVm): Vm_VarMapReadVarsOutNum(pVm);
    ppNames = ALLOC( char *, nNames );

    // find .mv line
    rewind( pFile );
    while ( pTemp = fgets( Buffer, MV_PLA_BUFFER_SIZE, pFile ) )
        if (  fInputs && strcmp( Buffer, ".ilb" ) == 0 || 
             !fInputs && strcmp( Buffer, ".ob" )  == 0 )
            break;
    if ( pTemp == NULL )
    {
        int nDigits;
	    for ( nDigits = 0, i = nNames - 1; i;  i /= 10,  nDigits++ );
        // create the artificial names
        for ( i = 0; i < nNames; i++ )
        {
            sprintf( Buffer, "%c%0*d", fInputs? 'x': 'z', nDigits, i );
            ppNames[i] = util_strsav(Buffer);
        }
    }
    else
    {
        // read .mv line
        pTemp = strtok( Buffer, " \n\r\t" );
        assert (  fInputs && strcmp( pTemp, ".ilb" ) == 0 || 
                 !fInputs && strcmp( pTemp, ".ob" )  == 0 );
        // read the names
        for ( i = 0; i < nNames; i++ )
        {
            pTemp = strtok( NULL, " \n\r\t" );
            if ( pTemp == NULL )
            {
                fprintf( pOutput, "Not enough entries on the input/output names line.\n" );
                return NULL;
            }
            ppNames[i] = util_strsav(pTemp);
        }
        // check the remaining chars on the line
        pTemp = strtok( NULL, " \n\r\t" );
        if ( pTemp != NULL )
        {
            fprintf( pOutput, "Trailing symbols on the input/output names line.\n" );
            return NULL;
        }
    }
    return ppNames;
}

/**Function*************************************************************

  Synopsis    [Reads the MV-PLA table from the input file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Io_ReadPlaMvGetMvc( FILE * pFile, FILE * pOutput, Vm_VarMap_t * pVm, Mvc_Manager_t * pMvcMan )
{
    char Buffer[MV_PLA_BUFFER_SIZE];
    char * pTemp;
    int i, v, k, nValuesTotal;
    int * pValuesFirst;
    Mvc_Cover_t * pMvc;
    Mvc_Cube_t * pCube;

    // find the first table line
    rewind( pFile );
    while ( pTemp = fgets( Buffer, MV_PLA_BUFFER_SIZE, pFile ) )
        if (  Buffer[0] != '#' && Buffer[0] != '.' )
            break;
    if ( pTemp == NULL )
    {
        fprintf( pOutput, "Cannot find the beginning of the MV-PLA table.\n" );
        return NULL;
    }

    // create the cover
    nValuesTotal = Vm_VarMapReadValuesNum(pVm);
    pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);
    pMvc = Mvc_CoverAlloc( pMvcMan, nValuesTotal );

    // read the table, line by line
    do
    {
        // skip if it is the end of file
        if ( Buffer[0] == '.' )
            break;

        // start the cube
        pCube = Mvc_CubeAlloc( pMvc );
        Mvc_CubeBitFill( pCube );

        // skip the meaningless bits
        for ( k = 0; Buffer[k]; k++ )
            if ( Buffer[k] == '0' || Buffer[k] == '1' )
                break;
        // read the bits of this cube
        i = v = 0; 
        for ( ; Buffer[k]; k++ )
            if ( Buffer[k] == '1' )
            {
                v++;
            }
            else if ( Buffer[k] == '0' )
            {
                Mvc_CubeBitInsert( pCube, v );
                v++;
            }
            else
            {
                // skip all empty chars
                for ( ; Buffer[k]; k++ )
                    if ( Buffer[k] == '0' || Buffer[k] == '1' )
                        break;
                k--;
                // increment the number of vars
                i++;
                // check that the number of values is okay
                assert( pValuesFirst[i] == v );
            }
        // add the cube
        Mvc_CoverAddCubeTail( pMvc, pCube );
    }
    while ( pTemp = fgets( Buffer, MV_PLA_BUFFER_SIZE, pFile ) );
    return pMvc;
}

/**Function*************************************************************

  Synopsis    [Creates the network.]

  Description [Create the network with one internal node which has
  the cover specified in the MV-PLA file. If the network is binary output 
  (fBinaryOutput == 1), then the input/output variables in the input file
  are used as input variables, and the new binary variable is added as 
  the output variable. Otherwise, the last variables in the file is 
  treated as the output variable.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadPlaMvConstructNetwork( Vm_VarMap_t * pVm, 
    char ** ppNamesInput, char ** ppNamesOutput, Mvc_Cover_t * pMvc,
    Mv_Frame_t * pMvsis, char * FileName, bool fBinaryOutput )
{
    Ntk_Network_t * pNetNew;
    Ntk_Node_t * pNodePi, * pNode;
    int nVarsIn, nVarsOut;
    int * pValues;
    int i, nValuesOut, nValuesIn;
    Vm_VarMap_t * pVmNew;
    Cvr_Cover_t * pCover;
    Mvc_Cover_t ** ppIsets;
    Mvc_Cube_t * pCube, * pCubeCopy;

    // allocate the empty network
    pNetNew = Ntk_NetworkAlloc( pMvsis );

    // register the name of the node as the name of the network
    if ( ppNamesOutput[0] )
        pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, ppNamesOutput[0] );
    // register the network spec file name
//    if ( SpecName )
//        pNetNew->pSpec = Ntk_NetworkRegisterNewName( pNetNew, FileName );

    // get the number of variables and values
    nVarsIn  = Vm_VarMapReadVarsInNum(pVm);
    nVarsOut = Vm_VarMapReadVarsOutNum(pVm);
    assert( nVarsOut == 1 );
    pValues    = Vm_VarMapReadValuesArray(pVm);
    nValuesIn  = Vm_VarMapReadValuesInNum(pVm);
    nValuesOut = fBinaryOutput? 2 : pValues[nVarsIn];

    // create the CI nodes
    for ( i = 0; i < nVarsIn; i++ )
    {
        pNodePi = Ntk_NodeCreate( pNetNew, ppNamesInput[i], MV_NODE_CI, pValues[i] );
        Ntk_NetworkAddNode( pNetNew, pNodePi, 1 );
        i++;
    }

    // add the output variable as input, if the overall output is binary
    if ( fBinaryOutput )
    {
        pNodePi = Ntk_NodeCreate( pNetNew, ppNamesOutput[0], MV_NODE_CI, pValues[nVarsIn] );
        Ntk_NetworkAddNode( pNetNew, pNodePi, 1 );
        i++;
    }

    // create the given node
    pNode = Ntk_NodeCreate( pNetNew, "out", MV_NODE_INT, nValuesOut );
    // add the fanins to the given node
    Ntk_NetworkForEachCi( pNetNew, pNodePi )
        Ntk_NodeAddFanin( pNode, pNodePi );
    // add the internal node
    Ntk_NetworkAddNode( pNetNew, pNode, 1 );

    // create the functionality
    if ( fBinaryOutput )
    {
        pVmNew = Vm_VarMapCreateOnePlus( pVm, 2 );
        ppIsets = ALLOC( Mvc_Cover_t *, 2 );
        ppIsets[0] = NULL;
        ppIsets[1] = pMvc;
        pCover = Cvr_CoverCreate( pVmNew, ppIsets );
    }
    else
    {
        pVmNew = Vm_VarMapCreateOneMinus( pVm );
        ppIsets = ALLOC( Mvc_Cover_t *, nValuesOut );
        for ( i = 0; i < nValuesOut; i++ )
            ppIsets[i] = Mvc_CoverAlloc( pMvc->pMem, nValuesIn );
        // sort the cubes
        Mvc_CoverForEachCube( pMvc, pCube )
        {
            for ( i = 0; i < nValuesOut; i++ )
                if ( Mvc_CubeBitValue(pCube, nValuesIn + i) )
                {
                    pCubeCopy = Mvc_CubeDup( ppIsets[i], pCube );
                    Mvc_CubeBitCleanUnused( pCubeCopy );
                    Mvc_CoverAddCubeTail( ppIsets[i], pCubeCopy );
                }
        }
        pCover = Cvr_CoverCreate( pVmNew, ppIsets );
        Mvc_CoverFree( pMvc );
    }
    // set the functionality
    Ntk_NodeWriteFuncVm( pNode, pVmNew );
    Ntk_NodeWriteFuncCvr( pNode, pCover );


    // create the CO node
    Ntk_NetworkAddNodeCo( pNetNew, pNode, 1 );

    // put IDs into a proper order
    Ntk_NetworkReassignIds( pNetNew );

    if ( !Ntk_NetworkCheck( pNetNew ) )
        fprintf( Mv_FrameReadOut(pMvsis), "Io_ReadPlaMvConstructNetwork(): Network check has failed.\n" );
    return pNetNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


