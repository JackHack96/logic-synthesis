/**CFile****************************************************************

  FileName    [autReadAut.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioReadAut.c,v 1.1 2004/02/19 03:06:41 alanmi Exp $]

***********************************************************************/

#include "aioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct IoReadKissStruct Aut_ReadKiss_t;
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
    char *      pDotAcc;
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
    // storage for the names of the acc states
    st_table *  tAcc;
    // the covers of POs
//    Mvc_Cover_t ** ppOns;
//    Mvc_Cover_t ** ppDcs;
//    Mvc_Cover_t ** ppOffs;
    // the i-sets of the NS var
//    Mvc_Cover_t ** ppNexts;
    // the functionality manager
    Fnc_Manager_t * pMan;
    Mvc_Cover_t * pMvc;
    Aut_Man_t * pManDd;
};


static bool            Aut_ReadKissFindDotLines( Aut_ReadKiss_t * p ); 
static bool            Aut_ReadKissHeader( Aut_ReadKiss_t * p );
static Aut_Auto_t *    Aut_ReadKissAutBody( Aut_ReadKiss_t * p );
static void            Aut_ReadKissCleanUp( Aut_ReadKiss_t * p );
static Aut_Auto_t *    Aut_ReadKissFsmBody( Aut_ReadKiss_t * p );

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
Aut_Auto_t * Aio_AutoReadAut( Mv_Frame_t * pMvsis, char * FileName, Aut_Man_t * pManDd )
{
    Aut_Auto_t * pAut = NULL;
    Aut_ReadKiss_t * p;

    // allocate the structure
    p = ALLOC( Aut_ReadKiss_t, 1 );
    memset( p, 0, sizeof(Aut_ReadKiss_t) );
    // set the output file stream for errors
    p->pOutput = Mv_FrameReadErr(pMvsis);
    p->pMan = Mv_FrameReadMan(pMvsis);
    p->pManDd = pManDd;

    // read the file
    p->pBuffer = Io_ReadFileFileContents( FileName, &p->FileSize );
    // remove comments if any
    Io_ReadFileRemoveComments( p->pBuffer, NULL, NULL );

    // split the file into parts
    if ( !Aut_ReadKissFindDotLines( p ) )
        goto cleanup;

    // read the header of the file
    if ( !Aut_ReadKissHeader( p ) )
        goto cleanup;

    // read the body of the file
    if ( !(pAut = Aut_ReadKissAutBody( p )) )
        goto cleanup;
    Aut_AutoSetName( pAut, Extra_FileNameGeneric(FileName) );

    // construct the network
//    pAut = Aut_ReadKissAutConstruct( p );
//    Aut_AutoRelCreate( pAut );

cleanup:
    // free storage
    Aut_ReadKissCleanUp( p );
    return pAut;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
bool Aut_ReadKissFindDotLines( Aut_ReadKiss_t * p )
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
            else if ( pCur[1] == 'a' && pCur[2] == 'c' && pCur[3] == 'c' )
            {
                if ( p->pDotAcc )
                {
                    fprintf( p->pOutput, "The KISS file contains multiple .accepting lines.\n" );
                    return 0;
                }
                p->pDotAcc = pCur;
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
    if ( p->pDotAcc && pCurMax < p->pDotAcc )
        pCurMax = p->pDotAcc;
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
bool Aut_ReadKissHeader( Aut_ReadKiss_t * p )
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
    if ( p->nProducts < 0 || p->nProducts > 1000000 )
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

    // fill up the table with accepting states
    if ( p->pDotAcc )
    {
        // delimit this line with 0 at the end
        for ( pTemp = p->pDotAcc; pTemp && *pTemp != '\n'; pTemp++ );
        if ( pTemp == NULL )
        {
            fprintf( p->pOutput, "Something failed while reading the input file.\n" );
            return 0;
        }
        assert( *pTemp == '\n' );
        *pTemp = '\0';

        p->tAcc = st_init_table(strcmp, st_strhash);
        pTemp = strtok( p->pDotAcc, " \t\n\r" );
        assert( strncmp( pTemp, ".acc", 4 ) == 0 );
        pTemp = strtok( NULL, " \t\n\r" );
        while ( pTemp )
        {
            if ( st_insert( p->tAcc, pTemp, NULL ) )
            {
                fprintf( p->pOutput, "The state name \"%s\" occurs multiple times on the .accepting line.\n", pTemp );
                return 0;
            }
            pTemp = strtok( NULL, " \t\n\r" );
        }
        // overwrite 0 introduced by strtok()
//        *(pTemp + strlen(pTemp)) = ' ';
    }

    if ( p->nOutputs == 0 && p->pDotAcc == NULL )
    {
        fprintf( p->pOutput, "The file does not specify the accepting states (no .accepting line).\n" );
        fprintf( p->pOutput, "All the states of the automaton are assumed to be accepting.\n" );
    }

    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aut_ReadKissCleanUp( Aut_ReadKiss_t * p )
{
    if ( p->tAcc )
        st_free_table( p->tAcc );
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
Aut_Auto_t * Aut_ReadKissAutBody( Aut_ReadKiss_t * p )
{
    Aut_Auto_t * pAut;
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube;
    Aut_Trans_t * pTrans;
    Mvc_Cover_t * pMvc;
    Vmx_VarMap_t * pVmx;
    Vm_VarMap_t * pVm;
    char Buffer[100];
    char ** pParts[4];
    char * pName, * pTemp;
    int nDigitsIn;
    int i, iLine, nLines;
    Aut_State_t * pState, * pStateCs, * pStateNs;
    DdManager * dd;
    DdNode ** pbCodes;
    DdNode * bCond;
    Aut_Var_t ** pVars;
    Aut_State_t ** pInit;

/*
    if ( p->nOutputs != 0 )
    {
        fprintf( p->pOutput, "Aut_AutoReadKiss(): Can only read KISS files with automata.\n" );
        fprintf( p->pOutput, "In automata files, there are no outputs (\".o 0\").\n" );
        return 0;
    }
*/
    if ( p->nOutputs > 0 )
        return Aut_ReadKissFsmBody( p );



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


    // start the automaton
    pVmx = Vmx_VarMapCreateBinary( Fnc_ManagerReadManVm(p->pMan), 
        Fnc_ManagerReadManVmx(p->pMan), p->nInputs, p->nOutputs );

    pAut = Aut_AutoAlloc( pVmx, p->pManDd );
//    pAut->nInputs = p->nInputs;
//    pAut->nOutputs = p->nOutputs;
//    pAut->nStates = 0;
//    pAut->nStatesAlloc = p->nStates + 3;
//    pAut->pMan = p->pMan;
    // alloc storage for names
//    pAut->ppNamesInput  = ALLOC( char *, pAut->nInputs );
//    pAut->ppNamesOutput = NULL;


    // get the number of digits in the numbers
	for ( nDigitsIn = 0,  i = p->nInputs - 1;  i;  i /= 10,  nDigitsIn++ );

    // create the input/output names

    pVars = Aut_AutoReadVars( pAut );
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

        pVars[i] = Aut_VarAlloc();
        Aut_VarSetName( pVars[i], util_strsav(pName) );
        Aut_VarSetValueNum( pVars[i], 2 );
    }


    // create the states
//    pAut->pStates = ALLOC( Aut_State_t *, pAut->nStatesAlloc );
    for ( i = 0; i < p->nProducts; i++ )
    {
        if ( !Aut_AutoFindState( pAut, pParts[1][i] ) )
        {
//            pAut->pStates[pAut->nStates] = Aut_AutoStateAlloc();
            pState = Aut_StateAlloc( pAut );
//            pName = util_strsav(pParts[1][i]);
            pName = Aut_AutoRegisterName( pAut, pParts[1][i] );
//            pAut->pStates[pAut->nStates]->pName = pName;
            Aut_StateSetName( pState, pName );
//            st_add_direct( pAut->tName2State, pName, (char *)pState );
//            pAut->nStates++;
            Aut_AutoStateAddLast( pState );
        }
        // else, it is an old state, we already collected it
    }
    if ( Aut_AutoReadStateNum(pAut) != p->nStates )
    {
        // try collecting the states that do not have transitions from them
        for ( i = 0; i < p->nProducts; i++ )
        {
            if ( !Aut_AutoFindState( pAut, pParts[2][i] ) )
            {
//                pAut->pStates[pAut->nStates] = Aut_AutoStateAlloc();
                pState = Aut_StateAlloc( pAut );
//                pName = util_strsav(pParts[2][i]);
                pName = Aut_AutoRegisterName( pAut, pParts[2][i] );
//                pAut->pStates[pAut->nStates]->pName = pName;
                Aut_StateSetName( pState, pName );
//                st_add_direct( pAut->tName2State, pName, (char *)pState );
//                pAut->nStates++;
                Aut_AutoStateAddLast( pState );
            }
        }
    }
    assert( Aut_AutoReadStateNum(pAut) == p->nStates );

    // assign the initial state
    Aut_AutoSetInitNum( pAut, 1 );
    pInit = Aut_AutoReadInitAll( pAut );
    pInit[0] = Aut_AutoFindState( pAut, pParts[1][0] );


    // read the lines
    pCover = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(p->pMan), 2 * p->nInputs );
    for ( iLine = 0; iLine < p->nProducts; iLine++ )
    {
        // read the cube
        pTemp = pParts[0][iLine];
        pCube = Mvc_CubeAlloc( pCover );
        Mvc_CubeBitFill( pCube );
        if ( (int)strlen(pTemp) != p->nInputs )
        {
            fprintf( p->pOutput, "Cannot correctly scan the input part of the table.\n" );
            FREE( pParts[0] );
            FREE( pParts[1] );
            FREE( pParts[2] );
//            FREE( pParts[3] );
            return 0;
        }
        // read the cube
        for ( i = 0; i < p->nInputs; i++ )
            if ( pTemp[i] == '0' )
                Mvc_CubeBitRemove( pCube, i * 2 + 1 );
            else if ( pTemp[i] == '1' )
                Mvc_CubeBitRemove( pCube, i * 2 );


        // read the current state
//        if ( !st_lookup( pAut->tName2State, pParts[1][iLine], (char **)&pStateCs ) )
//        {
//            assert( 0 );
//        }
        pStateCs = Aut_AutoFindState( pAut, pParts[1][iLine] );
        assert( pStateCs );

        // read the next state
//        if ( !st_lookup( pAut->tName2State, pParts[2][iLine], (char **)&pStateNs ) )
//        {
//            assert( 0 );
//        }
        pStateNs = Aut_AutoFindState( pAut, pParts[2][iLine] );
        assert( pStateNs );

        // find the transtion with this CS and NS
        Aut_StateForEachTransitionFrom( pStateCs, pTrans )
            if ( Aut_TransReadStateTo(pTrans) == pStateNs )
                break;
        if ( pTrans == NULL )
        {
            // start a new transition
            pTrans = Aut_TransAlloc( pAut );
            Aut_TransSetStateFrom( pTrans, pStateCs );
            Aut_TransSetStateTo  ( pTrans, pStateNs );
            // create the cover with the cube
            pMvc = Mvc_CoverClone(pCover);
            Mvc_CoverAddCubeTail( pMvc, pCube );
            Aut_TransSetData( pTrans, (char *)pMvc );
            // add the transition
            Aut_AutoAddTransition( pTrans );
        }
        else
        {
            // add the cube
            pMvc = (Mvc_Cover_t *)Aut_TransReadData(pTrans);
            Mvc_CoverAddCubeTail( pMvc, pCube );
        }

        // make this state as accepting
//        pAut->pStates[cs]->fAcc = (pParts[3][iLine][0] == '1');
    }
    Mvc_CoverFree( pCover );

    // convert the covers to BDDs
    dd = Aut_AutoReadDd( pAut );
    pVm = Vmx_VarMapReadVm( pVmx );
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx );

    // enable reordering
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    Aut_AutoForEachState( pAut, pState )
    {
        Aut_StateForEachTransitionFrom( pState, pTrans )
        {
            pMvc = (Mvc_Cover_t *)Aut_TransReadData( pTrans );
            Aut_TransSetData( pTrans, NULL );
            bCond = Fnc_FunctionDeriveMddFromSop( dd, pVm, pMvc, pbCodes ); Cudd_Ref( bCond );
            Aut_TransSetCond( pTrans, bCond ); // takes ref
            Mvc_CoverFree( pMvc );
        }
    }
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );

    // reorder one last time
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1000 );
    // disable reordering
    Cudd_AutodynDisable( dd );


    // set the accepting states
    if ( p->pDotAcc == NULL )
    {
        // set all states to be accepting
        Aut_AutoForEachState( pAut, pState )
            Aut_StateSetAcc( pState, 1 );
    }
    else
    {
        // determine each accepting state
        Aut_AutoForEachState( pAut, pState )
            if ( st_is_member( p->tAcc, Aut_StateReadName(pState) ) )
                Aut_StateSetAcc( pState, 1 );
    }

    FREE( pParts[0] );
    FREE( pParts[1] );
    FREE( pParts[2] );
//    FREE( pParts[3] );
    return pAut;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Aut_ReadKissFsmBody( Aut_ReadKiss_t * p )
{
    Aut_Auto_t * pAut;
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube;
    Aut_Trans_t * pTrans;
    Mvc_Cover_t * pMvc;
    Vmx_VarMap_t * pVmx;
    Vm_VarMap_t * pVm;
    char Buffer[1000];
    char ** pParts[4];
    char * pName, * pTemp;
    int nDigitsIn, nDigitsOut;
    int i, iLine, nLines;
    Aut_State_t * pState, * pStateCs, * pStateNs;
    DdManager * dd;
    DdNode ** pbCodes;
    DdNode * bCond;
    Aut_Var_t ** pVars;
    Aut_State_t ** pInit;

    assert( p->nOutputs > 0 );

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


    // start the automaton
    pVmx = Vmx_VarMapCreateBinary( Fnc_ManagerReadManVm(p->pMan), 
        Fnc_ManagerReadManVmx(p->pMan), p->nInputs + p->nOutputs, 0 );

    pAut = Aut_AutoAlloc( pVmx, p->pManDd );


    // get the number of digits in the numbers
	for ( nDigitsIn = 0,  i = p->nInputs - 1;  i;  i /= 10,  nDigitsIn++ );
	for ( nDigitsOut = 0, i = p->nOutputs - 1; i;  i /= 10,  nDigitsOut++ );

    // create the input/output names

    pVars = Aut_AutoReadVars( pAut );
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

        pVars[i] = Aut_VarAlloc();
        Aut_VarSetName( pVars[i], util_strsav(pName) );
        Aut_VarSetValueNum( pVars[i], 2 );
    }
    for (  ; i < p->nInputs + p->nOutputs; i++ )
    {
        // get the input name
        if ( p->pDotIlb )
            pName = p->pNamesIn[i];
        else
        {
	        sprintf( Buffer, "z%0*d", nDigitsOut, i - p->nInputs );
            pName = Buffer;
        }

        pVars[i] = Aut_VarAlloc();
        Aut_VarSetName( pVars[i], util_strsav(pName) );
        Aut_VarSetValueNum( pVars[i], 2 );
    }


    // create the states
//    pAut->pStates = ALLOC( Aut_State_t *, pAut->nStatesAlloc );
    for ( i = 0; i < p->nProducts; i++ )
    {
        if ( !Aut_AutoFindState( pAut, pParts[1][i] ) )
        {
//            pAut->pStates[pAut->nStates] = Aut_AutoStateAlloc();
            pState = Aut_StateAlloc( pAut );
//            pName = util_strsav(pParts[1][i]);
            pName = Aut_AutoRegisterName( pAut, pParts[1][i] );
//            pAut->pStates[pAut->nStates]->pName = pName;
            Aut_StateSetName( pState, pName );
//            st_add_direct( pAut->tName2State, pName, (char *)pState );
//            pAut->nStates++;
            Aut_AutoStateAddLast( pState );
        }
        // else, it is an old state, we already collected it
    }
    if ( Aut_AutoReadStateNum(pAut) != p->nStates )
    {
        // try collecting the states that do not have transitions from them
        for ( i = 0; i < p->nProducts; i++ )
        {
            if ( !Aut_AutoFindState( pAut, pParts[2][i] ) )
            {
//                pAut->pStates[pAut->nStates] = Aut_AutoStateAlloc();
                pState = Aut_StateAlloc( pAut );
//                pName = util_strsav(pParts[2][i]);
                pName = Aut_AutoRegisterName( pAut, pParts[2][i] );
//                pAut->pStates[pAut->nStates]->pName = pName;
                Aut_StateSetName( pState, pName );
//                st_add_direct( pAut->tName2State, pName, (char *)pState );
//                pAut->nStates++;
                Aut_AutoStateAddLast( pState );
            }
        }
    }
    assert( Aut_AutoReadStateNum(pAut) == p->nStates );

    // assign the initial state
    Aut_AutoSetInitNum( pAut, 1 );
    pInit = Aut_AutoReadInitAll( pAut );
    pInit[0] = Aut_AutoFindState( pAut, pParts[1][0] );


    // read the lines
    pCover = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(p->pMan), 
        2 * (p->nInputs + p->nOutputs) );

    for ( iLine = 0; iLine < p->nProducts; iLine++ )
    {
        // read the cube
//        pTemp = pParts[0][iLine];
        sprintf( Buffer, "%s%s", pParts[0][iLine], pParts[3][iLine] );
        pTemp = Buffer;

        pCube = Mvc_CubeAlloc( pCover );
        Mvc_CubeBitFill( pCube );
        if ( (int)strlen(pTemp) != p->nInputs + p->nOutputs )
        {
            fprintf( p->pOutput, "Cannot correctly scan the input part of the table.\n" );
            FREE( pParts[0] );
            FREE( pParts[1] );
            FREE( pParts[2] );
            FREE( pParts[3] );
            return 0;
        }
        // read the cube
        for ( i = 0; i < p->nInputs + p->nOutputs; i++ )
            if ( pTemp[i] == '0' )
                Mvc_CubeBitRemove( pCube, i * 2 + 1 );
            else if ( pTemp[i] == '1' )
                Mvc_CubeBitRemove( pCube, i * 2 );


        // read the current state
//        if ( !st_lookup( pAut->tName2State, pParts[1][iLine], (char **)&pStateCs ) )
//        {
//            assert( 0 );
//        }
        pStateCs = Aut_AutoFindState( pAut, pParts[1][iLine] );
        assert( pStateCs );

        // read the next state
//        if ( !st_lookup( pAut->tName2State, pParts[2][iLine], (char **)&pStateNs ) )
//        {
//            assert( 0 );
//        }
        pStateNs = Aut_AutoFindState( pAut, pParts[2][iLine] );
        assert( pStateNs );

        // find the transtion with this CS and NS
        Aut_StateForEachTransitionFrom( pStateCs, pTrans )
            if ( Aut_TransReadStateTo(pTrans) == pStateNs )
                break;
        if ( pTrans == NULL )
        {
            // start a new transition
            pTrans = Aut_TransAlloc( pAut );
            Aut_TransSetStateFrom( pTrans, pStateCs );
            Aut_TransSetStateTo  ( pTrans, pStateNs );
            // create the cover with the cube
            pMvc = Mvc_CoverClone(pCover);
            Mvc_CoverAddCubeTail( pMvc, pCube );
            Aut_TransSetData( pTrans, (char *)pMvc );
            // add the transition
            Aut_AutoAddTransition( pTrans );
        }
        else
        {
            // add the cube
            pMvc = (Mvc_Cover_t *)Aut_TransReadData(pTrans);
            Mvc_CoverAddCubeTail( pMvc, pCube );
        }

        // make this state as accepting
//        pAut->pStates[cs]->fAcc = (pParts[3][iLine][0] == '1');
    }
    Mvc_CoverFree( pCover );

    // convert the covers to BDDs
    dd = Aut_AutoReadDd( pAut );
    pVm = Vmx_VarMapReadVm( pVmx );
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx );

    // enable reordering
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    Aut_AutoForEachState( pAut, pState )
    {
        Aut_StateForEachTransitionFrom( pState, pTrans )
        {
            pMvc = (Mvc_Cover_t *)Aut_TransReadData( pTrans );
            Aut_TransSetData( pTrans, NULL );
            bCond = Fnc_FunctionDeriveMddFromSop( dd, pVm, pMvc, pbCodes ); Cudd_Ref( bCond );
            Aut_TransSetCond( pTrans, bCond ); // takes ref
            Mvc_CoverFree( pMvc );
        }
    }
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );

    // reorder one last time
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1000 );
    // disable reordering
    Cudd_AutodynDisable( dd );


    // set the accepting states
    if ( p->pDotAcc == NULL )
    {
        // set all states to be accepting
        Aut_AutoForEachState( pAut, pState )
            Aut_StateSetAcc( pState, 1 );
    }
    else
    {
        // determine each accepting state
        Aut_AutoForEachState( pAut, pState )
            if ( st_is_member( p->tAcc, Aut_StateReadName(pState) ) )
                Aut_StateSetAcc( pState, 1 );
    }

    FREE( pParts[0] );
    FREE( pParts[1] );
    FREE( pParts[2] );
    FREE( pParts[3] );
    return pAut;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


