/**CFile****************************************************************

  FileName    [ioReadPla.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to read the binary PLA file.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioReadPla.c,v 1.11 2003/11/18 18:54:52 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct IoReadPlaStruct Io_ReadPla_t;
struct IoReadPlaStruct
{
    // the file contents
    char *      pBuffer;
    int         FileSize;
    // the file parts
    char *      pDotType;
    char *      pDotI;
    char *      pDotO;
    char *      pDotP;
    char *      pDotIlb;
    char *      pDotOb;
    char *      pDotStart;
    char *      pDotEnd;
    // output for error messages
    FILE *      pOutput;
    // the number of inputs and outputs
    int         nInputs;
    int         nOutputs;
    // the input/output names
    char **     pNamesIn;
    char **     pNamesOut;
    // the covers
    Mvc_Cover_t ** ppOns;
    Mvc_Cover_t ** ppDcs;
    // the functionality manager
    Fnc_Manager_t * pMan;
};


static bool Io_ReadPlaFindDotLines( Io_ReadPla_t * p ); 
static bool Io_ReadPlaHeader( Io_ReadPla_t * p );
static bool Io_ReadPlaBody( Io_ReadPla_t * p );
static Ntk_Network_t * Io_ReadPlaConstructNetwork( Io_ReadPla_t * p, Mv_Frame_t * pMvsis, char * FileName );
static void Io_ReadPlaCleanUp( Io_ReadPla_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
/**Function*************************************************************

  Synopsis    [Reads Espresso PLA format.]

  Description [This procedure reads the standard (fd) Espresso PLA
  file format. The don't-cares are either ignored or added to the
  i-sets.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadPla( Mv_Frame_t * pMvsis, char * FileName )
{
    Ntk_Network_t * pNet = NULL;
    Io_ReadPla_t * p;

    // allocate the structure
    p = ALLOC( Io_ReadPla_t, 1 );
    memset( p, 0, sizeof(Io_ReadPla_t) );
    // set the output file stream for errors
    p->pOutput = Mv_FrameReadErr(pMvsis);
    p->pMan = Mv_FrameReadMan(pMvsis);

    // read the file
    p->pBuffer = Io_ReadFileFileContents( FileName, &p->FileSize );
    // remove comments if any
    Io_ReadFileRemoveComments( p->pBuffer, NULL, NULL );

    // split the file into parts
    if ( !Io_ReadPlaFindDotLines( p ) )
        goto cleanup;

    // read the header of the file
    if ( !Io_ReadPlaHeader( p ) )
        goto cleanup;

    // read the body of the file
    if ( !Io_ReadPlaBody( p ) )
        goto cleanup;

    // construct the network
    pNet = Io_ReadPlaConstructNetwork( p, pMvsis, FileName );

cleanup:
    // free storage
    Io_ReadPlaCleanUp( p );
    return pNet;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Io_ReadPlaFindDotLines( Io_ReadPla_t * p )
{
    char * pCur, * pCurMax;
    for ( pCur = p->pBuffer; *pCur; pCur++ )
        if ( *pCur == '.' )
        {
            if ( pCur[1] == 'i' && pCur[2] == ' ' )
            {
                if ( p->pDotI )
                {
                    fprintf( p->pOutput, "The PLA file contains multiple .i lines.\n" );
                    return 0;
                }
                p->pDotI = pCur;
            }
            else if ( pCur[1] == 'o' && pCur[2] == ' ' )
            {
                if ( p->pDotO )
                {
                    fprintf( p->pOutput, "The PLA file contains multiple .o lines.\n" );
                    return 0;
                }
                p->pDotO = pCur;
            }
            else if ( pCur[1] == 'p' && pCur[2] == ' ' )
            {
                if ( p->pDotP )
                {
                    fprintf( p->pOutput, "The PLA file contains multiple .p lines.\n" );
                    return 0;
                }
                p->pDotP = pCur;
            }
            else if ( pCur[1] == 'i' && pCur[2] == 'l' && pCur[3] == 'b' && pCur[4] == ' ' )
            {
                if ( p->pDotIlb )
                {
                    fprintf( p->pOutput, "The PLA file contains multiple .ilb lines.\n" );
                    return 0;
                }
                p->pDotIlb = pCur;
            }
            else if ( pCur[1] == 'o' && pCur[2] == 'b' && pCur[3] == ' ' )
            {
                if ( p->pDotOb )
                {
                    fprintf( p->pOutput, "The PLA file contains multiple .ob lines.\n" );
                    return 0;
                }
                p->pDotOb = pCur;
            }
            else if ( pCur[1] == 't' && pCur[2] == 'y' && pCur[3] == 'p' && pCur[4] == 'e' && pCur[5] == ' ' )
            {
                if ( p->pDotType )
                {
                    fprintf( p->pOutput, "The PLA file contains multiple .type lines.\n" );
                    return 0;
                }
                p->pDotType = pCur;
            }
            else if ( pCur[1] == 'e' )
            {
                p->pDotEnd = pCur;
                break;
            }
        }
    if ( p->pDotI == NULL )
    {
        fprintf( p->pOutput, "The PLA file does not contain .i line.\n" );
        return 0;
    }
    if ( p->pDotO == NULL )
    {
        fprintf( p->pOutput, "The PLA file does not contain .o line.\n" );
        return 0;
    }
    // find the beginning of the table
    pCurMax = p->pBuffer;
    if ( p->pDotI && pCurMax < p->pDotI )
        pCurMax = p->pDotI;
    if ( p->pDotO && pCurMax < p->pDotO )
        pCurMax = p->pDotO;
    if ( p->pDotP && pCurMax < p->pDotP )
        pCurMax = p->pDotP;
    if ( p->pDotIlb && pCurMax < p->pDotIlb )
        pCurMax = p->pDotIlb;
    if ( p->pDotOb && pCurMax < p->pDotOb )
        pCurMax = p->pDotOb;
    if ( p->pDotType && pCurMax < p->pDotType )
        pCurMax = p->pDotType;
    // go to the next new line symbol
    for ( ; *pCurMax; pCurMax++ )
        if ( *pCurMax == '\n' )
        {
            p->pDotStart = pCurMax + 1;
            break;
        }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Io_ReadPlaHeader( Io_ReadPla_t * p )
{
    char * pTemp;
    int i;

    // read .type line
    if ( p->pDotType )
    {
        pTemp = strtok( p->pDotType, " \t\n\r" );
        assert( strcmp( p->pDotType, ".type" ) == 0 );
        pTemp = strtok( NULL, " \t\n\r" );
        if ( strcmp( p->pDotType, "fd" ) != 0 )
        {
            fprintf( p->pOutput, "The PLA file is of an unsupported type (%s).\n", pTemp );
            return 0;
        }
    }

    // get the number of inputs and outputs
    pTemp = strtok( p->pDotI, " \t\n\r" );
    assert( strcmp( pTemp, ".i" ) == 0 );
    pTemp = strtok( NULL, " \t\n\r" );
    p->nInputs = atoi( pTemp );
    if ( p->nInputs < 1 || p->nInputs > 10000 )
    {
        fprintf( p->pOutput, "Unrealistic number of inputs in .i line (%d).\n", p->nInputs );
        return 0;
    }

    pTemp = strtok( p->pDotO, " \t\n\r" );
    assert( strcmp( pTemp, ".o" ) == 0 );
    pTemp = strtok( NULL, " \t\n\r" );
    p->nOutputs = atoi( pTemp );
    if ( p->nOutputs < 1 || p->nOutputs > 10000 )
    {
        fprintf( p->pOutput, "Unrealistic number of outputs in .o line (%d).\n", p->nOutputs );
        return 0;
    }

    // store away the input names
    if ( p->pDotIlb )
    {
        p->pNamesIn = ALLOC( char *, p->nInputs );
        pTemp = strtok( p->pDotIlb, " \t\n\r" );
        assert( strcmp( pTemp, ".ilb" ) == 0 );
        for ( i = 0; i < p->nInputs; i++ )
        {
            p->pNamesIn[i] = strtok( NULL, " \t\n\r" );
            if ( p->pNamesIn[i] == NULL )
            {
                fprintf( p->pOutput, "Insufficient number of input names on .ilb line.\n" );
                return 0;
            }
        }
        pTemp = strtok( NULL, " \t\n\r" );
        if ( strcmp( pTemp, ".ob" ) )
        {
            fprintf( p->pOutput, "Trailing symbols on .ilb line (%s).\n", pTemp );
            return 0;
        }
        // overwrite 0 introduced by strtok()
        *(pTemp + strlen(pTemp)) = ' ';
    }

    // store away the output names
    if ( p->pDotOb )
    {
        p->pNamesOut = ALLOC( char *, p->nOutputs );
        pTemp = strtok( p->pDotOb, " \t\n\r" );
        assert( strcmp( pTemp, ".ob" ) == 0 );
        for ( i = 0; i < p->nOutputs; i++ )
        {
            p->pNamesOut[i] = strtok( NULL, " \t\n\r" );
            if ( p->pNamesOut[i] == NULL )
            {
                fprintf( p->pOutput, "Insufficient number of output names on .ob line.\n" );
                return 0;
            }
        }
        pTemp = strtok( NULL, " \t\n\r" );
        if ( *pTemp >= 'a' && *pTemp <= 'z' )
        {
            fprintf( p->pOutput, "Trailing symbols on .ob line (%s).\n", pTemp );
            return 0;
        }
        // overwrite 0 introduced by strtok()
        *(pTemp + strlen(pTemp)) = ' ';
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Io_ReadPlaBody( Io_ReadPla_t * p )
{
    Mvc_Cube_t * pCube, * pCubeDup;
    int nOuts, iPos, iPosOld, i;
    char * pfOuts, * pTemp;

    // allocate covers
    p->ppOns = ALLOC( Mvc_Cover_t *, p->nOutputs );
    p->ppDcs = ALLOC( Mvc_Cover_t *, p->nOutputs );
    for ( i = 0; i < p->nOutputs; i++ )
    {
        p->ppOns[i] = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(p->pMan), p->nInputs * 2 );
        p->ppDcs[i] = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(p->pMan), p->nInputs * 2 );
    }

    // allocate the output part
    pfOuts = ALLOC( char, p->nOutputs );

    // read the table
    pTemp = strtok( p->pDotStart, " |\t\r\n" );
    while ( pTemp )
    {
        // read the cube
        iPos = 0;
        pCube = Mvc_CubeAlloc( p->ppOns[0] );
        Mvc_CubeBitFill( pCube );
        while ( 1 )
        {
            iPosOld = iPos;
            iPos += strlen( pTemp );
            if ( iPos > p->nInputs )
            {
                fprintf( p->pOutput, "Cannot correctly scan the input part of the table.\n" );
                FREE( pfOuts );
                return 0;
            }
            // read the cube
            for ( i = iPosOld; i < iPos; i++ )
                if ( pTemp[i-iPosOld] == '0' )
                    Mvc_CubeBitRemove( pCube, i * 2 + 1 );
                else if ( pTemp[i-iPosOld] == '1' )
                    Mvc_CubeBitRemove( pCube, i * 2 );
            // read the next part
            pTemp = strtok( NULL, " |\t\r\n" );
            // check if it is time to quit
            if ( iPos == p->nInputs )
                break;
        }

        // read the output part
        nOuts = 0;
        iPos = 0;
        memset( pfOuts, 0, sizeof(char) * p->nOutputs );
        while ( 1 )
        {
            iPosOld = iPos;
            iPos += strlen( pTemp );
            if ( iPos > p->nOutputs )
            {
                fprintf( p->pOutput, "Cannot correctly scan the output part of the table.\n" );
                FREE( pfOuts );
                return 0;
            }
            // read the cube
            for ( i = iPosOld; i < iPos; i++ )
            {
                pfOuts[i] = pTemp[i-iPosOld];
                if ( pfOuts[i] != '0' && pfOuts[i] != '~' )
                    nOuts++;
            }
            // read the next part
            pTemp = strtok( NULL, " |\t\r\n" );
            // check if it is time to quit
            if ( iPos == p->nOutputs )
                break;
        }

        // add this cube to the outputs
        for ( i = 0; i < p->nOutputs; i++ )
            if ( pfOuts[i] == '1' )
            {
                if ( --nOuts == 0 )
                {
                    Mvc_CoverAddCubeTail( p->ppOns[i], pCube );
                }
                else
                {
                    pCubeDup = Mvc_CubeDup( p->ppOns[i], pCube );
                    Mvc_CoverAddCubeTail( p->ppOns[i], pCubeDup );
                }
            }
            else if ( pfOuts[i] == '-' )
            {
                if ( --nOuts == 0 )
                {
                    Mvc_CoverAddCubeTail( p->ppDcs[i], pCube );
                }
                else
                {
                    pCubeDup = Mvc_CubeDup( p->ppDcs[i], pCube );
                    Mvc_CoverAddCubeTail( p->ppDcs[i], pCubeDup );
                }
            }
            else if ( pfOuts[i] != '0' && pfOuts[i] != '~' )
            {
                fprintf( p->pOutput, "Strange char (%c) in the output part of the table.\n", pfOuts[i] );
                FREE( pfOuts );
                return 0;
            }
        if ( pTemp >= p->pDotEnd )
            break;
    }

    FREE( pfOuts );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadPlaConstructNetwork( Io_ReadPla_t * p, Mv_Frame_t * pMvsis, char * FileName )
{
    Fnc_Manager_t * pMan;
    char Buffer[10];
    char * pName;
    Vm_VarMap_t * pVm;
    Cvr_Cover_t * pCvr;
    Mvr_Relation_t * pMvr, * pMvrDc;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNodeGen, * pNode, * pDriver;
    DdNode * pbIsets[2];
    Mvc_Cover_t ** ppIsets, * pMvcTemp;
    int * pValues;
    int i, nDigitsIn, nDigitsOut;

    // start the network
    pNet = Ntk_NetworkAlloc( pMvsis );
    Ntk_NetworkSetName( pNet, Ntk_NetworkRegisterNewName( pNet, FileName ) );
    Ntk_NetworkSetSpec( pNet, Ntk_NetworkRegisterNewName( pNet, FileName ) );

    // get the manager
    pMan = Ntk_NetworkReadMan( pNet );

    // get the number of digits in the numbers
	for ( nDigitsIn = 0,  i = p->nInputs - 1;  i;  i /= 10,  nDigitsIn++ );
	for ( nDigitsOut = 0, i = p->nOutputs - 1; i;  i /= 10,  nDigitsOut++ );

    // create the var map for this function
    pValues = ALLOC( int, p->nInputs + 1 );
    for ( i = 0; i <= p->nInputs; i++ )
        pValues[i] = 2;
    pVm = Vm_VarMapLookup( Fnc_ManagerReadManVm(pMan), p->nInputs, 1, pValues );
    FREE( pValues );


    // create the PI nodes
    for ( i = 0; i < p->nInputs; i++ )
    {
        // get the input name
        if ( p->pDotIlb )
            pName = p->pNamesIn[i];
        else
        {
	        sprintf( Buffer, "x%0*d", nDigitsIn, i );
            pName = Buffer;
        }
        // add the input node
        pNode = Ntk_NetworkFindOrAddNodeByName( pNet, pName, MV_NODE_CI );
        Ntk_NodeSetValueNum( pNode, 2 );
    }

    // create the generic internal node
    pNodeGen = Ntk_NodeCreateFromNetwork( pNet, NULL );
    Ntk_NodeSetValueNum( pNodeGen, 2 );

    // create the internal nodes
    for ( i = 0; i < p->nOutputs; i++ )
    {
        // get the output name
        if ( p->pDotOb )
            pName = p->pNamesOut[i];
        else
        {
	        sprintf( Buffer, "z%0*d", nDigitsOut, i );
            pName = Buffer;
        }
        // copy the generic node
        pNode = Ntk_NodeDup( pNet, pNodeGen );
        // set the node's name
        Ntk_NodeSetName( pNode, Ntk_NetworkRegisterNewName( pNet, pName ) );
        // add the node to the network
        Ntk_NetworkAddNode( pNet, pNode, 1 );

        // create Cvr for this node
        ppIsets = ALLOC( Mvc_Cover_t *, 2 );
        ppIsets[0] = NULL;
        ppIsets[1] = p->ppOns[i]; 
        p->ppOns[i] = NULL;

        // create Cvr
        pCvr = Cvr_CoverCreate( pVm, ppIsets );

        // set the current representation at the node
        Ntk_NodeWriteFuncVm( pNode, pVm );
        Ntk_NodeWriteFuncCvr( pNode, pCvr );

        // create the CO node
        Ntk_NetworkAddNodeCo( pNet, pNode, 1 );
    }
    Ntk_NodeDelete( pNodeGen );

    // if some nodes have don't-cares, we create Mvr instead
    i = 0;
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        if ( Mvc_CoverReadCubeNum( p->ppDcs[i] ) == 0 )
        {
            i++;
            continue;
        }

        // remember the on-set
        pCvr = Ntk_NodeReadFuncCvr( pDriver );
        ppIsets = Cvr_CoverReadIsets( pCvr );
        assert( ppIsets[0] == NULL );
        pMvcTemp = ppIsets[1];

        // derive the relation for the function
        pMvr = Fnc_FunctionDeriveMvrFromCvr( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), pCvr );

        // set the DC set
        ppIsets[1] = p->ppDcs[i];
        // derive the relation for the dc-set
        pMvrDc = Fnc_FunctionDeriveMvrFromCvr( Fnc_ManagerReadManMvr(pMan), Fnc_ManagerReadManVmx(pMan), pCvr );
        // reset back the i-sets
        ppIsets[1] = pMvcTemp;

        // remove the DC cover
        Mvc_CoverFree( p->ppDcs[i] );
        p->ppDcs[i] = NULL;

        

        // add the relation of DC to the relation of on-set
        // cofactor w.r.t. the output variable
        Mvr_RelationCofactorsDerive( pMvrDc, pbIsets, Vm_VarMapReadVarsInNum(pVm), 2 );
        Mvr_RelationAddDontCare( pMvr, pbIsets[1] );
        Mvr_RelationCofactorsDeref( pMvr, pbIsets, Vm_VarMapReadVarsInNum(pVm), 2 );

        Mvr_RelationFree( pMvrDc );

        // set the relation at the node (this will free the old Cvr)
        Ntk_NodeSetFuncMvr( pDriver, pMvr );
        i++;
    }

    // order the fanins
    Ntk_NetworkOrderFanins( pNet );

    // make the nodes minimum base
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
        Ntk_NodeMakeMinimumBase( pDriver );

    if ( !Ntk_NetworkCheck( pNet ) )
    {
        fprintf( p->pOutput, "Io_ReadPla(): Network check has failed.\n" );
        Ntk_NetworkDelete( pNet );
        pNet = NULL;
    }
    return pNet; 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadPlaCleanUp( Io_ReadPla_t * p )
{
    int i;
    for ( i = 0; i < p->nOutputs; i++ )
    {
        if ( p->ppOns && p->ppOns[i] )
            Mvc_CoverFree( p->ppOns[i] );
        if ( p->ppDcs && p->ppDcs[i] )
            Mvc_CoverFree( p->ppDcs[i] );
    }
    FREE( p->pBuffer );
    FREE( p->ppOns );
    FREE( p->ppDcs );
    FREE( p->pNamesIn );
    FREE( p->pNamesOut );
    FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadFileType( char * FileName )
{
    FILE * pFile;
    char * pBuffer;
    int nFileSize;
    int SizeLimit;
    int FileType;

    // open the BLIF file for binary reading
    pFile = fopen( FileName, "rb" );
    if ( pFile == NULL )
        return IO_FILE_NONE;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // determine the size limit
    SizeLimit = (nFileSize < 5000)? nFileSize: 5000;
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer   = ALLOC( char, SizeLimit + 1 );
    fread( pBuffer, SizeLimit, 1, pFile );
    pBuffer[SizeLimit] = 0;

    // detect the file type
    if ( strstr( pBuffer, ".inputs" ) && strstr( pBuffer, ".outputs" ) )
        FileType = IO_FILE_BLIF;
    else if ( strstr( pBuffer, ".mv" ) )
        FileType = IO_FILE_PLA_MV;
    else if ( strstr( pBuffer, ".i " ) && strstr( pBuffer, ".o " ) )
        FileType = IO_FILE_PLA;
    else
        FileType = IO_FILE_NONE;
    FREE( pBuffer );
    fclose( pFile );
    return FileType;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


