/**CFile****************************************************************

  FileName    [aioWriteAut.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various network writing procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: aioWriteAut.c,v 1.1 2004/02/19 03:06:42 alanmi Exp $]

***********************************************************************/

#include "aioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Aio_AutoWriteAutOne( FILE * pFile, Aut_Auto_t * pAut, bool fWriteFsm );
static int  Aio_AutoWriteBlifState( FILE * pFile, Aut_State_t * pState, Mvc_Manager_t * pMvcMan );
static void Aio_AutoWriteBlifCover( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, char * pStateCur, char * pStateNext, int nLength );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the automaton into an AUT file with the given name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_AutoWriteAut( Aut_Auto_t * pAut, char * FileName, bool fWriteFsm )
{
    FILE * pFile;
    int nProducts;

    pFile = fopen( FileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Aio_WriteNetwork(): Cannot open the output file.\n" );
        return;
    }
    // write the network
    nProducts = Aio_AutoWriteAutOne( pFile, pAut, fWriteFsm );
    // finalize the file
    fprintf( pFile, ".e\n" );
    fclose( pFile );

    // write the topmost lines
    pFile = fopen( FileName, "r+" );
    // write the stats
    fprintf( pFile, ".i %d\n", Aut_AutoReadVarNum( pAut ) );
    fprintf( pFile, ".o %d\n", 0 );
    fprintf( pFile, ".s %d\n", Aut_AutoReadStateNum( pAut ) );
    fprintf( pFile, ".p %d", nProducts );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_AutoWriteAutOne( FILE * pFile, Aut_Auto_t * pAut, bool fWriteFsm )
{
    Mvc_Manager_t * pMvcMan;
    Aut_State_t * pState;
    Aut_Var_t ** pVars;
    int i, nVars, nProducts;
    int nLength;
    
    // create ZDD variables for writing the covers
    Cudd_zddVarsFromBddVars( Aut_AutoReadDd(pAut), 2 );

    // write the placeholder (50 symbols)
    fprintf( pFile, "          " );
    fprintf( pFile, "          " );
    fprintf( pFile, "          " );
    fprintf( pFile, "          " );
    fprintf( pFile, "          \n" );

    nVars = Aut_AutoReadVarNum( pAut );
    pVars = Aut_AutoReadVars( pAut );

    fprintf( pFile, ".ilb" );
    for ( i = 0; i < nVars; i++ )
        fprintf( pFile, " %s", Aut_VarReadName( pVars[i] ) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".ob" );
    fprintf( pFile, "\n" );

    // write the POs
    fprintf( pFile, ".accepting" );
    Aut_AutoForEachState( pAut, pState )
        if ( Aut_StateReadAcc(pState) )
            fprintf( pFile, " %s", Aut_StateReadName(pState) );
    fprintf( pFile, "\n" );

    // put the initial state in front
    pState = Aut_AutoReadInit( pAut );
    Aut_AutoStateDelete( pState );
    Aut_AutoStateAddFirst( pState );

    // get the longest state name
    nLength = 0;
    Aut_AutoForEachState( pAut, pState )
        if ( nLength < (int)strlen( Aut_StateReadName(pState) ) )
            nLength = strlen( Aut_StateReadName(pState) );
    Aut_AutoSetData( pAut, (char *)nLength );

    // write the next state node
    nProducts = 0;
    pMvcMan = Mvc_ManagerStart();
    Aut_AutoForEachState( pAut, pState )
        nProducts += Aio_AutoWriteBlifState( pFile, pState, pMvcMan );
    Mvc_ManagerFree(pMvcMan);
    return nProducts;
}

/**Function*************************************************************

  Synopsis    [Write the node into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aio_AutoWriteBlifState( FILE * pFile, Aut_State_t * pState, Mvc_Manager_t * pMvcMan )
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
    int nProducts;
    int nLength;

    nLength = (int)Aut_AutoReadData( pAut );

    nProducts = 0;
    nVars = Aut_AutoReadVarNum( pAut );;
    pVmx = Aut_AutoReadVmx( pAut );
    Aut_StateForEachTransitionFrom( pState, pTrans )
    {
        // get the function
        bFunc = Aut_TransReadCond(pTrans);
        // derive the cover for the condition
        pMvc = Fnc_FunctionDeriveSopFromMddSpecial( pMvcMan, 
                dd, bFunc, bFunc, pVmx, nVars );
        // write the cover
        pStateName1 = Aut_StateReadName( Aut_TransReadStateFrom(pTrans) );
        pStateName2 = Aut_StateReadName( Aut_TransReadStateTo(pTrans) );
        Aio_AutoWriteBlifCover( pFile, Vmx_VarMapReadVm(pVmx), pMvc, 
            pStateName1, pStateName2, nLength );
        nProducts += Mvc_CoverReadCubeNum(pMvc);
        // delete the cover
        Mvc_CoverFree( pMvc );
    }
    return nProducts;
}

/**Function*************************************************************

  Synopsis    [Write the cover as BLIF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aio_AutoWriteBlifCover( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, 
    char * pName1, char * pName2, int nLength )
{
    Mvc_Cube_t * pCube;
    int v;

    int * pValues      = Vm_VarMapReadValuesArray(pVm);
    int * pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);
    int nVarsIn        = Vm_VarMapReadVarsInNum(pVm);

    Mvc_CoverForEachCube( Cover, pCube ) 
    {
        // write the inputs
        for ( v = 0; v < nVarsIn; v++ )
            if ( Mvc_CubeBitValue(  pCube,  2*v ) )
            {
                if ( Mvc_CubeBitValue(  pCube,  2*v + 1 ) )
                    fprintf( pFile, "%c", '-' );
                else 
                    fprintf( pFile, "%c", '0' );
            }
            else 
            {
                if ( Mvc_CubeBitValue(  pCube,  2*v + 1 ) )
                    fprintf( pFile, "%c", '1' );
                else 
                    fprintf( pFile, "%c", '?' );
            }
        // write the output
        fprintf( pFile, " %*s %*s\n", nLength, pName1, nLength, pName2 );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

