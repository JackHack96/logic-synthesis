/**CFile****************************************************************

  FileName    [auReadPara.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with automata and FSMs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auReadPara.c,v 1.1 2004/02/19 03:06:47 alanmi Exp $]

***********************************************************************/

#include "auInt.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct IoReadParaStruct Au_ReadPara_t;
struct IoReadParaStruct
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
    char *      pDotAccept;
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

    // has to do with channels
    int nChans;                 // the number of channels
    int nChanVars[10];          // the number of vars in each channel
    char * pChanNames[10][100]; // the names of vars in each channel
    int pChanNums[10][100];     // the number of the corresponding vars in the list
};


static bool    Au_ReadParaFindDotLines( Au_ReadPara_t * p ); 
static bool    Au_ReadParaHeader( Au_ReadPara_t * p );
static int     Au_ReadParaAutBody( Au_ReadPara_t * p, char * pInputString, char * FileNameOut );
static void    Au_ReadParaCleanUp( Au_ReadPara_t * p );

static int     Au_ReadParaMapping( Au_ReadPara_t * p, char * pInputString );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoReadPara( Mv_Frame_t * pMvsis, char * pInputString, char * FileNameIn, char * FileNameOut )
{
    Au_ReadPara_t * p;
    int RetValue = 0;

    // allocate the structure
    p = ALLOC( Au_ReadPara_t, 1 );
    memset( p, 0, sizeof(Au_ReadPara_t) );
    // set the output file stream for errors
    p->pOutput = Mv_FrameReadErr(pMvsis);
    p->pMan = Mv_FrameReadMan(pMvsis);

    // read the file
    p->pBuffer = Io_ReadFileFileContents( FileNameIn, &p->FileSize );
    // remove comments if any
    Io_ReadFileRemoveComments( p->pBuffer, NULL, NULL );

    // split the file into parts
    if ( !Au_ReadParaFindDotLines( p ) )
        goto cleanup;

    // read the header of the file
    if ( !Au_ReadParaHeader( p ) )
        goto cleanup;

    // read the body of the file
    if ( !Au_ReadParaAutBody( p, pInputString, FileNameOut ) )
        goto cleanup;
    RetValue = 1;

cleanup:
    // free storage
    Au_ReadParaCleanUp( p );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Au_ReadParaFindDotLines( Au_ReadPara_t * p )
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
            else if ( pCur[1] == 'a' && pCur[2] == 'c' && pCur[3] == 'c' && pCur[4] == 'e' && pCur[5] == 'p' && pCur[6] == 't' )
            {
                if ( p->pDotAccept )
                {
                    fprintf( p->pOutput, "The KISS file contains multiple .accepting lines.\n" );
                    return 0;
                }
                p->pDotAccept = pCur;
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
    if ( p->pDotAccept == NULL )
    {
        fprintf( p->pOutput, "The KISS file does not contain .accepting line.\n" );
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
    if ( p->pDotAccept && pCurMax < p->pDotAccept )
        pCurMax = p->pDotAccept;
    // go to the next new line symbol
    for ( ; *pCurMax; pCurMax++ )
        if ( *pCurMax == '\n' )
        {
            p->pDotStart = pCurMax + 1;

            *pCurMax = 0;
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
bool Au_ReadParaHeader( Au_ReadPara_t * p )
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
/*
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
*/
    }

    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_ReadParaCleanUp( Au_ReadPara_t * p )
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
int Au_ReadParaAutBody( Au_ReadPara_t * p, char * pInputString, char * FileNameOut )
{
    FILE * pFile;
//    char Buffer[10];
    char ** pParts[4];
    char * pTemp;
//    int nDigitsIn, nDigitsOut;
    int nDigitsProd;
    int i, nLines, nLength, iLen;
    st_table * tStateNames;
    int c, v;
    int iChanIn, iChanOut;
    char StateName[1000];
    int iState;


    if ( p->nOutputs > 0 )
    {
        fprintf( p->pOutput, "Au_ReadPara(): Can only read KISS files with automata.\n" );
        fprintf( p->pOutput, "In the automata files, there are no outputs (\".o 0\")\n" );
        return 0;
    }


    // create the mapping from the channel number and var number 
    // into the variable's position in the string
    if ( Au_ReadParaMapping( p, pInputString ) )
        return 0;


    // allocate room for the lines
    pParts[0] = ALLOC( char *, p->nProducts + 10000 );
    pParts[1] = ALLOC( char *, p->nProducts + 10000 );
    pParts[2] = ALLOC( char *, p->nProducts + 10000 );
//    pParts[3] = ALLOC( char *, p->nProducts + 10000 );
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
//        pParts[3][nLines] = strtok( NULL, " |\t\r\n" );
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
//	for ( nDigitsIn = 0,  i = p->nInputs - 1;  i;  i /= 10,  nDigitsIn++ );
//	for ( nDigitsOut = 0, i = p->nOutputs - 1; i;  i /= 10,  nDigitsOut++ );
    for ( nDigitsProd = 0, i = p->nProducts - 1; i;  i /= 10,  nDigitsProd++ );

    // determine the length of the longest state name
    nLength = 0;
    for ( i = 0; i < p->nProducts; i++ )
    {
        iLen = (int)strlen(pParts[1][i]) + (int)strlen(pParts[2][i]) + 2;
        if ( nLength < iLen )
            nLength = iLen;
    }
    nLength += nDigitsProd;


    pFile = fopen( FileNameOut, "w" );
    fprintf( pFile, ".i %d\n", p->nInputs + p->nChans );
    fprintf( pFile, ".o %d\n", p->nOutputs );
    fprintf( pFile, ".s %d\n", p->nStates + p->nProducts );
    fprintf( pFile, ".p %d\n", 2 * p->nProducts + p->nStates );

    assert( p->pDotIlb );
    fprintf( pFile, ".ilb" );
    for ( c = 0; c < p->nChans; c++ )
    {
        // print the input names for this channel
        for ( v = 0; v < p->nChanVars[c]; v++ )
            fprintf( pFile, " %s", p->pNamesIn[ p->pChanNums[c][v] ] );
        // print the Epsilon var for this channel
        fprintf( pFile, " E%d ", c );
    }
    fprintf( pFile, "\n" );

    fprintf( pFile, ".ob" );
    fprintf( pFile, "\n" );


    // set all the states as accepting
    iState = 0;
    p->pNamesSt = ALLOC( char *, p->nStates );

//    fprintf( pFile, ".accepting" );
    tStateNames = st_init_table(strcmp, st_strhash);
    for ( i = 0; i < p->nProducts; i++ )
        if ( !st_find_or_add( tStateNames, pParts[1][i], NULL ) )
        {
//            fprintf( pFile, " %s", pParts[1][i] );
            p->pNamesSt[iState++] = util_strsav(pParts[1][i]);
        }
    assert( iState == p->nStates );
    assert( st_count(tStateNames) == p->nStates );
    st_free_table( tStateNames );

    // remove the last char of .accepting
    p->pDotAccept[ strlen(p->pDotAccept) - 1 ] = 0;
    fprintf( pFile, "%s", p->pDotAccept );
    fprintf( pFile, "\n" );

    // print the lines
    // for each product, print two lines:
    // the one with the input channel 
    // transiting from the current state into an intermediate state
    // the other with the output channel 
    // transiting from the intermediate state into the next state

    // INPUT CHANNELS SHOULD FOLLOW FIRST
    for ( i = 0; i < p->nProducts; i++ )
    {
        // find the number of the input and output channels of this transition
        iChanIn  = -1;
        iChanOut = -1;
        for ( c = 0; c < p->nChans; c++ )
        {
            for ( v = 0; v < p->nChanVars[c]; v++ )
                if ( pParts[0][i][ p->pChanNums[c][v] ] != '.' )
                {
                    if ( iChanIn == -1 )
                    {
                        iChanIn = c;
                        break;
                    }
                    assert( iChanOut == -1 );
                    iChanOut = c;
                    break;
                }
        }
        assert( iChanIn != -1 );
        assert( iChanOut != -1 );

        // print the input to intermediate transition
        for ( c = 0; c < p->nChans; c++ )
        {
            if ( c == iChanIn )
            {
                for ( v = 0; v < p->nChanVars[c]; v++ )
                    fprintf( pFile, "%c", pParts[0][i][ p->pChanNums[c][v] ] );
                fprintf( pFile, "1" );
            }
/*
            else if ( c == iChanOut )
            {
                for ( v = 0; v < p->nChanVars[c]; v++ )
                    fprintf( pFile, "-" );
                fprintf( pFile, "0" );
            }
*/
            else
            {
                for ( v = 0; v < p->nChanVars[c]; v++ )
                    fprintf( pFile, "-" );
                fprintf( pFile, "0" );
            }
        }

        // get the next state name
        sprintf( StateName, "%s_%s_%0*d", pParts[1][i], pParts[2][i], nDigitsProd, i );
        // print the current and next state
        fprintf( pFile, " %*s %*s\n", nLength, pParts[1][i], nLength, StateName );

        // print the intermediate to output transition
        for ( c = 0; c < p->nChans; c++ )
        {
            if ( c == iChanOut )
            {
                for ( v = 0; v < p->nChanVars[c]; v++ )
                    fprintf( pFile, "%c", pParts[0][i][ p->pChanNums[c][v] ] );
                fprintf( pFile, "1" );
            }
/*
            else if ( c == iChanIn )
            {
                for ( v = 0; v < p->nChanVars[c]; v++ )
                    fprintf( pFile, "-" );
                fprintf( pFile, "1" );
            }
*/
            else
            {
                for ( v = 0; v < p->nChanVars[c]; v++ )
                    fprintf( pFile, "-" );
                fprintf( pFile, "0" );
            }
        }
        // print the current and next state
        fprintf( pFile, " %*s %*s\n", nLength, StateName, nLength, pParts[2][i] );
    }

    // add the self-loops for each state

    // Nina said, the epsilon moves should not be added for the non-accepting states

    for ( i = 0; i < p->nStates; i++ )
    {
        for ( c = 0; c < p->nChans; c++ )
        {
            for ( v = 0; v < p->nChanVars[c]; v++ )
                fprintf( pFile, "-" );
            fprintf( pFile, "0" );
        }
        fprintf( pFile, " %*s %*s\n", nLength, p->pNamesSt[i], nLength, p->pNamesSt[i] );
    }


    fprintf( pFile, ".e" );
    fprintf( pFile, "\n" );
    fclose( pFile );

    for ( i = 0; i < p->nStates; i++ )
        FREE( p->pNamesSt[i] );
    FREE( p->pNamesSt );

    FREE( pParts[0] );
    FREE( pParts[1] );
    FREE( pParts[2] );
//    FREE( pParts[3] );
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_ReadParaMapping( Au_ReadPara_t * p, char * pInputString )
{
    char * pTemp;
    int c, v, i;
    int nVars;

    // has to do with channels
//    int nChans;                 // the number of channels
//    int nChanVars[10];          // the number of vars in each channel
//    char * pChanNames[10][100]; // the names of vars in each channel
//    int pChanNums[10][100];     // the number of the corresponding vars in the list

    // count the number of chans and the number of vars in each chan
    p->nChans = 0;
    p->nChanVars[p->nChans] = 1;
    for ( pTemp = pInputString; *pTemp; pTemp++ )
        if ( *pTemp == ',' )
            p->nChanVars[p->nChans]++;
        else if ( *pTemp == '|' )
        {
            p->nChans++;
            p->nChanVars[p->nChans] = 1;
        }
    p->nChans++;

    // save the chan vars
    nVars = 0;
    pTemp = strtok( pInputString, ",|" );
    for ( c = 0; c < p->nChans; c++ )
        for ( v = 0; v < p->nChanVars[c]; v++ )
        {
            p->pChanNames[c][v] = pTemp;
            pTemp = strtok( NULL, ",|" );
            nVars++;
        }
    assert( pTemp == NULL );

    if ( nVars != p->nInputs )
    {
        printf( "The number of vars in the channels is not the same as in the automaton.\n" );
        return 1;
    }

    // detect the mapping
    for ( c = 0; c < p->nChans; c++ )
        for ( v = 0; v < p->nChanVars[c]; v++ )
        {
            for ( i = 0; i < nVars; i++ )
                if ( strcmp( p->pChanNames[c][v], p->pNamesIn[i] ) == 0 )
                {
                    p->pChanNums[c][v] = i;
                    break;
                }
        }
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


