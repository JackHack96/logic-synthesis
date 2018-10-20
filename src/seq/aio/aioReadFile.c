/**CFile****************************************************************

  FileName    [ioReadFile.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Loads data from the file into the internal IO data structures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioReadFile.c,v 1.2 2004/05/12 04:30:16 alanmi Exp $]

***********************************************************************/

#include "aioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void   Aio_ReadFileLineClean( char * pLineBeg, char * pLineEnd );
static void   Aio_ReadFileLineAdd( Aio_Read_t * p, char * pLine );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prepares the reading structure for preparsing.]

  Description [Loads the contents of the file into memory, removes 
  comments while counting the number of lines and dot-statements.
  Splits the file contents into lines and counts the number of 
  different dot-statements. The new-line and space-like characters 
  at the end of each line are overwritten with zeros ('\0').]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_ReadFile( Aio_Read_t * p )
{
    // make sure the file name is given
    assert( p->FileName );
    // read the contents of the file into memory
    p->pFileContents = Io_ReadFileFileContents( p->FileName, &p->FileSize );
    if ( p->pFileContents == NULL )
    {
        p->LineCur = 0;
        sprintf( p->sError, "Cannot open input file." );
        Aio_ReadPrintErrorMessage( p );
        return 1;
    }
    // remove comments and count the number of dots and lines
    Io_ReadFileRemoveComments( p->pFileContents, &p->nDots, &p->nLines );
    if ( p->nLines < 4 || p->nDots < 4 )
    {
        p->LineCur = 0;
        sprintf( p->sError, "The file does not appear to be a BLIF/BLIF-MV file." );
        Aio_ReadPrintErrorMessage( p );
        return 1;
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Marks up lines and dots; counts .mv, .latch, .node/.table]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_ReadFileMarkLinesAndDots( Aio_Read_t * p )
{
    char * pCur;
    int nDotsInit, nLinesInit, i;

    // allocate new memory
    p->pLines = ALLOC( char *, p->nLines );      
    p->pDots  = ALLOC( int, p->nDots ); 

    // save the initial parameters
    nDotsInit  = p->nDots;
    nLinesInit = p->nLines;
    p->nDots   = 0;
    p->nLines  = 0;

    // add the first line
    Aio_ReadFileLineAdd( p, p->pFileContents );
    // add other lines
    for ( pCur = p->pFileContents; *pCur; pCur++ )
        if ( *pCur == '\n' && *(pCur + 1) != '\0' ) // the next line exists
            Aio_ReadFileLineAdd( p, pCur + 1 );
    // make sure the number of lines is read correctly
    assert( p->nLines <= nLinesInit );
    assert( p->nDots <= nDotsInit );

    // remove space-like chars from the end of each line
    for ( i = 0; i < p->nLines - 1; i++ )
        Aio_ReadFileLineClean( p->pLines[i], p->pLines[i+1] - 1 );
/*
printf( "The lines are:\n" );
for ( i = 0; i < p->nLines-1; i++ )
    printf( "Line %2d: %s\n", i+1, p->pLines[i] );
printf( "End of printout\n" );
*/
    // make sure the parameters are reasonable
    if ( p->nDotDefs > p->nDotNodes )
    {
        p->LineCur = 0;
        sprintf( p->sError, "There are more .def statements (%d) than nodes (%d).\n",
            p->nDotDefs, p->nDotNodes );
        Aio_ReadPrintErrorMessage( p );
        return 1;
    }

    // allocate additional memory
    if ( p->nDotInputs )
        p->pDotInputs = ALLOC( int, p->nDotInputs ); 
    if ( p->nDotOutputs )
        p->pDotOutputs = ALLOC( int, p->nDotOutputs ); 
    if ( p->nDotNodes )
        p->pAioNodes = ALLOC( Aio_Node_t, p->nDotNodes );      
    if ( p->nDotMvs )
        p->pDotMvs = ALLOC( int, p->nDotMvs );      
    if ( p->nDotLatches )
        p->pAioLatches = ALLOC( Aio_Latch_t, p->nDotLatches ); 
    return 0;
}


/**Function*************************************************************

  Synopsis    [Loads the contents of the file into memory.]

  Description [Returns the pointer to the 0-terminated string
  storing the contents of the file in memory.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aio_ReadFileFileContents( char * FileName, int * pFileSize )
{
    FILE * pFile;
    int nFileSize;
    char * pBuffer;

    // open the BLIF file for binary reading
    pFile = fopen( FileName, "rb" );
    if ( pFile == NULL )
        return NULL;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer   = ALLOC( char, nFileSize + 10 );
    fread( pBuffer, nFileSize, 1, pFile );
    // terminate the string with '\0'
    pBuffer[ nFileSize + 0] = '\n';
    pBuffer[ nFileSize + 1] = '.';
    pBuffer[ nFileSize + 2] = 'e';
    pBuffer[ nFileSize + 3] = 'n';
    pBuffer[ nFileSize + 4] = 'd';
    pBuffer[ nFileSize + 5] = '\n';
    pBuffer[ nFileSize + 6] = '\0';
    // close file
    fclose( pFile );
    // return
    if ( pFileSize )
        *pFileSize = nFileSize;
    return pBuffer;
}
    
/**Function*************************************************************

  Synopsis    [Eliminates comments from the input file.]

  Description [As a byproduct, this procedure also counts the number
  lines and dot-statements in the input file. This also joins non-comment 
  lines that are joined with a backspace '\']
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_ReadFileRemoveComments( char * pBuffer, int * pnDots, int * pnLines )
{
    char * pCur;
    int nDots, nLines;
    // scan through the buffer and eliminate comments
    // (in the BLIF file, comments are lines starting with "#")
    nDots = nLines = 0;
    for ( pCur = pBuffer; *pCur; pCur++ )
    {
        // if this is the beginning of comment
        // clean it with spaces until the new line statement
        if ( *pCur == '#' )
            while ( *pCur != '\n' )
                *pCur++ = ' ';
	
        // count the number of new lines and dots
        if ( *pCur == '\n' ) {
	    if (*(pCur-1)=='\r') {
		// DOS(R) file support
		if (*(pCur-2)!='\\') nLines++;
		else {
		    // rewind to backslash and overwrite with a space
		    *(pCur-2) = ' ';
		    *(pCur-1) = ' ';
		    *pCur = ' ';
		}
	    } else {
		// UNIX(TM) file support
		if (*(pCur-1)!='\\') nLines++;
		else {
		    // rewind to backslash and overwrite with a space
		    *(pCur-1) = ' ';
		    *pCur = ' ';
		}
	    }
	}
        else if ( *pCur == '.' )
            nDots++;
    }
    if ( pnDots )
        *pnDots = nDots; 
    if ( pnLines )
        *pnLines = nLines; 
}


/**Function*************************************************************

  Synopsis    [Adds zero-char symbols at the end of the line.]

  Description [Overwrites with zeros all space-line characters 
  starting from pLineEnd to pLineBeg, while moving backwards.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_ReadFileLineClean( char * pLineBeg, char * pLineEnd )
{
    char * pTemp;
    // remove spaces from the end of the previous line
    for ( pTemp = pLineEnd; pTemp >= pLineBeg && 
        (*pTemp == '\n' || *pTemp == '\r' || *pTemp == ' ' || *pTemp == '\t'); pTemp-- )
        *pTemp = '\0';
}


/**Function*************************************************************

  Synopsis    [Add one line to storage.]

  Description [As a byproduct, this procedure also counts the number
  of different dot-statements in the input file.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_ReadFileLineAdd( Aio_Read_t * p, char * pLine )
{
    // skip spaces at the beginning of this line
    while ( *pLine == ' ' || *pLine == '\t' )
        pLine++;

    // mark this line if it is a dot-line
    if ( *pLine == '.' )
    {
        p->pDots[ p->nDots++ ] = p->nLines;
        if      ( pLine[1] == 'n' && pLine[2] == 'a' && pLine[3] == 'm' )
            p->nDotNodes++;
        else if ( pLine[1] == 't' && pLine[2] == 'a' && pLine[3] == 'b' )
            p->nDotNodes++;
        else if ( pLine[1] == 'd' && pLine[2] == 'e' && pLine[3] == 'f' && pLine[4] == 'a' && pLine[5] == 'u' && pLine[6] == 'l' && pLine[7] == 't' && pLine[8] == ' ' ) 
            p->nDotDefs++;
        else if ( pLine[1] == 'd' && pLine[2] == 'e' && pLine[3] == 'f' && pLine[4] == ' ' ) 
            p->nDotDefs++;
        else if ( pLine[1] == 'm' && pLine[2] == 'v' )
            p->nDotMvs++;
        else if ( pLine[1] == 'l' && pLine[2] == 'a' && pLine[3] == 't' )
            p->nDotLatches++;
        else if ( pLine[1] == 'r' && pLine[2] == ' ' || pLine[1] == 'r' && pLine[2] == 'e' && pLine[3] == 's' ) 
            p->nDotReses++; // the reset line
        else if ( pLine[1] == 'i' && pLine[2] == 'n' && pLine[3] == 'p' ) 
            p->nDotInputs++;
        else if ( pLine[1] == 'o' && pLine[2] == 'u' && pLine[3] == 't' ) 
            p->nDotOutputs++;
    }
    // save this line
    p->pLines[ p->nLines++ ] = pLine;
}

/**Function*************************************************************

  Synopsis    [Tries to find a file name with a different extension.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aio_ReadFileGetSimilar( char * pFileNameWrong, char * pS1, char * pS2, char * pS3, char * pS4, char * pS5 )
{
    FILE * pFile;
    char * pFileNameOther;
    char * pFileGen;

    if ( pS1 == NULL )
        return NULL;

    // get the generic file name
    pFileGen = Extra_FileNameGeneric( pFileNameWrong );
    pFileNameOther = Extra_FileNameAppend( pFileGen, pS1 );
    pFile = fopen( pFileNameOther, "r" );
    if ( pFile == NULL && pS2 )
    { // try one more
        pFileNameOther = Extra_FileNameAppend( pFileGen, pS2 );
        pFile = fopen( pFileNameOther, "r" );
        if ( pFile == NULL && pS3 )
        { // try one more
            pFileNameOther = Extra_FileNameAppend( pFileGen, pS3 );
            pFile = fopen( pFileNameOther, "r" );
            if ( pFile == NULL && pS4 )
            { // try one more
                pFileNameOther = Extra_FileNameAppend( pFileGen, pS4 );
                pFile = fopen( pFileNameOther, "r" );
                if ( pFile == NULL && pS5 )
                { // try one more
                    pFileNameOther = Extra_FileNameAppend( pFileGen, pS5 );
                    pFile = fopen( pFileNameOther, "r" );
                }
            }
        }
    }
    FREE( pFileGen );
    if ( pFile )
    {
        fclose( pFile );
        return pFileNameOther;
    }
    // did not find :(
    return NULL;            
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


