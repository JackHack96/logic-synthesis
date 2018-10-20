/**CFile****************************************************************

  FileName    [ntkiSymms.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Computation of symmetries using BDDs and S&S.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiSymms.c,v 1.4 2005/07/08 01:01:22 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern unsigned ** Fraig_ManGetTrueSupports( Fraig_Man_t * p, int fExact );
extern char *      Fraig_SymmsComputeUsingSandS( Fraig_Man_t * p, int Out, int nRoundLimit );
//extern int         Fraig_NodeReadSuppSize( Fraig_Man_t * p, Fraig_Node_t * pNode, int fStruct );

static void Ntk_NetworkComputeSymmetriesUsingBdds( Ntk_Network_t * pNet, int fNaive, int fVerbose );
static void Ntk_NetworkComputeSymmetriesUsingSandS( Ntk_Network_t * pNet, int fVerbose, int nRoundLimit );
static void Ntk_NetworkSymmsBdd( Ntk_Network_t * pNet, int fNaive, int fVerbose );
static void Ntk_NetworkSymmsSandS( Ntk_Network_t * pNet, Fraig_Man_t * pMan, int fVerbose, int nRoundLimit );
static void Ntk_NetworkSymmsPrint( Ntk_Network_t * pNet, Extra_SymmInfo_t * pSymms );

// stats variables
int clkFra;    // runtime to construct initial FRAIG
int clkSup;    // runtime to determine true support of the PO functions
int clkMov;    // runtime to move PO functions into a new manager
int clkStr;    // runtime of structural symmetry detection
int clkSim;    // runtime of simulation
int clkSat;    // runtime of SAT check
int clkOth;    // runtime of one specific process

int symsTot;   // the number of all variable pairs
int symsSym;   // the number of all symmetric variable pairs
int symsStr;   // the number of (symmetric) pairs proved by structural methods
int symsSim;   // the number of (non-symmetric) pairs proved by simulation
int symsSat;   // the number of (symmetric/non-symmetric) pair processed by SAT
int symsTrn;   // the number of (symmetric/non-symmetric) pairs infered by transitivity

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [The top level procedure to compute functional support.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkComputeSupports( Ntk_Network_t * pNet, int nSimLimit, int fExact, int fVerbose )
{
    Fraig_Man_t * pMan;
    int fFuncRed  = 1;
    int fFeedBack = 1;
    int fDoSparse = 1;
    int fChoicing = 0; 
    int fBalance  = 0; 
    int fVerboseF = 0;
    int clkStart = clock();

    // derive the FRAIG manager containing the network
    pMan = Ntk_NetworkFraig( pNet, fFuncRed, fBalance, fDoSparse, fChoicing, fVerboseF );
    // print the total number of AND nodes
    if ( fVerbose )
        printf( "AND gates = %d.\n", Fraig_ManReadNodeNum(pMan)-Fraig_ManReadInputNum(pMan) );
    // compute true supports of the nodes (without this symm computation will not work)
//    Fraig_ManComputeSupports( pMan, nSimLimit, fExact, fVerbose );
    Fraig_ManGetTrueSupports( pMan, 1 );
    // get rid of the manager
    Fraig_ManFree( pMan );
}


/**Function*************************************************************

  Synopsis    [The top level procedure to compute symmetries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkComputeSymmetries( Ntk_Network_t * pNet, int fUseBdds, int fNaive, int fVerbose, int nRoundLimit )
{
    if ( pNet == NULL )
        return;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only fraig binary networks.\n" );
        return;
    }

    if ( fUseBdds || fNaive )
        Ntk_NetworkComputeSymmetriesUsingBdds( pNet, fNaive, fVerbose );
    else
        Ntk_NetworkComputeSymmetriesUsingSandS( pNet, fVerbose, nRoundLimit );
}



/**Function*************************************************************

  Synopsis    [Symmetry computation using BDDs (both naive and smart).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkComputeSymmetriesUsingBdds( Ntk_Network_t * pNet, int fNaive, int fVerbose )
{
    bool fDynEnable = 1;
    bool fLatchOnly = 0;
    int clk, clkBdd, clkSym;

    // free the old global manager if necessary
    Ntk_NetworkSetDdGlo( pNet, NULL );
    // compute the global functions
clk = clock();
    Ntk_NetworkGlobalComputeCo( pNet, fDynEnable, fLatchOnly, fVerbose );
clkBdd = clock() - clk;
    // create the collapsed network
clk = clock();
    Ntk_NetworkSymmsBdd( pNet, fNaive, fVerbose );
clkSym = clock() - clk;
    // undo the global functions
    Ntk_NetworkGlobalDeref( pNet );

PRT( "Constructing BDDs", clkBdd );
PRT( "Computing symms  ", clkSym );
PRT( "TOTAL            ", clkBdd + clkSym );
}

/**Function*************************************************************

  Synopsis    [Symmetry computation using BDDs (both naive and smart).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSymmsBdd( Ntk_Network_t * pNet, int fNaive, int fVerbose )
{
    Extra_SymmInfo_t * pSymms;
    Ntk_Node_t * pNetNode;
    DdManager * dd;
    DdNode * bFunc;
    int nSymms = 0;

    // collect global functions into the array
    dd = Ntk_NetworkReadDdGlo( pNet );
    Cudd_zddVarsFromBddVars( dd, 2 );
    Cudd_AutodynDisable( dd );
    // compute symmetry info for each PO
    Ntk_NetworkForEachCo( pNet, pNetNode )
    {
        bFunc = (Ntk_NodeReadFuncGlob(pNetNode))[1];
        if ( Cudd_Regular(bFunc) == b1 )
            continue;
        if ( fNaive )
            pSymms = Extra_SymmPairsComputeNaive( dd, bFunc );
        else
            pSymms = Extra_SymmPairsCompute( dd, bFunc );
        nSymms += pSymms->nSymms;
        if ( fVerbose )
        {
            printf( "Output %6s (%d): ", Ntk_NodeGetNamePrintable(pNetNode), pSymms->nSymms );
            Ntk_NetworkSymmsPrint( pNet, pSymms );
        }
//Extra_SymmPairsPrint( pSymms );
        Extra_SymmPairsDissolve( pSymms );
    }
    printf( "The total number of symmetries is %d.\n", nSymms );
}


/**Function*************************************************************

  Synopsis    [Symmetry computation using S&S.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkComputeSymmetriesUsingSandS( Ntk_Network_t * pNet, int fVerbose, int nRoundLimit )
{
    Fraig_Man_t * pMan;
    int fFuncRed  = 1;
    int fFeedBack = 1;
    int fDoSparse = 1;
    int fChoicing = 0; 
    int fBalance  = 0; 
    int fVerboseF = 0;
    int clk, clkStart = clock();

clkFra = 0;
clkSup = 0;
clkMov = 0;
clkStr = 0;
clkSim = 0;
clkSat = 0;
clkOth = 0;

symsTot = 0;
symsSym = 0;
symsStr = 0;
symsSim = 0;
symsSat = 0;
symsTrn = 0;

    // derive the FRAIG manager containing the network
clk = clock();
    pMan = Ntk_NetworkFraig( pNet, fFuncRed, fBalance, fDoSparse, fChoicing, fVerboseF );
clkFra = clock() - clk;
    // compute true supports of the nodes (without this symm computation will not work)
clk = clock();
    Fraig_ManGetTrueSupports( pMan, 1 );
clkSup = clock() - clk;
    // print the total number of AND nodes
    if ( fVerbose )
        printf( "AND gates = %d.\n", Fraig_ManReadNodeNum(pMan)-Fraig_ManReadInputNum(pMan) );
    // compute the symmetries
    Ntk_NetworkSymmsSandS( pNet, pMan, fVerbose, nRoundLimit );
    // get rid of the manager
    Fraig_ManFree( pMan );

// count how many pairs were processed by transitivity
symsTrn = symsTot - symsStr - symsSim - symsSat;
printf( "SYMMS  = %6.2f %%.\n", 100.0 * symsSym/symsTot );
printf( "Struct = %6.2f %%.\n", 100.0 * symsStr/symsTot );
printf( "Sim    = %6.2f %%.\n", 100.0 * symsSim/symsTot );
printf( "Sat    = %6.2f %%.\n", 100.0 * symsSat/symsTot );
printf( "Trans  = %6.2f %%.\n", 100.0 * symsTrn/symsTot );

PRT( "Fraiging  ", clkFra );
PRT( "Support   ", clkSup );
PRT( "Transfer  ", clkMov );
PRT( "Structural", clkStr );
PRT( "Simulation", clkSim );
PRT( "Sat       ", clkSat );
PRT( "Cleanup   ", clkOth );
PRT( "TOTAL     ", clock() - clkStart );
PRT( "TOTAL S&S ", clkFra + clkSup + clkStr + clkSim + clkSat );
}

/**Function*************************************************************

  Synopsis    [Symmetry computation using S&S.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSymmsSandS( Ntk_Network_t * pNet, Fraig_Man_t * pMan, int fVerbose, int nRoundLimit )
{
    Ntk_Node_t * pNetNode;
    Extra_SymmInfo_t * pSymms;
    Fraig_Node_t * pNode;
    int i;//, nSupp;

    i = 0;
    Ntk_NetworkForEachCo( pNet, pNetNode )
    {
        if ( !fVerbose )
        {
//            printf( "." );// fflush( stdout );
        }

        pNode = Fraig_Regular(Fraig_ManReadOutputs(pMan)[i]);
        if ( !Fraig_NodeIsAnd(pNode) )
        {
            if ( fVerbose )
                printf( "Output %6s (%d):\n", Ntk_NodeGetNamePrintable(pNetNode), 0 );
            i++;
            continue;
        }

//        nSupp = Fraig_NodeReadSuppSize( pMan, pNode, 1 );
//        printf( "StructSupp=%d ", nSupp );

//        if ( nSupp > 35 )
//        {
//            printf( "T" );
//            continue;
//        }

        // compute symmetries for this output (use true support info)
        pSymms = (Extra_SymmInfo_t * )Fraig_SymmsComputeUsingSandS( pMan, i, nRoundLimit );
        symsSym += pSymms->nSymms;
        if ( fVerbose )
        {
            printf( "Output %6s (%d): ", Ntk_NodeGetNamePrintable(pNetNode), pSymms->nSymms );
            Ntk_NetworkSymmsPrint( pNet, pSymms );
        }
        Extra_SymmPairsDissolve( pSymms );
        i++;
    }
    printf( "\n" );
    printf( "The total number of symmetries is %d.\n", symsSym );
}

/**Function*************************************************************

  Synopsis    [Printing symmetry groups from the symmetry data structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkSymmsPrint( Ntk_Network_t * pNet, Extra_SymmInfo_t * pSymms )
{
    Ntk_Node_t * pNode;
    char ** pInputNames;
    int * pVarTaken;
    int i, k, nVars, nSize, fStart;

    // get variable names
    nVars = Ntk_NetworkReadCiNum(pNet);
    pInputNames = ALLOC( char *, nVars );
    i = 0; 
    Ntk_NetworkForEachCi( pNet, pNode )
        pInputNames[i++] = util_strsav( Ntk_NodeGetNamePrintable(pNode) );

    // alloc the array of marks
    pVarTaken = ALLOC( int, nVars );
    memset( pVarTaken, 0, sizeof(int) * nVars );

    // print the groups
    fStart = 1;
    nSize = pSymms->nVars;
    for ( i = 0; i < nSize; i++ )
    {
        // skip the variable already considered
        if ( pVarTaken[i] )
            continue;
        // find all the vars symmetric with this one
        for ( k = 0; k < nSize; k++ )
        {
            if ( k == i )
                continue;
            if ( pSymms->pSymms[i][k] == 0 )
                continue;
            // vars i and k are symmetric
            assert( pVarTaken[k] == 0 );
            // there is a new symmetry pair 
            if ( fStart == 1 )
            {  // start a new symmetry class
                fStart = 0;
                printf( "  { %s", pInputNames[ pSymms->pVars[i] ] );
                // mark the var as taken
                pVarTaken[i] = 1;
            }
            printf( " %s", pInputNames[ pSymms->pVars[k] ] );
            // mark the var as taken
            pVarTaken[k] = 1;
        }
        if ( fStart == 0 )
        {
            printf( " }" );
            fStart = 1; 
        }   
    }   
    printf( "\n" );

    i = 0; 
    Ntk_NetworkForEachCi( pNet, pNode )
        free( pInputNames[i++] );
    free( pInputNames );
    free( pVarTaken );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


