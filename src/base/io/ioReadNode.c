/**CFile****************************************************************

  FileName    [ioReadNode.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Deriving MV SOP for a node specified using BLIF/BLIF-MVS/BLIF-MV.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ioReadNode.c,v 1.22 2003/11/18 18:54:52 alanmi Exp $]

***********************************************************************/

#include "ioInt.h"
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

// BLIF
static int    Io_ReadNodeBlifCover( Io_Read_t * p, Mvc_Cover_t * pCovers[] );
static int    Io_ReadNodeBlifCoverCube( Io_Read_t * p, Mvc_Cube_t * pCube, int * pPhase );
// BLIF-MVS
static int    Io_ReadNodeBlifMvsCover( Io_Read_t * p, Mvc_Cover_t * pCovers[] );
static int    Io_ReadNodeBlifMvsCoverCube( Io_Read_t * p, Mvc_Cube_t * pCube, char ** pPhase );
// BLIF-MV
static int    Io_ReadNodeBlifMvCover( Io_Read_t * p, Mvc_Cover_t * pCovers[] );
static int    Io_ReadNodeBlifMvCoverCube( Io_Read_t * p, Mvc_Cube_t * pCube, char * pPhase, int * pEqual1, int * pEqual2 );
static int    Io_ReadNodeBlifMvCoverCubeLiteral( Io_Read_t * p, int Lit, char * pPhase, int * pEqual1, int * pEqual2, char * pString );
static int    Io_ReadNodeBlifMvCoverCubeLiteralValue( Io_Read_t * p, int Lit, char * pString );
static char * Io_ReadNodeBlifMvCoverCubePart( char * pLine, char ** pPart );
static char * Io_ReadNodeBlifMvSkipSpaces( char * pStr );
static char * Io_ReadNodeBlifMvSkipNonSpaces( char * pStr );
static char * Io_ReadNodeBlifMvScrollToBrace( char * pStr );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the functionality of the node.]

  Description [Derives the functionality of the node. First, creates
  the variable map using the information about the number of values
  of the fanins. Second, derives the MV SOP representation. Third,
  derives the relation from the MV SOP representation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNodeFunctions( Io_Read_t * p, Ntk_Node_t * pNode )
{
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** pCovers;
    int RetValue;
    int i, nValues;

    // make sure that the node belongs to the network
    assert( Ntk_NodeReadNetwork(pNode) );
    assert( Fnc_FunctionReadMvr( Ntk_NodeReadFunc(pNode) ) == NULL );
    assert( Ntk_NodeIsInternal(pNode) );

    // set the current node
    p->pNodeCur = pNode;

    // create Cvr and get hold of the covers
    nValues = Ntk_NodeReadValueNum(pNode);
    pCovers = ALLOC( Mvc_Cover_t *, nValues );
    memset( pCovers, 0, sizeof(Mvc_Cover_t *) * nValues );

    // set the 0 covers for the node without functionality
    if ( Ntk_NodeReadData(pNode) == NULL )
    {
        for ( i = 1; i < nValues; i++ )
            pCovers[i] = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(Mv_FrameReadMan(p->pMvsis)), 0 );
    }
    else
    {
        // derive the MV SOP representation
        if ( p->Type == IO_FILE_BLIF )
            RetValue = Io_ReadNodeBlifCover( p, pCovers );
        else if ( p->Type == IO_FILE_BLIF_MVS )
            RetValue = Io_ReadNodeBlifMvsCover( p, pCovers );
        else if ( p->Type == IO_FILE_BLIF_MV )
            RetValue = Io_ReadNodeBlifMvCover( p, pCovers );
        if ( RetValue == 1 )
        {
            FREE( pCovers );
            return 1;
        }
    }
    
    // create the covers
    pCvr = Cvr_CoverCreate( Fnc_FunctionReadVm( Ntk_NodeReadFunc(pNode) ), pCovers );
    Ntk_NodeSetFuncCvr( pNode, pCvr );
    return 0;
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNodeBlifCover( Io_Read_t * p, Mvc_Cover_t * pCovers[] )
{
    Ntk_Node_t * pNode;
    Io_Node_t * pIoNode;
    Vm_VarMap_t * pVm;
    Mvc_Cover_t * pCover;
    int LineCounter, k;
    int Phase, PhasePrev;

    // initialize important pointers
    pNode   = p->pNodeCur;
    pIoNode = (Io_Node_t *)Ntk_NodeReadData(pNode);
    pVm     = Fnc_FunctionReadVm( Ntk_NodeReadFunc(pNode) );

    // start the cover
    pCover = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(Mv_FrameReadMan(p->pMvsis)), pVm->nValuesIn );
    // adjust the temporary cube
    p->pCube->iLast   = pCover->nWords - 1 + (pCover->nWords == 0);
    p->pCube->nUnused = pCover->nUnused;


    if ( pIoNode->nLines )
    {
        // go through the cubes
        LineCounter = 0;
        PhasePrev   = -1;
        for ( k = pIoNode->LineTable; p->pLines[k][0] != '.'; k++ )
            if ( p->pLines[k][0] != '\0' )
            {
                // get the line number
                p->LineCur = k;
                // parse the cube
                if ( Io_ReadNodeBlifCoverCube( p, p->pCube, &Phase ) )
                {
                    Mvc_CoverFree( pCover );
                    return 1;
                }
                // add the cube
                Mvc_CoverAddDupCubeTail( pCover, p->pCube );


                // save the phase
                if ( PhasePrev == -1 )
                    PhasePrev = Phase;
                else if ( PhasePrev != Phase )
                {
                    Mvc_CoverFree( pCover );
                    sprintf( p->sError, "The phases of the cubes are out of synch." );
                    Io_ReadPrintErrorMessage( p );
                    return 1;
                }
                LineCounter++;
            }
        // make sure we read the correct number of lines
        assert( LineCounter == pIoNode->nLines );
    }
    else
    {
//        pCover->count = 1; // set one cube (empty cube) 
        Mvc_CoverMakeEmpty( pCover );
        Phase = 1;        // set the positive phase
    }

    // set the resulting cover
    if ( Phase == 0 )
    {
        pIoNode->Default = 1;
        pCovers[0] = pCover;
    }
    else
    {
        pIoNode->Default = 0;
        pCovers[1] = pCover;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNodeBlifCoverCube( Io_Read_t * p, Mvc_Cube_t * pCube, int * pPhase )
{
    Ntk_Node_t * pNode;
    Io_Node_t * pIoNode;
    Vm_VarMap_t * pVm; 
    char * pLine, * pTemp;
    int Length, i;

    // initialize important pointers
    pNode   = p->pNodeCur;
    pIoNode = (Io_Node_t *)Ntk_NodeReadData(pNode);
    pVm    = Fnc_FunctionReadVm( Ntk_NodeReadFunc(pNode) );

    // get the line 
    pLine = p->pLines[p->LineCur];

    // get the input part of the cube
    pTemp = strtok( pLine, " \t" );
    // get its length
    Length = strlen(pTemp);

    // if more than one variable in the map, this is the input part of the cube
    if ( pVm->nVarsIn != 0 )
    {
        assert( pVm->nVarsOut == 1 );
        // check the length of the input cube
        if ( Length != pVm->nVarsIn )
        {
            sprintf( p->sError, "The input cube size (%d) is not equal to the fanin count (%d).", 
                Length, pVm->nVarsIn );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }

        // clean the cube
        Mvc_CubeBitFill( pCube );

        // remove values from the cube, based on the input part of the cube
        for ( i = 0; i < Length; i++ )
            if ( pTemp[i] == '0' )
                Mvc_CubeBitRemove( pCube, 2*i + 1 );
            else if ( pTemp[i] == '1' )
                Mvc_CubeBitRemove( pCube, 2*i );
            else if ( pTemp[i] != '-' )
            {
                sprintf( p->sError, "Strange char (%c) in the input cube (%s).", pTemp[i], pTemp );
                Io_ReadPrintErrorMessage( p );
                return 1;
            }

        // get the output part of the cube (phase)
        pTemp = strtok( NULL, " \t" );
        // get its length
        Length = strlen(pTemp);
    }

    // check the length
    if ( Length != 1 )
    {
        sprintf( p->sError, "The size of the output part of the cube (%s) is not equal to 1.", pTemp );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }

    // otherwise, this is the phase
    if ( pTemp[0] == '1' )
        *pPhase = 1;
    else if ( pTemp[0] == '0' )
        *pPhase = 0;
    else 
    {
        sprintf( p->sError, "Strange char (%c) in the output part of the cube (%s).", pTemp[0], pTemp );
        Io_ReadPrintErrorMessage( p );
        return 1;
    }
    return 0;
}






/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNodeBlifMvsCover( Io_Read_t * p, Mvc_Cover_t * pCovers[] )
{
    Ntk_Node_t * pNode;
    Io_Node_t * pIoNode;
    Mvc_Cover_t * pCover;
    Vm_VarMap_t * pVm; 
    int LineCounter;
    char * pPhase;
    int i, k;
    int fFirstTime;
    int nValues;

    // initialize important pointers
    pNode   = p->pNodeCur;
    pIoNode = (Io_Node_t *)Ntk_NodeReadData(pNode);
    pVm    = Fnc_FunctionReadVm( Ntk_NodeReadFunc(pNode) );
    nValues = Ntk_NodeReadValueNum(pNode);

    // go through the cubes
    fFirstTime  = 1;
    LineCounter = 0;
    for ( k = pIoNode->LineTable; p->pLines[k][0] != '.'; k++ )
        if ( p->pLines[k][0] != '\0' )
        {
            // get the line number
            p->LineCur = k;
            // parse the cube
            if ( Io_ReadNodeBlifMvsCoverCube( p, p->pCube, &pPhase ) )
            {
                for ( i = 0; i < nValues; i++ )
                    if ( pCovers[i] )
                    {
                        Mvc_CoverFree( pCovers[i] );
                        pCovers[i] = NULL;
                    }
                return 1;
            }
            // allocate the covers
            if ( fFirstTime )
            {
                fFirstTime = 0;
                assert( strlen(pPhase) == (unsigned)nValues );
                for ( i = 0; i < nValues; i++ )
                    if ( pPhase[i] == 'D' )
                        pCovers[i] = NULL;
                    else
                    {
                        pCovers[i] = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(Mv_FrameReadMan(p->pMvsis)), pVm->nValuesIn );
                        pCover = pCovers[i];
                    }
                // adjust the temporary cube
                p->pCube->iLast   = pCover->nWords - 1 + (pCover->nWords == 0);
                p->pCube->nUnused = pCover->nUnused;
            }
            // add the cube
            for ( i = 0; i < nValues; i++ )
                if ( pCovers[i] && pPhase[i] != '-' )
                    Mvc_CoverAddDupCubeTail( pCovers[i], p->pCube );

            // count the parsed line
            LineCounter++;
        }
    // make sure the correct number of lines was read
    assert( LineCounter == pIoNode->nLines );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNodeBlifMvsCoverCube( Io_Read_t * p, Mvc_Cube_t * pCube, char ** pPhase )
{
    Ntk_Node_t * pNode;
    Io_Node_t * pIoNode;
    Vm_VarMap_t * pVm; 
    char * pLine, * pTemp;
    int Length, v, i, iValue;
    int nVars, nValuesIn;

    // initialize important pointers
    pNode     = p->pNodeCur;
    pIoNode   = (Io_Node_t *)Ntk_NodeReadData(pNode);
    pVm       = Ntk_NodeReadFuncVm(pNode);
    nValuesIn = Vm_VarMapReadValuesInNum(pVm);
    nVars     = Vm_VarMapReadVarsNum(pVm);

    // get the line 
    pLine = p->pLines[p->LineCur];
    // clean the cube
    Mvc_CubeBitFill( pCube );

    // get the next literal
    pTemp = strtok( pLine, " \t" );
    Length = strlen(pTemp);

    // read the literals
    iValue = 0;
    assert( pVm->nVarsOut == 1 );
    for ( v = 0; v < nVars; v++ )
    {
        // add to the total number of values
        iValue += pVm->pValues[v];
        if ( pTemp == NULL )
        {
            sprintf( p->sError, "The number of literals (%d) is less than the number of variables (%d).", 
                v, nVars );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
        // compare the literal length
        if ( Length != pVm->pValues[v] )
        {
            sprintf( p->sError, "Literal %d has size %d, not equal to the number of values (%d).",
                v, Length, pVm->pValues[v] );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
        // if it is the output variable, assign the phase and stop
        if ( v == nVars - 1 )
        {
            *pPhase = pTemp;
            break;
        }
        // set the values of this literal in the cube
        for ( i = 0; i < Length; i++ )
            if ( pTemp[i] == '-' )
                Mvc_CubeBitRemove( pCube, pVm->pValuesFirst[v] + i );

        // get the next literal
        pTemp = strtok( NULL, " \t" );
        Length = strlen(pTemp);
    }
    assert( iValue == Vm_VarMapReadValuesNum(pVm) );
    return 0;
}






/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNodeBlifMvCover( Io_Read_t * p, Mvc_Cover_t * pCovers[] )
{
    Ntk_Node_t * pNode;
    Io_Node_t * pIoNode;
    Mvc_Cover_t * pCover;
    Vm_VarMap_t * pVm; 
    char * pPhase;
    int Equal1, Equal2;
    int nValuesOut, Default;
    int LineCounter, i, v, k;

    // initialize important pointers
    pNode   = p->pNodeCur;
    pIoNode = (Io_Node_t *)Ntk_NodeReadData(pNode);
    pVm    = Fnc_FunctionReadVm( Ntk_NodeReadFunc(pNode) );
    pPhase  = (char *)pVm->pMan->pArray1;

    // get the number of values and the default
    nValuesOut = Ntk_NodeReadValueNum(pNode);
    Default = pIoNode->Default;
    assert( nValuesOut == Vm_VarMapReadValuesOutput(pVm) );

    // allocate the covers 
    for ( i = 0; i < nValuesOut; i++ )
        if ( i == Default )
            pCovers[i] = NULL;
        else
        {
            pCovers[i] = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(Mv_FrameReadMan(p->pMvsis)), pVm->nValuesIn );
            pCover = pCovers[i];
        }
    // adjust the temporary cube
    p->pCube->iLast   = pCover->nWords - 1 + (pCover->nWords == 0);
    p->pCube->nUnused = pCover->nUnused;

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
                if ( Io_ReadNodeBlifMvCoverCube( p, p->pCube, pPhase, &Equal1, &Equal2 ) )
                {
                    for ( i = 0; i < nValuesOut; i++ )
                        if ( pCovers[i] )
                        {
                            Mvc_CoverFree( pCovers[i] );
                            pCovers[i] = NULL;
                        }
                    return 1;
                }
                // add the cube
                if ( Equal1 == -1 )
                { // there is no =construct
                    // add the cube to the i-sets
                    for ( i = 0; i < nValuesOut; i++ )
                        if ( pCovers[i] && pPhase[i] != '-' )
                            Mvc_CoverAddDupCubeTail( pCovers[i], p->pCube );

                    LineCounter++;
                    continue;
                }

                // add as many cubes as there are values of the given fanin node
                // the values corresponding to this fanin are not currently set in the cube
                assert( Equal1 >= 0 && Equal1 <= pVm->nVarsIn );
                assert( Equal2 >= 0 && Equal2 <= pVm->nVarsIn );
                assert( pVm->pValues[Equal1] == pVm->pValues[Equal2] );
                assert( Equal1 < Equal2 );
                
                // remember the values saved in the literal (Equal1) of the cube
                for ( v = 0; v < pVm->pValues[Equal1]; v++ )
                    if ( Mvc_CubeBitValue( p->pCube, pVm->pValuesFirst[Equal1] + v ) )
                        pPhase[v] = '1';
                    else
                        pPhase[v] = '-';
                pPhase[v] = 0;

                if ( Equal2 < pVm->nVarsIn )
                { // the equality does not involve the output

                    // remove all values belonging to the variables
                    for ( v = 0; v < pVm->pValues[Equal1]; v++ )
                        Mvc_CubeBitRemove( p->pCube, pVm->pValuesFirst[Equal1] + v );
                    for ( v = 0; v < pVm->pValues[Equal2]; v++ )
                        Mvc_CubeBitRemove( p->pCube, pVm->pValuesFirst[Equal2] + v );

                    for ( v = 0; v < pVm->pValues[Equal1]; v++ )
                        if ( pPhase[v] == '1' )
                        { 
                            // add a new value
                            Mvc_CubeBitInsert( p->pCube, pVm->pValuesFirst[Equal1] + v );
                            Mvc_CubeBitInsert( p->pCube, pVm->pValuesFirst[Equal2] + v );
                            // add the cube to all the related i-sets
                            for ( i = 0; i < nValuesOut; i++ )
                                if ( pCovers[i] && pPhase[i] != '-' )
                                    Mvc_CoverAddDupCubeTail( pCovers[i], p->pCube );
                            // remove previously added value to the var and the output
                            Mvc_CubeBitRemove( p->pCube, pVm->pValuesFirst[Equal1] + v );
                            Mvc_CubeBitRemove( p->pCube, pVm->pValuesFirst[Equal2] + v );
                        }
                }
                else // the output is involved
                {
                    // remove all values belonging to the first variable
                    for ( v = 0; v < pVm->pValues[Equal1]; v++ )
                        Mvc_CubeBitRemove( p->pCube, pVm->pValuesFirst[Equal1] + v );

                    for ( v = 0; v < pVm->pValues[Equal1]; v++ )
                        if ( pPhase[v] == '1' )
                        {
                            // add a new value
                            Mvc_CubeBitInsert( p->pCube, pVm->pValuesFirst[Equal1] + v );
                            // add the cube to the corresponding i-set
                            if ( pCovers[v] )
                                 Mvc_CoverAddDupCubeTail( pCovers[v], p->pCube );
                            // remove previously added value to the var and the output
                            Mvc_CubeBitRemove( p->pCube, pVm->pValuesFirst[Equal1] + v );
                       }
                }
                // count the number of lines
                LineCounter++;
            }
        // make sure the correct number of lines was read
        assert( LineCounter == pIoNode->nLines );
    }
    else
    {
        if ( Default < 0 )
        {
            p->LineCur = pIoNode->LineNames;
            sprintf( p->sError, "The node has no default value and no cubes." );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Reads one line of BLIF-MV table.]

  Description [This function reads one line (Line) stored in the reading
  manager (Io_Read_t). The variable map in the node shows the expected values
  of the fanins. The result goes in Espresso cube (Set). The string pPhase is
  set to point to the output literal. The two last variables (pEqual1, pEqual2), 
  if they are not -1, are the 0-based numbers of the (fanin or output) variables
  which are connected by the equality relation (=construct).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_ReadNodeBlifMvCoverCube( Io_Read_t * p, Mvc_Cube_t * pCube, char * pPhase, int * pEqual1, int * pEqual2 )
{
    Ntk_Node_t * pNode;
    Io_Node_t * pIoNode;
    Vm_VarMap_t * pVm; 
    char * pLine, * pPart;
    int nValues, iValue, v, i;
    int nValuesIn, nVars;

    // initialize important pointers
    pNode     = p->pNodeCur;
    pIoNode   = (Io_Node_t *)Ntk_NodeReadData(pNode);
    pVm       = Ntk_NodeReadFuncVm(pNode);
    nValuesIn = Vm_VarMapReadValuesInNum(pVm);
    nVars     = Vm_VarMapReadVarsNum(pVm);

    // get the line 
    pLine = p->pLines[p->LineCur];
    // clean the cube
    Mvc_CubeBitFill( pCube );

    // get the number of output values
    nValues = Ntk_NodeReadValueNum(pNode);
    assert( nValues == Vm_VarMapReadValuesOutput(pVm) );

    // clearn Equals
    *pEqual1 = -1;
    *pEqual2 = -1;

    // get the literals
    iValue = 0;
    for ( v = 0; v <= pVm->nVarsIn; v++ )
    {
        // add to the total number of values
        iValue += pVm->pValues[v];

        // get the next part
        pLine = Io_ReadNodeBlifMvCoverCubePart( pLine, &pPart );
        if ( pPart == NULL )
        {
            sprintf( p->sError, "This line cannot be properly parsed." );
            Io_ReadPrintErrorMessage( p );
            return 1;
        }

        // set the value set to empty
        for ( i = 0; i < pVm->pValues[v]; i++ )
            pPhase[i] = '-';
        pPhase[pVm->pValues[v]] = 0;

        // read the values of this literal
        if ( Io_ReadNodeBlifMvCoverCubeLiteral( p, v, pPhase, pEqual1, pEqual2, pPart ) )
            return 1;

        // if it is the output variable, stop (the phase is already assigned)
        if ( v == pVm->nVarsIn )
            break;

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
    return 0;
}


/**Function*************************************************************

  Synopsis    [Reads the next part of the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Io_ReadNodeBlifMvCoverCubePart( char * pLine, char ** pPart )
{
    char * pCur;

    // skip the spaces at the beginning
    pCur = Io_ReadNodeBlifMvSkipSpaces( pLine );
    if ( *pCur == 0 )
        return NULL;

    // assign the part, which begins here
    *pPart = pCur;

    // test what is the first symbol in this line
    if ( *pCur == '-' )
        pCur++;
    else if ( *pCur == '=' )
    {
        pCur = Io_ReadNodeBlifMvSkipSpaces( pCur + 1 );
        if ( pCur == NULL )
            return NULL;
        pCur = Io_ReadNodeBlifMvSkipNonSpaces( pCur );
        if ( pCur == NULL )
            return NULL;
    }
    else if ( *pCur == '(' || *pCur == '{' )
    {
        pCur = Io_ReadNodeBlifMvScrollToBrace( pCur );
        if ( pCur == NULL )
            return NULL;
        assert( *pCur == ')' || *pCur == '}' );
        pCur++;
    }
    else  // symbolic name or value number
    {
        pCur = Io_ReadNodeBlifMvSkipNonSpaces( pCur );
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
char * Io_ReadNodeBlifMvSkipSpaces( char * pStr )
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
char * Io_ReadNodeBlifMvSkipNonSpaces( char * pStr )
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
char * Io_ReadNodeBlifMvScrollToBrace( char * pStr )
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
int Io_ReadNodeBlifMvCoverCubeLiteral( Io_Read_t * p, int Lit, char * pPhase, int * pEqual1, int * pEqual2, char * pString  )
{
    Ntk_Node_t * pNode, * pFanin;
    Ntk_Pin_t * pPin;
    Vm_VarMap_t * pVm;
    int i, iFanin, Value, ValueStart, ValueStop;
    char * pTemp;

    // initialize the important pointers
    pNode   = p->pNodeCur;
    pVm    = Fnc_FunctionReadVm( Ntk_NodeReadFunc(pNode) );

    if ( *pString == '=' )
    {
        pString = Io_ReadNodeBlifMvSkipSpaces( pString + 1 );
        Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, iFanin )
        {
            if ( strcmp( Ntk_NodeReadName(pFanin), pString ) == 0 )
            {
                if ( *pEqual1 != -1 )
                {
                    sprintf( p->sError, "The =construct appears twice in this line." );
                    Io_ReadPrintErrorMessage( p );
                    return 1;
                }
                if ( Lit == iFanin )
                {
                    sprintf( p->sError, "The =construct relates the same variable." );
                    Io_ReadPrintErrorMessage( p );
                    return 1;
                }
                if ( pVm->pValues[iFanin] != pVm->pValues[Lit] )
                {
                    sprintf( p->sError, "The =construct relates variables with different number of values." );
                    Io_ReadPrintErrorMessage( p );
                    return 1;
                }
                if ( iFanin < Lit )
                {
                    *pEqual1 = iFanin;
                    *pEqual2 = Lit;
                }
                else 
                {
                    *pEqual1 = Lit;
                    *pEqual2 = iFanin;
                }
                return 0;
            }
        }
        sprintf( p->sError, "The =construct refers to \"%s\", not a fanin of the node.", pString );
        Io_ReadPrintErrorMessage( p );
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
        ValueStart = Io_ReadNodeBlifMvCoverCubeLiteralValue( p, Lit, pTemp );
        if ( ValueStart == -1 )
            return 1;
        pTemp = strtok( NULL, " -}\t" );
        ValueStop = Io_ReadNodeBlifMvCoverCubeLiteralValue( p, Lit, pTemp );
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
            if ( Io_ReadNodeBlifMvCoverCubeLiteral( p, Lit, pPhase, pEqual1, pEqual2, p->pTokens[i] ) )
                return 1;
        return 0;
    }
    // this is a value
    Value = Io_ReadNodeBlifMvCoverCubeLiteralValue( p, Lit, pString );
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
int Io_ReadNodeBlifMvCoverCubeLiteralValue( Io_Read_t * p, int Lit, char * pString )
{
    Ntk_Node_t * pNode;
    Vm_VarMap_t * pVm;
    Io_Node_t * pIoNode;
    Io_Var_t * pVar;
    char * pNodeName;
    int Value, i;

    // initialize the important pointers
    pNode   = p->pNodeCur;
    pIoNode = (Io_Node_t *)Ntk_NodeReadData(pNode);
    pVm    = Fnc_FunctionReadVm( Ntk_NodeReadFunc(pNode) );

    // get the pointer to the fanin var if present
    assert( Lit <= Ntk_NodeReadFaninNum(pNode) );
    if ( Lit < Ntk_NodeReadFaninNum(pNode) )
        pNodeName = Ntk_NodeReadName( Ntk_NodeReadFaninNode(pNode, Lit) );
    else
        pNodeName = Ntk_NodeReadName( pNode );
    if ( p->tName2Var && st_lookup( p->tName2Var, pNodeName, (char **)&pVar ) )
    {
        // check if any of the symbolic value names of this var apply
        for ( i = 0; i < pVar->nValues; i++ )
            if ( strcmp( pVar->pValueNames[i], pString ) == 0 )
                return i;
        if ( i == pVar->nValues )
        {
            sprintf( p->sError, "No symbolic value for \"%s\" among those of node \"%s\".",
                pString, pNodeName );
            Io_ReadPrintErrorMessage( p );
            return -1;
        }
    }
    // try to read it as a number
    if ( pString[0] < '0' || pString[0] > '9' )
    {
        sprintf( p->sError, "Cannot read numeric value (%s) of node \"%s\".", pString, pNodeName );
        Io_ReadPrintErrorMessage( p );
        return -1;
    }
    Value = atoi( pString );
    // see if the value applies
    if ( Value >= pVm->pValues[Lit] )
    {
        sprintf( p->sError, "The value (%s) is out of range [0; %d].", pString, pVm->pValues[Lit]-1 );
        Io_ReadPrintErrorMessage( p );
        return -1;
    }
    return Value;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


