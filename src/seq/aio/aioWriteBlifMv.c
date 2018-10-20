/**CFile****************************************************************

  FileName    [ioWrite.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various network writing procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioWriteBlifMv.c,v 1.3 2005/06/02 03:34:19 alanmi Exp $]

***********************************************************************/

#include "aioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define  IO_WRITE_LINE_LENGTH    78    // the output line length

static void Aio_AutoWriteBlifMvOne( FILE * pFile, Aut_Auto_t * pAut, bool fWriteFsm );
static void Aio_AutoWriteBlifMvState( FILE * pFile, Aut_State_t * pState, Mvc_Manager_t * pMvcMan, Aut_State_t * pStateDc );
static void Aio_AutoWriteBlifMvCover( FILE * pFile, Aut_Auto_t * pAut, Vm_VarMap_t * pVm, Aut_Var_t ** pVars, Mvc_Cover_t * Cover, char * pStateCur, char * pStateNext, int nLength );
static void Aio_AutoWriteVariables( FILE * pFile, Aut_Auto_t * pAut );
static void Aio_AutoWriteMarkedStateNames( FILE * pFile, Aut_Auto_t * pAut, bool fCommaSep );

extern Aut_State_t * Auti_AutoFindDcState( Aut_Auto_t * pAut );
extern int           Auti_AutoCheckIsComplete( Aut_Auto_t * pAut, int nInputs );
extern int           Auti_AutoCheckIsNd( FILE * pOut, Aut_Auto_t * pAut, int nInputs, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the automaton into a BLIF file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_AutoWriteBlifMv( Aut_Auto_t * pAut, char * FileName, bool fWriteFsm )
{
    FILE * pFile;
    pFile = fopen( FileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Aio_WriteNetwork(): Cannot open the output file.\n" );
        return;
    }
    // write the model name
    fprintf( pFile, ".model %s\n", Aut_AutoReadName(pAut) );
    // write the network
    Aio_AutoWriteBlifMvOne( pFile, pAut, fWriteFsm );
    // finalize the file
    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_AutoWriteBlifMvOne( FILE * pFile, Aut_Auto_t * pAut, bool fWriteFsm )
{
    Mvc_Manager_t * pMvcMan;
    Aut_State_t * pState;
    Aut_State_t ** ppStates;
    Aut_State_t * pStateDc;
    Aut_Var_t ** pVars;
    char ** pValueNames;
    int i, k, nVars, nInit, nValues;
    int nAccepting, nNonAccepting;
    int nLength;
    
    // create ZDD variables for writing the covers
    Cudd_zddVarsFromBddVars( Aut_AutoReadDd(pAut), 2 );

    // write the PIs
    fprintf( pFile, ".inputs" );
    Aio_AutoWriteVariables( pFile, pAut );
    fprintf( pFile, "\n" );

    // write the POs
    fprintf( pFile, ".outputs Acc" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // write the .mv lines for the input variables
    nVars = Aut_AutoReadVarNum( pAut );
    pVars = Aut_AutoReadVars( pAut );
    for ( i = 0; i < nVars; i++ )
    {
        nValues = Aut_VarReadValueNum(pVars[i]);
        pValueNames = Aut_VarReadValueNames(pVars[i]);
        if ( nValues == 2 )
        {
            if ( pValueNames == NULL )
                continue;
            fprintf( pFile, ".mv %s %d %s %s\n", 
                Aut_VarReadName(pVars[i]), nValues, pValueNames[0], pValueNames[1] );            
        }
        else
        {
            fprintf( pFile, ".mv %s %d", Aut_VarReadName(pVars[i]), nValues );
            if ( pValueNames )
            {
                for ( k = 0; k < nValues; k++ )
                    fprintf( pFile, " %s", pValueNames[k] );
            }
            fprintf( pFile, "\n" );
        }
    }

    // write the current state and next state variables
    fprintf( pFile, ".mv CS, NS  %d", Aut_AutoReadStateNum(pAut) );
//    if ( Aut_AutoReadStateNum(pAut) > 5 )
//        fprintf( pFile, "\\\n" );
//    else
        fprintf( pFile, "  " );
    // mark all states
    Aut_AutoSetMark( pAut );
    Aio_AutoWriteMarkedStateNames( pFile, pAut, 0 );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // write the latch
    fprintf( pFile, ".latch NS CS\n" );
    fprintf( pFile, ".reset CS\n" );
    nInit = Aut_AutoReadInitNum( pAut );
    ppStates = Aut_AutoReadInitAll( pAut );
    if ( Aut_AutoReadStateNum(pAut) > 0 )
    {
        if ( nInit > 1 )
            fprintf( pFile, "(" );
        for ( i = 0; i < nInit; i++ )
            fprintf( pFile, "%s%s", (i?",":""), Aut_StateReadName(ppStates[i]) );
        if ( nInit > 1 )
            fprintf( pFile, ")" );
        fprintf( pFile, "\n" );
    }
    else
    {
        fprintf( pFile, "-\n" );
    }
    fprintf( pFile, "\n" );

    // write the output node
    nAccepting = Aut_AutoCountAccepting(pAut);
    nNonAccepting = Aut_AutoReadStateNum(pAut) - nAccepting;

    if ( nNonAccepting == 0 && nAccepting == 0 )
        fprintf( stdout, "Warning: The automaton has no states.\n" );
    else if ( nAccepting == 0 )
        fprintf( stdout, "Warning: The automaton has no accepting states.\n" );

    if ( nNonAccepting == 0 )
    {
        fprintf( pFile, ".table ->Acc\n" );
        fprintf( pFile, "1\n" );
    }
    else if ( nAccepting == 0 )
    {
        fprintf( pFile, ".table ->Acc\n" );
        fprintf( pFile, "0\n" );
    }
    else if ( nAccepting > Aut_AutoReadStateNum(pAut)/2 )
    {
        fprintf( pFile, ".table CS ->Acc\n" );
        fprintf( pFile, ".default 1\n" );
        if ( nNonAccepting > 1 )
            fprintf( pFile, "(" );
        // mark non-accepting states
        Aut_AutoSetMarkNonAccepting( pAut );
        Aio_AutoWriteMarkedStateNames( pFile, pAut, 1 );
        if ( nNonAccepting > 1 )
            fprintf( pFile, ")" );
        fprintf( pFile, " 0\n" );
    }
    else
    {
        fprintf( pFile, ".table CS ->Acc\n" );
        fprintf( pFile, ".default 0\n" );
        if ( nAccepting > 1 )
            fprintf( pFile, "(" );
        // mark accepting states
        Aut_AutoSetMarkAccepting( pAut );
        Aio_AutoWriteMarkedStateNames( pFile, pAut, 1 );
        if ( nAccepting > 1 )
            fprintf( pFile, ")" );
        fprintf( pFile, " 1\n" );
    }
    fprintf( pFile, "\n" );

    // write the table line
    fprintf( pFile, ".table" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " %s", Aut_VarReadName(pVars[i]) );
    fprintf( pFile, " CS ->NS\n" );

    // find if there is a DC state
    // if yes, and if the automaton is complete, use .default line
    // only do so when the automaton is deterministic!!!
    pStateDc = Auti_AutoFindDcState( pAut );
//    pStateDc = NULL;
    if ( pStateDc && 
            Auti_AutoCheckIsComplete( pAut, Aut_AutoReadVarNum(pAut) ) == 0 &&
            Auti_AutoCheckIsNd( stdout, pAut, Aut_AutoReadVarNum(pAut), 0 ) == 0 )
        fprintf( pFile, ".default %s\n", Aut_StateReadName(pStateDc) );
    else 
        pStateDc = NULL;

    // get the longest state name
    nLength = 0;
    Aut_AutoForEachState( pAut, pState )
        if ( nLength < (int)strlen( Aut_StateReadName(pState) ) )
            nLength = strlen( Aut_StateReadName(pState) );
    Aut_AutoSetData( pAut, (char *)nLength );

    // write the next state node
    pMvcMan = Mvc_ManagerStart();
    Aut_AutoForEachState( pAut, pState )
        Aio_AutoWriteBlifMvState( pFile, pState, pMvcMan, pStateDc );
    Mvc_ManagerFree(pMvcMan);
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_AutoWriteVariables( FILE * pFile, Aut_Auto_t * pAut )
{
    int LineLength;
    int AddedLength;
    int NameCounter;
    int i, nVars;
    Aut_Var_t ** pVars;
    char * pName;

    LineLength  = 7;
    NameCounter = 0;
    nVars = Aut_AutoReadVarNum( pAut );
    pVars = Aut_AutoReadVars( pAut );
    for ( i = 0; i < nVars; i++ )
    {
        pName = Aut_VarReadName( pVars[i] );
        // get the line length after this name is written
        AddedLength = strlen(pName) + 1;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \\\n" );
            // reset the line length
            LineLength  = 0;
            NameCounter = 0;
        }
        fprintf( pFile, " %s", pName );
        LineLength += AddedLength;
        NameCounter++;
    }
}

/**Function*************************************************************

  Synopsis    [Writes the state name list.]

  Description [Writes the names of only marked states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_AutoWriteMarkedStateNames( FILE * pFile, Aut_Auto_t * pAut, bool fCommaSep )
{
    Aut_State_t * pState;
    int LineLength;
    int AddedLength;
    int NameCounter;
    bool fFirst;

    fFirst = 1;
    LineLength  = 20;
    NameCounter = 0;
    Aut_AutoForEachState( pAut, pState )
    {
        if ( !Aut_StateReadMark( pState ) )
            continue;
        // add comma
        if ( fFirst )
            fFirst = 0;
        else
            fprintf( pFile, "%c", fCommaSep? ',': ' ' );
        // get the line length after this name is written
        AddedLength = strlen(Aut_StateReadName(pState)) + 1;
        if ( NameCounter && LineLength + AddedLength + 3 > IO_WRITE_LINE_LENGTH )
        { // write the line extender
            fprintf( pFile, " \\\n" );
            // reset the line length
            LineLength  = 0;
            NameCounter = 0;
        }
        fprintf( pFile, "%s", Aut_StateReadName(pState) );
        LineLength += AddedLength;
        NameCounter++;
    }
}

/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_AutoWriteBlifMvState( FILE * pFile, Aut_State_t * pState, Mvc_Manager_t * pMvcMan, Aut_State_t * pStateDc )
{
    Aut_Auto_t * pAut = Aut_StateReadAut( pState );
    DdManager * dd = Aut_AutoReadDd( pAut );
    Aut_Trans_t * pTrans;
    DdNode * bFunc;
    Mvc_Cover_t * pMvc;
    Vmx_VarMap_t * pVmx;
    char * pStateName1;
    char * pStateName2;
    int nVars;
    int nLength;

    if ( pState == pStateDc )
        return;

    nLength = (int)Aut_AutoReadData( pAut );

    nVars = Aut_AutoReadVarNum( pAut );;
    pVmx = Aut_AutoReadVmx( pAut );
    Aut_StateForEachTransitionFrom( pState, pTrans )
    {
        if ( pStateDc && Aut_TransReadStateTo(pTrans) == pStateDc )
            continue;
        // get the function
        bFunc = Aut_TransReadCond(pTrans);
        // derive the cover for the condition
        pMvc = Fnc_FunctionDeriveSopFromMddSpecial( pMvcMan, 
                dd, bFunc, bFunc, pVmx, nVars );
        // write the cover
        pStateName1 = Aut_StateReadName( Aut_TransReadStateFrom(pTrans) );
        pStateName2 = Aut_StateReadName( Aut_TransReadStateTo(pTrans) );
        Aio_AutoWriteBlifMvCover( pFile, pAut, Vmx_VarMapReadVm(pVmx), Aut_AutoReadVars(pAut), pMvc, 
            pStateName1, pStateName2, nLength );
        // delete the cover
        Mvc_CoverFree( pMvc );
    }
}

/**Function*************************************************************

  Synopsis    [Write the cover as BLIF-MV.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_AutoWriteBlifMvCover( FILE * pFile, Aut_Auto_t * pAut, Vm_VarMap_t * pVm, Aut_Var_t ** pVars, Mvc_Cover_t * Cover, 
    char * pName1, char * pName2, int nLength )
{
    Mvc_Cube_t * pCube;
    char ** pValueNames;
    int Counter, FirstValue;
    int i, v;

    int * pValues      = Vm_VarMapReadValuesArray(pVm);
    int * pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);
//    int nVarsIn        = Vm_VarMapReadVarsInNum(pVm);
    int nVarsIn        = Aut_AutoReadVarNum(pAut);

    Mvc_CoverForEachCube( Cover, pCube ) 
    {
        // write the variables
        for ( i = 0; i < nVarsIn; i++ )
        {
            pValueNames = Aut_VarReadValueNames( pVars[i] );

            // count the number of values in this literal
            Counter  = 0;
            for ( v = 0; v < pValues[i]; v++ )
                if ( Mvc_CubeBitValue( pCube,  pValuesFirst[i] + v) ) 
                    Counter++;
            assert( Counter > 0 );

            if ( Counter == pValues[i] )
                fprintf( pFile, "- " );
            else if ( Counter == 1 )
            {
                for ( v = 0; v < pValues[i]; v++ )
                    if ( Mvc_CubeBitValue( pCube,  pValuesFirst[i] + v) ) 
                    {
                        if ( pValueNames )
                            fprintf( pFile, "%s ", pValueNames[v] );
                        else
                            fprintf( pFile, "%d ", v );
                    }
            }
            else
            {
                fprintf( pFile, "(" );
                FirstValue = 1;
                for ( v = 0; v < pValues[i]; v++ )
                    if ( Mvc_CubeBitValue( pCube,  pValuesFirst[i] + v) ) 
                    {
                        if ( FirstValue )
                            FirstValue = 0;
                        else
                            fprintf( pFile, "," );
                        if ( pValueNames )
                            fprintf( pFile, "%s", pValueNames[v] );
                        else
                            fprintf( pFile, "%d", v );
                    }
                fprintf( pFile, ") " );
            }
        }
        // write the output
        if ( nVarsIn > 0 )
            fprintf( pFile, " " );
        fprintf( pFile, " %*s  %*s\n", nLength, pName1, nLength, pName2 );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

