/**CFile****************************************************************

  FileName    [ntkiRetime.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Retiming procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiRetime.c,v 1.4 2005/06/02 03:34:10 alanmi Exp $]

***********************************************************************/
 
#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////



// storing l-Values in the FRAIG nodes
#define NTK_SET_LVALUE( pNode, Value )      ((pNode)->pCopy = (Ntk_Node_t *)(Value))
#define NTK_READ_LVALUE( pNode )            ((int)(pNode)->pCopy)

// the states of the retiming process
enum { NTK_RETIMING_FAIL, NTK_RETIMING_CONTINUE, NTK_RETIMING_DONE };

// the large l-Value
#define NTK_INT_LARGE                       (10000000)
#define NTK_MAX(a,b)                        (((a) > (b))? (a) : (b))

// the internal procedures
static void Ntk_NetworkFraigRetimeInt( Ntk_Network_t * pNet );
static int Ntk_NetworkRetimeSearch_rec( Ntk_Network_t * pNet, int FiMin, int FiMax );
static int Ntk_NetworkRetimeForPeriod( Ntk_Network_t * pNet, int Fi );
static int Ntk_NetworkRetimeForPeriodIteration( Ntk_Network_t * pNet, int Fi );
static int Ntk_ComputeLValue_rec( Ntk_Network_t * pNet, Ntk_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Retimes the current network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkRetime( Ntk_Network_t * pNet, int fVerbose )
{
    int clk;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only choice binary networks.\n" );
        return;
    }

    if ( Ntk_NetworkReadLatchNum(pNet) == 0 )
    {
        printf( "The network has no latches.\n" );
        return;
    }

clk = clock();
    Ntk_NetworkFraigRetimeInt( pNet );
PRT( "Network retiming", clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Retimes the current network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkFraigRetimeInt( Ntk_Network_t * pNet )
{
    int nLevelMax, FiBest;

    // get the maximum level
    nLevelMax = Ntk_NetworkGetNumLevels(pNet);

    // make sure the maximum-level clock period is feasible
    assert( Ntk_NetworkRetimeForPeriod( pNet, nLevelMax ) );

    // search for the optimal clock period between 0 and nLevelMax
    FiBest = Ntk_NetworkRetimeSearch_rec( pNet, 0, nLevelMax );

    // print the result
    printf( "The best clock period is %3d.\n", FiBest );
}

/**Function*************************************************************

  Synopsis    [Performs binary search for the optimal clock period.]

  Description [Assumes that FiMin is infeasible while FiMax is feasible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkRetimeSearch_rec( Ntk_Network_t * pNet, int FiMin, int FiMax )
{
    int Median;

    assert( FiMin < FiMax );
    if ( FiMin + 1 == FiMax )
        return FiMax;

    Median = FiMin + (FiMax - FiMin)/2;

    if ( Ntk_NetworkRetimeForPeriod( pNet, Median ) )
        return Ntk_NetworkRetimeSearch_rec( pNet, FiMin, Median ); // Median is feasible
    else 
        return Ntk_NetworkRetimeSearch_rec( pNet, Median, FiMax ); // Median is infeasible
}

/**Function*************************************************************

  Synopsis    [Returns 1 if retiming with this clock period is feasible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkRetimeForPeriod( Ntk_Network_t * pNet, int Fi )
{
    Ntk_Node_t * pNode;
    int RetValue, nIter;

    // clean the l-values and traversal IDs of all nodes
    Ntk_NetworkForEachCi( pNet, pNode )
        NTK_SET_LVALUE( pNode, -NTK_INT_LARGE );
    Ntk_NetworkForEachCo( pNet, pNode )
        NTK_SET_LVALUE( pNode, -NTK_INT_LARGE );
    Ntk_NetworkForEachNode( pNet, pNode )
        NTK_SET_LVALUE( pNode, -NTK_INT_LARGE );

    // initialize the real PIs
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH )
            continue;
        NTK_SET_LVALUE( pNode, 0 );
    }

    // iterate until convergence
    for ( nIter = 0; nIter < 1000; nIter++ )
    {
        RetValue = Ntk_NetworkRetimeForPeriodIteration( pNet, Fi );
        if ( RetValue == NTK_RETIMING_FAIL )
        {
            printf( "Period = %3d.  nIter = %3d.  Infeasible\n", Fi, nIter );
            return 0;
        }
        else if ( RetValue == NTK_RETIMING_DONE )
        {
            printf( "Period = %3d.  nIter = %3d.  Feasible\n", Fi, nIter );
            return 1;
        }
    }
    printf( "The limit on the number of iterations is exceeded\n" );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs one iteration of l-Value computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkRetimeForPeriodIteration( Ntk_Network_t * pNet, int Fi )
{
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pNode, * pDriver, * pLatchIn, * pLatchOut;
    int fChange, lValuePo, lValuePi;

    // increment the traversal ID
    pNet->nTravIds++;
    // mark all the PIs as already visited in this iteration
    Ntk_NetworkForEachCi( pNet, pNode )
        pNode->TravId = pNet->nTravIds;

    // recursively compute l-Values starting from the POs
    Ntk_NetworkForEachCoDriver( pNet, pNode, pDriver )
    {
        // skip the constant node
        if ( Ntk_NodeReadFaninNum(pDriver) == 0 ) 
            continue;
        // compute lValue of this PO
        lValuePo = Ntk_ComputeLValue_rec( pNet, pDriver );
        // set the l-Value of the PO
        NTK_SET_LVALUE( pNode, lValuePo );
        // skip if the lValue exceeds the limit
        if ( pNode->Subtype != MV_BELONGS_TO_LATCH && lValuePo > Fi )
            return NTK_RETIMING_FAIL;
    }

    // set the lValues of the PIs for the next iteration
    fChange = 0;
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        // get the latch input (l-value is computed) and output (l-value should be set)
        pLatchIn  = pLatch->pInput;
        pLatchOut = pLatch->pOutput;
        // compute the new l-value of the PI as the MAX of this and previous value
        lValuePi = NTK_MAX( NTK_READ_LVALUE(pLatchOut), NTK_READ_LVALUE(pLatchIn) - Fi );
        if ( NTK_READ_LVALUE(pLatchOut) < lValuePi )
        {
            NTK_SET_LVALUE( pLatchOut, lValuePi );
            fChange = 1;
        }
        // set the new traversal ID to the PIs
//        pLatchOut->TravId = pNet->nTravIds;
    }
    if ( fChange )
        return NTK_RETIMING_CONTINUE;
    return NTK_RETIMING_DONE;
}


/**Function*************************************************************

  Synopsis    [Computes the l-value of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_ComputeLValue_rec( Ntk_Network_t * pNet, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ntk_Pin_t * pPin;
    int lValueRes, lValueCur;

    // check for special cases when recursion can be terminated
    if ( pNode->TravId == pNet->nTravIds ) // the node is visited
        return NTK_READ_LVALUE( pNode );

    // mark the node as visited
    pNode->TravId = pNet->nTravIds;

    // compute the l-value of the node
    lValueRes = -NTK_INT_LARGE;
    Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    {
        lValueCur = Ntk_ComputeLValue_rec( pNet, pFanin );
        if ( lValueCur == -NTK_INT_LARGE )
            continue;
        lValueRes = NTK_MAX( lValueRes, lValueCur );
    }
    if ( lValueRes == -NTK_INT_LARGE )
        NTK_SET_LVALUE( pNode, lValueRes );
    else
    {
        lValueRes += 1;
        if ( NTK_READ_LVALUE(pNode) < lValueRes )
            NTK_SET_LVALUE( pNode, lValueRes );
    }
    return NTK_READ_LVALUE(pNode);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


