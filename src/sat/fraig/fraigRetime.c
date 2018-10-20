/**CFile****************************************************************

  FileName    [fraigRetime.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Retiming procedures using Pan's approach.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 12, 2005]

  Revision    [$Id: fraigRetime.c,v 1.5 2005/07/08 01:43:21 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the procedure to be exported
//extern void Fraig_ManRetime( Fraig_Man_t * pMan, int nLatches, int fVerbose );


// storing l-Values in the FRAIG nodes
#define FRAIG_SET_LVALUE( pNode, Value )      ((pNode)->pData1 = (Fraig_Node_t *)(Value))
#define FRAIG_READ_LVALUE( pNode )            ((int)(pNode)->pData1)

// the states of the retiming process
enum { FRAIG_RETIMING_FAIL, FRAIG_RETIMING_CONTINUE, FRAIG_RETIMING_DONE };

// the large l-Value
#define FRAIG_INT_LARGE                       (10000000)

// the internal procedures
static int Fraig_ManRetimeSearch_rec( Fraig_Man_t * pMan, int nLatches, int FiMin, int FiMax );
static int Fraig_ManRetimeForPeriod( Fraig_Man_t * pMan, int nLatches, int Fi );
static int Fraig_ManRetimeForPeriodIteration( Fraig_Man_t * pMan, int nLatches, int Fi );
static int Fraig_ComputeLValue_rec( Fraig_Man_t * pMan, Fraig_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Optimally retimes FRAIG with choice nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManRetimeAig( Fraig_Man_t * pMan, int nLatches, int fVerbose )
{
    int nLevelMax, FiBest;

    // get the maximum level
    nLevelMax = Fraig_GetMaxLevel( pMan );

    // make sure the maximum-level clock period is feasible
    assert( Fraig_ManRetimeForPeriod( pMan, nLatches, nLevelMax ) );

    // search for the optimal clock period between 0 and nLevelMax
    FiBest = Fraig_ManRetimeSearch_rec( pMan, nLatches, 0, nLevelMax );

    // print the result
    printf( "The best clock period is %3d.\n", FiBest );
}


/**Function*************************************************************

  Synopsis    [Performs binary search for the optimal clock period.]

  Description [Assumes that FiMin is infeasible while FiMax is feasible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManRetimeSearch_rec( Fraig_Man_t * pMan, int nLatches, int FiMin, int FiMax )
{
    int Median;

    assert( FiMin < FiMax );
    if ( FiMin + 1 == FiMax )
        return FiMax;

    Median = FiMin + (FiMax - FiMin)/2;

    if ( Fraig_ManRetimeForPeriod( pMan, nLatches, Median ) )
        return Fraig_ManRetimeSearch_rec( pMan, nLatches, FiMin, Median ); // Median is feasible
    else 
        return Fraig_ManRetimeSearch_rec( pMan, nLatches, Median, FiMax ); // Median is infeasible
}

/**Function*************************************************************

  Synopsis    [Returns 1 if retiming with this clock period is feasible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ManRetimeForPeriod( Fraig_Man_t * pMan, int nLatches, int Fi )
{
    Fraig_Node_t * pNode;
    int RetValue, nIter, i;

    // clean the l-values and traversal IDs of all nodes
    for ( i = 0; i < pMan->vNodes->nSize; i++ )
    {
        pNode = pMan->vNodes->pArray[i];
        FRAIG_SET_LVALUE( pNode, -FRAIG_INT_LARGE );
        pNode->TravId = 0;
    }

    // initialize the real PIs
    for ( i = 0; i < pMan->vInputs->nSize - nLatches; i++ )
    {
        pNode = pMan->vInputs->pArray[i];
        FRAIG_SET_LVALUE( pNode, 0 );
    }

    // iterate until convergence
    for ( nIter = 0; nIter < 1000; nIter++ )
    {
        RetValue = Fraig_ManRetimeForPeriodIteration( pMan, nLatches, Fi );
        if ( RetValue == FRAIG_RETIMING_FAIL )
        {
            printf( "Period = %3d.  nIter = %3d.  Infeasible\n", Fi, nIter );
            return 0;
        }
        else if ( RetValue == FRAIG_RETIMING_DONE )
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
int Fraig_ManRetimeForPeriodIteration( Fraig_Man_t * pMan, int nLatches, int Fi )
{
    Fraig_Node_t * pNode, * pLatchIn, * pLatchOut;
    int fChange, lValuePo, lValuePi, i;

    // increment the traversal ID
    pMan->nTravIds++;
    // mark all the PIs as already visited in this iteration
    for ( i = 0; i < pMan->vInputs->nSize; i++ )
        pMan->vInputs->pArray[i]->TravId = pMan->nTravIds;

    // recursively compute l-Values starting from the POs
    for ( i = 0; i < pMan->vOutputs->nSize; i++ )
    {
        // get the output node
        pNode = Fraig_Regular(pMan->vOutputs->pArray[i]);
        // skip the constant node
        if ( Fraig_NodeIsConst(pNode) ) 
            continue;
        // compute lValue of this PO
        lValuePo = Fraig_ComputeLValue_rec( pMan, pNode );
        // skip if the lValue exceeds the limit
        if ( i < pMan->vOutputs->nSize - nLatches && lValuePo > Fi )
            return FRAIG_RETIMING_FAIL;
    }

    // set the lValues of the PIs for the next iteration
    fChange = 0;
    for ( i = 0; i < nLatches; i++ )
    {
        // get the latch input (l-value is computed) and output (l-value should be set)
        pLatchIn  = Fraig_Regular(pMan->vOutputs->pArray[ pMan->vOutputs->nSize - nLatches + i ]);
        pLatchOut = pMan->vInputs->pArray[ pMan->vInputs->nSize - nLatches + i ];
        // compute the new l-value of the PI as the MAX of this and previous value
        lValuePi = FRAIG_MAX( FRAIG_READ_LVALUE(pLatchOut), FRAIG_READ_LVALUE(pLatchIn) - Fi );
        if ( FRAIG_READ_LVALUE(pLatchOut) < lValuePi )
        {
            FRAIG_SET_LVALUE( pLatchOut, lValuePi );
            fChange = 1;
        }
        // set the new traversal ID to the PIs
//        pLatchOut->TravId = pMan->nTravIds;
    }
    if ( fChange )
        return FRAIG_RETIMING_CONTINUE;
    return FRAIG_RETIMING_DONE;
}


/**Function*************************************************************

  Synopsis    [Computes the l-value of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_ComputeLValue_rec( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    int lValue, lValueE, lValue1, lValue2;

    assert( !Fraig_IsComplement( pNode ) );
    // check for special cases when recursion can be terminated
    if ( pNode->TravId == pMan->nTravIds ) // the node is visited
        return FRAIG_READ_LVALUE( pNode );

    // mark the node as visited
    pNode->TravId = pMan->nTravIds;

    // compute the l-value of the node
    lValue1 = Fraig_ComputeLValue_rec( pMan, Fraig_Regular(pNode->p1) );
    lValue2 = Fraig_ComputeLValue_rec( pMan, Fraig_Regular(pNode->p2) );
    lValue  = 1 + FRAIG_MAX( lValue1, lValue2 );

    // if there are choices, compute lValue for them, and take the minimum
    if ( pNode->pNextE )
    {
        // we only need to look at one equivalent node (others will be looked recursively)
        lValueE = Fraig_ComputeLValue_rec( pMan, pNode->pNextE );
        lValue  = FRAIG_MIN( lValue, lValueE );
    }

    if ( FRAIG_READ_LVALUE(pNode) < lValue )
        FRAIG_SET_LVALUE( pNode, lValue );
    return lValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


