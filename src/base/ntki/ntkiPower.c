/**CFile****************************************************************

  FileName    [ntkiPower.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiPower.c,v 1.4 2003/09/01 04:56:09 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Ntk_Node_t * Ntk_NetworkCreateNonDeterminizer( Ntk_Network_t * pNet, 
         Ntk_Node_t * pFanin, int nValues, double Prob );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkComputePower( Ntk_Network_t * pNet, int Degree )
{
    Vm_VarMap_t * pVm, * pVmNew;
    Mvr_Relation_t * pMvr, * pMvrNew;
    Ntk_Node_t * pNode;
    DdManager * dd;
    int nFaninsMax;
    int i;

    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "As of today, can \"power\" only binary networks.\n" );
        return;
    }

    // check the trivial case
    if ( Degree == 1 )
    {
        fprintf( Ntk_NetworkReadMvsisOut(pNet), "Degree = %d. No changes are performed.\n", Degree );
        return;
    }

    // get the largest fanin count
    nFaninsMax = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        if ( nFaninsMax < Ntk_NodeReadFaninNum(pNode) )
            nFaninsMax = Ntk_NodeReadFaninNum(pNode);

    // extend the number of variables in the relation DD manager
    dd = Ntk_NetworkReadManDdLoc( pNet );
    for ( i = 0; i < Degree * nFaninsMax; i++ )
        Cudd_bddIthVar( dd, i );
    Cudd_zddVarsFromBddVars( dd, 2 );

    // change the number of values of all nodes
    Ntk_NetworkForEachCi( pNet, pNode )
        pNode->nValues = (1 << Degree);
    Ntk_NetworkForEachCo( pNet, pNode )
        pNode->nValues = (1 << Degree);
    Ntk_NetworkForEachNode( pNet, pNode )
        pNode->nValues = (1 << Degree);

    // for each internal node, get the power of the map and the relation
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        // square the variable map
        pVm    = Ntk_NodeReadFuncVm( pNode );
        pVmNew = Vm_VarMapCreatePower( pVm, Degree );
        // square the relation
        pMvr    = Ntk_NodeGetFuncMvr( pNode );
        pMvrNew = Mvr_RelationCreatePower( pMvr, Degree );
        // get the new objects
        Ntk_NodeSetFuncVm( pNode, pVmNew );
        Ntk_NodeWriteFuncMvr( pNode, pMvrNew );
    }
}

/**Function*************************************************************

  Synopsis    [For each fanin pin of the node, adds the ND buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkNonDeterminize( Ntk_Network_t * pNet, double Prob )
{
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pFanin, * pNodeND, * pNode;
    Ntk_Pin_t * pPin;
    int nNodes, i;

    // set the seed for random numbers
    srand( time(NULL) );

    // put internal nodes into the array
    nNodes = Ntk_NetworkReadNodeIntNum(pNet);
    ppNodes = ALLOC( Ntk_Node_t *, nNodes );
    i = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        ppNodes[i++] = pNode;

    // add the ND nodes for each fanin of each internal node
    for ( i = 0; i < nNodes; i++ )
    {
        Ntk_NodeForEachFanin( ppNodes[i], pPin, pFanin )
        {
            assert( pNode != pFanin );
            // get the new ND node to be used as a fanin
            pNodeND = Ntk_NetworkCreateNonDeterminizer( pNet, pFanin, pFanin->nValues, Prob );
//Ntk_NodePrintMvr( stdout, pNodeND );
            // update the fanin pointer
            pPin->pNode = pNodeND;
            // extract the fanout pin of pFanin
            Ntk_NodeFanoutListDelete( pFanin, pPin->pLink );
            // insert the fanout pin into pNodeND
            Ntk_NodeFanoutListAddLast( pNodeND, pPin->pLink );
            // add the new ND node to the network
            Ntk_NetworkAddNode( pNet, pNodeND, 1 );
        }
    }
    FREE( ppNodes );
}

/**Function*************************************************************

  Synopsis    [Creates a single-input single-output non-deterministic node.]

  Description [Creates the buffer with the given number of values (nValues).
  Inserts random minterms into this buffer with the probability Prob. For
  example, the input-output space of a ternary buffer consists of 9 minterms.
  Out of these, 3 are used (00,11,22), the other 6 are not (01,02,10,12,20,21). 
  The ternary non-determinizer is created by chosing randomly (with the given
  probability) some unused input-output minterms, and adding them to the used 
  oned. For example, adding a minterm (10) to the used minterms, lead to the 
  non-deterministic node, which is like the buffer, but also can output 0
  when the input is 1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Node_t * Ntk_NetworkCreateNonDeterminizer( Ntk_Network_t * pNet, 
         Ntk_Node_t * pFanin, int nValues, double Prob )
{
    unsigned BufferND[32];
    int i, k;

    if ( nValues > 32 )
        return NULL;

    // set the buffer
    memset( BufferND, nValues, sizeof(BufferND) );
    for ( i = 0; i < nValues; i++ )
        BufferND[i] = (1 << i);
    // sprinkle the don't-care minterms
    for ( i = 0; i < nValues; i++ )
        for ( k = 0; k < nValues; k++ )
            if ( rand() % 1000 < Prob * 1000 )
                BufferND[i] |= (1 << k);
    // create the node and return
    return Ntk_NodeCreateOneInputNode( pNet, pFanin, nValues, nValues, BufferND );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


