/**CFile****************************************************************

  FileName    [ioReadLines.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Extracts preliminary information about the network in the file.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioReadLines.c,v 1.21 2004/06/18 00:20:39 satrajit Exp $]

***********************************************************************/

#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int        Io_ReadLinesDotMv( Io_Read_t * p, int Line );
static Io_Var_t * Io_ReadLinesDotMvCopyVar( Io_Var_t * pVar );
static char *     Io_ReadLinesDotMvPivot( char * pStr );
static int        Io_ReadLinesDotLatch( Io_Read_t * p, Io_Latch_t * pIoLatch );
static int        Io_ReadLinesDotNode( Io_Read_t * p, Io_Node_t * pIoNode );
static int        Io_ReadLinesDotRes( Io_Read_t * p, int Line );
static char *     Io_ReadLinesLastEntry( char * pStr );
static void       Io_ReadLinesRemoveExtenders( Io_Read_t * p, int Line );
static int        Io_ReadLinesDetectFileType( Io_Read_t * p, Io_Node_t * pIoNode );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Preliminary parsing actions before compiling the network.]

  Description [Assumes that the file has been split into lines. Sorts
  through the lines and assigns the Io_Node_t structure for each would-be node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadLines( Io_Read_t * p )
{
    char * pDot;
    int nDotNodesInit, nDotMvsInit;
    int nDotResesInit, nDotLatchesInit; 
    int nDotInputsInit, nDotOutputsInit; 
    int i, n;

    // set a flag to show that this is the EXDC network
    p->fParsingExdcNet = 0;
    if ( p->LineExdc )
        p->fParsingExdcNet = 1;

    // save the initial parameters
    nDotInputsInit  = p->nDotInputs;
    nDotOutputsInit = p->nDotOutputs;
    nDotNodesInit   = p->nDotNodes;
    nDotMvsInit     = p->nDotMvs;
    nDotLatchesInit = p->nDotLatches;
    nDotResesInit   = p->nDotReses;
    p->nDotInputs   = 0;
    p->nDotOutputs  = 0;
    p->nDotNodes    = 0;
    p->nDotMvs      = 0;
    p->nDotLatches  = 0;
    p->nDotReses    = 0;
    p->nDotInputArr = 0;
    p->LineModel    = -1;
    p->LineSpec     = -1;

    // clean the tables for EXDC network
    if ( p->fParsingExdcNet )
        Io_ReadStructCleanTables( p );

    // sort the dot statements
    for ( i = p->fParsingExdcNet? p->LineExdc + 1: 0; i < p->nDots; i++ )
    {
        // get the pointer to the line with the current dot-statement
        pDot = p->pLines[ p->pDots[i] ];
        assert( pDot[0] == '.' );

        // sort through dot-statements
        // strncmp() is not used to make string comparison faster
        if ( pDot[1] == 'n' && pDot[2] == 'a' && pDot[3] == 'm' && pDot[4] == 'e' && pDot[5] == 's' )
            p->pIoNodes[ p->nDotNodes++ ].LineNames = p->pDots[i];
        else if ( pDot[1] == 't' && pDot[2] == 'a' && pDot[3] == 'b' && pDot[4] == 'l' && pDot[5] == 'e' )
        {
            p->Type = IO_FILE_BLIF_MV;
            p->pIoNodes[ p->nDotNodes++ ].LineNames = p->pDots[i];
        }
        else if ( pDot[1] == 'd' && pDot[2] == 'e' && pDot[3] == 'f' && pDot[4] == ' ' ) // default line
            continue;
        else if ( pDot[1] == 'd' && pDot[2] == 'e' && pDot[3] == 'f' && pDot[4] == 'a' && pDot[5] == 'u' && pDot[6] == 'l' && pDot[7] == 't' && pDot[8] == ' ' ) // default line
            continue;
        else if ( pDot[1] == 'm' && pDot[2] == 'v' )
            p->pDotMvs[ p->nDotMvs++ ] = p->pDots[i];
        else if ( pDot[1] == 'l' && pDot[2] == 'a' && pDot[3] == 't' && pDot[4] == 'c' && pDot[5] == 'h' )
            p->pIoLatches[ p->nDotLatches++ ].LineLatch = p->pDots[i];
        else if ( pDot[1] == 'm' && pDot[2] == 'o' && pDot[3] == 'd' && pDot[4] == 'e' && pDot[5] == 'l' )
            p->LineModel = p->pDots[i];
        else if ( pDot[1] == 's' && pDot[2] == 'p' && pDot[3] == 'e' && pDot[4] == 'c' )
            p->LineSpec = p->pDots[i];
        else if ( pDot[1] == 'i' && pDot[2] == 'n' && pDot[3] == 'p' && pDot[4] == 'u' && pDot[5] == 't' && pDot[6] == 's' )
            p->pDotInputs[ p->nDotInputs++ ] = p->pDots[i];
        else if ( pDot[1] == 'o' && pDot[2] == 'u' && pDot[3] == 't' && pDot[4] == 'p' && pDot[5] == 'u' && pDot[6] == 't' && pDot[7] == 's' )
            p->pDotOutputs[ p->nDotOutputs++ ] = p->pDots[i];
        else if ( pDot[1] == 'e' && pDot[2] == 'n' && pDot[3] == 'd' )
        {
            p->LineEnd = p->pDots[i];
            break;
        }
        else if ( pDot[1] == 'e' && pDot[2] == 'x' && pDot[3] == 'd' && pDot[4] == 'c' )
        {
            p->LineExdc = i;
            break;
        }
        else if ( pDot[1] == 'r' && pDot[2] == ' ' || pDot[1] == 'r' && pDot[2] == 'e' && pDot[3] == 's' ) // reset line
        {
            if ( Io_ReadLinesDotRes( p, p->pDots[i] ) )
                return 1;
        }
        else if ( pDot[1] == 'i' && pDot[2] == 'n' && pDot[3] == 'p' && pDot[4] == 'u' && pDot[5] == 't' && pDot[6] == '_' && pDot[7] == 'a' )
            p->pDotInputArr[ p->nDotInputArr++ ] = p->pDots[i];
	else if ( strncmp( ".default_input_arrival", pDot, 22 ) == 0 )
	{
            p->pLineDefaultInputArrival = pDot;
	}
        else
        {
            char * pTemp;
            p->LineCur = p->pDots[i];
            pTemp = strtok( pDot, " \t" );
            if ( strncmp( pTemp, ".wire_load_slope", 15 ) )
            {
                sprintf( p->sError, "Ignoring \"%s\" (warning only).", pTemp );
                Io_ReadPrintErrorMessage( p );
            }
            // overwrite this line with spaces
            for ( n = 0; pDot[n]; n++ )
                pDot[n] = 0;
        }
    }

    // make sure the lines are read correctly
    if ( !p->fParsingExdcNet && !p->LineExdc )
    { // this is not an EXDC network and there is no EXDC network
        assert( nDotInputsInit  == p->nDotInputs );
        assert( nDotOutputsInit == p->nDotOutputs );
        assert( nDotNodesInit   == p->nDotNodes );
        assert( nDotMvsInit     == p->nDotMvs );
        assert( nDotLatchesInit == p->nDotLatches );
        assert( nDotResesInit   == p->nDotReses );
    }

    // error diagnostics
    if ( !p->fParsingExdcNet && p->LineModel < 0 )
    { // this is a primary network
        p->LineCur = 0;
        sprintf( p->sError, ".model line is missing in the file." );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    if ( p->nDotInputs == 0 )
    {
        p->LineCur = 0;
        sprintf( p->sError, ".inputs line for the %s network is missing.", p->fParsingExdcNet? "EXDC" : "primary" );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    if ( p->nDotOutputs == 0 )
    {
        p->LineCur = 0;
        sprintf( p->sError, ".outputs line for the %s network is missing.", p->fParsingExdcNet? "EXDC" : "primary" );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    if ( (!p->LineExdc || p->fParsingExdcNet ) && p->LineEnd < 0 )
    { // there is no EXDC network or this is an EXDC network
        p->LineCur = 0;
        sprintf( p->sError, ".end line is missing in the file." );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    if ( p->nDotReses != 0 && p->nDotLatches != p->nDotReses )
    {
        p->LineCur = 0;
        sprintf( p->sError, "The number of latches differs from the number of reset lines." );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }

    // remove backward slash line extenders
    for ( n = 0; n < p->nDotInputs; n++ )
        Io_ReadLinesRemoveExtenders( p, p->pDotInputs[n] );
    for ( n = 0; n < p->nDotOutputs; n++ )
        Io_ReadLinesRemoveExtenders( p, p->pDotOutputs[n] );
    for ( n = 0; n < p->nDotNodes; n++ )
        Io_ReadLinesRemoveExtenders( p, p->pIoNodes[n].LineNames );

    // preparse the .mv statements
    for ( n = 0; n < p->nDotMvs; n++ )
        if ( Io_ReadLinesDotMv( p, p->pDotMvs[n] ) )
            return 1;

    // preparse the latch statements
    for ( n = 0; n < p->nDotLatches; n++ )
        if ( Io_ReadLinesDotLatch( p, p->pIoLatches + n ) )
            return 1;

    // preparse the nodes
    for ( n = 0; n < p->nDotNodes; n++ )
        if ( Io_ReadLinesDotNode( p, p->pIoNodes + n ) )
            return 1;

    // preparse the latch reset tables
    for ( n = 0; n < p->nDotLatches; n++ )
        if ( p->pIoLatches[n].IoNode.LineNames != -1 )
            if ( Io_ReadLinesDotNode( p, &(p->pIoLatches[n].IoNode) ) )
                return 1;

    return 0;
}

/**Function*************************************************************

  Synopsis    [Parses one .mv line.]

  Description [Parses one .mv line. Derives the MV variable 
  (represented by the number of values and the list of value names).
  For each node name associated with this variable, stores the 
  copy of the MV variable in the hash table (p->tMvLines).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadLinesDotMv( Io_Read_t * p, int Line )
{
    Io_Var_t * pVar;
    char * pLine, * pTemp, * pPivot;
    int nValues;
    int i;

    // get the line
    pLine = p->pLines[ Line ];
    assert( strncmp( pLine, ".mv", 3 ) == 0 );
    // find the place in the line where the number of values is written
    pPivot = Io_ReadLinesDotMvPivot( pLine );
    // separate the line in two
    *(pPivot - 1) = '\0';

    // read the number of values;
    pTemp = strtok( pPivot, " \t" );
	if ( pTemp == NULL )
    {
        p->LineCur = Line;
        sprintf( p->sError, "The number of values is not specified." );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }

    // get the variable
    nValues = atoi( pTemp );
    if ( nValues < 2 || nValues > 32000 )
    {
        p->LineCur = Line;
        sprintf( p->sError, "The number of values (%s) is wrong.", pTemp );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }

    // read the first value name
    pTemp = strtok( NULL, " \t" );
    if ( pTemp != NULL )
    {
        // allocate the variable
        pVar = ALLOC( Io_Var_t, 1 );
        // assing the number of values
        pVar->nValues = nValues;
        // allocate the value names
        pVar->pValueNames = ALLOC( char *, pVar->nValues );
        // assing the value names one by one
        i = 0;
        do  pVar->pValueNames[i++] = util_strsav( pTemp );
        while ( pTemp = strtok( NULL, " \t" ) );
        // check if the number of value names is okay
        if ( i != pVar->nValues )
        {
            p->LineCur = Line;
            sprintf( p->sError, "The number of value names (%d) should be %d.", i, pVar->nValues );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
        // allocate the table to store value names if it does not exist
        if ( p->tName2Var == NULL )
            p->tName2Var = st_init_table(strcmp, st_strhash);
        // allocate the table to store number of values if it does not exist
        if ( p->tName2Values == NULL )
            p->tName2Values = st_init_table(strcmp, st_strhash);

        // skip the .mv line
        pTemp = strtok( pLine, " \t" );
        // get the node names, to which this variable belongs
        for ( i = 0; pTemp = strtok( NULL, ", \t" ); i++ )
        {
            if ( i )
                pVar = Io_ReadLinesDotMvCopyVar( pVar );
//            st_insert( p->tName2Var, pTemp, (char *)pVar );
            st_insert( p->tName2Var, util_strsav(pTemp), (char *)pVar );

            // store the number of values if it is larger than 2
            if ( nValues > 2 )
            if ( Io_ReadLinesAddNumValues( p, pTemp, nValues ) )
                return 1;
        }
    }
    else if ( nValues > 2 )
    {
        // allocate the table to store number of values if it does not exist
        if ( p->tName2Values == NULL )
            p->tName2Values = st_init_table(strcmp, st_strhash);

        // get the node names, to which this num values applies
        // skip the .mv line
        pTemp = strtok( pLine, " \t" );
        // read the node names
        for ( i = 0; pTemp = strtok( NULL, ", \t" ); i++ )
            if ( Io_ReadLinesAddNumValues( p, pTemp, nValues ) )
                return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Produces a copy of the MV variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Io_Var_t * Io_ReadLinesDotMvCopyVar( Io_Var_t * pVar )
{
    Io_Var_t * pCopy;
    int i;
    pCopy = ALLOC( Io_Var_t, 1 );
    pCopy->nValues     = pVar->nValues;
    pCopy->pValueNames = ALLOC( char *, pVar->nValues );
    for ( i = 0; i < pVar->nValues; i++ )
        pCopy->pValueNames[i] = util_strsav( pVar->pValueNames[i] );
    return pCopy;
}

/**Function*************************************************************

  Synopsis    [Finds the place in MV line where the number of values is written.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Io_ReadLinesDotMvPivot( char * pStr )
{
    char * pComma, * pTemp;
    // find the last comma
    pComma = NULL;
    for ( pTemp = pStr; *pTemp; pTemp++ )
        if ( *pTemp == ',' )
            pComma = pTemp;
    if ( pComma )
    {
        // find the next non-space char
        pTemp = pComma + 1;
        while ( *pTemp && (*pTemp == ' ' || *pTemp == '\t') )
            pTemp++;
        // find the next space char
        while ( *pTemp && *pTemp != ' ' && *pTemp != '\t' )
            pTemp++;
        // find the next non-space char
        while ( *pTemp && (*pTemp == ' ' || *pTemp == '\t') )
            pTemp++;
    }
    else
    { // find the third token on the line (.mv + signal + nValues + ...)
        pTemp = pStr;
        // find the next space char
        while ( *pTemp && *pTemp != ' ' && *pTemp != '\t' )
            pTemp++;
        // find the next non-space char
        while ( *pTemp && (*pTemp == ' ' || *pTemp == '\t') )
            pTemp++;
        // find the next space char
        while ( *pTemp && *pTemp != ' ' && *pTemp != '\t' )
            pTemp++;
        // find the next non-space char
        while ( *pTemp && (*pTemp == ' ' || *pTemp == '\t') )
            pTemp++;
    }
    // return this position
    return pTemp;
}




/**Function*************************************************************

  Synopsis    [Parses one .latch line.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadLinesDotLatch( Io_Read_t * p, Io_Latch_t * pIoLatch )
{
    char * pTemp;

    // set the current parsing line
    p->LineCur = pIoLatch->LineLatch;

    // skip the latch word
    pTemp = strtok( p->pLines[pIoLatch->LineLatch], " \t" );
    assert( strcmp( pTemp, ".latch" ) == 0 );

    // get the latch input
    pTemp = strtok( NULL, " \t" ); 
    if ( pTemp == NULL )
    {
        sprintf( p->sError, "The latch directive is incomplete." );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    pIoLatch->pInputName = pTemp;

    // get the latch output
    pTemp = strtok( NULL, " \t" ); 
    if ( pTemp == NULL )
    {
        sprintf( p->sError, "The latch directive is incomplete." );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    pIoLatch->pOutputName = pTemp;

    // get the reset value if it exists
    pTemp = strtok( NULL, " \t" ); 
    if ( pTemp )
    {
        pIoLatch->ResValue = atoi(pTemp);
        // if the reset value is a don't-care, set it to be zero
        if ( pIoLatch->ResValue == 2 || pIoLatch->ResValue == 3 )
            pIoLatch->ResValue = 0;
        if ( pIoLatch->ResValue < 0 || pIoLatch->ResValue > 1 )
        {
            sprintf( p->sError, "The latch reset value is invalid (%s).", pTemp );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
    }
    else
        pIoLatch->ResValue = -1;

    // get the pointer to the number of reset line 
    // this number will be used to preparset the reset table later
    pIoLatch->IoNode.LineNames = -1;
    if ( p->tLatch2ResLine )
        if ( !st_lookup( p->tLatch2ResLine, pIoLatch->pOutputName, (char**)&(pIoLatch->IoNode.LineNames) ) )
        {
            p->LineCur = pIoLatch->LineLatch;
            sprintf( p->sError, "The latch \"%s\" does not have .reset line.", pIoLatch->pOutputName );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Preparses the .res statement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadLinesDotRes( Io_Read_t * p, int Line )
{
    char * pTemp;
    // get the latch output name (the last entry on .res line)
    pTemp = Io_ReadLinesLastEntry( p->pLines[Line] );
    if ( pTemp[0] == '-' && pTemp[1] == '>' )
        pTemp += 2;
    // hash latch output names into the beginning line of the reset table
    if ( p->tLatch2ResLine == NULL )
        p->tLatch2ResLine = st_init_table(strcmp, st_strhash);
    if ( st_insert( p->tLatch2ResLine, pTemp, (char*)Line ) )
    {
        sprintf( p->sError, "Latch \"%s\" has multiple reset lines.", pTemp );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    p->nDotReses++;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Preparses the .node/.table/.res statement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadLinesDotNode( Io_Read_t * p, Io_Node_t * pIoNode )
{
    char * pTemp, * pLine, * pLimit;
    int LineNext, nValues, i;
    Io_Var_t * pVar;

    // if the output identifier (->) appears on the .names line, remove it
    if ( pTemp = strstr( p->pLines[ pIoNode->LineNames ], "->" ) )
    {
        pTemp[0] = ' ';
        pTemp[1] = ' ';
    }

    // get the output name as the last name on the .names line 
    pIoNode->pOutput = Io_ReadLinesLastEntry( p->pLines[ pIoNode->LineNames ] );

    // find the line after the .names/.table line
    pLimit = p->pLines[pIoNode->LineNames] + strlen( p->pLines[pIoNode->LineNames] );
    for ( LineNext = pIoNode->LineNames + 1; p->pLines[LineNext] < pLimit; LineNext++ );
    pLine = p->pLines[ LineNext ];
    // process the .def line if present
    pIoNode->Default = -1;
    if ( pLine[0] == '.' && pLine[1] == 'd' && pLine[2] == 'e' && pLine[3] == 'f' )
    {
        assert( p->nDotDefs > 0 );
        // read the default value
        pTemp = strtok( pLine, " \t" );
        pTemp = strtok( NULL, " \t" );
        // first try to read the symbolic value
        if ( p->tName2Var && st_lookup( p->tName2Var, pIoNode->pOutput, (char**)&pVar ) )
        {
            // try to find the symbol variable name
            for ( i = 0; i < pVar->nValues; i++ )
                if ( strcmp( pVar->pValueNames[i], pTemp ) == 0 )
                {
                    pIoNode->Default = i;
                    break;
                }
            if ( i == pVar->nValues )
            {
                p->LineCur = LineNext;
                sprintf( p->sError, "No symbolic value for \"%s\" in the default line for node \"%s\".",
                    pTemp, pIoNode->pOutput );
                Io_ReadPrintErrorMessage( p );
                return 1;
            }
        }
        else 
        {
            if ( pTemp[0] < '0' || pTemp[0] > '9' )
            {
                p->LineCur = LineNext;
                sprintf( p->sError, "Cannot read numeric value (%s) in the default line for node \"%s\".", 
                    pTemp, pIoNode->pOutput );
                Io_ReadPrintErrorMessage( p );
                return 1;
            }
            pIoNode->Default = atoi( pTemp );
            if ( pIoNode->Default < 0 || pIoNode->Default > 32000 )
            {
                p->LineCur = LineNext;
                sprintf( p->sError, "Default value (%s) is not correct.", pTemp );
                Io_ReadPrintErrorMessage( p );
                return 1;
            }
        }
        // go to the next line
        pLine = p->pLines[ ++LineNext ];
    }

    // assign the first table line and count the lines
    pIoNode->LineTable = -1;
    pIoNode->nLines    = 0;
    for ( ; *pLine != '.'; LineNext++, pLine = p->pLines[LineNext] )
        if ( *pLine != '\0' )
        {
            if ( pIoNode->LineTable == -1 )
                pIoNode->LineTable = LineNext;
            pIoNode->nLines++;
        }

    // detect the file type
    if ( p->Type == IO_FILE_NONE )
        p->Type = Io_ReadLinesDetectFileType( p, pIoNode );

    // get the number of values
    if ( p->Type == IO_FILE_BLIF )
        pIoNode->nValues = 2;
    else if ( p->Type == IO_FILE_BLIF_MVS )
        pIoNode->nValues = strlen( Io_ReadLinesLastEntry( p->pLines[ pIoNode->LineTable ] ) );
    else if ( p->Type == IO_FILE_BLIF_MV )
    {
        // get the number of values if present in the table
        if ( p->tName2Values && st_lookup( p->tName2Values, pIoNode->pOutput, (char **)&nValues ) )
            pIoNode->nValues = nValues;
        else
            pIoNode->nValues = 2;
    }
    else
    {
        assert( 0 );
    }
    // update the largest number of values
    if ( p->nValuesMax < pIoNode->nValues )
        p->nValuesMax = pIoNode->nValues;

    // set the pointer to the parent
    // this is useful to reduce the number of arguments passed in the procedures
//  pIoNode->p = p;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the last entry in the line.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Io_ReadLinesLastEntry( char * pStr )
{
    char * pTemp, * pEnd;
    pEnd = pStr + strlen(pStr) - 1;
    assert( *pEnd != ' ' && *pEnd != '\t' );
    for ( pTemp = pEnd; *pTemp && *pTemp != ' ' && *pTemp != '\t'; pTemp-- );
    return pTemp + 1;
}

/**Function*************************************************************

  Synopsis    [Gracefully treats long lines.]

  Description [Removes the back slashes at the end of long lines and
  combines several 0-separated lines into one line, for smooth parsing
  of name lists.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadLinesRemoveExtenders( Io_Read_t * p, int Line )
{
    char * pLine, * pCur;
    while ( 1 )
    {
        // make sure the next line exists
        assert( Line < p->nLines - 1 );
        // get the next line
        pLine = p->pLines[ Line + 1 ];
        // walk backward till the first non-space char
        for ( pCur = pLine - 1; *pCur == 0 || *pCur == ' ' || *pCur == '\t'; pCur-- );
        // if this char is a backslash, walk forward and overwrite it with spaces
        if ( *pCur == '\\' )
        {
            *pCur = ' ';
            for ( pCur++; *pCur == 0 || *pCur == ' ' || *pCur == '\n' || *pCur == '\t'; pCur++ )
                *pCur = ' ';
        }
        else
            break;
        // go to the next line
        Line++;
    }
}

/**Function*************************************************************

  Synopsis    [Adds the number of values to the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadLinesAddNumValues( Io_Read_t * p, char * pNodeName, int nValues )
{
    int * pnValuesCur;
    assert( nValues > 2 );
    if ( p->tName2Values == NULL )
        p->tName2Values = st_init_table(strcmp, st_strhash);
    if ( st_find_or_add( p->tName2Values, pNodeName, (char***)&pnValuesCur ) )
    {
        if ( nValues != *pnValuesCur )
        {
            sprintf( p->sError, "Node \"%s\" has with different number of values in different places (%d and %d).", pNodeName, nValues, *pnValuesCur );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
    }
    else // set the number of values
        *pnValuesCur = nValues;
    return 0;
}


/**Function*************************************************************

  Synopsis    [Detects the file type using all the available info.]

  Description [Theoretically, this detector can be wrong if the 
  following three conditions are true:
  (1) the input file is BLIF-MV
  (2) it is a binary file without .mv and .default directives
  (3) the first table of the first node has an output literal composed 
  of several values, for example, {0,1}.
  I wonder if these three conditions will ever happen at the same time.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadLinesDetectFileType( Io_Read_t * p, Io_Node_t * pIoNode )
{
    // it is BLIF-MV if there are some obvious signs (.mv, .def, etc)
    if ( p->pDotMvs || p->nDotDefs )
        return IO_FILE_BLIF_MV;
    // it is BLIF if the node has no table (and there is no default lines)
    if ( pIoNode->nLines == 0 )
        return IO_FILE_BLIF;
    // it is BLIF-MVS if the last entry in the first line 
    // of the table has more than one character
    if ( strlen( Io_ReadLinesLastEntry( p->pLines[ pIoNode->LineTable ] ) ) > 1 )
        return IO_FILE_BLIF_MVS;
    return IO_FILE_BLIF;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


