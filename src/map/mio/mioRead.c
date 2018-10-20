/**CFile****************************************************************

  FileName    [mioRead.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mioRead.c,v 1.9 2004/10/19 06:40:16 satrajit Exp $]

***********************************************************************/

#include "mioInt.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

static Mio_Library_t * Mio_LibraryReadOne( Mv_Frame_t * pMvsis, char * FileName, bool fExtendedFormat, st_table * tExcludeGate );
static int          Mio_LibraryReadInternal( Mio_Library_t * pLib, char * pBuffer, bool fExtendedFormat, st_table * tExcludeGate );
static Mio_Gate_t * Mio_LibraryReadGate( char ** ppToken, bool fExtendedFormat );
static Mio_Pin_t *  Mio_LibraryReadPin( char ** ppToken, bool fExtendedFormat );
static char *       chomp( char *s );
static void         Mio_LibraryDetectSpecialGates( Mio_Library_t * pLib );

#ifdef WIN32
extern int          isspace( int c );  // to silence the warning in VC
#endif

/**Function*************************************************************

  Synopsis    [Read the genlib type of library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Library_t * Mio_LibraryRead( Mv_Frame_t * pMvsis, char * FileName, char * ExcludeFile )
{
    Mio_Library_t * pLib;
    int num;

    st_table * tExcludeGate = 0;

    if ( ExcludeFile )
    {
        tExcludeGate = st_init_table(strcmp, st_strhash);
        if ( (num = Mio_LibraryReadExclude( pMvsis, ExcludeFile, tExcludeGate )) == -1 )
        {
            st_free_table( tExcludeGate );
            tExcludeGate = 0;
            return 0;
        }
       
        fprintf ( Mv_FrameReadOut( pMvsis ), "Read %d gates from exclude file\n", num );
    }

    pLib = Mio_LibraryReadOne( pMvsis, FileName, 0, tExcludeGate );       // try normal format first ..
    if ( pLib == NULL )
    {
        pLib = Mio_LibraryReadOne( pMvsis, FileName, 1, tExcludeGate );   // .. otherwise try extended format 
        if ( pLib != NULL )
            printf ( "Warning: Read extended GENLIB format but ignoring extensions\n" );
    }

    return pLib;
}

/**Function*************************************************************

  Synopsis    [Read the genlib type of library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Library_t * Mio_LibraryReadOne( Mv_Frame_t * pMvsis, char * FileName, bool fExtendedFormat, st_table * tExcludeGate )
{
    Mio_Library_t * pLib;
    char * pBuffer = 0;

    // allocate the genlib structure
    pLib = ALLOC( Mio_Library_t, 1 );
    memset( pLib, 0, sizeof(Mio_Library_t) );
    pLib->pName = util_strsav( FileName );
    pLib->pMan = Mv_FrameReadMan(pMvsis);     
    pLib->tName2Gate = st_init_table(strcmp, st_strhash);

    // read the file and clean comments
    // pBuffer = Io_ReadFileFileContents( FileName, NULL );
    // we don't use above function but actually do the same thing explicitly
    // to handle open_path expansion correctly

    {
        FILE * pFile;
        int nFileSize;

        // open the BLIF file for binary reading
        pFile = Io_FileOpen( FileName, "open_path", "rb" );
        // if we got this far, file should be okay otherwise would
        // have been detected by caller
        assert ( pFile != NULL );
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
    }

    Io_ReadFileRemoveComments( pBuffer, NULL, NULL );

    // parse the contents of the file
    if ( Mio_LibraryReadInternal( pLib, pBuffer, fExtendedFormat, tExcludeGate ) )
    {
        Mio_LibraryDelete( pLib );
        free( pBuffer );
        return NULL;
    }
    free( pBuffer );

    // derive the functinality of gates
    if ( Mio_LibraryParseFormulas( pLib ) )
    {
        Mio_LibraryDelete( pLib );
        return NULL;
    }

    // detect INV and NAND2
    Mio_LibraryDetectSpecialGates( pLib );
//Mio_WriteLibrary( stdout, pLib );
    return pLib;
}

/**Function*************************************************************

  Synopsis    [Read the genlib type of library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_LibraryReadInternal( Mio_Library_t * pLib, char * pBuffer, bool fExtendedFormat, st_table * tExcludeGate )
{
    Mio_Gate_t * pGate, ** ppGate;
    char * pToken;
    int nDel = 0;

    // start the linked list of gates
    pLib->pGates = NULL;
    ppGate = &pLib->pGates;

    // read gates one by one
    pToken = strtok( pBuffer, " \t\r\n" );
    while ( pToken && strcmp( pToken, MIO_STRING_GATE ) == 0 )
    {
        // derive the next gate
        pGate = Mio_LibraryReadGate( &pToken, fExtendedFormat );
        if ( pGate == NULL )
            return 1;
        
        // set the library
        pGate->pLib = pLib;

        // printf ("Processing: '%s'\n", pGate->pName);

        if ( tExcludeGate && st_is_member( tExcludeGate, pGate->pName ) )
        {
            //printf ("Excluding: '%s'\n", pGate->pName);
            Mio_GateDelete( pGate );
            nDel++;
        } 
        else
        {
            // add this gate to the list
            *ppGate = pGate;
            ppGate  = &pGate->pNext;
            *ppGate = NULL;

            // remember this gate by name
            if ( !st_is_member( pLib->tName2Gate, pGate->pName ) )
                st_insert( pLib->tName2Gate, pGate->pName, (char *)pGate );
            else
                printf( "The gate with name \"%s\" appears more than once.\n", pGate->pName );
        }
    }

    // check what is the last word read
    if ( pToken && strcmp( pToken, ".end" ) != 0 )
        return 1;

    if ( nDel != 0 ) 
        printf( "Actually excluded %d cells\n", nDel );

    return 0;
}

/**Function*************************************************************

  Synopsis    [Read the genlib type of gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Gate_t * Mio_LibraryReadGate( char ** ppToken, bool fExtendedFormat )
{
    Mio_Gate_t * pGate;
    Mio_Pin_t * pPin, ** ppPin;
    char * pToken = *ppToken;

    // allocate the gate structure
    pGate = ALLOC( Mio_Gate_t, 1 );
    memset( pGate, 0, sizeof(Mio_Gate_t) );

    // read the name
    pToken = strtok( NULL, " \t\r\n" );
    pGate->pName = util_strsav( pToken );

    // read the area
    pToken = strtok( NULL, " \t\r\n" );
    pGate->dArea = atof( pToken );

    // read the formula

    // first the output name
    pToken = strtok( NULL, "=" );
    pGate->pOutName = chomp( pToken );

    // then rest of the expression 
    pToken = strtok( NULL, ";" );
    pGate->pForm = util_strsav( pToken );

    // read the pin info
    // start the linked list of pins
    pGate->pPins = NULL;
    ppPin = &pGate->pPins;

    // read gates one by one
    pToken = strtok( NULL, " \t\r\n" );
    while ( pToken && strcmp( pToken, MIO_STRING_PIN ) == 0 )
    {
        // derive the next gate
        pPin = Mio_LibraryReadPin( &pToken, fExtendedFormat );
        if ( pPin == NULL )
        {
            Mio_GateDelete( pGate );
            *ppToken = pToken;
            return NULL;
        }
        // add this gate to the list
        *ppPin = pPin;
        ppPin  = &pPin->pNext;
        *ppPin = NULL;

        // get the next token
        pToken = strtok( NULL, " \t\r\n" );
    }

    *ppToken = pToken;
    return pGate;
}



/**Function*************************************************************

  Synopsis    [Read the genlib type of pin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Pin_t * Mio_LibraryReadPin( char ** ppToken, bool fExtendedFormat )
{
    Mio_Pin_t * pPin;
    char * pToken = *ppToken;

    // allocate the gate structure
    pPin = ALLOC( Mio_Pin_t, 1 );
    memset( pPin, 0, sizeof(Mio_Pin_t) );

    // read the name
    pToken = strtok( NULL, " \t\r\n" );
    pPin->pName = util_strsav( pToken );

    // read the pin phase
    pToken = strtok( NULL, " \t\r\n" );
    if ( strcmp( pToken, MIO_STRING_UNKNOWN ) == 0 )
        pPin->Phase = MIO_PHASE_UNKNOWN;
    else if ( strcmp( pToken, MIO_STRING_INV ) == 0 )
        pPin->Phase = MIO_PHASE_INV;
    else if ( strcmp( pToken, MIO_STRING_NONINV ) == 0 )
        pPin->Phase = MIO_PHASE_NONINV;
    else 
    {
        printf( "Cannot read pin phase specification\n" );
        Mio_PinDelete( pPin );
        *ppToken = pToken;
        return NULL;
    }

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dLoadInput = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dLoadMax = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dDelayBlockRise = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dDelayFanoutRise = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dDelayBlockFall = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dDelayFanoutFall = atof( pToken );

    if ( fExtendedFormat )
    {
        /* In extended format, the field after dDelayFanoutRise
         * is to be ignored
         **/

        pPin->dDelayBlockFall  = pPin->dDelayFanoutFall;

        pToken = strtok( NULL, " \t" );
        pPin->dDelayFanoutFall = atof( pToken );

        /* last field is ignored */
        pToken = strtok( NULL, " \t\r\n" );
    }

    if ( pPin->dDelayBlockRise > pPin->dDelayBlockFall )
        pPin->dDelayBlockMax = pPin->dDelayBlockRise;
    else
        pPin->dDelayBlockMax = pPin->dDelayBlockFall;

    *ppToken = pToken;
    return pPin;
}


/**Function*************************************************************

  Synopsis    [Duplicates string and returns it with leading and 
               trailing spaces removed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *chomp( char *s )
{
    char *b = ALLOC(char, strlen(s)+1), *c = b;
    while (*s && isspace(*s))
        ++s;
    while (*s && !isspace(*s))
        *c++ = *s++;
    *c = 0;
    return b;
}   
        
/**Function*************************************************************

  Synopsis    [Duplicates string and returns it with leading and 
               trailing spaces removed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibraryDetectSpecialGates( Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;
    DdNode * bFuncInv;
    DdNode * bFuncNand2;

    bFuncInv   = Cudd_Not( pLib->dd->vars[0] );                                  Cudd_Ref( bFuncInv );
    bFuncNand2 = Cudd_bddNand( pLib->dd, pLib->dd->vars[0], pLib->dd->vars[1] ); Cudd_Ref( bFuncNand2 );

    Mio_LibraryForEachGate( pLib, pGate )
        if ( pLib->pGateInv == NULL && pGate->bFunc == bFuncInv )
        {
            pLib->pGateInv = pGate;
            break;
        }
    if ( pLib->pGateInv == NULL )
    {
        printf( "Warnings: GENLIB library reader cannot detect the invertor gate.\n" );
        printf( "Some parts of the supergate-based technology mapper may not work correctly.\n" );
    }

    Mio_LibraryForEachGate( pLib, pGate )
        if ( pLib->pGateNand2 == NULL && pGate->bFunc == bFuncNand2 )
        {
            pLib->pGateNand2 = pGate;
            break;
        }
    if ( pLib->pGateNand2 == NULL )
    {
        printf( "Warnings: GENLIB library reader cannot detect the NAND2 gate.\n" );
        printf( "Some parts of the supergate-based technology mapper may not work correctly.\n" );
    }

    Cudd_RecursiveDeref( pLib->dd, bFuncInv );
    Cudd_RecursiveDeref( pLib->dd, bFuncNand2 );
}

/**Function*************************************************************

  Synopsis    [populate hash table of gates to be exlcuded from genlib]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_LibraryReadExclude( Mv_Frame_t * pMvsis, char * ExcludeFile, st_table * tExcludeGate )
{
    int nDel = 0;
    FILE *pEx;
    char buffer[128];

    assert ( tExcludeGate );

    if ( ExcludeFile )
    {
        pEx = fopen( ExcludeFile, "r" );

        if ( pEx == NULL )
        {
            fprintf ( Mv_FrameReadErr( pMvsis ), "Error: Could not open exclude file %s. Stop.\n", ExcludeFile );
            return -1;
        }

        while (1 == fscanf( pEx, "%127s", buffer ))
        {
            //printf ("Read: '%s'\n", buffer );
            st_insert( tExcludeGate, util_strsav( buffer ), (char *)0 );
            nDel++;
        }

        fclose( pEx );
    }

    return nDel;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


