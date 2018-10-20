/**CFile****************************************************************

  FileName    [auFsm.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Syntactic conversion from FSMs into automata.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auReadFsm.c,v 1.1 2004/02/19 03:06:47 alanmi Exp $]

***********************************************************************/

#include "auInt.h"
#include "ioInt.h"


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct IoReadKissStruct Au_ReadKiss_t;
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
//    Mvc_Cover_t ** ppOns;
//    Mvc_Cover_t ** ppDcs;
//    Mvc_Cover_t ** ppOffs;
    // the i-sets of the NS var
//    Mvc_Cover_t ** ppNexts;
    // the functionality manager
    Fnc_Manager_t * pMan;
    Mvc_Cover_t * pMvc;
};


static bool            Au_ReadKissFindDotLines( Au_ReadKiss_t * p ); 
static bool            Au_ReadKissHeader( Au_ReadKiss_t * p );
static int             Au_ReadKissAutBody( Au_ReadKiss_t * p, char * FileNameOut );
static void            Au_ReadKissCleanUp( Au_ReadKiss_t * p );

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
int Au_FsmReadKiss( Mv_Frame_t * pMvsis, char * FileName, char * FileNameOut )
{
    Au_ReadKiss_t * p;
    int RetValue = 0;

    // allocate the structure
    p = ALLOC( Au_ReadKiss_t, 1 );
    memset( p, 0, sizeof(Au_ReadKiss_t) );
    // set the output file stream for errors
    p->pOutput = Mv_FrameReadErr(pMvsis);
    p->pMan = Mv_FrameReadMan(pMvsis);

    // read the file
    p->pBuffer = Io_ReadFileFileContents( FileName, &p->FileSize );
    // remove comments if any
    Io_ReadFileRemoveComments( p->pBuffer, NULL, NULL );

    // split the file into parts
    if ( !Au_ReadKissFindDotLines( p ) )
        goto cleanup;

    // read the header of the file
    if ( !Au_ReadKissHeader( p ) )
        goto cleanup;

    // read the body of the file
    if ( !Au_ReadKissAutBody( p, FileNameOut ) )
        goto cleanup;
    RetValue = 1;

cleanup:
    // free storage
    Au_ReadKissCleanUp( p );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Au_ReadKissFindDotLines( Au_ReadKiss_t * p )
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
bool Au_ReadKissHeader( Au_ReadKiss_t * p )
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
    if ( p->nOutputs < 0 || p->nOutputs > 10000 )
    {
        fprintf( p->pOutput, "Unrealistic number of outputs in .o line (%d).\n", p->nOutputs );
        return 0;
    }

    pTemp = strtok( p->pDotS, " \t\n\r" );
    assert( strcmp( pTemp, ".s" ) == 0 );
    pTemp = strtok( NULL, " \t\n\r" );
    p->nStates = atoi( pTemp );
/*
    if ( p->nStates < 1 || p->nStates > 32 )
    {
        fprintf( p->pOutput, "The number of state in .s line is (%d). Currently this reader cannot read more than 32 states.\n", p->nStates );
        return 0;
    }
*/

    pTemp = strtok( p->pDotP, " \t\n\r" );
    assert( strcmp( pTemp, ".p" ) == 0 );
    pTemp = strtok( NULL, " \t\n\r" );
    p->nProducts = atoi( pTemp );
    if ( p->nProducts < 0 || p->nProducts > 100000 )
    {
        fprintf( p->pOutput, "Unrealistic number of product terms in .p line (%d).\n", p->nProducts );
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
void Au_ReadKissCleanUp( Au_ReadKiss_t * p )
{
    FREE( p->pBuffer );
    FREE( p->pNamesIn );
    FREE( p->pNamesOut );
    FREE( p->pNamesSt );
    FREE( p );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_ReadKissAutBody( Au_ReadKiss_t * p, char * FileNameOut )
{
    FILE * pFile;
    char Buffer[10];
    char ** pParts[4];
    char * pName, * pTemp;
    int nDigitsIn, nDigitsOut;
    int i, nLines, nLength;
    st_table * tStateNames;


    if ( p->nOutputs == 0 )
    {
        fprintf( p->pOutput, "Au_FsmReadKiss(): Can only read KISS files with FSMs.\n" );
        fprintf( p->pOutput, "In FSM files, there is at least one output (\".o N\", N > 0)\n" );
        return 0;
    }

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

    if ( pParts[1][0][0] == '*' )
    {
        fprintf( p->pOutput, "Currently, cannot read FSMs with the reset state.\n" );
        return 0;
    }

    // get the number of digits in the numbers
	for ( nDigitsIn = 0,  i = p->nInputs - 1;  i;  i /= 10,  nDigitsIn++ );
	for ( nDigitsOut = 0, i = p->nOutputs - 1; i;  i /= 10,  nDigitsOut++ );

    // determine the length of the longest state name
    nLength = 0;
    for ( i = 0; i < p->nProducts; i++ )
    {
        if ( nLength < (int)strlen(pParts[1][i]) )
            nLength = strlen(pParts[1][i]);
        if ( nLength < (int)strlen(pParts[2][i]) )
            nLength = strlen(pParts[2][i]);
    }


    pFile = fopen( FileNameOut, "w" );
    fprintf( pFile, ".i %d\n", p->nInputs + p->nOutputs );
    fprintf( pFile, ".o %d\n", 0 );
    fprintf( pFile, ".s %d\n", p->nStates );
    fprintf( pFile, ".p %d\n", p->nProducts );

    fprintf( pFile, ".ilb" );
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
        fprintf( pFile, " %s", pName );
    }
    for ( i = 0; i < p->nOutputs; i++ )
    {
        // get the input name
        if ( p->pDotOb )
            pName = p->pNamesOut[i];
        else
        {
	        sprintf( Buffer, "z%0*d", nDigitsOut, i );
            pName = Buffer;
        }
        fprintf( pFile, " %s", pName );
    }
    fprintf( pFile, "\n" );

    fprintf( pFile, ".ob" );
    fprintf( pFile, "\n" );


    // set all the states as accepting
    fprintf( pFile, ".accepting" );
    tStateNames = st_init_table(strcmp, st_strhash);
    for ( i = 0; i < p->nProducts; i++ )
        if ( !st_find_or_add( tStateNames, pParts[1][i], NULL ) )
            fprintf( pFile, " %s", pParts[1][i] );
    assert( st_count(tStateNames) == p->nStates );
    st_free_table( tStateNames );
    fprintf( pFile, "\n" );

    // print the lines
    for ( i = 0; i < p->nProducts; i++ )
        fprintf( pFile, "%s%s %*s %*s\n", 
            pParts[0][i], pParts[3][i], nLength, pParts[1][i], nLength, pParts[2][i] );

    fprintf( pFile, ".e" );
    fprintf( pFile, "\n" );
    fclose( pFile );

    FREE( pParts[0] );
    FREE( pParts[1] );
    FREE( pParts[2] );
    FREE( pParts[3] );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


