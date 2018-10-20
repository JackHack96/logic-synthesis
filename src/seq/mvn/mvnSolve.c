/**CFile****************************************************************

  FileName    [mvnSolve.c]
  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Solves the language equation using partitioned representations.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnSolve.c,v 1.6 2005/06/02 03:34:22 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"
#include "aut.h"
#include "autiInt.h"
#include "ioInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


typedef struct Mvn_Solve_t_   Mvn_Solve_t;
struct Mvn_Solve_t_
{
    // the networks for Fixed and Spec
    Ntk_Network_t * pNetF;
    Ntk_Network_t * pNetS;
    // manager and variable map
    DdManager *     dd;
    Vmx_VarMap_t *  pVmx;
    // mapping of the variable name into its number in the map
    st_table *      tName2Num;    // maps the names of all vars (except U) into their numbers
    // the variables of all four sets
    int             nVarsI;       // the I variables 
    char **         pVarsI;
    int             nVarsO;       // the O variables 
    char **         pVarsO;
    int             nVarsU;       // the U variables 
    char **         pVarsU;
    int             nVarsV;       // the V variables 
    char **         pVarsV;
    // partitions
    int             nPartsT;      // the transition partitions (TF, TS, U)
    DdNode **       pbPartsT;
    int             nPartsO;      // the output partitions (OFj = OSj)
    DdNode **       pbPartsO;     

    // information used by the image computation
    DdNode *        bCubeCs;      // the quantification cube 
    DdNode *        bCubeNs;      // the quantification cube 
    DdNode *        bCubeI;       // the quantification cube 
    DdNode *        bCubeU;       // the quantification cube 
    DdNode *        bCubeV;       // the quantification cube 
    // the initial state
    DdNode *        bInit;        // the cube of initial states
    // image scheduling trees
    Extra_ImageTree_t *  pTreeT;   // the tree for the transition image
    Extra_ImageTree_t ** ppTreeOs; // the tree for the output conformance images
//    Extra_ImageTree2_t *  pTreeT;   // the tree for the transition image
//    Extra_ImageTree2_t ** ppTreeOs; // the tree for the output conformance images
    // there are as many output images as there are output partitions

    // various parameters
    int             fVerbose;
    int             fUseOinT;
};

static Mvn_Solve_t * Mvn_NetworkGetNames( Ntk_Network_t * pNetF, Ntk_Network_t * pNetS, char * pStrU, char * pStrV, int fVerbose );
static void Mvn_PrintNetworkStats( Ntk_Network_t * pNet, char * pStr );
static void Mvn_PrintVarRange( char ** pStrs, int nStrs, char * pDescr );
static void Mvn_CreateVariables( Mvn_Solve_t * p );
static void Mvn_ComputeGlobal( Mvn_Solve_t * p );
static void Mvn_ComputeGlobalShuffle( Mvn_Solve_t * p );
static void Mvn_ComputeStart( Mvn_Solve_t * p );
static void Mvn_ComputeSchedule( Mvn_Solve_t * p );
static void Mvn_NetworkSolveVarsAndResetValues( Mvn_Solve_t * p, int * pVarsCs, int * pVarsNs, int * pCodes );
static void Mvn_NetworkSolveClean( Mvn_Solve_t * p );
static Aut_Auto_t * Mvn_ComputeAutomaton( Mvn_Solve_t * p, int nStateLimit, int fUseLongNames, int fMoore );
static void Mvn_SolveDeriveStateNames( Mvn_Solve_t * p, Aut_Auto_t * pAut );
static Aut_Var_t * Mvn_NetworkSolveGetVar( Ntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkSolve( Ntk_Network_t * pNetF, Ntk_Network_t * pNetS,  
    char * pStrU, char * pStrV, char * pFileName, int nStateLimit, 
    int fProgressive, int fUseLongNames, int fMoore, int fVerbose,
    int fWriteAut, int fAlgorithm )
{
    Mvn_Solve_t * p;
    Aut_Auto_t * pAut;
    int clk, clk1;
/*
    if ( Ntk_NetworkReadCoNum(pNetS) - Ntk_NetworkReadLatchNum(pNetS) > 1 )
    {
        printf( "The current implementation cannot process network S\n" );
        printf( "with more than one PO (there may be many latches)\n" );
        return 1;
    }
*/
    // get the variables
    p = Mvn_NetworkGetNames( pNetF, pNetS, pStrU, pStrV, fVerbose );
    if ( p == NULL )
        return 1;
    p->fUseOinT = fAlgorithm;

    // map the variables and start the manager
    Mvn_CreateVariables( p );

    // compute global functions and derive partitions
    Mvn_ComputeGlobal( p );

    // collect the cubes, the init state, sets up the var map
    Mvn_ComputeStart( p );

    // schedule the image computations
    Mvn_ComputeSchedule( p );

    // derive the automaton
clk = clock();
    pAut = Mvn_ComputeAutomaton( p, nStateLimit, fUseLongNames, fMoore );
if ( p->fVerbose )
{
    PRT( "The total runtime of the partitioned computation", clock() - clk );
}
    if ( pAut )
    {
        if ( fProgressive )
        {
            int nStateBefore, nStatesAfter;
            if ( p->fVerbose )
            {
                printf( "Computing progressive solution...\n" );
                fflush( stdout );
            }
clk1 = clock();
            nStateBefore = Aut_AutoReadStateNum(pAut);
            Auti_AutoProgressive( pAut, p->nVarsU, 0, 1 );
            nStatesAfter = Aut_AutoReadStateNum(pAut);
PRT( "Progressive", clock() - clk1 );
//            Aut_AutoComplete( pAut, pAut->nVars, 0, 0 );

            if ( p->fVerbose )
                printf( "The resulting automaton is made progressive (%d states -> %d states).\n", 
                    nStateBefore, nStatesAfter );
        }
//        Aut_AutoWriteKiss( pAut, pFileName );
        Aio_AutoWrite( pAut, pFileName, fWriteAut, 0 );
        if ( p->fVerbose )
            printf( "The solution is written into file \"%s\".\n", pFileName );
    }
    else
        printf( "No output produced.\n" );

    // clean the remaning stuff
    Mvn_NetworkSolveClean( p );
    if ( pAut )
        Aut_AutoFree( pAut );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_NetworkSolveClean( Mvn_Solve_t * p )
{
    int i;
    for ( i = 0; i < p->nVarsI; i++ )
        free( p->pVarsI[i] );
    free( p->pVarsI );

    for ( i = 0; i < p->nVarsO; i++ )
        free( p->pVarsO[i] );
    free( p->pVarsO );

    for ( i = 0; i < p->nVarsU; i++ )
        free( p->pVarsU[i] );
    free( p->pVarsU );

    for ( i = 0; i < p->nVarsV; i++ )
        free( p->pVarsV[i] );
    free( p->pVarsV );

    if ( p->tName2Num )
    st_free_table( p->tName2Num );

    if ( p->dd && p->bCubeCs )
    Cudd_RecursiveDeref( p->dd, p->bCubeCs );
    if ( p->dd && p->bCubeNs )
    Cudd_RecursiveDeref( p->dd, p->bCubeNs );
    if ( p->dd && p->bCubeI )
    Cudd_RecursiveDeref( p->dd, p->bCubeI );
    if ( p->dd && p->bCubeU )
    Cudd_RecursiveDeref( p->dd, p->bCubeU );
    if ( p->dd && p->bCubeV )
    Cudd_RecursiveDeref( p->dd, p->bCubeV );

    if ( p->dd && p->bInit )
    Cudd_RecursiveDeref( p->dd, p->bInit );

    if ( p->dd && p->pbPartsT )
    Ntk_NodeGlobalDerefFuncs( p->dd, p->pbPartsT, p->nPartsT );
    if ( p->dd && p->pbPartsO )
    Ntk_NodeGlobalDerefFuncs( p->dd, p->pbPartsO, p->nPartsO );
    FREE( p->pbPartsT );
    FREE( p->pbPartsO );

    if ( p->dd && p->pTreeT )
    Extra_bddImageTreeDelete( p->pTreeT );
//    Extra_bddImageTreeDelete2( p->pTreeT );
    if ( p->dd && p->ppTreeOs )
    {
        for ( i = 0; i < p->nVarsO; i++ )
            Extra_bddImageTreeDelete( p->ppTreeOs[i] );
//            Extra_bddImageTreeDelete2( p->ppTreeOs[i] );
    }
    FREE( p->ppTreeOs );

//    if ( p->dd )
//    Extra_StopManager( p->dd );

    FREE( p );
}    


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvn_Solve_t * Mvn_NetworkGetNames( Ntk_Network_t * pNetF, Ntk_Network_t * pNetS,  
    char * pStrU, char * pStrV, int fVerbose )
{
    Mvn_Solve_t * p;
    Ntk_Node_t * pNodeS, * pNodeF;
    char * pTemp;
    int fError = 0;
    int i, k;

    p = ALLOC( Mvn_Solve_t, 1 );
    memset( p, 0, sizeof(Mvn_Solve_t) );
    p->pNetF = pNetF;
    p->pNetS = pNetS;
    p->fVerbose = fVerbose;

    p->pVarsI = ALLOC( char *, 1000 );
    p->pVarsO = ALLOC( char *, 1000 );
    p->pVarsU = ALLOC( char *, 1000 );
    p->pVarsV = ALLOC( char *, 1000 );

    if ( strcmp( pStrU, "@" ) == 0 )
    {
        p->nVarsU = 0;
        printf( "Warning: The list of U variables is empty.\n" );
    }
    else
    {
        pTemp = strtok( pStrU, "," );
        while ( pTemp )
        {
            p->pVarsU[ p->nVarsU++ ] = util_strsav( pTemp );
            pTemp = strtok( NULL, "," );
        }
    }

    if ( strcmp( pStrV, "@" ) == 0 )
    {
        p->nVarsV = 0;
        printf( "Warning: The list of V variables is empty.\n" );
    }
    else
    {
        pTemp = strtok( pStrV, "," );
        while ( pTemp )
        {
            p->pVarsV[ p->nVarsV++ ] = util_strsav( pTemp );
            pTemp = strtok( NULL, "," );
        }
    }

    // Is are the PIs of Fixed, which are not V's
    Ntk_NetworkForEachCi( pNetF, pNodeF )
    {
        if ( pNodeF->Subtype != MV_BELONGS_TO_NET )
            continue;
        // look through the list of Vs
        for ( i = 0; i < p->nVarsV; i++ )
            if ( strcmp( pNodeF->pName, p->pVarsV[i] ) == 0 )
                break;
        if ( i < p->nVarsV ) // found such V
            continue;
        p->pVarsI[ p->nVarsI++ ] = util_strsav( pNodeF->pName );
    }
    // Os are the POs of the spec
    Ntk_NetworkForEachCo( pNetS, pNodeS )
    {
        if ( pNodeS->Subtype != MV_BELONGS_TO_NET )
            continue;
        p->pVarsO[ p->nVarsO++ ] = util_strsav( pNodeS->pName );
    }


    if ( fVerbose )
    {
        printf( "\n" );
        printf( "COMMUNICATION TOPOLOGY:  \n" );
        printf( "   . . . . . . . . .     \n" );
        printf( "   .               .     \n" );
        printf( " I .  -----------  . O   \n" );
        printf( " ---->|    F    |---->   \n" );
        printf( "   .  --|----A---  .     \n" );
        printf( "   .    |    |     .     \n" );
        printf( "   .  U |    | V   . S   \n" );
        printf( "   .  --V----|---  .     \n" );
        printf( "   .  |    X    |  .     \n" );
        printf( "   .  -----------  .     \n" );
        printf( "   . . . . . . . . .     \n" );
        printf( "                         \n" );

        Mvn_PrintNetworkStats( pNetF, "Fixed network (F)" );
        Mvn_PrintNetworkStats( pNetS, "Spec network (S)" );

        Mvn_PrintVarRange( p->pVarsI, p->nVarsI, "I" );
        Mvn_PrintVarRange( p->pVarsO, p->nVarsO, "O" );

        Mvn_PrintVarRange( p->pVarsU, p->nVarsU, "U" );
        Mvn_PrintVarRange( p->pVarsV, p->nVarsV, "V" );
        printf( "\n" );
    }


    // if network S has CS or NS vars names the same as network F, rename them
    Ntk_NetworkForEachCo( pNetS, pNodeS )
    {
        if ( pNodeS->Subtype == MV_BELONGS_TO_NET )
            continue;
        pNodeF = Ntk_NetworkFindNodeByName( pNetF, pNodeS->pName );
        if ( pNodeF )
        {
            // rename pNodeS
//            if ( fVerbose )
//            printf( "Warning: Latch inputs of F and S have the same name; renaming \"%s\" in S", pNodeS->pName );
            Ntk_NodeRemoveName( pNodeS );
            Ntk_NodeAssignName( pNodeS, NULL );
//            if ( fVerbose )
//            printf( " to be \"%s\".\n", pNodeS->pName );
        }
    }
    Ntk_NetworkForEachCi( pNetS, pNodeS )
    {
        if ( pNodeS->Subtype == MV_BELONGS_TO_NET )
            continue;
        pNodeF = Ntk_NetworkFindNodeByName( pNetF, pNodeS->pName );
        if ( pNodeF )
        {
            // rename pNodeS
//            if ( fVerbose )
//            printf( "Warning: Latch outputs of F and S have the same name; renaming \"%s\" in S", pNodeS->pName );
            Ntk_NodeRemoveName( pNodeS );
            Ntk_NodeAssignName( pNodeS, NULL );
//            if ( fVerbose )
//            printf( " to be \"%s\".\n", pNodeS->pName );
        }
    }
    // the ordering of fanins may have changed, but it does not matter



    // perform checking of various assumptions:
    // (1) Every PI of S is also be a PI of F
    if ( fVerbose )
    {
        printf( "Checking assumptions about variables in I, V, U, and O:\n" );
//        printf( "(1) Every PI of S is a PI or PO of F " );
        printf( "(1) Every PI of S is a PI of F " );
    }
    Ntk_NetworkForEachCi( pNetS, pNodeS )
    {
        if ( pNodeS->Subtype != MV_BELONGS_TO_NET )
            continue;
        pNodeF = Ntk_NetworkFindNodeByName( pNetF, pNodeS->pName );
        if ( pNodeF == NULL )
        {
            printf( "\nError: PI \"%s\" of S is not in F.\n", pNodeS->pName );
            fError = 1;
        }
//        else if ( pNodeF->Type != MV_NODE_CI && pNodeF->Type != MV_NODE_CO || 
        else if ( pNodeF->Type != MV_NODE_CI || 
                  pNodeF->Subtype != MV_BELONGS_TO_NET )
        {
            printf( "\nError: PI \"%s\" of S is not a PI or PO of F.\n", pNodeS->pName );
            fError = 1;
        }
    }
    if ( fVerbose && fError == 0 )
        printf( " -- true.\n" );

    // (2) Every PO of S is also a PO of F 
    if ( fVerbose )
        printf( "(2) Every PO of S is also a PO of F " );
    Ntk_NetworkForEachCo( pNetS, pNodeS )
    {
        if ( pNodeS->Subtype != MV_BELONGS_TO_NET )
            continue;
        pNodeF = Ntk_NetworkFindNodeByName( pNetF, pNodeS->pName );
        if ( pNodeF == NULL )
        {
            printf( "\nError: PO \"%s\" of S is not in F.\n", pNodeS->pName );
            fError = 1;
        }
        else if ( pNodeF->Type != MV_NODE_CO || pNodeF->Subtype != MV_BELONGS_TO_NET )
        {
            printf( "\nError: PO \"%s\" of S is not a PO of F.\n", pNodeS->pName );
            fError = 1;
        }
    }
    if ( fVerbose && fError == 0 )
        printf( " -- true.\n" );


    // (3) Variables in U are not duplicated
    if ( fVerbose )
        printf( "(3) Variables in U are not duplicated " );
    for ( i = 0; i < p->nVarsU; i++ )
    {
        for ( k = i + 1; k < p->nVarsU; k++ )
        {
            if ( strcmp( p->pVarsU[i], p->pVarsU[k] ) == 0 )
            {
                printf( "\nError: Name \"%s\" appears twice in the U variable list.\n", p->pVarsU[i] );
                fError = 1;
            }
        }
    }
    if ( fVerbose && fError == 0 )
        printf( " -- true.\n" );

    // (4) Variables in V are not duplicated
    if ( fVerbose )
        printf( "(4) Variables in V are not duplicated " );
    for ( i = 0; i < p->nVarsV; i++ )
    {
        for ( k = i + 1; k < p->nVarsV; k++ )
        {
            if ( strcmp( p->pVarsV[i], p->pVarsV[k] ) == 0 )
            {
                printf( "\nError: Name \"%s\" appears twice in the V variable list.\n", p->pVarsV[i] );
                fError = 1;
            }
        }
    }
    if ( fVerbose && fError == 0 )
        printf( " -- true.\n" );

    // (5) Variables in the sets I and V do not overlap
    if ( fVerbose )
        printf( "(5) Variables in the sets I and V do not overlap " );
    for ( i = 0; i < p->nVarsI; i++ )
    for ( k = 0; k < p->nVarsV; k++ )
        if ( strcmp( p->pVarsI[i], p->pVarsV[k] ) == 0 )
        {
            printf( "\nError: Name \"%s\" appears in both I and V.\n", p->pVarsI[i] );
            fError = 1;
        }
    if ( fVerbose && fError == 0 )
        printf( " -- true.\n" );

    // (6) Variables in the set V are PIs of F
    if ( fVerbose )
        printf( "(6) Variables in the set V are PIs of F " );
    for ( i = 0; i < p->nVarsV; i++ )
    {
        pNodeF = Ntk_NetworkFindNodeByName( pNetF, p->pVarsV[i] );
        if ( pNodeF == NULL )
        {
            printf( "\nError: Name \"%s\" in V is not a node of F.\n", p->pVarsV[i] );
            fError = 1;
        }
        else if ( pNodeF->Type != MV_NODE_CI || pNodeF->Subtype != MV_BELONGS_TO_NET )
        {
            printf( "\nError: Name \"%s\" in V is not a PI of F.\n", p->pVarsV[i] );
            fError = 1;
        }
    }
    if ( fVerbose && fError == 0 )
        printf( " -- true.\n" );

    // (7) Variables in the set U are signals of F
    if ( fVerbose )
        printf( "(7) Variables in the set U are signals of F " );
    for ( i = 0; i < p->nVarsU; i++ )
    {
        pNodeF = Ntk_NetworkFindNodeByName( pNetF, p->pVarsU[i] );
        if ( pNodeF == NULL )
        {
            printf( "\nError: Name \"%s\" in U is not a node of F.\n", p->pVarsU[i] );
            fError = 1;
        }
    }
    if ( fVerbose && fError == 0 )
        printf( " -- true.\n" );

/*
    if ( fVerbose )
        printf( "(7) Variables in the sets Us and Os do not overlap " );
    for ( i = 0; i < p->nVarsU; i++ )
    for ( k = 0; k < p->nVarsO; k++ )
        if ( strcmp( p->pVarsU[i], p->pVarsO[k] ) == 0 )
        {
            printf( "\nError: Name \"%s\" appears in both U and O.\n", p->pVarsU[i] );
            fError = 1;
        }
    if ( fVerbose && fError == 0 )
        printf( " -- true.\n" );
*/
/*
    if ( fVerbose )
        printf( "(8) Variables in the set Us are POs or latch outputs of F " );
    for ( i = 0; i < p->nVarsU; i++ )
    {
        pNodeF = Ntk_NetworkFindNodeByName( pNetF, p->pVarsU[i] );
        if ( pNodeF == NULL )
        {
            printf( "\nError: Name \"%s\" in U is not a node of F.\n", p->pVarsU[i] );
            fError = 1;
        }
        else if ( pNodeF->Type != MV_NODE_CO )
        {
            printf( "\nError: Name \"%s\" in U is not a PO or latch of F.\n", p->pVarsU[i] );
            fError = 1;
        }
    }
    if ( fVerbose && fError == 0 )
        printf( " -- true.\n" );
*/

    if ( fError )
    {
        Mvn_NetworkSolveClean( p );
        p = NULL;
    }
    else
    {
        if ( fVerbose )
            printf( "\nSolving the problem...\n" );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_PrintNetworkStats( Ntk_Network_t * pNet, char * pStr )
{
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pNode;
    int nLatches;
    nLatches = Ntk_NetworkReadLatchNum( pNet );
    printf( "%s: %d primary inputs, %d primary outputs, %d latches\n",
        pStr, Ntk_NetworkReadCiNum(pNet) - nLatches, 
        Ntk_NetworkReadCoNum(pNet) - nLatches, nLatches );
    printf( "    PIs: " );
    Ntk_NetworkForEachCi( pNet, pNode )
        if ( pNode->Subtype == MV_BELONGS_TO_NET )
            printf( " %s(%d)", pNode->pName, pNode->nValues );
    printf( "\n" );
    printf( "    POs: " );
    Ntk_NetworkForEachCo( pNet, pNode )
        if ( pNode->Subtype == MV_BELONGS_TO_NET )
            printf( " %s(%d)", pNode->pName, pNode->nValues );
    printf( "\n" );
    printf( "    Latches: " );
    Ntk_NetworkForEachLatch( pNet, pLatch )
        printf( " [%s,%s](%d)", pLatch->pInput->pName, pLatch->pOutput->pName, pLatch->pOutput->nValues );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_PrintVarRange( char ** pStrs, int nStrs, char * pDescr )
{
    int i;
    printf( "%s variables: {", pDescr );
    for ( i = 0; i < nStrs; i++ )
        printf( " %s", pStrs[i] );
    printf( " }\n" );

}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_CreateVariables( Mvn_Solve_t * p )
{
    Ntk_Node_t * pNode;
    int * pValues;
    int v, iVar;
    int nBits;
    int fVerbose = 0;

    // the ordering of variables should be this:
    // u -> v -> all other varibles (i, o, cs(F), cs(S), ns(F), ns(S))
    // putting u and v in front allows us to use the same var map
    // when computing the SOPs of the conditions in the resulting automaton

    pValues = ALLOC( int, 1000 );
    p->tName2Num  = st_init_table(strcmp, st_strhash);

    // get the uv variables first
    iVar = 0;
    for ( v = 0; v < p->nVarsU; v++ )
    {
        pNode = Ntk_NetworkFindNodeByName( p->pNetF, p->pVarsU[v] );
        assert( pNode );
        pValues[iVar] = pNode->nValues;
        // do not add the U variable into the table!!!
if ( fVerbose )
printf( "VarU  %2d : %s\n", iVar, pNode->pName );
        iVar++;
    }
    for ( v = 0; v < p->nVarsV; v++ )
    {
        pNode = Ntk_NetworkFindNodeByName( p->pNetF, p->pVarsV[v] );
        assert( pNode );
        pValues[iVar] = pNode->nValues;
        st_insert( p->tName2Num, pNode->pName, (char *)iVar );
if ( fVerbose )
printf( "VarV  %2d : %s\n", iVar, pNode->pName );
        iVar++;
    }

    // add the I and CS variable names of F
    Ntk_NetworkForEachCi( p->pNetF, pNode )
    {
        if ( st_is_member( p->tName2Num, pNode->pName ) )
            continue;
        pValues[iVar] = pNode->nValues;
        st_insert( p->tName2Num, pNode->pName, (char *)iVar );
if ( fVerbose )
{
    if ( pNode->Subtype == MV_BELONGS_TO_NET )
printf( "VarFi  %2d : %s\n", iVar, pNode->pName );
    else
printf( "VarFcs %2d : %s\n", iVar, pNode->pName );
}
        iVar++;
    }
    // add the I and CS variable names of S
    Ntk_NetworkForEachCi( p->pNetS, pNode )
    {
        if ( st_is_member( p->tName2Num, pNode->pName ) )
            continue;
        pValues[iVar] = pNode->nValues;
        st_insert( p->tName2Num, pNode->pName, (char *)iVar );
if ( fVerbose )
{
    if ( pNode->Subtype == MV_BELONGS_TO_NET )
printf( "VarSi  %2d : %s\n", iVar, pNode->pName );
    else
printf( "VarScs %2d : %s\n", iVar, pNode->pName );
}
        iVar++;
    }

    // add the NS variable names of F
    Ntk_NetworkForEachCo( p->pNetF, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_NET ) // skip PO
            continue;
        if ( st_is_member( p->tName2Num, pNode->pName ) )
            continue;
        pValues[iVar] = pNode->nValues;
        st_insert( p->tName2Num, pNode->pName, (char *)iVar );
if ( fVerbose )
printf( "VarFns %2d : %s\n", iVar, pNode->pName );
        iVar++;
    }
    // add the NS variable names of S
    Ntk_NetworkForEachCo( p->pNetS, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_NET ) // skip PO
            continue;
        if ( st_is_member( p->tName2Num, pNode->pName ) )
            continue;
        pValues[iVar] = pNode->nValues;
        st_insert( p->tName2Num, pNode->pName, (char *)iVar );
if ( fVerbose )
printf( "VarSns %2d : %s\n", iVar, pNode->pName );
        iVar++;
    }

    p->pVmx = Vmx_VarMapCreateSimple( Ntk_NetworkReadManVm(p->pNetF), 
        Ntk_NetworkReadManVmx(p->pNetF), iVar, 0, pValues );
    FREE( pValues );

    // start the manager
    nBits = Vmx_VarMapReadBitsNum( p->pVmx );
    p->dd = Cudd_Init( nBits, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_ComputeGlobal( Mvn_Solve_t * p )
{
    Ntk_Node_t * pNode, * pNodeF, * pNodeS;
    DdManager * dd;
    Vmx_VarMap_t * pVmx;
    DdNode ** pbCodes;
    DdNode ** pbFuncs1, ** pbFuncs2;
    int nLatch1, nLatch2;
    int * pValuesFirst;
    int iVar, i, v;

    int fDropInternal = 0;
    int fDynEnable = 1;
    int fLatchOnly = 0;
    int fVerbose = 0;
    int clk;

    dd = p->dd;
    pVmx = p->pVmx;

    Ntk_NetworkSetDdGlo( p->pNetF, dd );
    Ntk_NetworkSetVmxGlo( p->pNetF, pVmx );

    Ntk_NetworkSetDdGlo( p->pNetS, dd );
    Ntk_NetworkSetVmxGlo( p->pNetS, pVmx );

    // get the codes
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx ); 
    pValuesFirst = Vm_VarMapReadValuesFirstArray( Vmx_VarMapReadVm(pVmx) );

    // set the PI variables according to the table
    Ntk_NetworkForEachCi( p->pNetF, pNode )
    {
        if ( !st_lookup( p->tName2Num, pNode->pName, (char **)&iVar ) )
        {
            assert( 0 );
        }
        Ntk_NodeWriteFuncGlob( pNode, pbCodes + pValuesFirst[iVar] );
        Ntk_NodeWriteFuncDd( pNode, dd );
    }
    Ntk_NetworkForEachCi( p->pNetS, pNode )
    {
        if ( !st_lookup( p->tName2Num, pNode->pName, (char **)&iVar ) )
        {
            assert( 0 );
        }
        Ntk_NodeWriteFuncGlob( pNode, pbCodes + pValuesFirst[iVar] );
        Ntk_NodeWriteFuncDd( pNode, dd );
    }

    // dereference the codes
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );

clk = clock();
    // compute the global functions of both networks
    Ntk_NetworkGlobalComputeOne( p->pNetF, 0, fDropInternal, fDynEnable, fLatchOnly, fVerbose );
    Ntk_NetworkGlobalComputeOne( p->pNetS, 0, fDropInternal, fDynEnable, fLatchOnly, fVerbose );
//PRT( "Global functions", clock() - clk );

clk = clock();
    // reorder one more time
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
    Cudd_AutodynDisable( dd );
//PRT( "Variable reordering", clock() - clk );


clk = clock();
    // now, shuffle the manager to have the variables in U aligned close to 
    // the corresponding variables in I and CSF
    Mvn_ComputeGlobalShuffle( p );
//PRT( "Variable shuffling", clock() - clk );

    // get the codes
    pbCodes = Vmx_VarMapEncodeMap( dd, pVmx ); 

//    int            nPartsO;      // the output partitions (OFj = OSj)
//    DdNode **      pbPartsO;     
clk = clock();
    // derive the PO partitions
    p->nPartsO  = p->nVarsO;
    p->pbPartsO = ALLOC( DdNode *, p->nPartsO );
    for ( i = 0; i < p->nVarsO; i++ )
    {
        pNodeF = Ntk_NetworkFindNodeByName( p->pNetF, p->pVarsO[i] );
        assert( pNodeF ); /* corretta invece di assert( pNodeS ); */
        pNodeS = Ntk_NetworkFindNodeByName( p->pNetS, p->pVarsO[i] );
        assert( pNodeS ); /* corretta invece di assert( pNodeF ); */
        pbFuncs1 = Ntk_NodeReadFuncGlob(pNodeF);
        pbFuncs2 = Ntk_NodeReadFuncGlob(pNodeS);
        p->pbPartsO[i] = Ntk_NodeGlobalImplyFuncs( dd, pbFuncs1, pbFuncs2, pNodeS->nValues );  
//        p->pbPartsO[i] = Ntk_NodeGlobalConvolveFuncs( dd, pbFuncs1, pbFuncs2, pNodeS->nValues );  
//        p->pbPartsO[i] = Cudd_bddAnd( dd, pbFuncs1[1], pbFuncs2[1] );
        Cudd_Ref( p->pbPartsO[i] );
    }
//PRT( "Output partitions", clock() - clk );

//    int            nPartsT;      // the transition partitions (TF, TS, U)
//    DdNode **      pbPartsT;
clk = clock();
    // derive the transition partitions
    nLatch1 = Ntk_NetworkReadLatchNum( p->pNetF );
    nLatch2 = Ntk_NetworkReadLatchNum( p->pNetS );
    p->nPartsT  = nLatch1 + nLatch2 + p->nVarsU;
    p->pbPartsT = ALLOC( DdNode *, p->nPartsT + p->nVarsO );
    i = 0;
    // derive the U partitions
    for ( v = 0; v < p->nVarsU; v++ )
    {
        pNodeF = Ntk_NetworkFindNodeByName( p->pNetF, p->pVarsU[v] );
        assert( pNodeF );
        // get the functions
        pbFuncs1 = Ntk_NodeReadFuncGlob(pNodeF);
        // get the starting code
        // the v-th U variable is ordered as number v in the variable map
        iVar = v;
        // create the partition
        p->pbPartsT[i] = Ntk_NodeGlobalConvolveFuncs( dd, 
            pbFuncs1, pbCodes + pValuesFirst[iVar], pNodeF->nValues );  
        Cudd_Ref( p->pbPartsT[i] );
        i++;
    }
    Ntk_NetworkForEachCo( p->pNetS, pNodeS )
    {
        if ( pNodeS->Subtype == MV_BELONGS_TO_NET )
            continue;
        // get the functions
        pbFuncs1 = Ntk_NodeReadFuncGlob(pNodeS);
        // get the starting code
        if ( !st_lookup( p->tName2Num, pNodeS->pName, (char **)&iVar ) )
        {
            assert( 0 );
        }
        // create the partition
        p->pbPartsT[i] = Ntk_NodeGlobalConvolveFuncs( dd, 
            pbFuncs1, pbCodes + pValuesFirst[iVar], pNodeS->nValues );  
        Cudd_Ref( p->pbPartsT[i] );
        i++;
    }
    Ntk_NetworkForEachCo( p->pNetF, pNodeF )
    {
        if ( pNodeF->Subtype == MV_BELONGS_TO_NET )
            continue;
        // get the functions
        pbFuncs1 = Ntk_NodeReadFuncGlob(pNodeF);
        // get the starting code
        if ( !st_lookup( p->tName2Num, pNodeF->pName, (char **)&iVar ) )
        {
            assert( 0 );
        }
        // create the partition
        p->pbPartsT[i] = Ntk_NodeGlobalConvolveFuncs( dd, 
            pbFuncs1, pbCodes + pValuesFirst[iVar], pNodeF->nValues );  
        Cudd_Ref( p->pbPartsT[i] );
        i++;
    }
    assert( i == p->nPartsT );
//PRT( "Other partitions", clock() - clk );

    // dereference the codes
    Vmx_VarMapEncodeDeref( dd, pVmx, pbCodes );

    // clean the networks 
    Ntk_NetworkGlobalClean( p->pNetF );
    Ntk_NetworkGlobalClean( p->pNetS );
    Ntk_NetworkWriteDdGlo( p->pNetF, NULL );
    Ntk_NetworkWriteDdGlo( p->pNetS, NULL );
    // reorder ?
/*
    if ( fDynEnable )
    {
        // report stats
        if ( p->fVerbose )
        {
        printf( "BDD nodes in the T partitions = %d.\n", Cudd_SharingSize( p->pbPartsT, p->nPartsT ) );
        printf( "BDD nodes in the O partitions = %d.\n", Cudd_SharingSize( p->pbPartsO, p->nPartsO ) );
        printf( "Reordering variables...\n" );
        }
        // reorder the BDDs of partitions one more time
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
        // report stats
        if ( p->fVerbose )
        {
        printf( "BDD nodes in the T partitions = %d.\n", Cudd_SharingSize( p->pbPartsT, p->nPartsT ) );
        printf( "BDD nodes in the O partitions = %d.\n", Cudd_SharingSize( p->pbPartsO, p->nPartsO ) );
        }
    }
*/
}

/**Function*************************************************************

  Synopsis    [Shuffles the manager to be more efficient.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_ComputeGlobalShuffle( Mvn_Solve_t * p )
{
    DdManager * dd = p->dd;
    Ntk_Latch_t * pLatch;
    int * pVarPres;    // for each variable, 1 means that it should remain the same
    int * pVarMapU;    // for each variable, contains the U  var that should be near it
    int * pVarMapNS;   // for each variable, contains the NS var that should be near it
    int * pPermute;    // for each level, contins the variable that should be on this level
    int * pBits, * pBitsFirst, * pBitsOrder;
    int i, v, lev, iVar, iMvVar, iBinVarToMove;
    int iNsVar, iCsVar;

    pBits      = Vmx_VarMapReadBits( p->pVmx );
    pBitsFirst = Vmx_VarMapReadBitsFirst( p->pVmx );
    pBitsOrder = Vmx_VarMapReadBitsOrder( p->pVmx );

    pPermute  = ALLOC( int, dd->size );
    pVarPres  = ALLOC( int, dd->size );
    pVarMapU  = ALLOC( int, dd->size );
    pVarMapNS = ALLOC( int, dd->size );
    for ( i = 0; i < dd->size; i++ )
    {
        pVarMapU[i] = pVarMapNS[i] = -1; // assume that all vars have no corresponding ones
        pVarPres[i] = 1;                 // assume that all vars are not moved
    }

//printf( "Before shuffling:\n" );
//for ( i = 0; i < dd->size; i++ )
//    printf( "Level %2d: Var %2d.\n", i, dd->invperm[i] );

    // go though the U variables, find the corresponding I or CSF variables 
    for ( v = 0; v < p->nVarsU; v++ )
    {
        if ( !st_lookup( p->tName2Num, p->pVarsU[v], (char **)&iMvVar ) )
            continue;
        // we found the corresponding MV var (iMvVar)
        assert( pBits[iMvVar] == pBits[v] );
        // map the U vars to be close
        for ( i = 0; i < pBits[iMvVar]; i++ )
        {
            // mark the var as the one that should be gone
            iBinVarToMove = pBitsOrder[pBitsFirst[v] + i];
            assert( pVarPres[iBinVarToMove] == 1 );
            pVarPres[iBinVarToMove] = 0;
            // schedule the move
            pVarMapU[ pBitsOrder[pBitsFirst[iMvVar] + i] ] = iBinVarToMove;
        }
    }

    // go through the NS variables that find the corresponding CS variables
    Ntk_NetworkForEachLatch( p->pNetF, pLatch )
    {
        // find the number of latch input (NS) and latch output (CS) var
        if ( !st_lookup( p->tName2Num, pLatch->pOutput->pName, (char **)&iCsVar ) )
        {
            assert( 0 );
        }
        if ( !st_lookup( p->tName2Num, pLatch->pInput->pName, (char **)&iNsVar ) )
        {
            assert( 0 );
        }
        // we found the corresponding MV var (iMvVar)
        assert( pBits[iCsVar] == pBits[iNsVar] );
        // map the NS vars to be close
        for ( i = 0; i < pBits[iCsVar]; i++ )
        {
            // mark the var as the one that should be gone
            iBinVarToMove = pBitsOrder[pBitsFirst[iNsVar] + i];
            assert( pVarPres[iBinVarToMove] == 1 );
            pVarPres[iBinVarToMove] = 0;
            // schedule the move
            pVarMapNS[ pBitsOrder[pBitsFirst[iCsVar] + i] ] = iBinVarToMove;
        }
    }

    // go through the NS variables that find the corresponding CS variables
    Ntk_NetworkForEachLatch( p->pNetS, pLatch )
    {
        // find the number of latch input (NS) and latch output (CS) var
        if ( !st_lookup( p->tName2Num, pLatch->pOutput->pName, (char **)&iCsVar ) )
        {
            assert( 0 );
        }
        if ( !st_lookup( p->tName2Num, pLatch->pInput->pName, (char **)&iNsVar ) )
        {
            assert( 0 );
        }
        // we found the corresponding MV var (iMvVar)
        assert( pBits[iCsVar] == pBits[iNsVar] );
        // map the NS vars to be close
        for ( i = 0; i < pBits[iCsVar]; i++ )
        {
            // mark the var as the one that should be gone
            iBinVarToMove = pBitsOrder[pBitsFirst[iNsVar] + i];
            assert( pVarPres[iBinVarToMove] == 1 );
            pVarPres[iBinVarToMove] = 0;
            // schedule the move
            pVarMapNS[ pBitsOrder[pBitsFirst[iCsVar] + i] ] = iBinVarToMove;
        }
    }

    // create the permutation array
    iVar = 0;
    for ( lev = 0; lev < dd->size; lev++ )
    {
        // get the variable number on this level
        i = dd->invperm[lev];
        if ( pVarPres[i] )
        {
            // place this var in the order
            pPermute[iVar++] = i;
            // place the corresponding NS var
            if ( pVarMapNS[i] >= 0 ) 
                pPermute[iVar++] = pVarMapNS[i];
            // place the corresponding U var
            if ( pVarMapU[i] >= 0 ) 
                pPermute[iVar++] = pVarMapU[i];
        }
    }
    assert( iVar == dd->size );


    // perform the shuffling
    Cudd_ShuffleHeap( dd, pPermute );
    FREE( pPermute );
    FREE( pVarPres );
    FREE( pVarMapU );
    FREE( pVarMapNS );

//printf( "After shuffling:\n" );
//for ( i = 0; i < dd->size; i++ )
//    printf( "Level %2d: Var %2d.\n", i, dd->invperm[i] );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_ComputeStart( Mvn_Solve_t * p )
{
    DdManager * dd;
    Vmx_VarMap_t * pVmxExt;
    int * pCodes, * pVarsCs, * pVarsNs, * pVars;
    int nLatches, i;

    // create the codes
    dd      = p->dd;
    pVmxExt = p->pVmx;

    // collect the CS and NS var numbers and the reset values
    pVarsCs = ALLOC( int, dd->size );
    pVarsNs = ALLOC( int, dd->size );
    pCodes  = ALLOC( int, dd->size );
    Mvn_NetworkSolveVarsAndResetValues( p, pVarsCs, pVarsNs, pCodes );

    // create the replacement variable map for the CUDD package
    nLatches = Ntk_NetworkReadLatchNum(p->pNetF) + Ntk_NetworkReadLatchNum(p->pNetS);
    Vmx_VarMapRemapSetup( dd, pVmxExt, pVarsCs, pVarsNs, nLatches );

    // get the initial state
    p->bInit = Vmx_VarMapCodeCube( dd, pVmxExt, pVarsCs, pCodes, nLatches );  Cudd_Ref( p->bInit );
    // get the CS cube
    p->bCubeCs = Vmx_VarMapCharCubeArray( dd, pVmxExt, pVarsCs, nLatches );   Cudd_Ref( p->bCubeCs );
    // get the NS cube
    p->bCubeNs = Cudd_bddVarMap( dd, p->bCubeCs );                            Cudd_Ref( p->bCubeNs );
    FREE( pVarsCs );
    FREE( pVarsNs );
    FREE( pCodes );

    // get the I cube
    pVars = ALLOC( int, dd->size );
    for ( i = 0; i < p->nVarsI; i++ )
        if ( !st_lookup( p->tName2Num, p->pVarsI[i], (char **)&(pVars[i]) ) )
        {
            assert( 0 );
        }
    p->bCubeI = Vmx_VarMapCharCubeArray( dd, pVmxExt, pVars, p->nVarsI );     Cudd_Ref( p->bCubeI );

    // get the U cube
    // the i-th U variables is ordered number i in the map
    for ( i = 0; i < p->nVarsU; i++ )
        pVars[i] = i;
    p->bCubeU = Vmx_VarMapCharCubeArray( dd, pVmxExt, pVars, p->nVarsU );     Cudd_Ref( p->bCubeU );

    // get the V cube
    for ( i = 0; i < p->nVarsV; i++ )
        if ( !st_lookup( p->tName2Num, p->pVarsV[i], (char **)&(pVars[i]) ) )
        {
            assert( 0 );
        }
    p->bCubeV = Vmx_VarMapCharCubeArray( dd, pVmxExt, pVars, p->nVarsV );     Cudd_Ref( p->bCubeV );
    FREE( pVars );
}

/**Function*************************************************************

  Synopsis    [Collects CS and NS variables and latch reset values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_NetworkSolveVarsAndResetValues( Mvn_Solve_t * p, int * pVarsCs, int * pVarsNs, int * pCodes )
{
    Ntk_Latch_t * pLatch;
    int iLatch, iVar;
    
    // go through the latches of S
    iLatch = 0;
    Ntk_NetworkForEachLatch( p->pNetS, pLatch )
    {
        // find the number of latch-output variable
        if ( !st_lookup( p->tName2Num, pLatch->pOutput->pName, (char **)&iVar ) )
        {
            assert( 0 );
        }
        pVarsCs[ iLatch ] = iVar;
        // find the number of latch-input variable
        if ( !st_lookup( p->tName2Num, pLatch->pInput->pName, (char **)&iVar ) )
        {
            assert( 0 );
        }
        pVarsNs[ iLatch ] = iVar;
        // add the latch reset value
        pCodes[iLatch] = Ntk_LatchReadReset( pLatch );
        iLatch++;
    }
    
    // go through the latches of F
    Ntk_NetworkForEachLatch( p->pNetF, pLatch )
    {
        // find the number of latch-output variable
        if ( !st_lookup( p->tName2Num, pLatch->pOutput->pName, (char **)&iVar ) )
        {
            assert( 0 );
        }
        pVarsCs[ iLatch ] = iVar;
        // find the number of latch-input variable
        if ( !st_lookup( p->tName2Num, pLatch->pInput->pName, (char **)&iVar ) )
        {
            assert( 0 );
        }
        pVarsNs[ iLatch ] = iVar;
        // add the latch reset value
        pCodes[iLatch] = Ntk_LatchReadReset( pLatch );
        iLatch++;
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_ComputeSchedule( Mvn_Solve_t * p )
{
    DdManager * dd = p->dd;
    DdNode * bCubeUn, * bTemp;
    DdNode ** pbVarsUn;
    int nVarsUn, i;
//    int clk;

//if ( p->fVerbose )
if ( 0 )
{
PRB( dd, p->bCubeU );
PRB( dd, p->bCubeV );
PRB( dd, p->bCubeI );
PRB( dd, p->bCubeCs );
PRB( dd, p->bCubeNs );
PRB( dd, p->bInit );
}

    // the unquantifiable vars of both images are: V + U + NS
    // the quantifiable vars are: CS and I
    bCubeUn = Cudd_bddAnd( dd, p->bCubeU, p->bCubeV );         Cudd_Ref( bCubeUn );
//    bCubeUn = Cudd_bddAnd( dd, bTemp = bCubeUn, p->bCubeI );   Cudd_Ref( bCubeUn );
//    Cudd_RecursiveDeref( dd, bTemp );
    bCubeUn = Cudd_bddAnd( dd, bTemp = bCubeUn, p->bCubeNs );  Cudd_Ref( bCubeUn );
    Cudd_RecursiveDeref( dd, bTemp );
//PRB( dd, p->bCubeCs );
//PRB( dd, bCubeUn );
    // get the array of these vars
    pbVarsUn = Extra_bddComputeVarArray( dd, bCubeUn, &nVarsUn ); 
    Cudd_RecursiveDeref( dd, bCubeUn );

    // set up the transition image tree
//    p->pTreeT = Extra_bddImageStart( dd, p->bCubeCs, p->nPartsT, p->pbPartsT, nVarsUn, pbVarsUn, 0 );
//    p->pTreeT = Extra_bddImageStart( dd, p->bCubeCs, p->nPartsT + p->nPartsO, p->pbPartsT, nVarsUn, pbVarsUn, 0 );
//    p->pTreeT = Extra_bddImageStart2( dd, p->bCubeCs, p->nPartsT, p->pbPartsT, nVarsUn, pbVarsUn, 0 );
//PRT( "Scheduling transition image", clock() - clk );

    ////////////////////////////////////////////////////////
    if ( p->fUseOinT )
    {
        // add the output partitions
        for ( i = 0; i < p->nPartsO; i++ )
            p->pbPartsT[p->nPartsT+i] = p->pbPartsO[i];
        p->pTreeT = Extra_bddImageStart( dd, p->bCubeCs, p->nPartsT + p->nPartsO, p->pbPartsT, nVarsUn, pbVarsUn, 0 );
    }
    else
        p->pTreeT = Extra_bddImageStart( dd, p->bCubeCs, p->nPartsT, p->pbPartsT, nVarsUn, pbVarsUn, 0 );
    ////////////////////////////////////////////////////////

    // deref all transition partitions, except the first U partitions
    Ntk_NodeGlobalDerefFuncs( p->dd, p->pbPartsT + p->nVarsU, p->nPartsT - p->nVarsU );

    // iteratively create the image trees for each output
    p->ppTreeOs = ALLOC( Extra_ImageTree_t *, p->nPartsO );
//    p->ppTreeOs = ALLOC( Extra_ImageTree2_t *, p->nPartsO );
    for ( i = 0; i < p->nPartsO; i++ )
    {
        // add the output partition
        p->pbPartsT[p->nVarsU] = Cudd_Not(p->pbPartsO[i]);
        // set up the image
        p->ppTreeOs[i] = Extra_bddImageStart( dd, p->bCubeCs, p->nVarsU + 1, p->pbPartsT, nVarsUn, pbVarsUn, 0 );
//        p->ppTreeOs[i] = Extra_bddImageStart2( dd, p->bCubeCs, p->nVarsU + 1, p->pbPartsT, nVarsUn, pbVarsUn, 0 );
    }
    FREE( pbVarsUn );
//PRT( "Scheduling output images", clock() - clk );
    // deref the remaining partitions
    Ntk_NodeGlobalDerefFuncs( p->dd, p->pbPartsT, p->nVarsU );
    Ntk_NodeGlobalDerefFuncs( p->dd, p->pbPartsO, p->nVarsO );
    FREE( p->pbPartsT );
    FREE( p->pbPartsO );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Auto_t * Mvn_ComputeAutomaton( Mvn_Solve_t * p, int nStateLimit, int fUseLongNames, int fMoore )
{
    Ntk_Node_t * pNode;
    Aut_Man_t * pMan;
    Aut_Auto_t * pAut;
//    Vm_VarMap_t * pVm;
//    Vmx_VarMap_t * pVmxIO;
    DdManager * dd = p->dd;
    ProgressBar * pProgress;
    Aut_State_t * pState, * pS, * pStateDc;
    Aut_State_t ** pEntry;
    Aut_Trans_t * pTrans;
    st_table * tSubset2Num;
    char Buffer[100];
    DdNode * bTrans, * bMint, * bSubset, * bCond, * bTemp, * bCubeUV;
    DdNode * bFormP, * bFormP2, * bFormCuv, * bImage, * bStateSubset;
    int nDigits, nBitsIO, nBitsO, s, i;
//    int * pBitsFirst, * pBitsOrder, * pBitsOrderR;
    int clkIm = 0, clkForm = 0, clkDet = 0, clkSop = 0;
    int clk;
    Aut_Var_t ** ppVars;

    // get the parameters
    nBitsIO = Vmx_VarMapBitsNumRange( p->pVmx, 0, p->nVarsU + p->nVarsV );
    nBitsO  = Vmx_VarMapBitsNumRange( p->pVmx, p->nVarsU, p->nVarsV );
    bCubeUV = Cudd_bddAnd( dd, p->bCubeU, p->bCubeV );     Cudd_Ref( bCubeUV );

    // introduce ZDD variables
    Cudd_zddVarsFromBddVars( dd, 2 );

    // start the new automaton
//    pAut = Aut_AutoAlloc();
    pMan = Aut_ManCreate( dd );
    pAut = Aut_AutoAlloc( p->pVmx, pMan );
//    pAut->pName        = util_strsav( "sol" );
    Aut_AutoSetName( pAut, util_strsav( "csf" ) );
//    pAut->pMan         = p->pNetF->pMan;
//    pAut->nVars      = nBitsIO;
    Aut_AutoSetVarNum( pAut, p->nVarsU + p->nVarsV );

//    pAut->nOutputs     = 0;
//    pAut->nStates      = 0;
//    pAut->nStatesAlloc = 1000;
//    pAut->pStates      = ALLOC( Aut_State_t *, pAut->nStatesAlloc );

//    pAut->ppNamesInput  = ALLOC( char *, pAut->nVars );
//    pAut->ppNamesOutput = NULL;
/*
    // assign variable names
    pBits = Vmx_VarMapReadBits( p->pVmx );
    iBit = 0;
    for ( i = 0; i < p->nVarsU; i++ )
    {
        if ( pBits[i] == 1 )
        {
            pAut->ppNamesInput[iBit++] = util_strsav( p->pVarsU[i] );
            continue;
        }
        for ( k = 0; k < pBits[i]; k++ )
        {
            sprintf( Buffer, "%s_%d", p->pVarsU[i], pBits[i] - 1 - k );
            pAut->ppNamesInput[iBit++] = util_strsav( Buffer );
        }
    }
    for ( i = 0; i < p->nVarsV; i++ )
    {
        if ( pBits[p->nVarsU + i] == 1 )
        {
            pAut->ppNamesInput[iBit++] = util_strsav( p->pVarsV[i] );
            continue;
        }
        for ( k = 0; k < pBits[p->nVarsU + i]; k++ )
        {
            sprintf( Buffer, "%s_%d", p->pVarsV[i], pBits[p->nVarsU + i] - 1 - k );
            pAut->ppNamesInput[iBit++] = util_strsav( Buffer );
        }
    }
    assert( iBit == pAut->nVars );
*/
    // set up the variables
    ppVars = Aut_AutoReadVars( pAut );
    for ( i = 0; i < p->nVarsU; i++ )
    {
        pNode = Ntk_NetworkFindNodeByName(p->pNetF, p->pVarsU[i]);
        ppVars[i] = Mvn_NetworkSolveGetVar(pNode);
    }
    for ( i = 0; i < p->nVarsV; i++ )
    {
        pNode = Ntk_NetworkFindNodeByName(p->pNetF, p->pVarsV[i]);
        ppVars[p->nVarsU + i] = Mvn_NetworkSolveGetVar(pNode);
    }

/*
    // create the map to be used for the computation of SOP covers
//    pVmxIO = Vmx_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), Fnc_ManagerReadManVmx(pAut->pMan), pAut->nVars, 0 );
    // reverse the ordering of the binary bits
    pBits       = Vmx_VarMapReadBits( p->pVmx );
    pBitsFirst  = Vmx_VarMapReadBitsFirst( p->pVmx ); 
    pBitsOrder  = Vmx_VarMapReadBitsOrder( p->pVmx );
    pBitsOrderR = ALLOC( int, pAut->nVars );
    iBit = 0;
    for ( i = 0; i < p->nVarsU; i++ )
        for ( k = 0; k < pBits[i]; k++ )
            pBitsOrderR[iBit++] =  pBitsOrder[pBitsFirst[i] + pBits[i] - 1 - k];
    for ( i = 0; i < p->nVarsV; i++ )
        for ( k = 0; k < pBits[p->nVarsU + i]; k++ )
            pBitsOrderR[iBit++] =  pBitsOrder[pBitsFirst[p->nVarsU + i] + pBits[p->nVarsU + i] - 1 - k];
    assert( iBit == pAut->nVars );
    pVm = Vm_VarMapCreateBinary( Fnc_ManagerReadManVm(pAut->pMan), pAut->nVars, 0 );
    pVmxIO = Vmx_VarMapLookup( Fnc_ManagerReadManVmx(pAut->pMan), pVm, pAut->nVars, pBitsOrderR );
    FREE( pBitsOrderR );
*/



    // create the initial state
//    pState = Aut_AutoStateAlloc();
//    pState->fAcc = 1;
//    pState->bSubset = p->bInit;           Cudd_Ref( pState->bSubset ); 
    // add this state to the set of states
//    pAut->pStates[ pAut->nStates++ ] = pState;

    // create the initial state
    pState = Aut_StateAlloc( pAut );
    Aut_StateSetAcc( pState, 1 );
    Aut_StateSetData( pState, (char *)p->bInit ); Cudd_Ref( p->bInit ); 
    Aut_AutoStateAddLast( pState );
    // set the initial state
    Aut_AutoSetInitNum( pAut, 1 );
    Aut_AutoSetInit( pAut, pState );

    // start the table to collect the reachable state subsets
    tSubset2Num = st_init_table(st_ptrcmp,st_ptrhash);
//    st_insert( tSubset2Num, (char *)pState->bSubset, (char *)0 );
    st_insert( tSubset2Num, (char *)p->bInit, (char *)pState );

/*
    // create the non-accepting DC state
    pState = Aut_AutoStateAlloc();
    pState->fAcc = 0;
    pState->fMark = fMoore;
    pState->bSubset = b1;                 Cudd_Ref( pState->bSubset ); 
    // add the transition to itself
    pTrans = Aut_AutoTransAlloc();
    pTrans->pCond = Mvc_CoverAlloc( Fnc_ManagerReadManMvc(pAut->pMan), 2 * nBitsIO );
    Mvc_CoverMakeTautology( pTrans->pCond );
    pTrans->StateCur  = 1;
    pTrans->StateNext = 1;
    Aut_AutoTransAdd( pState, pTrans );
    // add this state to the set of states
    pAut->pStates[ pAut->nStates++ ] = pState;
    // do not add this state to the table
*/

    // create the non-accepting DC state
    pStateDc = Aut_AutoCreateDcState( pAut );
    // add the DC state
    Aut_AutoStateAddLast( pStateDc );

    
    //  The next state subsets reachable from state subset S(cs)
    //  P(u,v,ns) = Exist i,cs [ U(i,v,csF,u) & TF(i,v,csF,nsF) & TS(i,csS,nsS) & O(i,v,cs) & S(cs) ]

    //  The condition under which we transit into the accepting state subsets
    //  P2(u,v) = ForAll i,cs [ U(i,v,csF,u) => [ S(cs) => O(i,v,cs) ] ], or
    //  P2(u,v) = !Exist i,cs [ U(i,v,csF,u) & S(cs) & NOT[O(i,v,cs)] ],    
    //      where O(i,v,cs) = Prod j [ OFj(i,v,csF) => OSj(i,csS) ]
    //      is the conformance condition for outputs of F and S

    //  The transition into the non-accepting DC state happens when
    //  C(u,v) = NOT[ P2(u,v) ]

    //  The remaining transitions into the accepting state subsets
    //  T(u,v,ns) = P(u,v,ns) & P2(u,v)


    // iteratively process the subsets
//    if ( !p->fVerbose )
        pProgress = Extra_ProgressBarStart( stdout, 1000 );
//    for ( s = 0; s < pAut->nStates; s++ )
    s = 0;
    Aut_AutoForEachState( pAut, pState )
    {
        // skip the non-accepting DC state
        if ( s == 1 )
        {
            s++;
            continue;
        }
        if ( s == nStateLimit )
        {
            printf( "The limit on the number of states (%d) is reached; computation aborted.\n", nStateLimit );
            Aut_AutoFree( pAut );   pAut = NULL;
            Cudd_Quit( dd );
            p->dd = NULL;
            goto FINISH;
        }
        // get the state
//        pState = pAut->pStates[s];
        bStateSubset = (DdNode *)Aut_StateReadData(pState);

        // compute the formulas 

        // get the sum of images for P2
        bFormP2 = b0;   Cudd_Ref( bFormP2 );
        for ( i = 0; i < p->nPartsO; i++ )
        {
clk = clock();
            bImage = Extra_bddImageCompute( p->ppTreeOs[i], bStateSubset );   
//            bImage = Extra_bddImageCompute2( p->ppTreeOs[i], bStateSubset );   
clkIm += clock() - clk;
            if ( bImage == NULL )
            {
                Aut_AutoFree( pAut );   pAut = NULL;
                goto FINISH;
            }
            Cudd_Ref( bImage );
            // add this image to the result
            bFormP2 = Cudd_bddOr( dd, bTemp = bFormP2, bImage );  Cudd_Ref( bFormP2 );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bImage );
        }
        bFormP2 = Cudd_Not(bFormP2);
clk = clock();

        // impose the Moore restriction
        if ( fMoore )
        {
            bFormP2 = Cudd_bddUnivAbstract( dd, bTemp = bFormP2, p->bCubeU );  Cudd_Ref( bFormP2 );
            Cudd_RecursiveDeref( dd, bTemp );
            if ( bFormP2 == b0 ) // not a Moore state
                Aut_StateSetAcc( pState, 0 );
        }


        // get the transitions for this subset in term of NS vars
clk = clock();
        bFormP = Extra_bddImageCompute( p->pTreeT, bStateSubset ); 
//        bFormP = Extra_bddImageCompute2( p->pTreeT, bStateSubset ); 
clkIm += clock() - clk;
        if ( bFormP == NULL )
        {
            Aut_AutoFree( pAut );   pAut = NULL;
            goto FINISH;
        }
        Cudd_Ref( bFormP );


        // compute the condition of transition into the non-accepting DC state
//        bFormCuv = Cudd_bddAndAbstract( dd, bFormP1, Cudd_Not(bFormP2), p->bCubeI ); Cudd_Ref( bFormCuv );
//        bFormCuv = Cudd_bddExistAbstract( dd, Cudd_Not(bFormP2), p->bCubeI ); Cudd_Ref( bFormCuv );
        bFormCuv = Cudd_Not(bFormP2); Cudd_Ref( bFormCuv );
/*
        // compute the remaining transitions
        bTrans = Cudd_bddAndAbstract( dd, bFormP, bFormP2, p->bCubeI ); Cudd_Ref( bTrans );
        Cudd_RecursiveDeref( dd, bFormP );
        Cudd_RecursiveDeref( dd, bFormP1 );
        Cudd_RecursiveDeref( dd, bFormP2 );

        // remove the area covered by C(u,v)
        bTrans = Cudd_bddAnd( dd, bTemp = bTrans, Cudd_Not(bFormCuv) );    Cudd_Ref( bTrans );
        Cudd_RecursiveDeref( dd, bTemp );
*/
//        bTrans = Cudd_bddAndAbstract( dd, bFormP, Cudd_Not(bFormCuv), p->bCubeI ); Cudd_Ref( bTrans );
        bTrans = Cudd_bddAnd( dd, bFormP, bFormP2 ); Cudd_Ref( bTrans );
        Cudd_RecursiveDeref( dd, bFormP );
//        Cudd_RecursiveDeref( dd, bFormP1 );
        Cudd_RecursiveDeref( dd, bFormP2 );

        // replace the variables, so that the transitions were in terms of CS variables
        bTrans = Cudd_bddVarMap( dd, bTemp = bTrans );    Cudd_Ref( bTrans );
        Cudd_RecursiveDeref( dd, bTemp );

clkForm += clock() - clk;

        // at this point, bFormCuv is the condition of transition into the NA DC state
        // while pTrans are the remaining transitions into the accepting state subsets
        if ( bFormCuv != b0 )
        {
            pTrans = Aut_TransAlloc( pAut );
//clk = clock();
//            pTrans->pCond = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pAut->pMan), 
//                dd, bFormCuv, bFormCuv, pVmxIO, nBitsIO );
//clkSop += clock() - clk;
//            pTrans->StateCur  = s;
//            pTrans->StateNext = 1;
//            Aut_AutoTransAdd( pState, pTrans );
            Aut_TransSetStateFrom( pTrans, pState );
            Aut_TransSetStateTo  ( pTrans, pStateDc );
            Aut_TransSetCond     ( pTrans, bFormCuv );    Cudd_Ref( bFormCuv );
            Aut_AutoAddTransition( pTrans );
        }
        Cudd_RecursiveDeref( dd, bFormCuv );

clk = clock();

        // extract the subsets reachable by certain inputs
        while ( bTrans != b0 )
        {
            // get a minterm in terms of the input vars
            bMint = Extra_bddGetOneMinterm( dd, bTrans, bCubeUV );       Cudd_Ref( bMint );
            // get the state subset in terms of CS vars
            bSubset = Cudd_bddAndAbstract( dd, bTrans, bMint, bCubeUV ); Cudd_Ref( bSubset );
            Cudd_RecursiveDeref( dd, bMint );
            // check if this subset was already visited
            if ( !st_find_or_add( tSubset2Num, (char *)bSubset, (char ***)&pEntry ) )
            { // does not exist - add it to storage
                // realloc storage for states if necessary
//                pS = Aut_AutoStateAlloc();
//                pS->fAcc = 1;
//                pS->bSubset = bSubset;   Cudd_Ref( bSubset );
 //               pAut->pStates[pAut->nStates] = pS;
 //               *pEntry = pAut->nStates;
//                pAut->nStates++;
                pS = Aut_StateAlloc( pAut );
                Aut_StateSetAcc( pS, 1 );
                Aut_StateSetData( pS, (char *)bSubset );   Cudd_Ref( bSubset );
                Aut_AutoStateAddLast( pS );
                *pEntry = pS;
            }

            // get the condition
            bCond = Cudd_bddXorExistAbstract( dd, bTrans, bSubset, p->bCubeCs ); Cudd_Ref( bCond );
            bCond = Cudd_Not( bCond );
            Cudd_RecursiveDeref( dd, bSubset );

            // add the transition
//            pTrans = Aut_AutoTransAlloc();
//            pTrans->pCond = Fnc_FunctionDeriveSopFromMddSpecial( Fnc_ManagerReadManMvc(pAut->pMan), 
//                dd, bCond, bCond, pVmxIO, nBitsIO );
//            pTrans->StateCur  = s;
//            pTrans->StateNext = *pEntry;
            // add the transition
//            Aut_AutoTransAdd( pState, pTrans );

            pTrans = Aut_TransAlloc( pAut );
            Aut_TransSetStateFrom( pTrans, pState );
            Aut_TransSetStateTo  ( pTrans, *pEntry );
            Aut_TransSetCond     ( pTrans, bCond );    Cudd_Ref( bCond );
            Aut_AutoAddTransition( pTrans );

            // reduce the transitions
            bTrans = Cudd_bddAnd( dd, bTemp = bTrans, Cudd_Not( bCond ) ); Cudd_Ref( bTrans );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCond );
        }
        Cudd_RecursiveDeref( dd, bTrans );
clkDet += clock() - clk;

//        if ( !p->fVerbose )
            if ( s && s % 50 == 0 )
            {
                char Buffer[100];
                sprintf( Buffer, "%d states", s );
                Extra_ProgressBarUpdate( pProgress, 1000 * s / pAut->nStates, Buffer );
            }
        s++;
    }
    Cudd_RecursiveDeref( dd, bCubeUV );

clk = clock();

    // set the accepting attributes
//    for ( s = 0; s < pAut->nStates; s++ )
//        pAut->pStates[s]->fAcc = 1;
//    pAut->pStates[1]->fAcc = 0;

    // assign the state names
    if ( fUseLongNames )
        Mvn_SolveDeriveStateNames( p, pAut );
    else
    {
        nDigits = Extra_Base10Log( pAut->nStates );
        s = 0;
        Aut_AutoForEachState( pAut, pState )
        {
            if ( s == 1 )
            {
                s++;
                continue;
            }
            sprintf( Buffer, "s%0*d", nDigits, s );
            Aut_StateSetName( pState, util_strsav( Buffer ) );
            Aut_AutoStateAddToTable( pState );
            s++;
        }
    }

    // go through the states
    Aut_AutoForEachState( pAut, pState )
    {
        bSubset = (DdNode *)Aut_StateReadData(pState);
        if ( bSubset == NULL )
            continue;
        // deref state subsets of all states
        Cudd_RecursiveDeref( dd, bSubset );
        // add the state names to the table
//        st_insert( pAut->tStateNames, pAut->pStates[s]->pName, NULL );
    }

//    if ( !p->fVerbose )
        Extra_ProgressBarStop( pProgress );
        pProgress = NULL;
//PRT( "Assigning names", clock() - clk );
//PRT( "Image computations", clkIm );
//PRT( "Formula computations", clkForm );
//PRT( "Subset exploration", clkForm );
//PRT( "SOP computation", clkSop );

    // the resulting automaton may be completed with the accepting DC state
clk = clock();
    Auti_AutoComplete( pAut, pAut->nVars, 1 );

    // remove the non-accepting DC state
    Aut_AutoStateDelete( pStateDc );
    Aut_StateFree( pStateDc );

//PRT( "Completion with the accepting state", clock() - clk );

FINISH:
//    if ( !p->fVerbose )
    if ( pProgress )
        Extra_ProgressBarStop( pProgress );
    st_free_table( tSubset2Num );
    return pAut;
}


/**Function*************************************************************

  Synopsis    [Derive the long names of the automaton.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvn_SolveDeriveStateNames( Mvn_Solve_t * p, Aut_Auto_t * pAut )
{
    Aut_State_t * pS;
    DdManager * dd = p->dd;
    Vmx_VarMap_t * pVmxExt;
    DdNode * bFunc, * bTemp, * bMint;
    char Name1[200], Name2[200], * pNameCur, * pNamePrev, * pNameTemp;
    char * pNameNew;
    int * pCodes, * pVarsCs, * pVarsNs;
    int nLatches, i;
    int fFirst, s;

    // create the codes
    dd      = p->dd;
    pVmxExt = p->pVmx;
    nLatches = Ntk_NetworkReadLatchNum( p->pNetF ) + Ntk_NetworkReadLatchNum( p->pNetS );

    // collect the CS and NS var numbers and the reset values
    pVarsCs = ALLOC( int, dd->size );
    pVarsNs = ALLOC( int, dd->size );
    pCodes  = ALLOC( int, dd->size );
    Mvn_NetworkSolveVarsAndResetValues( p, pVarsCs, pVarsNs, pCodes );

    // generate the state names and acceptable conditions
    s = 0;
//    for ( s = 0; s < pAut->nStates; s++ )
     Aut_AutoForEachState( pAut, pS )
    {
        if ( s == 1 )
        {
            s++;
//            pAut->pStates[1]->pName = util_strsav( "DC1" );
            Aut_StateSetName( pS, util_strsav( "DC1" ) );
            Aut_AutoStateAddToTable( pS );
            continue;
        }

        // get the state
//        pS = pAut->pStates[s];

        // alloc the name
        pNameNew = ALLOC( char, 2 );
        pNameNew[0] = 0;

        // prepare to print the subset names
        fFirst = 1;
        // go though the components of this subset
        bFunc = (DdNode *)Aut_StateReadData(pS);   Cudd_Ref( bFunc );
        while ( bFunc != b0 )
        { 
            // extract one minterm from the encoded state
            bMint  = Extra_bddGetOneMinterm( dd, bFunc, p->bCubeCs );        Cudd_Ref( bMint );
            // convert this minterm to the code of the corresponding state
            Vmx_VarMapDecodeCube( dd, bMint, pVmxExt, pVarsCs, pCodes, nLatches );
            Cudd_RecursiveDeref( dd, bMint );

            // print the name of this code
            pNameCur  = Name1;
            pNamePrev = Name2;
            pNameCur[0]  = 0;
            pNamePrev[0] = 0;
            for ( i = 0; i < nLatches; i++ )
            {
                // swap the names
                pNameTemp = pNameCur;
                pNameCur  = pNamePrev;
                pNamePrev = pNameTemp;
                // add one more char to the name
                sprintf( pNameCur, "%s%d", pNamePrev, pCodes[i] );
            }

            // add this name to the subset name
            if ( fFirst )
                fFirst = 0;
            else
                strcat( pNameNew, "_" );
            pNamePrev = pNameNew;
            pNameNew = ALLOC( char, strlen(pNamePrev) + strlen(pNameCur) + 3 );
            strcpy( pNameNew, pNamePrev );
            strcat( pNameNew, pNameCur );
            FREE( pNamePrev );

            // convert this code back into the cube
            bMint = Vmx_VarMapCodeCube( dd, pVmxExt, pVarsCs, pCodes, nLatches );  Cudd_Ref( bMint );
            // subtract the minterm from the reachable states
            bFunc = Cudd_bddAnd( dd, bTemp = bFunc, Cudd_Not(bMint) );  Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bMint );
        }
        Cudd_RecursiveDeref( dd, bFunc );
        // set the new name   
        Aut_StateSetName( pS, Aut_AutoRegisterName(pAut, pNameNew) );
        Aut_AutoStateAddToTable( pS );
        FREE( pNameNew );
        s++;
    }

    FREE( pVarsCs );
    FREE( pVarsNs );
    FREE( pCodes );
}

/**Function*************************************************************

  Synopsis    [Derive the variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aut_Var_t * Mvn_NetworkSolveGetVar( Ntk_Node_t * pNode )
{
    Aut_Var_t * pVar;
    Io_Var_t * pIoVar;
    char ** pValueNames;
    int i;
    st_table * tName2Var = (st_table *)pNode->pNet->pData;

    pVar = Aut_VarAlloc();
    Aut_VarSetName( pVar, util_strsav( pNode->pName ) );
    Aut_VarSetValueNum( pVar, pNode->nValues );
    if ( tName2Var && st_lookup( tName2Var, pNode->pName, (char **)&pIoVar ) )
    {
        assert( pNode->nValues == pIoVar->nValues );
        pValueNames = ALLOC( char *, pIoVar->nValues );
        for ( i = 0; i < pIoVar->nValues; i++ )
            pValueNames[i] = util_strsav( pIoVar->pValueNames[i] );
        Aut_VarSetValueNames( pVar, pValueNames );
    }
    return pVar;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


