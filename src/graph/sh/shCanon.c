/**CFile****************************************************************

  FileName    [shCanon.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Pre-computing the canonical form of AND( AND(a,b), AND(c,d) ).]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shCanon.c,v 1.5 2004/04/12 03:42:29 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int      Sh_CanonGetOrderedList( Sh_Node_t * pNode1, Sh_Node_t * pNode2, Sh_Node_t * pNodes[] );
static unsigned Sh_CanonGetSign( Sh_Node_t * pNode1, Sh_Node_t * pNode2, Sh_Node_t * pNodes[], int nNodes );
static int      Sh_CanonGetEntryInList( Sh_Node_t * pNode, Sh_Node_t * pNodes[], int nNodes );
static void     Sh_CanonNodePrintInfo( unsigned Sign, unsigned SignCan );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Precomputes the canonical structures: AND(AND(a,b), AND(c,d)).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_CanonNodeAnd( Sh_Manager_t * p, Sh_Node_t * pNode1, Sh_Node_t * pNode2 )
{
    short Sign, SignCan;
    Sh_Node_t * pNodes[4];
    int nNodes;

    // if the nodes are identical up to complementation, return 
    if ( pNode1 == pNode2 )
        return pNode1;
    if ( pNode1 == Sh_Not(pNode2) )
        return Sh_Not( p->pConst1 );
    // if pNode1 is a constant, return
    if ( shNodeIsConst(pNode1) )
    {
        if ( Sh_IsComplement(pNode1) )   // pNode1 is const 0
            return Sh_Not( p->pConst1 ); // return const 0
        else                             // pNode1 is const 1
            return pNode2;               // pNode2
    }
    // if pNode2 is a constant, return
    if ( shNodeIsConst(pNode2) )
    {
        if ( Sh_IsComplement(pNode2) )   // pNode2 is const 0
            return Sh_Not( p->pConst1 ); // return const 0
        else                             // pNode2 is const 1
            return pNode1;               // pNode1
    }
    // if both nodes are PIs, return simple
    if ( !p->fTwoLevelHash || (shNodeIsVar(pNode1) && shNodeIsVar(pNode2)) )
        return Sh_TableLookup( p, pNode1, pNode2 );


    // get the ordered list of children
    nNodes = Sh_CanonGetOrderedList( pNode1, pNode2, pNodes );
    // get the signature
    Sign = Sh_CanonGetSign( pNode1, pNode2, pNodes, nNodes );
    // get the canonical signature
    SignCan = p->pCanTable[Sign];
//    Sh_CanonNodePrintInfo( Sign, SignCan );

    // consider trivial cases, for example: ((a') & (a & b))
    if ( SignCan == -1 )
        return Sh_Not( p->pConst1 ); // return const 0
    else if ( SignCan == -2 )
        return p->pConst1;           // return const 1
    // construct the network using the caninical signature
    return Sh_SignConstructNodes( p, SignCan, pNodes, nNodes );
}

/**Function*************************************************************

  Synopsis    [Get the ordered list of children.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_CanonGetOrderedList( Sh_Node_t * pNode1, Sh_Node_t * pNode2, Sh_Node_t * pNodes[] )
{
    Sh_Node_t * pN1, * pN2;
    Sh_Node_t * pNLL, * pNLR;
    Sh_Node_t * pNRL, * pNRR;
    int nNodes;

    pN1 = Sh_Regular(pNode1);
    pN2 = Sh_Regular(pNode2);

    // add the inputs of the first gate
    if ( Sh_NodeIsVar(pN1) )
    {
        pNodes[0] = pN1;
        nNodes = 1;
    }
    else
    {
        pNLL = Sh_Regular(pN1->pOne);
        pNLR = Sh_Regular(pN1->pTwo);
        assert( pNLL != pNLR );
        if ( pNLL < pNLR )
        {
            pNodes[0] = pNLL;
            pNodes[1] = pNLR;
        }
        else
        {
            pNodes[0] = pNLR;
            pNodes[1] = pNLL;
        }
        nNodes = 2;
    }

    // add the inputs of the second gate
    if ( Sh_NodeIsVar(pN2) )
    {
        pNRL = pN2;
        pNRR = NULL;
    }
    else
    {
        pNRL = Sh_Regular(pN2->pOne);
        pNRR = Sh_Regular(pN2->pTwo);
    }

    // add the first one
    if ( nNodes == 1 )
    {
        if ( pNodes[0] < pNRL )
        {
            pNodes[1] = pNRL;
            nNodes = 2;
        }
        else if ( pNodes[0] > pNRL )
        {
            pNodes[1] = pNodes[0];
            pNodes[0] = pNRL;
            nNodes = 2;
        }
    }
    else // if ( nNodes == 2 )
    {
        if ( pNodes[0] < pNRL )
        {
            if ( pNodes[1] < pNRL )
            {
                pNodes[2] = pNRL;
                nNodes = 3;
            }
            else if ( pNodes[1] > pNRL )
            {
                pNodes[2] = pNodes[1];
                pNodes[1] = pNRL;
                nNodes = 3;
            }
        }
        else if ( pNodes[0] > pNRL )
        {
            pNodes[2] = pNodes[1];
            pNodes[1] = pNodes[0];
            pNodes[0] = pNRL;
            nNodes = 3;
        }
    }

    if ( pNRR == NULL )
//        return nNodes;
        goto verify;

    // add the second one
    if ( nNodes == 1 )
    {
        if ( pNodes[0] < pNRR )
        {
            pNodes[1] = pNRR;
            nNodes = 2;
        }
        else if ( pNodes[0] > pNRR )
        {
            pNodes[1] = pNodes[0];
            pNodes[0] = pNRR;
            nNodes = 2;
        }
    }
    else if ( nNodes == 2 )
    {
        if ( pNodes[0] < pNRR )
        {
            if ( pNodes[1] < pNRR )
            {
                pNodes[2] = pNRR;
                nNodes = 3;
            }
            else if ( pNodes[1] > pNRR )
            {
                pNodes[2] = pNodes[1];
                pNodes[1] = pNRR;
                nNodes = 3;
            }
        }
        else if ( pNodes[0] > pNRR )
        {
            pNodes[2] = pNodes[1];
            pNodes[1] = pNodes[0];
            pNodes[0] = pNRR;
            nNodes = 3;
        }
    }
    else // if ( nNodes == 3 )
    {
        if ( pNodes[0] < pNRR )
        {
            if ( pNodes[1] < pNRR )
            {
                if ( pNodes[2] < pNRR )
                {
                    pNodes[3] = pNRR;
                    nNodes = 4;
                }
                else if ( pNodes[2] > pNRR )
                {
                    pNodes[3] = pNodes[2];
                    pNodes[2] = pNRR;
                    nNodes = 4;
                }
            }
            else if ( pNodes[1] > pNRR )
            {
                pNodes[3] = pNodes[2];
                pNodes[2] = pNodes[1];
                pNodes[1] = pNRR;
                nNodes = 4;
            }
        }
        else if ( pNodes[0] > pNRR )
        {
            pNodes[3] = pNodes[2];
            pNodes[2] = pNodes[1];
            pNodes[1] = pNodes[0];
            pNodes[0] = pNRR;
            nNodes = 4;
        }
    }

verify:
    if ( nNodes > 1 )
        assert( pNodes[0] < pNodes[1] );
    if ( nNodes > 2 )
        assert( pNodes[1] < pNodes[2] );
    if ( nNodes > 3 )
        assert( pNodes[2] < pNodes[3] );
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Get the signature of this combination.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Sh_CanonGetSign( Sh_Node_t * pNode1, Sh_Node_t * pNode2, Sh_Node_t * pNodes[], int nNodes )
{
    Sh_Node_t * pN1, * pN2;
    Sh_Node_t * pNLL, * pNLR;
    Sh_Node_t * pNRL, * pNRR;
    int fN1, fN2;
    int fNLL, fNLR, fNRL, fNRR;
    unsigned uSign, uAnd1, uAnd2;

    pN1 = Sh_Regular(pNode1);
    fN1 = (pN1 != pNode1);
    if ( Sh_NodeIsVar(pN1) )
    {
        pNLL = pN1;
        pNLR = pN1;
        fNLL = 0;
        fNLR = 0;
    }
    else
    {
        pNLL = Sh_Regular(pN1->pOne);
        pNLR = Sh_Regular(pN1->pTwo);
        fNLL = (pN1->pOne != pNLL);
        fNLR = (pN1->pTwo != pNLR);
    }

    pN2 = Sh_Regular(pNode2);
    fN2 = (pN2 != pNode2);
    if ( Sh_NodeIsVar(pN2) )
    {
        pNRL = pN2;
        pNRR = pN2;
        fNRL = 0;
        fNRR = 0;
    }
    else
    {
        pNRL = Sh_Regular(pN2->pOne);
        pNRR = Sh_Regular(pN2->pTwo);
        fNRL = (pN2->pOne != pNRL);
        fNRR = (pN2->pTwo != pNRR);
    }

    // get the first AND gate
    uAnd1 = 0;
    St_SignWriteAndInput1( uAnd1, Sh_CanonGetEntryInList( pNLL, pNodes, nNodes ) );
    St_SignWriteAndInput2( uAnd1, Sh_CanonGetEntryInList( pNLR, pNodes, nNodes ) );

    // get the second AND gate
    uAnd2 = 0;
    St_SignWriteAndInput1( uAnd2, Sh_CanonGetEntryInList( pNRL, pNodes, nNodes ) );
    St_SignWriteAndInput2( uAnd2, Sh_CanonGetEntryInList( pNRR, pNodes, nNodes ) );

    // collect the signature
    uSign = 0;
    St_SignWritePolL( uSign, fN1 );
    St_SignWritePolR( uSign, fN2 );
    St_SignWritePolLL( uSign, fNLL );
    St_SignWritePolLR( uSign, fNLR );
    St_SignWritePolRL( uSign, fNRL );
    St_SignWritePolRR( uSign, fNRR );
    St_SignWriteAnd1( uSign, uAnd1 );
    St_SignWriteAnd2( uSign, uAnd2 );
    return uSign;
}

/**Function*************************************************************

  Synopsis    [Get the number of this node in the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sh_CanonGetEntryInList( Sh_Node_t * pNode, Sh_Node_t * pNodes[], int nNodes )
{
    assert( nNodes > 0 && nNodes < 5 );
    if ( pNodes[0] == pNode )
        return 0;
    if ( nNodes > 1 && pNodes[1] == pNode )
        return 1;
    if ( nNodes > 2 && pNodes[2] == pNode )
        return 2;
    if ( nNodes > 3 && pNodes[3] == pNode )
        return 3;
    assert( 0 );
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_CanonNodePrintInfo( unsigned Sign, unsigned SignCan )
{
    unsigned s;
    s = (unsigned)Sign;
    Extra_PrintBinary( stdout, &s, 14 );
    printf( "  Sign = %16s", Sh_SignPrint(Sign) );
    printf( "  Sign = %16s\n", Sh_SignPrint(SignCan) );
}


/*
    {
        // experiment to check that structural hashing indeed works
        Sh_Node_t * pNa,  * pNb,  * pNc;
        Sh_Node_t * pN1,  * pN2,  * pNR;
        Sh_Node_t * pN1_, * pN2_, * pNR_;
        int i;

        pNa = ppShInputs[0];//  pNa->TravId = 0;
        pNb = ppShInputs[1];//  pNb->TravId = 1;
        pNc = ppShInputs[2];//  pNc->TravId = 2;

        pN1  = pNa;
        pN2  = Sh_CanonNodeAnd( pShNet, pNb, pNc );
        pNR  = Sh_CanonNodeAnd( pShNet, pN1, pN2 );

        pN1_ = pNc;
        pN2_ = Sh_CanonNodeAnd( pShNet, pNb, pNa );
        pNR_ = Sh_CanonNodeAnd( pShNet, pN1_, pN2_ );
        i = 1;
    }
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


