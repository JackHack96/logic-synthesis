/**CFile****************************************************************

  FileName    [lxuSingle.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Procedures to compute the set of single-cube divisors.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: lxuSingle.c,v 1.3 2004/02/02 20:46:48 satrajit Exp $]

***********************************************************************/

#include "lxuInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes and adds all single-cube divisors to storage.]

  Description [This procedure should be called once when the matrix is
  already contructed before the process of logic extraction begins..]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixComputeSingles( Lxu_Matrix * p )
{
    Lxu_Var * pVar;
    // iterate through the columns in the matrix
    Lxu_MatrixForEachVariable( p, pVar )
        Lxu_MatrixComputeSinglesOne( p, pVar );
}

/**Function*************************************************************

  Synopsis    [Adds the single-cube divisors associated with a new column.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lxu_MatrixComputeSinglesOne( Lxu_Matrix * p, Lxu_Var * pVar )
{
    int * pValue2Node = p->pValue2Node;
    Lxu_Lit * pLitV, * pLitH;
    Lxu_Var * pVar2;
    int Coin;
    int lWeightCur;			// Just logical weight

    // start collecting the affected vars
    Lxu_MatrixRingVarsStart( p );
    // go through all the literals of this variable
    for ( pLitV = pVar->lLits.pHead; pLitV; pLitV = pLitV->pVNext )
        // for this literal, go through all the horizontal literals
        for ( pLitH = pLitV->pHPrev; pLitH; pLitH = pLitH->pHPrev )
        {
            // get another variable
            pVar2 = pLitH->pVar;
//            CounterAll++;
            // skip the var if it is already used
            if ( pVar2->pOrder )
                continue;
            // skip the var if it belongs to the same node
            if ( pValue2Node[pVar->iVar] == pValue2Node[pVar2->iVar] )
                continue;
            // collect the var
            Lxu_MatrixRingVarsAdd( p, pVar2 );
        }
    // stop collecting the selected vars
    Lxu_MatrixRingVarsStop( p );

    // iterate through the selected vars
    Lxu_MatrixForEachVarInRing( p, pVar2 )
    {
//        CounterTest++;
        // count the coincidence
        Coin = Lxu_SingleCountCoincidence( p, pVar2, pVar );
        assert( Coin > 0 );
        // get the new weight
        lWeightCur = Coin - 2;

		// Lxu_MatrixAddSingle will find out physicalWeight and 
		// compute total weight
		
        if ( lWeightCur >= 0 ) {
            Lxu_MatrixAddSingle( p, pVar2, pVar, lWeightCur );
		}
    }

    // unmark the vars
    Lxu_MatrixRingVarsUnmark( p );
}

/**Function*************************************************************

  Synopsis    [Computes the coincidence count of two columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lxu_SingleCountCoincidence( Lxu_Matrix * p, Lxu_Var * pVar1, Lxu_Var * pVar2 )
{
	Lxu_Lit * pLit1, * pLit2;
	int Result;

	// compute the coincidence count
	Result = 0;
	pLit1  = pVar1->lLits.pHead;
	pLit2  = pVar2->lLits.pHead;
	while ( 1 )
	{
		if ( pLit1 && pLit2 )
		{
			if ( pLit1->pCube->pVar->iVar == pLit2->pCube->pVar->iVar )
			{ // the variables are the same
			    if ( pLit1->iCube == pLit2->iCube )
			    { // the literals are the same
				    pLit1 = pLit1->pVNext;
				    pLit2 = pLit2->pVNext;
				    // add this literal to the coincidence
				    Result++;
					
					// The variable of the cube would be a fanout node. Add to output netlist

					// Do not add this variable of the cube to the list of shrunken nets 
					
			    }
			    else if ( pLit1->iCube < pLit2->iCube )
				    pLit1 = pLit1->pVNext;
			    else
				    pLit2 = pLit2->pVNext;
			}
			else if ( pLit1->pCube->pVar->iVar < pLit2->pCube->pVar->iVar )
				pLit1 = pLit1->pVNext;
			else
				pLit2 = pLit2->pVNext;
		}
		else if ( pLit1 && !pLit2 )
			pLit1 = pLit1->pVNext;
		else if ( !pLit1 && pLit2 )
			pLit2 = pLit2->pVNext;
		else
			break;
	}
	return Result;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


