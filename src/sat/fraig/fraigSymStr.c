/**CFile****************************************************************

  FileName    [fraigSymms.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    []

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 4, 2005]

  Revision    [$Id: fraigSymStr.c,v 1.3 2005/07/08 01:01:33 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define FRAIG_READ_SYMMS(pNode)       ((Fraig_NodeVec_t *)pNode->pData0) 
#define FRAIG_SET_SYMMS(pNode,vVect)   (pNode->pData0 = (Fraig_Node_t *)(vVect)) 

static void              Fraig_SymmsStructComputeOne_rec( Fraig_Man_t * p, Fraig_Node_t * pNode );
static Extra_BitMat_t *  Fraig_SymmTransferToMatrix( Fraig_Man_t * p, Fraig_NodeVec_t * vSymms );
static void              Fraig_SymmsBalanceCollect_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes );
static void              Fraig_SymmsPartitionNodes( Fraig_NodeVec_t * vNodes, Fraig_NodeVec_t * vNodesPis0, Fraig_NodeVec_t * vNodesPis1, Fraig_NodeVec_t * vNodesOther );
static void              Fraig_SymmsAppendFromGroup( Fraig_Man_t * p, Fraig_NodeVec_t * vNodesPi, Fraig_NodeVec_t * vNodesOther, Fraig_NodeVec_t * vSymms );
static void              Fraig_SymmsAppendFromNode( Fraig_Man_t * p, Fraig_NodeVec_t * vSymms0, Fraig_NodeVec_t * vNodesOther, Fraig_NodeVec_t * vNodesPi0, Fraig_NodeVec_t * vNodesPi1, Fraig_NodeVec_t * vSymms );
static int               Fraig_SymmsIsCompatibleWithNodes( Fraig_Man_t * p, unsigned uSymm, Fraig_NodeVec_t * vNodesOther );
static int               Fraig_SymmsIsCompatibleWithGroup( unsigned uSymm, Fraig_NodeVec_t * vNodesPi );
static void              Fraig_SymmsPrint( Fraig_NodeVec_t * vSymms );
static void              Fraig_SymmsTrans( Fraig_NodeVec_t * vSymms );

static void              Fraig_SymmsSatCompute( Fraig_Man_t * p, Extra_BitMat_t * pMatSymm, Extra_BitMat_t * pMatAsymm );
static int               Fraig_SymmsSatProveOne( Fraig_Man_t * p, int Var1, int Var2 );
static int               Fraig_SymmsIsCliqueMatrix( Fraig_Man_t * p, Extra_BitMat_t * pMat );

extern void              Fraig_ManStructuralSupport( Fraig_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes symmetries for a single output function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitMat_t * Fraig_SymmsStructComputeOne( Fraig_Man_t * p, Fraig_Node_t * pNode )
{
    Extra_BitMat_t * pMat;
    Fraig_NodeVec_t * vSymms, * vSymmsTemp;
    Fraig_NodeVec_t * vNodes;
    Fraig_Node_t * pTemp;
    int i;

    // get the structural support
    Fraig_ManStructuralSupport( p );

    assert( !Fraig_IsComplement( pNode ) );
    // collect the nodes in the TFI cone of this output
    vNodes = Fraig_DfsOne( p, pNode, 0 );
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pTemp = vNodes->pArray[i];
        if ( Fraig_NodeIsVar(pTemp) )
            FRAIG_SET_SYMMS( pTemp, Fraig_NodeVecAlloc( 0 ) );
        else
            FRAIG_SET_SYMMS( pTemp, NULL );
    }
    assert( pNode == vNodes->pArray[vNodes->nSize-1] );
    // compute the symmetries
    Fraig_SymmsStructComputeOne_rec( p, pNode );
    // get symmetries
    vSymms = FRAIG_READ_SYMMS( pNode );
    FRAIG_SET_SYMMS( pNode, NULL );
    // free the node vectors
    for ( i = 0; i < vNodes->nSize - 1; i++ )
    {
        vSymmsTemp = FRAIG_READ_SYMMS( vNodes->pArray[i] );
        if ( vSymmsTemp == NULL )
            continue;
        Fraig_NodeVecFree( vSymmsTemp );
        FRAIG_SET_SYMMS( vNodes->pArray[i], NULL );
    }
    Fraig_NodeVecFree( vNodes );

    pMat = Fraig_SymmTransferToMatrix( p, vSymms );
    Fraig_NodeVecFree( vSymms );
    return pMat;
}

/**Function*************************************************************

  Synopsis    [Recursively computes symmetries. ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmsStructComputeOne_rec( Fraig_Man_t * p, Fraig_Node_t * pNode )
{
    Fraig_NodeVec_t * vNodes, * vNodesPi0, * vNodesPi1;
    Fraig_NodeVec_t * vNodesOther, * vSymms;
    Fraig_Node_t * pTemp;
    int i;

    // check if symmetries are already computed
    assert( !Fraig_IsComplement( pNode ) );
    if ( FRAIG_READ_SYMMS( pNode ) )
        return;
    assert( Fraig_NodeIsAnd( pNode ) );

    // allocate the temporary arrays
    vNodes      = Fraig_NodeVecAlloc( 10 );
    vNodesPi0   = Fraig_NodeVecAlloc( 10 );
    vNodesPi1   = Fraig_NodeVecAlloc( 10 );
    vNodesOther = Fraig_NodeVecAlloc( 10 );  

    // collect the fanins of the implication supergate
    Fraig_SymmsBalanceCollect_rec( pNode, vNodes );

    // sort the nodes in the implication supergate
    Fraig_SymmsPartitionNodes( vNodes, vNodesPi0, vNodesPi1, vNodesOther );

    // generate symmetries for the fanin nodes
    for ( i = 0; i < vNodesOther->nSize; i++ )
    {
        pTemp = Fraig_Regular(vNodesOther->pArray[i]);
        Fraig_SymmsStructComputeOne_rec( p, pTemp );
    }

    // start the resulting set
    vSymms = Fraig_NodeVecAlloc( 10 );
    // generate symmetries from the groups
    Fraig_SymmsAppendFromGroup( p, vNodesPi0, vNodesOther, vSymms );
    Fraig_SymmsAppendFromGroup( p, vNodesPi1, vNodesOther, vSymms );
    // add symmetries from other inputs
    for ( i = 0; i < vNodesOther->nSize; i++ )
    {
        pTemp = Fraig_Regular(vNodesOther->pArray[i]);
        Fraig_SymmsAppendFromNode( p, FRAIG_READ_SYMMS(pTemp), vNodesOther, vNodesPi0, vNodesPi1, vSymms );
    }
    Fraig_NodeVecFree( vNodes );
    Fraig_NodeVecFree( vNodesPi0 );
    Fraig_NodeVecFree( vNodesPi1 );
    Fraig_NodeVecFree( vNodesOther );

    // set the symmetry at the node
    FRAIG_SET_SYMMS( pNode, vSymms );
}


/**Function*************************************************************

  Synopsis    [Returns the array of nodes to be combined into one multi-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmsBalanceCollect_rec( Fraig_Node_t * pNode, Fraig_NodeVec_t * vNodes )
{
    // if the new node is complemented, another gate begins
    if ( Fraig_IsComplement(pNode) )
    {
        Fraig_NodeVecPushUnique( vNodes, pNode );
        return;
    }
    // if pNew is the PI node, return
    if ( Fraig_NodeIsVar(pNode) )
    {
        Fraig_NodeVecPushUnique( vNodes, pNode );
        return;    
    }
    // go through the branches
    Fraig_SymmsBalanceCollect_rec( pNode->p1, vNodes );
    Fraig_SymmsBalanceCollect_rec( pNode->p2, vNodes );
}

/**Function*************************************************************

  Synopsis    [Divides PI variables into groups.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmsPartitionNodes( Fraig_NodeVec_t * vNodes, Fraig_NodeVec_t * vNodesPis0, 
    Fraig_NodeVec_t * vNodesPis1, Fraig_NodeVec_t * vNodesOther )
{
    Fraig_Node_t * pNode;
    int i;
    for ( i = 0; i < vNodes->nSize; i++ )
    {
        pNode = vNodes->pArray[i];
        if ( !Fraig_NodeIsVar(pNode) )
            Fraig_NodeVecPush( vNodesOther, pNode );
        else if ( Fraig_IsComplement(pNode) )
            Fraig_NodeVecPush( vNodesPis0, pNode );
        else
            Fraig_NodeVecPush( vNodesPis1, pNode );
    }
}

/**Function*************************************************************

  Synopsis    [Makes the product of two partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmsAppendFromGroup( Fraig_Man_t * p, Fraig_NodeVec_t * vNodesPi, Fraig_NodeVec_t * vNodesOther, Fraig_NodeVec_t * vSymms )
{
    Fraig_Node_t * pNode1, * pNode2;
    unsigned uSymm;
    int i, k;

    if ( vNodesPi->nSize == 0 )
        return;

    // go through the pairs
    for ( i = 0; i < vNodesPi->nSize; i++ )
    for ( k = i+1; k < vNodesPi->nSize; k++ )
    {
        // get the two PI nodes
        pNode1 = Fraig_Regular(vNodesPi->pArray[i]);
        pNode2 = Fraig_Regular(vNodesPi->pArray[k]);
        assert( pNode1->NumPi != pNode2->NumPi );
        assert( pNode1->NumPi >= 0 );
        assert( pNode2->NumPi >= 0 );
        // generate symmetry
        if ( pNode1->NumPi < pNode2->NumPi )
            uSymm = ((pNode1->NumPi << 16) | pNode2->NumPi);
        else
            uSymm = ((pNode2->NumPi << 16) | pNode1->NumPi);
        // check if symmetry belongs
        if ( Fraig_SymmsIsCompatibleWithNodes( p, uSymm, vNodesOther ) )
            Fraig_NodeVecPushUnique( vSymms, (Fraig_Node_t *)uSymm );
    }
}

/**Function*************************************************************

  Synopsis    [Add the filters symmetries from the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmsAppendFromNode( Fraig_Man_t * p, Fraig_NodeVec_t * vSymms0, Fraig_NodeVec_t * vNodesOther, 
    Fraig_NodeVec_t * vNodesPi0, Fraig_NodeVec_t * vNodesPi1, Fraig_NodeVec_t * vSymms )
{
    unsigned uSymm;
    int i;

    if ( vSymms0->nSize == 0 )
        return;

    // go through the pairs
    for ( i = 0; i < vSymms0->nSize; i++ )
    {
        uSymm = (unsigned)vSymms0->pArray[i];
        // check if symmetry belongs
        if ( Fraig_SymmsIsCompatibleWithNodes( p, uSymm, vNodesOther ) &&
             Fraig_SymmsIsCompatibleWithGroup( uSymm, vNodesPi0 ) && 
             Fraig_SymmsIsCompatibleWithGroup( uSymm, vNodesPi1 ) )
            Fraig_NodeVecPushUnique( vSymms, (Fraig_Node_t *)uSymm );
    }
}

/**Function*************************************************************

  Synopsis    [Returns 1 if symmetry is compatible with the group of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_SymmsIsCompatibleWithNodes( Fraig_Man_t * p, unsigned uSymm, Fraig_NodeVec_t * vNodesOther )
{
    Fraig_NodeVec_t * vSymmsNode;
    Fraig_Node_t * pNode;
    int i, s, Ind1, Ind2, fIsVar1, fIsVar2;

    if ( vNodesOther->nSize == 0 )
        return 1;

    // get the indices of the PI variables
    Ind1 = (uSymm & 0xffff);
    Ind2 = (uSymm >> 16);

    // go through the nodes
    // if they do not belong to a support, it is okay
    // if one belongs, the other does not belong, quit
    // if they belong, but are not part of symmetry, quit
    for ( i = 0; i < vNodesOther->nSize; i++ )
    {
        pNode = Fraig_Regular(vNodesOther->pArray[i]);
//        fIsVar1 = Fraig_NodeHasVarStr( pNode, Ind1 );
//        fIsVar2 = Fraig_NodeHasVarStr( pNode, Ind2 );
        fIsVar1 = Fraig_BitStringHasBit( p->pSuppS[pNode->Num], Ind1 );
        fIsVar2 = Fraig_BitStringHasBit( p->pSuppS[pNode->Num], Ind2 );

        if ( !fIsVar1 && !fIsVar2 )
            continue;
        if ( fIsVar1 ^ fIsVar2 )
            return 0;
        // both belong
        // check if there is a symmetry
        vSymmsNode = FRAIG_READ_SYMMS( pNode );
        for ( s = 0; s < vSymmsNode->nSize; s++ )
            if ( uSymm == (unsigned)vSymmsNode->pArray[s] )
                break;
        if ( s == vSymmsNode->nSize )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if symmetry is compatible with the group of PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_SymmsIsCompatibleWithGroup( unsigned uSymm, Fraig_NodeVec_t * vNodesPi )
{
    Fraig_Node_t * pNode;
    int i, Ind1, Ind2, fHasVar1, fHasVar2;

    if ( vNodesPi->nSize == 0 )
        return 1;

    // get the indices of the PI variables
    Ind1 = (uSymm & 0xffff);
    Ind2 = (uSymm >> 16);

    // go through the PI nodes
    fHasVar1 = fHasVar2 = 0;
    for ( i = 0; i < vNodesPi->nSize; i++ )
    {
        pNode = Fraig_Regular(vNodesPi->pArray[i]);
        if ( pNode->NumPi == Ind1 )
            fHasVar1 = 1;
        else if ( pNode->NumPi == Ind2 )
            fHasVar2 = 1;
    }
    return fHasVar1 == fHasVar2;
}



/**Function*************************************************************

  Synopsis    [Improvements due to transitivity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmsTrans( Fraig_NodeVec_t * vSymms )
{
    unsigned uSymm, uSymma, uSymmr;
    int i, Ind1, Ind2;
    int k, Ind1a, Ind2a;
    int j;
    int nTrans = 0;

    for ( i = 0; i < vSymms->nSize; i++ )
    {
        uSymm = (unsigned)vSymms->pArray[i];
        Ind1 = (uSymm & 0xffff);
        Ind2 = (uSymm >> 16);
        // find other symmetries that have Ind1
        for ( k = i+1; k < vSymms->nSize; k++ )
        {
            uSymma = (unsigned)vSymms->pArray[k];
            if ( uSymma == uSymm )
                continue;
            Ind1a = (uSymma & 0xffff);
            Ind2a = (uSymma >> 16);
            if ( Ind1a == Ind1 )
            {
                // find the symmetry (Ind2,Ind2a)
                if ( Ind2 < Ind2a )
                    uSymmr = ((Ind2 << 16) | Ind2a);
                else
                    uSymmr = ((Ind2a << 16) | Ind2);
                for ( j = 0; j < vSymms->nSize; j++ )
                    if ( uSymmr == (unsigned)vSymms->pArray[j] )
                        break;
                if ( j == vSymms->nSize )
                    nTrans++;
            }
        }

    }
    printf( "Trans = %d.\n", nTrans );
}


/**Function*************************************************************

  Synopsis    [Transfers from the vector to the matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitMat_t * Fraig_SymmTransferToMatrix( Fraig_Man_t * p, Fraig_NodeVec_t * vSymms )
{
    Extra_BitMat_t * pMatSymm;
    int i, Ind1, Ind2;
    unsigned uSymm;

    pMatSymm = Extra_BitMatrixStart( p->vInputs->nSize );
    // add diagonal elements
    for ( i = 0; i < p->vInputs->nSize; i++ )
        Extra_BitMatrixInsert1( pMatSymm, i, i );
    // add non-diagonal elements
    for ( i = 0; i < vSymms->nSize; i++ )
    {
        uSymm = (unsigned)vSymms->pArray[i];
        Ind1 = (uSymm & 0xffff);
        Ind2 = (uSymm >> 16);
        Extra_BitMatrixInsert1( pMatSymm, Ind1, Ind2 );
        Extra_BitMatrixInsert2( pMatSymm, Ind1, Ind2 );
    }
    return pMatSymm;
}




////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


