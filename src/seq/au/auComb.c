/**CFile****************************************************************

  FileName    [auComb.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Various procedures working with automata and FSMs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: auComb.c,v 1.1 2004/02/19 03:06:44 alanmi Exp $]

***********************************************************************/

#include "auInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define PAIR_STATE_HASH(s1,s2)   ((((unsigned)s1) << 16) | (((unsigned)s2) & 0xffff))

static void Au_AutoSetupSatInstanceStatesOnly( Au_Auto_t * pAut, char * pFileName );
static void Au_AutoSetupSatInstance( Au_Auto_t * pAut, char * pFileName, 
              DdManager * dd, st_table * tTable, DdNode ** pbTransU, DdNode ** pbTrans, int nTrans, DdNode * bCubeV );
static int  Au_AutoReadSolution( char * pFileName, int * pSolution );

extern char * Io_ReadFileFileContents( char * FileName, int * pFileSize );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Finds the combinational solution if it exists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Au_Auto_t * Au_AutoCombinationalSat( Au_Auto_t * pAutInit, int nInputs, bool fVerbose )
{
    FILE * pFile = NULL;
    Au_Auto_t * pAut, * pAutR = NULL;
    Au_Trans_t * pTrans;
    DdManager * dd;
    Vm_VarMap_t * pVm;
    Vmx_VarMap_t * pVmx;
    DdNode ** pbTrans, ** pbTransU;
    st_table * tTable;  // mapping state pairs into their number
    DdNode * bCubeV, * bCond, * bComp, * bTemp;
    DdNode * bCombFunc;
    char Buffer[100];
    int * pSolution = NULL;
	int nSolution = 0;
    int nTrans, iTrans;
    int i, s;
    int clk;
    unsigned Hash;


    if ( nInputs > pAutInit->nInputs )
    {
        printf( "The number of FSM inputs is larger than the number of all inputs.\n" );
        return NULL;
    }

    // remove the non-accepting states and transitions into them
    for ( s = 0; s < pAutInit->nStates; s++ )
        pAutInit->pStates[s]->fMark = pAutInit->pStates[s]->fAcc;
    pAut = Au_AutoExtract( pAutInit, 0 );


    // get the manager and expand it if necessary
    dd = Cudd_Init( pAut->nInputs, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // get the var maps
//    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 0 );
    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), nInputs, 0 );
    pVmx   = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, -1, NULL );

    // get the U-conditions for each arc
    Au_ComputeConditions( dd, pAut, pVmx );

    // get the number of transitions
    nTrans = Au_AutoCountTransitions( pAut );

    // collect the transition conditions
    pbTransU = ALLOC( DdNode *, nTrans );
    // create the mapping of state pairs into their transitions
    tTable = st_init_table(st_numcmp,st_numhash);
    iTrans = 0;
    for ( i = 0; i < pAut->nStates; i++ )
    {
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            Hash = PAIR_STATE_HASH(pTrans->StateCur, pTrans->StateNext);
            if ( st_insert( tTable, (char *)Hash, (char *)iTrans ) )
            {
                assert( 0 );
            }
            pbTransU[iTrans] = pTrans->bCond;  Cudd_Ref( pTrans->bCond );
            iTrans++;
        }
    }


    // compute partitions
    Au_AutoStatePartitions( dd, pAut );
    if ( fVerbose )
        Au_AutoStatePartitionsPrint( pAut );
    Au_DeleteConditions( dd, pAut );



    // compute the complete conditions
    pVm    = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nInputs, 0 );
    pVmx   = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, -1, NULL );
    bCubeV = Vmx_VarMapCharCubeRange( dd, pVmx, nInputs, pAut->nInputs - nInputs );  Cudd_Ref( bCubeV );

    // get the conditions for each arch
    Au_ComputeConditions( dd, pAut, pVmx );
    // collect the transition conditions
    pbTrans = ALLOC( DdNode *, nTrans );
    iTrans = 0;
    for ( i = 0; i < pAut->nStates; i++ )
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            pbTrans[iTrans++] = pTrans->bCond;  //Cudd_Ref( pTrans->bCond );
        }


    // set up the BCP problem
//    Au_AutoSetupSatInstanceStatesOnly( pAut, "_sat_.cnf" );
clk = clock();
    Au_AutoSetupSatInstance( pAut, "_sat_.cnf", dd, tTable, pbTransU, pbTrans, nTrans, bCubeV );
    Au_AutoStatePartitionsFree( pAut );
if ( fVerbose )
{
PRT( "Setup time ", clock() - clk );
}

    for ( i = 0; i < nTrans; i++ )
        Cudd_RecursiveDeref( dd, pbTransU[i] );
    FREE( pbTransU );
    FREE( pbTrans );



    // solve the problem
    if ( pFile = fopen( "zchaff.exe", "r" ) )
        sprintf( Buffer, "zchaff.exe _sat_.cnf > _out_.cnf" );
    else if ( pFile = fopen( "..\\zchaff.exe", "r" ) )
        sprintf( Buffer, "..\\zchaff.exe _sat_.cnf > _out_.cnf" );
    else
    {
        assert( 0 );
    }
    if ( pFile )
        fclose( pFile );


clk = clock();
    if ( system( Buffer ) == -1 )
    {
        printf( "Cannot execute command \"%s\".\n", Buffer );
        goto QUITS;
    }
if ( fVerbose )
{
PRT( "zChaff time", clock() - clk );
}

    // read the solution
    pSolution = ALLOC( int, pAut->nStates + nTrans + 10 );
    nSolution = Au_AutoReadSolution( "_out_.cnf", pSolution );
    if ( nSolution == -1 )
    {
        printf( "There is no combinational solution.\n" );
        goto QUITS;
    }
    assert( nSolution == pAut->nStates + nTrans );


//printf("\n");
if ( fVerbose )
printf("SOLUTION:\nStates: ");
    for ( i = 0; i < pAut->nStates; i++ )
    {
        pAut->pStates[i]->fMark = pSolution[i];
        if ( fVerbose && pSolution[i] )
//            printf( " %d", i );
            printf( " %s", pAut->pStates[i]->pName );
    }
if ( fVerbose )
printf("\n");

if ( fVerbose )
printf("Transtions: ");
    // mark the transitions
    iTrans = 0;
    for ( i = 0; i < pAut->nStates; i++ )
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            pTrans->uCond = pSolution[ pAut->nStates + iTrans ];
            if ( fVerbose && pSolution[ pAut->nStates + iTrans ] )
//                printf( " %d->%d", pTrans->StateCur, pTrans->StateNext );
                printf( " %s->%s", pAut->pStates[pTrans->StateCur]->pName, 
                                   pAut->pStates[pTrans->StateNext]->pName );
            iTrans++;
        }
if ( fVerbose )
printf("\n");

    // derive the combinational function
    bCombFunc = b1;  Cudd_Ref( bCombFunc );
    for ( i = 0; i < pAut->nStates; i++ )
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            if ( pTrans->uCond == 0 )
                continue;
            // get the condition
            bCond = Cudd_bddExistAbstract( dd, pTrans->bCond, bCubeV );  Cudd_Ref( bCond );
            // get the component
            bComp = Cudd_bddOr( dd, Cudd_Not(bCond), pTrans->bCond );    Cudd_Ref( bComp );
            Cudd_RecursiveDeref( dd, bCond );
            // add this component
            bCombFunc = Cudd_bddAnd( dd, bTemp = bCombFunc, bComp );     Cudd_Ref( bCombFunc );
            Cudd_RecursiveDeref( dd, bComp );
            Cudd_RecursiveDeref( dd, bTemp );
        }
//PRB( dd, bCombFunc );

    // check that the function is well-defined
    bCond = Cudd_bddExistAbstract( dd, bCombFunc, bCubeV );  Cudd_Ref( bCond );
    if ( bCond != b1 )
    {
        printf( "There is no (well-defined) combinational solution.\n" );
        Cudd_RecursiveDeref( dd, bCond );
        Cudd_RecursiveDeref( dd, bCombFunc );
        goto QUITS;
    }
    Cudd_RecursiveDeref( dd, bCond );


    // extract the automaton with marked states and transitions
//    pAutR = Au_AutoExtract( pAut, 0 );

    // create a single state automaton with this transition
    pAutR = Au_AutoSingleState( pAut, dd, bCombFunc, pVmx );
    Cudd_RecursiveDeref( dd, bCombFunc );

QUITS:

    // clean the current automaton
    Cudd_RecursiveDeref( dd, bCubeV );
    Au_DeleteConditions( dd, pAut );
    Extra_StopManager( dd );

    Au_AutoFree( pAut );

	// free the pMatrix and the solution
    FREE( pSolution );
    st_free_table( tTable );
    return pAutR;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Au_AutoReadSolution( char * pFileName, int * pSolution )
{
    char * pBuffer, * pTemp;
    char * pBegin, * pEnd;
    int iSol, Number;
    
    pBuffer = Io_ReadFileFileContents( pFileName, NULL );

    // get the beginning
    pBegin = strstr( pBuffer, "Instance satisfiable" );
    if ( pBegin == NULL )
    {
        pEnd = strstr( pBuffer, "UNSAT" );
        if ( pEnd == NULL )
            printf( "Non-standard output file.\n" );
        free( pBuffer );
        return -1; // UNSAT
    }
    pBegin += strlen("Instance satisfiable");

    pEnd = strstr( pBuffer, "Max Decision Level" );
    if ( pEnd == NULL )
    {
        printf( "Non-standard output file.\n" );
        free( pBuffer );
        return -1; // UNSAT
    }
    *pEnd = 0;

    // read the solution
    pTemp = strtok( pBegin, " \n\r\t" );
    for ( iSol = 0; pTemp; iSol++ )
    {
        Number = atoi(pTemp);
        assert( Number == iSol + 1 || -Number == iSol + 1 );
        pSolution[iSol] = (int)(Number > 0);
        pTemp = strtok( NULL, " \n\r\t" );
    }

    free( pBuffer );
    return iSol;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoSetupSatInstanceStatesOnly( Au_Auto_t * pAut, char * pFileName )
{
    FILE * pFile;
    Au_State_t * pState;
    Au_SPInfo_t * p;
    int iClause, s, i, k;

    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cannot open the output file.\n" );
        return;
    }

    // reserve room for the topmost line
    fprintf( pFile, "p cnf                           \n" );

    // add the first clause for the initial state
    fprintf( pFile, "1 0\n" );
    iClause = 1;

    // go through the states and add clauses
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
        p = Au_AutoInfoGet( pState );
        for ( i = 0; i < p->nParts; i++ )
        {
            // skip self referencing clause
            for ( k = 0; k < p->pSizes[i]; k++ )
                if ( p->ppParts[i][k] == s )
                    break;
            if ( k < p->pSizes[i] )
                continue;

            // add this clause
//            sm_insert( pMatrix, iClause, 2*s + 1 );
            fprintf( pFile, "-%d", s + 1 );
            for ( k = 0; k < p->pSizes[i]; k++ )
//                sm_insert( pMatrix, iClause, 2*p->ppParts[i][k] );
                fprintf( pFile, " %d", p->ppParts[i][k] + 1 );
            fprintf( pFile, " 0\n" );

            iClause++;
        }
    }
    fclose( pFile );

    // write the topmost line
    pFile = fopen( pFileName, "r+" );
    fprintf( pFile, "p cnf %d %d", pAut->nStates, iClause );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Au_AutoSetupSatInstance( Au_Auto_t * pAut, char * pFileName, 
              DdManager * dd, st_table * tTable, DdNode ** pbTransU, DdNode ** pbTrans, int nTrans, DdNode * bCubeV )
{
    FILE * pFile;
    Au_State_t * pState;
    Au_Trans_t * pTrans;
    Au_SPInfo_t * p;
    int iClause, s, i, k;
    int iTrans;
    unsigned Hash;

    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cannot open the output file.\n" );
        return;
    }


    // reserve room for the topmost line
    fprintf( pFile, "p cnf                           \n" );

    // add the first clause for the initial state
    fprintf( pFile, "1 0\n" );
    iClause = 1;

    // go through the states and add clauses
    for ( s = 0; s < pAut->nStates; s++ )
    {
        pState = pAut->pStates[s];
        p = Au_AutoInfoGet( pState );
        for ( i = 0; i < p->nParts; i++ )
        {
/*
            // skip self referencing clause
            for ( k = 0; k < p->pSizes[i]; k++ )
                if ( p->ppParts[i][k] == s )
                    break;
            if ( k < p->pSizes[i] )
                continue;
*/

            // add this clause
//            sm_insert( pMatrix, iClause, 2*s + 1 );
            fprintf( pFile, "-%d", s + 1 );
            for ( k = 0; k < p->pSizes[i]; k++ )
            {
                Hash = PAIR_STATE_HASH(s, p->ppParts[i][k]);
                // find the corresponding transition
                if ( !st_lookup( tTable, (char *)Hash, (char **)&iTrans ) )
                {
                    assert( 0 );
                }
//                sm_insert( pMatrix, iClause, 2*p->ppParts[i][k] );
//                fprintf( pFile, " %d", p->ppParts[i][k] + 1 );
                fprintf( pFile, " %d", pAut->nStates + iTrans + 1 );
            }
            fprintf( pFile, " 0\n" );

            iClause++;
        }
    }

    // for each transition add the clauses Tij == Si & Sj
    // Tij => Si, Tij => Sj,  Si & Sj => Tij
    // !Tij + Si, !Tij + Sj, !Si + !Sj + Tij
    iTrans = 0;
    for ( i = 0; i < pAut->nStates; i++ )
        Au_StateForEachTransition( pAut->pStates[i], pTrans )
        {
            fprintf( pFile, "-%d %d 0\n", pAut->nStates + iTrans + 1, i + 1 );
            fprintf( pFile, "-%d %d 0\n", pAut->nStates + iTrans + 1, pTrans->StateNext + 1 );
            fprintf( pFile, "-%d -%d %d 0\n", 
                i + 1, pTrans->StateNext + 1, pAut->nStates + iTrans + 1 );
            iClause += 3;
            iTrans++;
        }


    // add mutual exclusiveness of the transitions
    // !Ti1j1 + !Ti2j2   iff   Cond(i1 -> j1) & Cond(i2 -> j2) == 0  
    for ( i = 0; i < nTrans; i++ )
        for ( k = i + 1; k < nTrans; k++ )
        {
            // if the input conditions do not overlap, skip
            if ( Cudd_bddLeq( dd, pbTransU[i], Cudd_Not(pbTransU[k]) ) ) // they don't intersect
                continue;
            // if the input conditions intersect, but there are no common outputs
            // add a clause
            if ( Cudd_bddLeq( dd, pbTrans[i], Cudd_Not(pbTrans[k]) ) ) // they don't intersect
            {
                fprintf( pFile, "-%d -%d 0\n", pAut->nStates + i + 1, pAut->nStates + k + 1 );
                iClause++;
            }
        }
    fclose( pFile );

    // write the topmost line
    pFile = fopen( pFileName, "r+" );
//    fprintf( pFile, "p cnf %d %d", pAut->nStates, iClause );
    fprintf( pFile, "p cnf %d %d", pAut->nStates + nTrans, iClause );
    fclose( pFile );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


