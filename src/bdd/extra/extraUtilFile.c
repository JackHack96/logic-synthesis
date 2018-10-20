/**CFile****************************************************************

  FileName    [extraUtilFile.c]

  PackageName [extra]

  Synopsis    [Various reusable software utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 1, 2003.]

  Revision    [$Id: extraUtilFile.c,v 1.5 2004/05/12 04:30:10 alanmi Exp $]

***********************************************************************/

#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_FileNameCheckExtension( char * FileName, char * Extension )
{
    char * pDot;
    // find "dot" if it is present in the file name
    pDot = strstr( FileName, "." );
    if ( pDot && strcmp( pDot+1, Extension ) == 0 )
        return 1;
    else
        return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the composite name of the file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileNameAppend( char * pBase, char * pSuffix )
{
    static char Buffer[500];
    sprintf( Buffer, "%s%s", pBase, pSuffix );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileNameGeneric( char * FileName )
{
    char * pDot;
    char * pUnd;
    char * pRes;
    
    // find the generic name of the file
    pRes = util_strsav( FileName );
    // find the pointer to the "." symbol in the file name
//  pUnd = strstr( FileName, "_" );
    pUnd = NULL;
    pDot = strstr( FileName, "." );
    if ( pUnd )
        pRes[pUnd - FileName] = 0;
    else if ( pDot )
        pRes[pDot - FileName] = 0;
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Returns the file size.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_FileSize( char * pFileName )
{
    FILE * pFile;
    int nFileSize;
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Extra_FileSize(): The file is unavailable (absent or open).\n" );
        return 0;
    }
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile ); 
    fclose( pFile );
    return nFileSize;
}


/**Function*************************************************************

  Synopsis    [Read the file into the internal buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileRead( FILE * pFile )
{
    int nFileSize;
    char * pBuffer;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer = ALLOC( char, nFileSize + 3 );
    fread( pBuffer, nFileSize, 1, pFile );
    // terminate the string with '\0'
    pBuffer[ nFileSize + 0] = '\n';
    pBuffer[ nFileSize + 1] = '\0';
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    [Returns the time stamp.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_TimeStamp()
{
    static char Buffer[100];
	time_t ltime;
	char * TimeStamp;
    // get the current time
	time( &ltime );
	TimeStamp = asctime( localtime( &ltime ) );
	TimeStamp[ strlen(TimeStamp) - 1 ] = 0;
    strcpy( Buffer, TimeStamp );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Appends the string.]

  Description [Assumes that the given string (pStrGiven) has been allocated
  before using malloc(). The additional string has not been allocated.
  Allocs more root, appends the additional part, frees the old given string.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_StringAppend( char * pStrGiven, char * pStrAdd )
{
    char * pTemp;
    if ( pStrGiven )
    {
        pTemp = ALLOC( char, strlen(pStrGiven) + strlen(pStrAdd) + 2 );
        sprintf( pTemp, "%s%s", pStrGiven, pStrAdd );
        free( pStrGiven );
    }
    else
        pTemp = util_strsav( pStrAdd );
    return pTemp;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


