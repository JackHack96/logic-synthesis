/**CFile****************************************************************

  FileName    [ntkiFraigVerify.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [SAT-AIG-based verification.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiFraigVerify.c,v 1.5 2005/05/18 04:14:37 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "sat.h"
#include "fraig.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ntk_NetworkReorderList( Ntk_NodeList_t * plCis1, Ntk_NodeList_t * plCis2 );
static void Ntk_NetworkVerifyFunctionsSat( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fFuncRed, int fFeedBack, int fDoSparse, int fVerbose );
static void Ntk_NetworkAnalyzeOutputs( Fraig_Man_t * pMan, Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose );
static char * Ntk_NetworkGenerateError( Ntk_Network_t * pNet, Fraig_Man_t * pMan );

extern int  Ntk_NetworkVerifyVariables( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////
    
/**Function*************************************************************

  Synopsis    [The top level verification procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkVerifySat( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fFuncRed, int fFeedBack, int fDoSparse, int fVerbose )
{
    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet1) || !Ntk_NetworkIsBinary(pNet2) )
    {
        printf( "Currently can only verify binary networks.\n" );
        return;
    }

    // verify the identity of network CI/COs
    if ( !Ntk_NetworkVerifyVariables( pNet1, pNet2, fVerbose ) )
        return;

    // make the ordering of PIs/POs of pNet2 the same as pNet1
    Ntk_NetworkReorderList( &pNet1->lCis, &pNet2->lCis );
    Ntk_NetworkReorderList( &pNet1->lCos, &pNet2->lCos );

    // verify the functionality
    Ntk_NetworkVerifyFunctionsSat( pNet1, pNet2, fFuncRed, fFeedBack, fDoSparse, fVerbose );
}


/**Function*************************************************************

  Synopsis    [The top level verification procedure.]

  Description [Order the second list the same way as the first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkReorderList( Ntk_NodeList_t * plCis1, Ntk_NodeList_t * plCis2 )
{
    Ntk_NodeList_t List = {NULL,NULL,0}, * pList = &List; 
    Ntk_Node_t * pNode, * pNode2;
    st_table * tTable;

    assert( plCis1->nItems == plCis2->nItems );

    tTable = st_init_table(strcmp, st_strhash);
    for ( pNode = plCis2->pHead; pNode; pNode = pNode->pNext )
        st_insert( tTable, pNode->pName, (char *)pNode );

    for ( pNode = plCis1->pHead; pNode; pNode = pNode->pNext )
    {
        if ( !st_lookup( tTable, pNode->pName, (char **)&pNode2 ) )
        {
            assert( 0 );
        }
        // add the node to the end of the list
        if ( pList->pHead == NULL )
        {
            pList->pHead = pNode2;
            pList->pTail = pNode2;
            pNode2->pPrev = NULL;
            pNode2->pNext = NULL;
        }
        else
        {
            pNode2->pNext = NULL;
            pList->pTail->pNext = pNode2;
            pNode2->pPrev = pList->pTail;
            pList->pTail = pNode2;
        }
        pList->nItems++;
    }
    st_free_table( tTable );

    // replace the second list
    *plCis2 = *pList;
}

/**Function*************************************************************

  Synopsis    [The top level verification procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkVerifyFunctionsSat( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, 
    int fFuncRed, int fFeedBack, int fDoSparse, int fVerbose )
{
    Fraig_Man_t * pMan;
    int clk, clk1, clkTotal = clock();
    int fRefCount = 0; // no ref counting
    int fChoicing = 0; // no choice nodes

    // start the manager 
    pMan = Fraig_ManCreate( Ntk_NetworkReadCiNum(pNet1), Ntk_NetworkReadCoNum(pNet1) );

    // set the parameters of the manager 
    Fraig_ManSetFuncRed ( pMan, fFuncRed );       
    Fraig_ManSetFeedBack( pMan, fFeedBack );      
    Fraig_ManSetDoSparse( pMan, fDoSparse );      
    Fraig_ManSetRefCount( pMan, fRefCount );      
    Fraig_ManSetChoicing( pMan, fChoicing );      
    Fraig_ManSetVerbose ( pMan, fVerbose );       

    // strash the network
clk = clock();

clk1 = clock();
    Ntk_NetworkFraigInt( pMan, pNet1, 0 );
//PRT( "Network 1", clock() - clk1 );
clk1 = clock();
    Ntk_NetworkFraigInt( pMan, pNet2, 1 );
//PRT( "Network 2", clock() - clk1 );

Fraig_ManSetTimeToGraph( pMan, clock() - clk );
clk = clock();



clk1 = clock();
    Ntk_NetworkAnalyzeOutputs( pMan, pNet1, pNet2, fVerbose );
//PRT( "Summary  ", clock() - clk1 );
Fraig_ManSetTimeToNet( pMan, clock() - clk );



Fraig_ManSetTimeTotal( pMan, clock() - clkTotal );
    // free the manager
    Ntk_NetworkFraigDeref( pMan, pNet1 );
    Ntk_NetworkFraigDeref( pMan, pNet2 );
    Fraig_ManFree( pMan );
}

/**Function*************************************************************

  Synopsis    [The top level verification procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkAnalyzeOutputs( Fraig_Man_t * pMan, Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, int fVerbose )
{
    ProgressBar * pProgress;
    Fraig_Node_t ** pgInputs, ** pgOutputs, * gNode1, * gNode2;
    Ntk_Node_t * pNode, * pDriver;
    int nInputs, nOutputs, fError, i, clk;
    int CounterStr, CounterErrors;
    char * pError, * pOutputName = NULL;

clk = clock();
    // start the strashed network
    nInputs  = Ntk_NetworkReadCiNum(pNet1);
    nOutputs = Ntk_NetworkReadCoNum(pNet1);
    pgInputs  = Fraig_ManReadInputs( pMan );
    pgOutputs = Fraig_ManReadOutputs( pMan );
//printf( "Final pass.\n" );
    // compare the outputs
    i = 0;
    fError = 0;
    CounterStr = CounterErrors = 0;
    pProgress = Extra_ProgressBarStart( stdout, nOutputs );
    Ntk_NetworkForEachCoDriver( pNet1, pNode, pDriver )
    {
        // get the AIG node of the first node
        gNode1 = NTK_STRASH_READ( pDriver );
        // get the AIG node of the second node
        gNode2 = pgOutputs[i++];
        // compare
        if ( gNode1 == gNode2 )
            CounterStr++;
        if ( !Fraig_NodesAreEqual( pMan, gNode1, gNode2, -1 ) )
        {
            if ( pOutputName == NULL )
                pOutputName = pNode->pName;
            CounterErrors++;
            if ( fVerbose )
            {
                pError = Ntk_NetworkGenerateError( pNet1, pMan );
                printf( "Primary outputs \"%s\" have different logic functions.\n", pNode->pName );
                printf( "    Distinguishing minterm is <%s>.\n", pError );
                free( pError );
            }
        }
        if ( i % 200 == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    assert( i == nOutputs );

    if ( CounterErrors == 0 )
        printf( "Combinational verification successful.\n", CounterErrors );
    else
        printf( "Combinational verification failed for %d outputs (%s, etc).\n", CounterErrors, pOutputName );

//PRT( "Final pass", clock() - clk );
//    printf( "Total = %d. Strash-equivalent = %d. Sat-equivalent = %d. NON-EQU = %d.\n", 
//        nOutputs, CounterStr, nOutputs - CounterStr - CounterErrors, CounterErrors );
}


/**Function*************************************************************

  Synopsis    [Computes the error trace.]

  Description [Returns the error trace computed for the pair of outputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ntk_NetworkGenerateError( Ntk_Network_t * pNet, Fraig_Man_t * pMan )
{
    char * pError;
    int i, nInputs, * pModel;
    int * pArray, nSize;
    // fill in the error trace with dashes
    nInputs = Ntk_NetworkReadCiNum( pNet );
    pError = ALLOC( char, nInputs + 1 );
    for ( i = 0; i < nInputs; i++ )
        pError[i] = '-';
    pError[nInputs] = 0;
    // fill in the number
    pArray = Sat_IntVecReadArray( (Sat_IntVec_t *)Fraig_ManReadVarsInt(pMan) );
    nSize  = Sat_IntVecReadSize( (Sat_IntVec_t *)Fraig_ManReadVarsInt(pMan) );
    pModel = Sat_SolverReadModelArray( (Sat_Solver_t *)Fraig_ManReadSat(pMan) );
    for ( i = 0; i < nSize; i++ )
        if ( pArray[i] <= nInputs )
            pError[ pArray[i]-1 ] = SAT_LITSIGN(pModel[ pArray[i] ])? '0' : '1';
    return pError;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


