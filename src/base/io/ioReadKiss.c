/**CFile****************************************************************

  FileName    [ioReadKiss.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [The facility to read FSMs in KISS2 format as MV networks.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioReadKiss.c,v 1.2 2003/11/18 18:54:52 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct IoReadKissStruct Io_ReadKiss_t;
struct IoReadKissStruct
{
    // the file contents
    char *      pBuffer;
    int         FileSize;
    // the file parts
    char *      pDotI;
    char *      pDotO;
    char *      pDotP;
    char *      pDotS;
    char *      pDotR;
    char *      pDotIlb;
    char *      pDotOb;
    char *      pDotStart;
    char *      pDotEnd;
    // output for error messages
    FILE *      pOutput;
    // the number of inputs and outputs
    int         nInputs;
    int         nOutputs;
    int         nStates;
    int         nProducts;
    char *      pResets;
    // the input/output names
    char **     pNamesIn;
    char **     pNamesOut;
    // the state names
    char **     pNamesSt;
    int         nNamesSt;
    // the covers of POs
    Mvc_Cover_t ** ppOns;
    Mvc_Cover_t ** ppOffs;
    // the i-sets of the NS var
    Mvc_Cover_t ** ppNexts;
    // the functionality manager
    Fnc_Manager_t * pMan;
};


static bool            Io_ReadKissFindDotLines( Io_ReadKiss_t * p ); 
static bool            Io_ReadKissHeader( Io_ReadKiss_t * p );
static bool            Io_ReadKissBody( Io_ReadKiss_t * p );
static Ntk_Network_t * Io_ReadKissConstructNetwork( Io_ReadKiss_t * p, Mv_Frame_t * pMvsis, char * FileName );
static bool            Io_ReadKissAutBody( Io_ReadKiss_t * p );
static Ntk_Network_t * Io_ReadKissAutConstructNetwork( Io_ReadKiss_t * p, Mv_Frame_t * pMvsis, char * FileName );
static void            Io_ReadKissCleanUp( Io_ReadKiss_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
/**Function*************************************************************

  Synopsis    [Reads KISS2 format.]

  Description [This procedure reads the standard KISS2 format and
  represents the resulting FSM as an MV network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadKiss( Mv_Frame_t * pMvsis, char * FileName, bool fAut )
{
    Ntk_Network_t * pNet = NULL;
    Io_ReadKiss_t * p;

    // allocate the structure
    p = ALLOC( Io_ReadKiss_t, 1 );
    memset( p, 0, sizeof(Io_ReadKiss_t) );
    // set the output file stream for errors
    p->pOutput = Mv_FrameReadErr(pMvsis);
    p->pMan = Mv_FrameReadMan(pMvsis);

    // read the file
    p->pBuffer = Io_ReadFileFileContents( FileName, &p->FileSize );
    // remove comments if any
    Io_ReadFileRemoveComments( p->pBuffer, NULL, NULL );

    // split the file into parts
    if ( !Io_ReadKissFindDotLines( p ) )
        goto cleanup;

    // read the header of the file
    if ( !Io_ReadKissHeader( p ) )
        goto cleanup;

    if ( !fAut )
    {
        // read the body of the file
        if ( !Io_ReadKissBody( p ) )
            goto cleanup;

        // construct the network
        pNet = Io_ReadKissConstructNetwork( p, pMvsis, FileName );
    }
    else
    {
        // read the body of the file
        if ( !Io_ReadKissAutBody( p ) )
            goto cleanup;

        // construct the network
        pNet = Io_ReadKissAutConstructNetwork( p, pMvsis, FileName );
    }

cleanup:
    // free storage
    Io_ReadKissCleanUp( p );
    return pNet;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Io_ReadKissFindDotLines( Io_ReadKiss_t * p )
{
    char * pCur, * pCurMax;
    for ( pCur = p->pBuffer; *pCur; pCur++ )
        if ( *pCur == '.' )
        {
            if ( pCur[1] == 'i' && pCur[2] == ' ' )
            {
                if ( p->pDotI )
                {
                    fprintf( p->pOutput, "The KISS file contains multiple .i lines.\n" );
                    return 0;
                }
                p->pDotI = pCur;
            }
            else if ( pCur[1] == 'o' && pCur[2] == ' ' )
            {
                if ( p->pDotO )
                {
                    fprintf( p->pOutput, "The KISS file contains multiple .o lines.\n" );
                    return 0;
                }
                p->pDotO = pCur;
            }
            else if ( pCur[1] == 'p' && pCur[2] == ' ' )
            {
                if ( p->pDotP )
                {
                    fprintf( p->pOutput, "The KISS file contains multiple .p lines.\n" );
                    return 0;
                }
                p->pDotP = pCur;
            }
            else if ( pCur[1] == 's' && pCur[2] == ' ' )
            {
                if ( p->pDotS )
                {
                    fprintf( p->pOutput, "The KISS file contains multiple .s lines.\n" );
                    return 0;
                }
                p->pDotS = pCur;
            }
            else if ( pCur[1] == 'r' && pCur[2] == ' ' )
            {
                if ( p->pDotR )
                {
                    fprintf( p->pOutput, "The KISS file contains multiple .r lines.\n" );
                    return 0;
                }
                p->pDotR = pCur;
            }
            else if ( pCur[1] == 'i' && pCur[2] == 'l' && pCur[3] == 'b' )
            {
                if ( p->pDotIlb )
                {
                    fprintf( p->pOutput, "The KISS file contains multiple .ilb lines.\n" );
                    return 0;
                }
                p->pDotIlb = pCur;
            }
            else if ( pCur[1] == 'o' && pCur[2] == 'b' )
            {
                if ( p->pDotOb )
                {
                    fprintf( p->pOutput, "The KISS file contains multiple .ob lines.\n" );
                    return 0;
                }
                p->pDotOb = pCur;
            }
            else if ( pCur[1] == 'e' )
            {
                p->pDotEnd = pCur;
                break;
            }
        }
    if ( p->pDotI == NULL )
    {
        fprintf( p->pOutput, "The KISS file does not contain .i line.\n" );
        return 0;
    }
    if ( p->pDotO == NULL )
    {
        fprintf( p->pOutput, "The KISS file does not contain .o line.\n" );
        return 0;
    }
    if ( p->pDotP == NULL )
    {
        fprintf( p->pOutput, "The KISS file does not contain .p line.\n" );
        return 0;
    }
    if ( p->pDotS == NULL )
    {
        fprintf( p->pOutput, "The KISS file does not contain .s line.\n" );
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
    if ( p->pDotS && pCurMax < p->pDotS )
        pCurMax = p->pDotS;
    if ( p->pDotR && pCurMax < p->pDotR )
        pCurMax = p->pDotR;
    if ( p->pDotIlb && pCurMax < p->pDotIlb )
        pCurMax = p->pDotIlb;
    if ( p->pDotOb && pCurMax < p->pDotOb )
        pCurMax = p->pDotOb;
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
bool Io_ReadKissHeader( Io_ReadKiss_t * p )
{
    char * pTemp;
    int i;

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

    pTemp = strtok( p->pDotS, " \t\n\r" );
    assert( strcmp( pTemp, ".s" ) == 0 );
    pTemp = strtok( NULL, " \t\n\r" );
    p->nStates = atoi( pTemp );
    if ( p->nStates < 1 || p->nStates > 32 )
    {
        fprintf( p->pOutput, "The number of state in .s line is (%d). Currently this reader cannot read more than 32 states.\n", p->nStates );
        return 0;
    }

    pTemp = strtok( p->pDotP, " \t\n\r" );
    assert( strcmp( pTemp, ".p" ) == 0 );
    pTemp = strtok( NULL, " \t\n\r" );
    p->nProducts = atoi( pTemp );
    if ( p->nProducts < 1 || p->nProducts > 10000 )
    {
        fprintf( p->pOutput, "Unrealistic number of outputs in .p line (%d).\n", p->nProducts );
        return 0;
    }

    if ( p->pDotR )
    {
        fprintf( p->pOutput, "Currently, this KISS reader ignores the .r line.\n" );

        pTemp = strtok( p->pDotR, " \t\n\r" );
        assert( strcmp( pTemp, ".r" ) == 0 );
        p->pResets = strtok( NULL, " \t\n\r" );
        if ( p->pResets[0] != '0' && p->pResets[0] != '1' )
        {
            fprintf( p->pOutput, "Suspicious reset string <%s> in .r line (%d).\n", p->pResets, p->nOutputs );
//            return 0;
        }
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
//        if ( strcmp( pTemp, ".ob" ) )
        if ( *pTemp >= 'a' && *pTemp <= 'z' )
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
void Io_ReadKissCleanUp( Io_ReadKiss_t * p )
{
    int i;
    for ( i = 0; i < p->nOutputs; i++ )
    {
        if ( p->ppOns && p->ppOns[i] )
            Mvc_CoverFree( p->ppOns[i] );
        if ( p->ppOffs && p->ppOffs[i] )
            Mvc_CoverFree( p->ppOffs[i] );
    }
    for ( i = 0; i < p->nStates; i++ )
    {
        if ( p->ppNexts && p->ppNexts[i] )
            Mvc_CoverFree( p->ppNexts[i] );
    }
    FREE( p->pBuffer );
    FREE( p->ppOns );
    FREE( p->ppOffs );
    FREE( p->ppNexts );
    FREE( p->pNamesIn );
    FREE( p->pNamesOut );
    FREE( p->pNamesSt );
    FREE( p );
}



/**Function*************************************************************

  Synopsis    []

  Description [This procedure treats the output part of the FSM table
  as if it where the output part of the PLA table. ("0" means this
  output takes value 0. "1" means this output takes value 1. "-" 
  means this output is a don't-care.)]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Io_ReadKissBody( Io_ReadKiss_t * p )
{
    Mvc_Cube_t * pCube, * pCubeDup;
    int i, iLine, k, v;
    char * pTemp;
    char ** pParts[4];
    int nLines;

    // allocate covers for the output nodes
    p->ppOns  = ALLOC( Mvc_Cover_t *, p->nOutputs );
    p->ppOffs = ALLOC( Mvc_Cover_t *, p->nOutputs );
    for ( i = 0; i < p->nOutputs; i++ )
    {
        p->ppOns[i]  = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(p->pMan), p->nInputs * 2 + p->nStates );
        p->ppOffs[i] = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(p->pMan), p->nInputs * 2 + p->nStates );
    }
    // allocate covers to the NS node
    p->ppNexts = ALLOC( Mvc_Cover_t *, p->nStates );
    for ( i = 0; i < p->nStates; i++ )
        p->ppNexts[i] = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(p->pMan), p->nInputs * 2 + p->nStates );


    // allocate room for the lines
    pParts[0] = ALLOC( char *, p->nProducts + 10000 );
    pParts[1] = ALLOC( char *, p->nProducts + 10000 );
    pParts[2] = ALLOC( char *, p->nProducts + 10000 );
    pParts[3] = ALLOC( char *, p->nProducts + 10000 );
    // read the table
    pTemp = strtok( p->pDotStart, " |\t\r\n" );
    nLines = -1;
    while ( pTemp )
    {
        nLines++;
        if ( nLines > p->nProducts + 1000 )
            break;
        // save the first part
        pParts[0][nLines] = pTemp;
        pParts[1][nLines] = strtok( NULL, " |\t\r\n" );
        pParts[2][nLines] = strtok( NULL, " |\t\r\n" );
        pParts[3][nLines] = strtok( NULL, " |\t\r\n" );
        // read the next part
        pTemp = strtok( NULL, " |\t\r\n" );
    }
    if ( nLines != p->nProducts )
    {
        fprintf( p->pOutput, "The number of products in .p line (%d) differs from the number of lines (%d) in the file.\n", 
            p->nProducts, nLines );
        return 0;
    }

//    if ( pParts[1][0][0] == '*' )
//    {
//        fprintf( p->pOutput, "Currently, cannot read FSMs with the reset state.\n" );
//        return 0;
//    }


    // collect the state names in the internal storage
    p->nNamesSt = 0;
    p->pNamesSt = ALLOC( char *, p->nStates );
    for ( i = 0; i < p->nProducts; i++ )
    {
        if ( pParts[1][i][0] == '*' )
            continue;
        for ( k = 0; k < p->nNamesSt; k++ )
            if ( strcmp( p->pNamesSt[k], pParts[1][i] ) == 0 )
                break;
        if ( k == p->nNamesSt ) // new state
            p->pNamesSt[ p->nNamesSt++ ] = pParts[1][i];
        // else, it is an old state, we already collected it
    }
    if ( p->nNamesSt != p->nStates )
    {
        for ( i = 0; i < p->nProducts; i++ )
        {
            for ( k = 0; k < p->nNamesSt; k++ )
                if ( strcmp( p->pNamesSt[k], pParts[2][i] ) == 0 )
                    break;
            if ( k == p->nNamesSt ) // new state
                p->pNamesSt[ p->nNamesSt++ ] = pParts[2][i];
            // else, it is an old state, we already collected it
        }
    }
    assert( p->nNamesSt == p->nStates );


    // read the table
    for ( iLine = 0; iLine < p->nProducts; iLine++ )
    {
        pTemp = pParts[0][iLine];

        // read the cube
        pCube = Mvc_CubeAlloc( p->ppOns[0] );
        Mvc_CubeBitFill( pCube );
        // read the cube
        for ( i = 0; i < p->nInputs; i++ )
        {
            if ( pTemp[i] == '0' )
                Mvc_CubeBitRemove( pCube, i * 2 + 1 );
            else if ( pTemp[i] == '1' )
                Mvc_CubeBitRemove( pCube, i * 2 );
            else if ( pTemp[i] != '-' )
            {
                fprintf( p->pOutput, "Cannot correctly scan the input part of the table.\n" );
                return 0;
            }
        }

        if ( pParts[1][iLine][0] != '*' )
        {
            // read the current state
            pTemp = pParts[1][iLine];
            for ( k = 0; k < p->nNamesSt; k++ )
                if ( strcmp( p->pNamesSt[k], pTemp ) == 0 )
                    break;
            assert( k != p->nNamesSt );
            // remove all the bits in the state part, except this one
            for ( v = 0; v < p->nNamesSt; v++ )
                if ( v != k )
                    Mvc_CubeBitRemove( pCube, 2 * p->nInputs + v );
        }

        // read the next state
        pTemp = pParts[2][iLine];
        for ( k = 0; k < p->nNamesSt; k++ )
            if ( strcmp( p->pNamesSt[k], pTemp ) == 0 )
                break;
        assert( k != p->nNamesSt );

        // add the cube to the corresponding i-set
        pCubeDup = Mvc_CubeDup( p->ppNexts[k], pCube );
        Mvc_CoverAddCubeTail( p->ppNexts[k], pCubeDup );


        // read the output part
        pTemp = pParts[3][iLine];
        for ( i = 0; i < p->nOutputs; i++ )
        {
            if ( pTemp[i] == '0' )
            {
                pCubeDup = Mvc_CubeDup( p->ppOffs[i], pCube );
                Mvc_CoverAddCubeTail( p->ppOffs[i], pCubeDup );
            }
            else if ( pTemp[i] == '1' )
            {
                pCubeDup = Mvc_CubeDup( p->ppOns[i], pCube );
                Mvc_CoverAddCubeTail( p->ppOns[i], pCubeDup );
            }
            else if ( pTemp[i] == '-' )
            {
                pCubeDup = Mvc_CubeDup( p->ppOffs[i], pCube );
                Mvc_CoverAddCubeTail( p->ppOffs[i], pCubeDup );
                pCubeDup = Mvc_CubeDup( p->ppOns[i], pCube );
                Mvc_CoverAddCubeTail( p->ppOns[i], pCubeDup );
            }
            else
            {
                fprintf( p->pOutput, "Cannot correctly scan the input part of the table.\n" );
                return 0;
            }
        }
        Mvc_CubeFree( p->ppOns[0], pCube );
    }

    FREE( pParts[0] );
    FREE( pParts[1] );
    FREE( pParts[2] );
    FREE( pParts[3] );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadKissConstructNetwork( Io_ReadKiss_t * p, Mv_Frame_t * pMvsis, char * FileName )
{ 
    Fnc_Manager_t * pMan;
    char Buffer[10];
    char * pName;
    Vm_VarMap_t * pVm, * pVmLatch;
    Cvr_Cover_t * pCvr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNodeGen, * pNode, * pDriver;
    Mvc_Cover_t ** ppIsets;
    int * pValues;
    int i, nDigitsIn, nDigitsOut;
    Ntk_Node_t * pLatchIn, * pLatchOut;
    Ntk_Latch_t * pLatch;

    // start the network
    pNet = Ntk_NetworkAlloc( pMvsis );
    Ntk_NetworkSetName( pNet, Ntk_NetworkRegisterNewName( pNet, FileName ) );
//    Ntk_NetworkSetSpec( pNet, Ntk_NetworkRegisterNewName( pNet, FileName ) );
    Ntk_NetworkSetSpec( pNet, NULL );

    // get the manager
    pMan = Ntk_NetworkReadMan( pNet );

    // get the number of digits in the numbers
	for ( nDigitsIn = 0,  i = p->nInputs - 1;  i;  i /= 10,  nDigitsIn++ );
	for ( nDigitsOut = 0, i = p->nOutputs - 1; i;  i /= 10,  nDigitsOut++ );

    // create the var map for this function
    pValues = ALLOC( int, p->nInputs + 2 );
    for ( i = 0; i < p->nInputs; i++ )
        pValues[i] = 2;
    pValues[p->nInputs+0] = p->nStates;
    pValues[p->nInputs+1] = 2;
    pVm = Vm_VarMapLookup( Fnc_ManagerReadManVm(pMan), p->nInputs + 1, 1, pValues );
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

    

    // create the MV input and output nodes
    pLatchIn  = Ntk_NodeCreate( pNet, "NS", MV_NODE_INT, p->nStates );
    pLatchOut = Ntk_NodeCreate( pNet, "CS", MV_NODE_CI,  p->nStates );
    // add fanins to the CS node
    Ntk_NetworkForEachCi( pNet, pNode )
        Ntk_NodeAddFanin( pLatchIn, pNode );

    // add the latch output as a CI
    Ntk_NetworkAddNode( pNet, pLatchOut, 1 );
    // add it as a last fanin
    Ntk_NodeAddFanin( pLatchIn, pLatchOut );
    // add the latch input to the network
    Ntk_NetworkAddNode( pNet, pLatchIn, 1 );

    // create the latch
    pLatch = Ntk_LatchCreate( pNet, NULL, 0, "NS", "CS" );
    // add the new latch to the network
    Ntk_NetworkAddLatch( pNet, pLatch );



    // create the the var map for the NS node
    pVmLatch = Vm_VarMapCreateOneDiff( pVm, p->nInputs + 1, p->nStates );
    // create the cover for the latch
    pCvr = Cvr_CoverCreate( pVmLatch, p->ppNexts );
    p->ppNexts = NULL;
    // set the current representation at the node
    Ntk_NodeWriteFuncVm( pLatchIn, pVmLatch );
    Ntk_NodeWriteFuncCvr( pLatchIn, pCvr );



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
        ppIsets[0] = p->ppOffs[i]; 
        ppIsets[1] = p->ppOns[i]; 
        p->ppOffs[i] = NULL;
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

    // create the new CO node       
    Ntk_NetworkAddNodeCo( pNet, pLatchIn, 1 );
    // adjust latch input nodes 
    Ntk_LatchAdjustInput( pNet, pLatch );

    // set the subtype    
    Ntk_NodeSetSubtype( pLatch->pInput,  MV_BELONGS_TO_LATCH );
    Ntk_NodeSetSubtype( pLatch->pOutput, MV_BELONGS_TO_LATCH );



    // order the fanins
    Ntk_NetworkOrderFanins( pNet );

    // use the default whenever possible
    Ntk_NetworkForceDefault( pNet );

    // make the nodes minimum base
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
        Ntk_NodeMakeMinimumBase( pDriver );

    // check that the network is okay
    if ( !Ntk_NetworkCheck( pNet ) )
    {
        fprintf( p->pOutput, "Io_ReadKiss(): Network check has failed.\n" );
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
bool Io_ReadKissAutBody( Io_ReadKiss_t * p )
{
    Mvc_Cube_t * pCube, * pCubeDup;
    int i, iLine, k, v;
    char * pfOuts, * pTemp;
    char ** pParts[4];
    int nLines;

    // allocate covers of the NS node
    p->ppNexts = ALLOC( Mvc_Cover_t *, p->nStates + 1 );
    p->ppNexts[0] = NULL;  // corresponds to the dummy state
    for ( i = 1; i <= p->nStates; i++ )
        p->ppNexts[i] = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(p->pMan), 
            p->nInputs * 2 + p->nOutputs * 2 + p->nStates + 1 );


    // allocate room for the lines
    pParts[0] = ALLOC( char *, p->nProducts + 10000 );
    pParts[1] = ALLOC( char *, p->nProducts + 10000 );
    pParts[2] = ALLOC( char *, p->nProducts + 10000 );
    pParts[3] = ALLOC( char *, p->nProducts + 10000 );
    // read the table
    pTemp = strtok( p->pDotStart, " |\t\r\n" );
    nLines = -1;
    while ( pTemp )
    {
        nLines++;
        if ( nLines > p->nProducts + 1000 )
            break;
        // save the first part
        pParts[0][nLines] = pTemp;
        pParts[1][nLines] = strtok( NULL, " |\t\r\n" );
        pParts[2][nLines] = strtok( NULL, " |\t\r\n" );
        pParts[3][nLines] = strtok( NULL, " |\t\r\n" );
        // read the next part
        pTemp = strtok( NULL, " |\t\r\n" );
    }
    if ( nLines != p->nProducts )
    {
        fprintf( p->pOutput, "The number of products in .p line (%d) differs from the number of lines (%d) in the file.\n", 
            p->nProducts, nLines );
        return 0;
    }

//    if ( pParts[1][0][0] == '*' )
//    {
//        fprintf( p->pOutput, "Currently, cannot read FSMs with the reset state.\n" );
//        return 0;
//    }


    // collect the state names in the internal storage
    p->nNamesSt = 0;
    p->pNamesSt = ALLOC( char *, p->nStates );
    for ( i = 0; i < p->nProducts; i++ )
    {
        if ( pParts[1][i][0] == '*' )
            continue;
        for ( k = 0; k < p->nNamesSt; k++ )
            if ( strcmp( p->pNamesSt[k], pParts[1][i] ) == 0 )
                break;
        if ( k == p->nNamesSt ) // new state
            p->pNamesSt[ p->nNamesSt++ ] = pParts[1][i];
        // else, it is an old state, we already collected it
    }
    if ( p->nNamesSt != p->nStates )
    {
        for ( i = 0; i < p->nProducts; i++ )
        {
            for ( k = 0; k < p->nNamesSt; k++ )
                if ( strcmp( p->pNamesSt[k], pParts[2][i] ) == 0 )
                    break;
            if ( k == p->nNamesSt ) // new state
                p->pNamesSt[ p->nNamesSt++ ] = pParts[2][i];
            // else, it is an old state, we already collected it
        }
    }
    assert( p->nNamesSt == p->nStates );



    // allocate the space for the input/output cube
    pfOuts = ALLOC( char, p->nInputs + p->nOutputs + 10 );

    // read the table
    for ( iLine = 0; iLine < p->nProducts; iLine++ )
    {
        // add the output part to the input part
        pfOuts[0] = 0;
        strcat( pfOuts, pParts[0][iLine] ); 
        strcat( pfOuts, pParts[3][iLine] ); 

        pTemp = pfOuts;

        // read the cube
        pCube = Mvc_CubeAlloc( p->ppNexts[1] );
        Mvc_CubeBitFill( pCube );
        if ( (int)strlen(pTemp) != p->nInputs + p->nOutputs )
        {
            fprintf( p->pOutput, "Cannot correctly scan the input part of the table.\n" );
            FREE( pfOuts );
            return 0;
        }
        // read the cube
        for ( i = 0; i < p->nInputs + p->nOutputs; i++ )
            if ( pTemp[i] == '0' )
                Mvc_CubeBitRemove( pCube, i * 2 + 1 );
            else if ( pTemp[i] == '1' )
                Mvc_CubeBitRemove( pCube, i * 2 );


        // read the current state
        if ( pParts[1][iLine][0] != '*' ) 
        {
            pTemp = pParts[1][iLine];
            for ( k = 0; k < p->nNamesSt; k++ )
                if ( strcmp( p->pNamesSt[k], pTemp ) == 0 )
                    break;
            assert( k != p->nNamesSt );
            // remove all the bits in the state part, except this one
            for ( v = -1; v < p->nNamesSt; v++ )
                if ( v != k )
                    Mvc_CubeBitRemove( pCube, 2 * (p->nInputs + p->nOutputs) + v + 1 );
        }

        // read the next state
        pTemp = pParts[2][iLine];
        for ( k = 0; k < p->nNamesSt; k++ )
            if ( strcmp( p->pNamesSt[k], pTemp ) == 0 )
                break;
        assert( k != p->nNamesSt );
        // add the cube to the corresponding i-set
        pCubeDup = Mvc_CubeDup( p->ppNexts[k+1], pCube );
        Mvc_CoverAddCubeTail( p->ppNexts[k+1], pCubeDup );
    }

    FREE( pfOuts );
    FREE( pParts[0] );
    FREE( pParts[1] );
    FREE( pParts[2] );
    FREE( pParts[3] );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t * Io_ReadKissAutConstructNetwork( Io_ReadKiss_t * p, Mv_Frame_t * pMvsis, char * FileName )
{ 
    Fnc_Manager_t * pMan;
    char Buffer[10];
    char * pName;
    Vm_VarMap_t * pVm, * pVmLatch;
    Cvr_Cover_t * pCvr;
    Ntk_Network_t * pNet;
    Ntk_Node_t * pNode, * pDriver;
    Mvc_Cover_t ** ppIsets;
    int * pValues;
    int i, nDigitsIn, nDigitsOut;
    Ntk_Node_t * pLatchIn, * pLatchOut;
    Ntk_Latch_t * pLatch;
    Mvc_Cube_t * pCube;

    // start the network
    pNet = Ntk_NetworkAlloc( pMvsis );
    Ntk_NetworkSetName( pNet, Ntk_NetworkRegisterNewName( pNet, FileName ) );
//    Ntk_NetworkSetSpec( pNet, Ntk_NetworkRegisterNewName( pNet, FileName ) );
    Ntk_NetworkSetSpec( pNet, NULL );

    // get the manager
    pMan = Ntk_NetworkReadMan( pNet );

    // get the number of digits in the numbers
	for ( nDigitsIn = 0,  i = p->nInputs - 1;  i;  i /= 10,  nDigitsIn++ );
	for ( nDigitsOut = 0, i = p->nOutputs - 1; i;  i /= 10,  nDigitsOut++ );


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

        // add the input node
        pNode = Ntk_NetworkFindOrAddNodeByName( pNet, pName, MV_NODE_CI );
        Ntk_NodeSetValueNum( pNode, 2 );
    }

    

    // create the MV input and output nodes
    pLatchIn  = Ntk_NodeCreate( pNet, "NS", MV_NODE_INT, p->nStates + 1 );
    pLatchOut = Ntk_NodeCreate( pNet, "CS", MV_NODE_CI,  p->nStates + 1 );
    // add fanins to the CS node
    Ntk_NetworkForEachCi( pNet, pNode )
        Ntk_NodeAddFanin( pLatchIn, pNode );

    // add the latch output as a CI
    Ntk_NetworkAddNode( pNet, pLatchOut, 1 );
    // add it as a last fanin
    Ntk_NodeAddFanin( pLatchIn, pLatchOut );
    // add the latch input to the network
    Ntk_NetworkAddNode( pNet, pLatchIn, 1 );

    // create the latch
    pLatch = Ntk_LatchCreate( pNet, NULL, 0, "NS", "CS" );
    // add the new latch to the network
    Ntk_NetworkAddLatch( pNet, pLatch );



    // create the var map for the output function of the automaton
    pValues = ALLOC( int, p->nInputs + p->nOutputs + 2 );
    pValues[0] = p->nStates + 1;
    pValues[1] = 2;
    pVm = Vm_VarMapLookup( Fnc_ManagerReadManVm(pMan), 1, 1, pValues );


    // create the var map for the NS function
    for ( i = 0; i < p->nInputs + p->nOutputs; i++ )
        pValues[i] = 2;
    pValues[p->nInputs + p->nOutputs + 0] = p->nStates + 1;
    pValues[p->nInputs + p->nOutputs + 1] = p->nStates + 1;
    pVmLatch = Vm_VarMapLookup( Fnc_ManagerReadManVm(pMan), p->nInputs + p->nOutputs + 1, 1, pValues );
    FREE( pValues );

    // create the cover for the latch
    pCvr = Cvr_CoverCreate( pVmLatch, p->ppNexts );
    p->ppNexts = NULL;
    // set the current representation at the node
    Ntk_NodeWriteFuncVm( pLatchIn, pVmLatch );
    Ntk_NodeWriteFuncCvr( pLatchIn, pCvr );



    // create the output node
    pNode = Ntk_NodeCreate( pNet, "Out", MV_NODE_INT, 2 );
    // add it as a last fanin
    Ntk_NodeAddFanin( pNode, pLatchOut );
    // add the node to the network
    Ntk_NetworkAddNode( pNet, pNode, 1 );

    // create the cover for the node
    ppIsets = ALLOC( Mvc_Cover_t *, 2 );
    ppIsets[0] = NULL;
    ppIsets[1] = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(p->pMan), p->nStates + 1 );
    // create the cube
    pCube = Mvc_CubeAlloc( ppIsets[1] );
    Mvc_CubeBitFill( pCube );
    Mvc_CubeBitRemove( pCube, 0 );
    // add the cube
    Mvc_CoverAddCubeTail( ppIsets[1], pCube );

    // create Cvr
    pCvr = Cvr_CoverCreate( pVm, ppIsets );
    // set the current representation at the node
    Ntk_NodeWriteFuncVm( pNode, pVm );
    Ntk_NodeWriteFuncCvr( pNode, pCvr );

    // create the CO node
    Ntk_NetworkAddNodeCo( pNet, pNode, 1 );



    // create the new CO node for the latch       
    Ntk_NetworkAddNodeCo( pNet, pLatchIn, 1 );
    // adjust latch input nodes 
    Ntk_LatchAdjustInput( pNet, pLatch );

    // set the subtype    
    Ntk_NodeSetSubtype( pLatch->pInput,  MV_BELONGS_TO_LATCH );
    Ntk_NodeSetSubtype( pLatch->pOutput, MV_BELONGS_TO_LATCH );


    // order the fanins
    Ntk_NetworkOrderFanins( pNet );

    // use the default whenever possible
    Ntk_NetworkForceDefault( pNet );

    // make the nodes minimum base
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
        Ntk_NodeMakeMinimumBase( pDriver );

    // check that the network is okay
    if ( !Ntk_NetworkCheck( pNet ) )
    {
        fprintf( p->pOutput, "Io_ReadKissAut(): Network check has failed.\n" );
        Ntk_NetworkDelete( pNet );
        pNet = NULL;
    }
    return pNet; 
}






////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


