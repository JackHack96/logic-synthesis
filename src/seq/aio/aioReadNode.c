/**CFile****************************************************************

  FileName    [ioReadNode.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Deriving MV SOP for a node specified using BLIF/BLIF-MVS/BLIF-MV.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioReadNode.c,v 1.2 2005/06/02 03:34:19 alanmi Exp $]

***********************************************************************/

#include "aioInt.h"
#include "autiInt.h"
#include "vmInt.h"

/*
    The following restrictions on BLIF-MV file are assumed:
    (1) No subcircuits.
    (2) No "!" directive in the tables.
    (3) No multi-output nodes.
    These restrictions seem to be true for the available benchmarks.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// BLIF-MV
static int    Aio_ReadNodeBlifMvCover( Aio_Read_t * p, Mvc_Cover_t * pCover, Aut_Auto_t * pAut );
static int    Aio_ReadNodeBlifMvCoverCube( Aio_Read_t * p, Mvc_Cube_t * pCube, Aut_Auto_t * pAut, char ** ppName1, char ** ppName2 );
static int    Aio_ReadNodeBlifMvCoverCubeLiteral( Aio_Read_t * p, int Lit, char * pPhase, int * pEqual1, int * pEqual2, char * pString );
static int    Aio_ReadNodeBlifMvCoverCubeLiteralValue( Aio_Read_t * p, int Lit, char * pString );
static char * Aio_ReadNodeBlifMvCoverCubePart( char * pLine, char ** pPart );
static char * Aio_ReadNodeBlifMvSkipSpaces( char * pStr );
static char * Aio_ReadNodeBlifMvSkipNonSpaces( char * pStr );
static char * Aio_ReadNodeBlifMvScrollToBrace( char * pStr );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_AutoReadNextStateTable( Aio_Read_t * p, Aut_Auto_t * pAut )
{
    Mvc_Cover_t * pCover;
    int nValues, RetValue;
    DdManager * dd;
    DdNode ** pbCodes;
    Mvc_Cover_t * pMvc;
    DdNode * bCond;
    Vmx_VarMap_t * pVmx;
    Aut_State_t * pState, * pStateDc;
    Aut_Trans_t * pTrans;
    Aio_Node_t * pIoNode;

    p->pTokens = ALLOC( char *, 1000 );
    pVmx = Aut_AutoReadVmx(pAut);
    p->pVm = Vmx_VarMapReadVm( pVmx );
    nValues = Vm_VarMapReadValuesNum( p->pVm );
    pCover = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(Mv_FrameReadMan(p->pMvsis)), nValues );
    RetValue = Aio_ReadNodeBlifMvCover( p, pCover, pAut );
    if ( RetValue )
        return RetValue;
    
    // convert the covers to BDDs
    dd = Aut_AutoReadDd( pAut );
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx );

    // enable reordering
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    Aut_AutoForEachState( pAut, pState )
    {
        Aut_StateForEachTransitionFrom( pState, pTrans )
        {
            pMvc = (Mvc_Cover_t *)Aut_TransReadData( pTrans );
            Aut_TransSetData( pTrans, NULL );
            bCond = Fnc_FunctionDeriveMddFromSop( dd, p->pVm, pMvc, pbCodes ); Cudd_Ref( bCond );
            Aut_TransSetCond( pTrans, bCond ); // takes ref
            Mvc_CoverFree( pMvc );
        }
    }
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );

    // get the completion state
    pIoNode = p->pIoNodeN;
    if ( pIoNode->Default >= 0 )
    {
        // make sure the default state is not used
        pState = Aut_AutoFindState( pAut, pIoNode->pDefName );
        assert( pState );
//        assert( Aut_StateReadNumTransFrom( pState ) == 0 );
//        assert( Aut_StateReadNumTransTo( pState ) == 0 );
        // delete this state from the lists and the table
        Aut_AutoStateDelete( pState );
        // complete the automaton
        if ( Auti_AutoComplete( pAut, pAut->nVars, pState->fAcc ) )
        {
            // completion added the state
            pStateDc = Aut_AutoReadTail( pAut );
            // rename the DC state
            Aut_AutoStateRename( pAut, pStateDc->pName, pState->pName );
        }
        else
        {
            printf( "There are no transitions into the state %s.\n", pState->pName );
            printf( "This state was deleted from the automaton.\n", pState->pName );
        }
        // remove the state
        Aut_StateFree( pState );
    }

    // reorder one last time
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1000 );
    // disable reordering
    Cudd_AutodynDisable( dd );

    Mvc_CoverFree( pCover );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_ReadNodeBlifMvCover( Aio_Read_t * p, Mvc_Cover_t * pCover, Aut_Auto_t * pAut )
{
    Aio_Node_t * pIoNode;
    Vm_VarMap_t * pVm; 
    int Default;
    int LineCounter, k;
    Aut_Trans_t * pTrans;
    Aut_State_t * pStateCs, * pStateNs;
    Mvc_Cube_t * pCubeDup;
    char * pName1, * pName2;
    Mvc_Cover_t * pMvc;
    int RetValue = 0;

    // initialize important pointers
    pIoNode = p->pIoNodeN;
    pVm    = p->pVm;

    // get the number of values and the default
    Default = pIoNode->Default;

    // adjust the temporary cube
    p->pCube = Mvc_CubeAlloc( pCover );

    if ( pIoNode->nLines )
    {
        // go through the cubes
        LineCounter = 0;
        for ( k = pIoNode->LineTable; p->pLines[k][0] != '.'; k++ )
            if ( p->pLines[k][0] != '\0' )
            {
                // get the line number
                p->LineCur = k;
                // parse the cube
                if ( Aio_ReadNodeBlifMvCoverCube( p, p->pCube, pAut, &pName1, &pName2 ) )
                {
                    RetValue = 1;
                    goto QUITS;
                }

                pStateCs = Aut_AutoFindState( pAut, pName1 );
                if ( pStateCs == NULL )
                {
                    p->LineCur = pIoNode->LineNames;
                    sprintf( p->sError, "Cannot find the current state %s from this line.", pName1 );
                    Aio_ReadPrintErrorMessage( p );
                    RetValue = 1;
                    goto QUITS;
                }
                pStateNs = Aut_AutoFindState( pAut, pName2 );
                if ( pStateNs == NULL )
                {
                    p->LineCur = pIoNode->LineNames;
                    sprintf( p->sError, "Cannot find the current state %s from this line.", pName2 );
                    Aio_ReadPrintErrorMessage( p );
                    RetValue = 1;
                    goto QUITS;
                }

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
                    pCubeDup = Mvc_CubeDup( pCover, p->pCube );

                    pMvc = Mvc_CoverClone(pCover);
                    Mvc_CoverAddCubeTail( pMvc, pCubeDup );
                    Aut_TransSetData( pTrans, (char *)pMvc );
                    // add the transition
                    Aut_AutoAddTransition( pTrans );
                }
                else
                {
                    // add the cube
                    pCubeDup = Mvc_CubeDup( pCover, p->pCube );

                    pMvc = (Mvc_Cover_t *)Aut_TransReadData(pTrans);
                    Mvc_CoverAddCubeTail( pMvc, pCubeDup );
                }

                // count the number of lines
                LineCounter++;
            }
        // make sure the correct number of lines was read
        assert( LineCounter == pIoNode->nLines );
    }
    else
    {
        p->LineCur = pIoNode->LineNames;
        sprintf( p->sError, "Warning: The next state table is empty." );
        Aio_ReadPrintErrorMessage( p );
//        return 1;
    }

QUITS:
    Mvc_CubeFree( pCover, p->pCube );
    p->pCube = NULL;
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Reads one line of BLIF-MV table.]

  Description [This function reads one line (Line) stored in the reading
  manager (Aio_Read_t). The variable map in the node shows the expected values
  of the fanins. The result goes in Espresso cube (Set). The string pPhase is
  set to point to the output literal. The two last variables (pEqual1, pEqual2), 
  if they are not -1, are the 0-based numbers of the (fanin or output) variables
  which are connected by the equality relation (=construct).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_ReadNodeBlifMvCoverCube( Aio_Read_t * p, Mvc_Cube_t * pCube, Aut_Auto_t * pAut, 
    char ** ppName1, char ** ppName2 )
{
    Aio_Node_t * pIoNode;
    Vm_VarMap_t * pVm; 
    char * pLine, * pPart;
    int iValue, v, i;
    int nValuesIn, nVars;
    int Equal1, Equal2;
    int * pEqual1 = &Equal1;
    int * pEqual2 = &Equal2;
    char * pPhase;

    // initialize important pointers
    pIoNode = p->pIoNodeN;
    pVm    = p->pVm;
    nValuesIn = Vm_VarMapReadValuesInNum(pVm);
    nVars     = Vm_VarMapReadVarsNum(pVm);

    pPhase  = (char *)pVm->pMan->pArray1;

    // get the line 
    pLine = p->pLines[p->LineCur];
    // clean the cube
    Mvc_CubeBitFill( pCube );

    // clearn Equals
    *pEqual1 = -1;
    *pEqual2 = -1;

    // get the literals
    iValue = 0;
    for ( v = 0; v < pVm->nVarsIn + pVm->nVarsOut; v++ )
    {
        // add to the total number of values
        iValue += pVm->pValues[v];

        // get the next part
        pLine = Aio_ReadNodeBlifMvCoverCubePart( pLine, &pPart );
        if ( pPart == NULL )
        {
            sprintf( p->sError, "This line cannot be properly parsed." );
            Aio_ReadPrintErrorMessage( p );
            return 1;
        }

        // set the value set to empty
        for ( i = 0; i < pVm->pValues[v]; i++ )
            pPhase[i] = '-';
        pPhase[pVm->pValues[v]] = 0;

        // read the values of this literal
        if ( Aio_ReadNodeBlifMvCoverCubeLiteral( p, v, pPhase, pEqual1, pEqual2, pPart ) )
            return 1;

        // if it is the output variable, stop (the phase is already assigned)
//        if ( v == pVm->nVarsIn )
//            break;

        // if it is =construct, do not change the cube
        if ( *pEqual1 != -1 )
            continue;
        
        // set the values of this literal in the cube
        for ( i = 0; i < pVm->pValues[v]; i++ )
            if ( pPhase[i] == '-' )
                Mvc_CubeBitRemove( pCube, pVm->pValuesFirst[v] + i );
    }
//printf( "\n" );
    assert( iValue == Vm_VarMapReadValuesNum(pVm) );

    // read the two states
    pLine = Aio_ReadNodeBlifMvCoverCubePart( pLine, &pPart );
    if ( pPart == NULL )
    {
        sprintf( p->sError, "This line cannot be properly parsed." );
        Aio_ReadPrintErrorMessage( p );
        return 1;
    }
    *ppName1 = pPart;

    pLine = Aio_ReadNodeBlifMvCoverCubePart( pLine, &pPart );
    if ( pPart == NULL )
    {
        sprintf( p->sError, "This line cannot be properly parsed." );
        Aio_ReadPrintErrorMessage( p );
        return 1;
    }
    *ppName2 = pPart;

    return 0;
}


/**Function*************************************************************

  Synopsis    [Reads the next part of the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aio_ReadNodeBlifMvCoverCubePart( char * pLine, char ** pPart )
{
    char * pCur;

    // skip the spaces at the beginning
    pCur = Aio_ReadNodeBlifMvSkipSpaces( pLine );
    if ( *pCur == 0 )
        return NULL;

    // assign the part, which begins here
    *pPart = pCur;

    // test what is the first symbol in this line
    if ( *pCur == '-' )
        pCur++;
    else if ( *pCur == '=' )
    {
        pCur = Aio_ReadNodeBlifMvSkipSpaces( pCur + 1 );
        if ( pCur == NULL )
            return NULL;
        pCur = Aio_ReadNodeBlifMvSkipNonSpaces( pCur );
        if ( pCur == NULL )
            return NULL;
    }
    else if ( *pCur == '(' || *pCur == '{' )
    {
        pCur = Aio_ReadNodeBlifMvScrollToBrace( pCur );
        if ( pCur == NULL )
            return NULL;
        assert( *pCur == ')' || *pCur == '}' );
        pCur++;
    }
    else  // symbolic name or value number
    {
        pCur = Aio_ReadNodeBlifMvSkipNonSpaces( pCur );
        if ( pCur == NULL )
            return NULL;
    }
    assert( *pCur == ' ' || *pCur == '\0' );
    *pCur  = '\0';
    return pCur + 1;
}


/**Function*************************************************************

  Synopsis    [Skips all the space chars in the string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aio_ReadNodeBlifMvSkipSpaces( char * pStr )
{
    while ( *pStr && (*pStr == ' ' || *pStr == '\t') )
        pStr++;
    return pStr;
}

/**Function*************************************************************

  Synopsis    [Skips all the non-space chars in the string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aio_ReadNodeBlifMvSkipNonSpaces( char * pStr )
{
    while ( *pStr && *pStr != ' ' && *pStr != '\t' )
        pStr++;
    return pStr;
}


/**Function*************************************************************

  Synopsis    [Scrolls to the corresponding closing brace.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Aio_ReadNodeBlifMvScrollToBrace( char * pStr )
{
    int BraceRound = 0, BraceCurly = 0;
    assert( *pStr == '(' || *pStr == '{' );
    while ( *pStr )
    {
        if ( *pStr == '(' )
            BraceRound++;
        else if ( *pStr == ')' )
            BraceRound--;
        else if ( *pStr == '{' )
            BraceCurly++;
        else if ( *pStr == '}' )
            BraceCurly--;
        if ( BraceRound == 0 && BraceCurly == 0 )
            return pStr;
        pStr++;
    }
    return NULL;
}



/**Function*************************************************************

  Synopsis    [Reads the MV literal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_ReadNodeBlifMvCoverCubeLiteral( Aio_Read_t * p, int Lit, char * pPhase, int * pEqual1, int * pEqual2, char * pString  )
{
    Vm_VarMap_t * pVm;
    int i, Value, ValueStart, ValueStop;
    char * pTemp;

    // initialize the important pointers
    pVm    = p->pVm;

    if ( *pString == '=' )
    {
        sprintf( p->sError, "The =construct cannot be used in the MV automata files." );
        Aio_ReadPrintErrorMessage( p );
        return 1;
    }

    if ( *pString == '-' )
    {
        for ( i = 0; i < pVm->pValues[Lit]; i++ )
            pPhase[i] = '1';
        return 0;
    }

    // consider three cases
    // this is an interval of values
    if ( *pString == '{' )
    {
        pTemp = strtok( pString + 1, " -}\t" );
        ValueStart = Aio_ReadNodeBlifMvCoverCubeLiteralValue( p, Lit, pTemp );
        if ( ValueStart == -1 )
            return 1;
        pTemp = strtok( NULL, " -}\t" );
        ValueStop = Aio_ReadNodeBlifMvCoverCubeLiteralValue( p, Lit, pTemp );
        if ( ValueStop == -1 )
            return 1;

        for ( i = ValueStart; i <= ValueStop; i++ )
            pPhase[i] = '1';
        return 0;
    }
    // this is a set of values
    if ( *pString == '(' )
    {
        int nTokens = 0, i;
        // split the line into tokens
        pTemp = strtok( pString + 1, ",)" );
        do p->pTokens[nTokens++] = pTemp;
        while ( pTemp = strtok( NULL, ",)" ) );
        // call parsing of each token
        for ( i = 0; i < nTokens; i++ )
            if ( Aio_ReadNodeBlifMvCoverCubeLiteral( p, Lit, pPhase, pEqual1, pEqual2, p->pTokens[i] ) )
                return 1;
        return 0;
    }
    // this is a value
    Value = Aio_ReadNodeBlifMvCoverCubeLiteralValue( p, Lit, pString );
    if ( Value == -1 )
        return 1;
    pPhase[Value] = '1';
    return 0;
}

/**Function*************************************************************

  Synopsis    [Reads the value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_ReadNodeBlifMvCoverCubeLiteralValue( Aio_Read_t * p, int Lit, char * pString )
{
    Aut_Auto_t * pAut = p->pAut;
    Aut_Var_t * pVar;
    int nVars;
    char ** pNameValues;
    int nValues;

    Vm_VarMap_t * pVm;
    Aio_Node_t * pIoNode;
    int Value, i;


    // initialize the important pointers
    pIoNode = p->pIoNodeN;
    pVm    = p->pVm;

    // get the pointer to the fanin var if present
    nVars = Aut_AutoReadVarNum(pAut);
    assert( Lit < nVars );
    pVar = Aut_AutoReadVars(pAut)[Lit];

    pNameValues = Aut_VarReadValueNames(pVar);
    nValues = Aut_VarReadValueNum(pVar);

    if ( p->tName2Var && pNameValues )
    {
        // check if any of the symbolic value names of this var apply
        for ( i = 0; i < nValues; i++ )
            if ( strcmp( pNameValues[i], pString ) == 0 )
                return i;
        if ( i == nValues )
        {
            sprintf( p->sError, "No symbolic value for \"%s\".", pString );
            Aio_ReadPrintErrorMessage( p );
            return -1;
        }
    }
    // try to read it as a number
    if ( pString[0] < '0' || pString[0] > '9' )
    {
        sprintf( p->sError, "Cannot read value (%s).", pString );
        Aio_ReadPrintErrorMessage( p );
        return -1;
    }
    Value = atoi( pString );
    // see if the value applies
    if ( Value >= pVm->pValues[Lit] )
    {
        sprintf( p->sError, "The value (%s) is out of range [0; %d].", pString, pVm->pValues[Lit]-1 );
        Aio_ReadPrintErrorMessage( p );
        return -1;
    }
    return Value;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


