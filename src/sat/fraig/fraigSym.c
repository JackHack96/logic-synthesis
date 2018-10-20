/**CFile****************************************************************

  FileName    [fraigSym.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [The top level procedure to compute symmetries.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 4, 2005]

  Revision    [$Id: fraigSym.c,v 1.4 2005/07/08 01:01:33 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Extra_BitMat_t *    Fraig_SymmsComputeOne( Fraig_Man_t * p, Fraig_Node_t * pOut, int nRoundLimit );
static void                Fraig_SymmVerifyNonIntersection( Extra_BitMat_t * pMatSym, Extra_BitMat_t * pMatNonSym );
static void                Fraig_SymmApplyTransitivity( Extra_BitMat_t * pMatSym, Extra_BitMat_t * pMatNonSym );
static Extra_SymmInfo_t *  Fraig_SymmsConvertToSimInfo( Extra_BitMat_t * pMatSym, int * pVarMapInv );
static void                Fraig_SymmsPrint( Fraig_NodeVec_t * vSymms );

// procedures from other files
extern Extra_BitMat_t *    Fraig_SymmsStructComputeOne( Fraig_Man_t * p, Fraig_Node_t * pNode );
extern Extra_BitMat_t *    Fraig_SymmsSimComputeOne( Fraig_Man_t * p, Fraig_Node_t * pNode, int nRoundLimit );
extern void                Fraig_SymmsSatComputeOne( Fraig_Man_t * p, Extra_BitMat_t * pMatSym, Extra_BitMat_t * pMatNonSym );

extern int clkSup;
extern int clkMov;
extern int clkStr;
extern int clkSim;
extern int clkSat;
extern int clkOth;

extern int symsTot;
extern int symsStr;
extern int symsSim;
extern int symsSat;
extern int symsTrn;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transfers the output into the new FRAIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Fraig_SymmsComputeUsingSandS( Fraig_Man_t * p, int Out, int nRoundLimit )
{
    Extra_SymmInfo_t * pResult;
    Extra_BitMat_t * pMatSym;
    Fraig_Man_t * pNew;
    Fraig_Node_t * pRoot, * pRootNew;
    int * pVarMap, * pVarMapInv;
    unsigned * pSuppF;
    int i, nVars, clk;

    // get the node and its true support
    pRoot = Fraig_Regular(p->vOutputs->pArray[Out]);
    pSuppF = p->pSuppF[pRoot->Num];

    // create mapping from the original support to the true functional support
    pVarMap = ALLOC( int, p->vInputs->nSize );
    pVarMapInv = ALLOC( int, p->vInputs->nSize );
    for ( i = nVars = 0; i < p->vInputs->nSize; i++ )
    {
        if ( Fraig_BitStringHasBit( pSuppF, i ) )
        {
            pVarMapInv[nVars] = i;
            pVarMap[i] = nVars++;
        }
        else
            pVarMap[i] = -1;
    }
symsTot += nVars * (nVars - 1) / 2;
//printf( "%d ", nVars );

//printf( "%d ", nVars );
//    if ( nVars > 40 )
//    {
//        free( pVarMap );
//        free( pVarMapInv );
//        return NULL;
//    }

    // transfer the PO into a new FRAIG manager and put it on the true support
    // create the new manager with the given number of inputs and one output
clk = clock();
    pNew = Fraig_ManCreate();
    for ( i = 0; i < nVars; i++ )
        Fraig_ManReadIthVar( pNew, i );

    // try to improve the performance of the FRAIG package
    p->fDoSparse = 0;  // skip the sparse functions
    p->nBTLimit  = 5;  // set the backtrack limit

    // transfer the graph into the new manager (defined in "fraigOper.c")
    pRootNew = Fraig_Transfer( p, pNew, pRoot, pVarMap );   Fraig_Ref( pRes );
    pNew->vOutputs->pArray[0] = pRootNew;
clkMov += clock() - clk;

    // compute symmetries of the PO
    pMatSym = Fraig_SymmsComputeOne( pNew, Fraig_Regular(pRootNew), nRoundLimit );
    pResult = Fraig_SymmsConvertToSimInfo( pMatSym, pVarMapInv );
    // free the array and the temporary manager
    Extra_BitMatrixStop( pMatSym );
clk = clock();
    Fraig_ManFree( pNew );
clkOth += clock() - clk;
    free( pVarMap );
    free( pVarMapInv );

    return (char *)pResult;
}

/**Function*************************************************************

  Synopsis    [Computes symmetries of one output in the new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_BitMat_t * Fraig_SymmsComputeOne( Fraig_Man_t * p, Fraig_Node_t * pOut, int nRoundLimit )
{
    Extra_BitMat_t * pMatNonSym;
    Extra_BitMat_t * pMatSym;
    int clk;

    // this is a temporary restriction
//    assert( p->vInputs->nSize < FRAIG_SUPP_SIGN );

    // compute symmetries using the structure of the circuit
clk = clock();
    pMatSym = Fraig_SymmsStructComputeOne( p, pOut );
clkStr += clock() - clk;
symsStr += Extra_BitMatrixCountOnesUpper( pMatSym );

    // compute non-symmeries using simulation
clk = clock();
    pMatNonSym = Fraig_SymmsSimComputeOne( p, pOut, nRoundLimit );
clkSim += clock() - clk;
symsSim += Extra_BitMatrixCountOnesUpper( pMatNonSym );

    // applies the transitivity conditions to the non-symm pairs using symm pairs
    Fraig_SymmApplyTransitivity( pMatSym, pMatNonSym );
    Fraig_SymmVerifyNonIntersection( pMatSym, pMatNonSym );

    // finally fill in the remaining entries by SAT
clk = clock();
    Fraig_SymmsSatComputeOne( p, pMatSym, pMatNonSym );
clkSat += clock() - clk;
    Extra_BitMatrixStop( pMatNonSym );
    return pMatSym;
}

/**Function*************************************************************

  Synopsis    [Verify non-intersection.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmVerifyNonIntersection( Extra_BitMat_t * pMatSym, Extra_BitMat_t * pMatNonSym )
{
    int nInputs, i, k;
    nInputs = Extra_BitMatrixReadSize( pMatSym );
    for ( i = 0; i < nInputs; i++ )
    for ( k = i + 1; k < nInputs; k++ )
        if ( Extra_BitMatrixLookup1( pMatSym, i, k ) )
        {
            if ( Extra_BitMatrixLookup1( pMatNonSym, i, k ) )
            {
                Extra_BitMatrixPrint( pMatSym );
                Extra_BitMatrixPrint( pMatNonSym );
                assert( 0 );
                return;
            }
        }
}

/**Function*************************************************************

  Synopsis    [Applies the transitivity conditions to the non-symmetric vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmApplyTransitivity( Extra_BitMat_t * pMatSym, Extra_BitMat_t * pMatNonSym )
{
    int nInputs, i, k;
    nInputs = Extra_BitMatrixReadSize( pMatSym );
    for ( i = 0; i < nInputs; i++ )
    for ( k = i + 1; k < nInputs; k++ )
        if ( Extra_BitMatrixLookup1( pMatSym, i, k ) )
            Extra_BitMatrixOrTwo( pMatNonSym, i, k );
}

/**Function*************************************************************

  Synopsis    [Converts symmetry information into the symmetry structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_SymmInfo_t *  Fraig_SymmsConvertToSimInfo( Extra_BitMat_t * pMatSym, int * pVarMapInv )
{
    Extra_SymmInfo_t * p;
    int nInputs, i, k;

    nInputs = Extra_BitMatrixReadSize( pMatSym );
    p = Extra_SymmPairsAllocate( nInputs );
    // copy the mapping info
    for ( i = 0; i < nInputs; i++ )
        p->pVars[i] = pVarMapInv[i];
    // add internal data
    for ( i = 0; i < nInputs; i++ )
    for ( k = i + 1; k < nInputs; k++ )
        if ( Extra_BitMatrixLookup1( pMatSym, i, k ) )
        {
            p->pSymms[i][k] = p->pSymms[k][i] = 1;
            p->nSymms++;
        }
    return p;
}

/**Function*************************************************************

  Synopsis    [Print the partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_SymmsPrint( Fraig_NodeVec_t * vSymms )
{
    unsigned uSymm;
    int i, Ind1, Ind2;

    printf( "Symmetries:" );
    for ( i = 0; i < vSymms->nSize; i++ )
    {
        uSymm = (unsigned)vSymms->pArray[i];
        Ind1  = (uSymm & 0xffff);
        Ind2  = (uSymm >> 16);
        printf( " (%d,%d)", Ind2, Ind1 );
    }
    printf( "\n" );
}



/**Function*************************************************************

  Synopsis    [Returns the number of vars in the structural support of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*

int Fraig_CountStructuralSupport( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    unsigned * pSuppS;
    int i, nVars;
    pSuppS = Fraig_Regular(pNode)->pSuppStr;
    for ( i = nVars = 0; i < pMan->vInputs->nSize; i++ )
        if ( Fraig_BitStringHasBit( pSuppS, i ) )
            nVars++;
    return nVars;
}

*/

/**Function*************************************************************

  Synopsis    [Returns the number of vars in the structural support of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fraig_CountBits( Fraig_Man_t * pMan, Fraig_Node_t * pNode )
{
    unsigned * pBitInfo;
    int i, nVars, nBits = FRAIG_SIM_ROUNDS * 32;
    pBitInfo = Fraig_Regular(pNode)->pSimsR->uTests;
    for ( i = nVars = 0; i < nBits; i++ )
        if ( Fraig_BitStringHasBit( pBitInfo, i ) )
            nVars++;
    return nVars;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


