/**CFile****************************************************************

  FileName    [dsdTree.c]

  PackageName [DSD: Disjoint-support decomposition package.]

  Synopsis    [Managing the decomposition tree.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 8.0. Started - September 22, 2003.]

  Revision    [$Id: dsdTree.c,v 1.1 2003/12/02 05:32:28 alanmi Exp $]

***********************************************************************/

#include "dsdInt.h"

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

static void Dsd_TreeUnmark_rec( Dsd_Node_t * pNode );
static void Dsd_TreeGetInfo_rec( Dsd_Node_t * pNode, int RankCur );
static int  Dsd_TreeCountNonTerminalNodes_rec( Dsd_Node_t * pNode );
static int  Dsd_TreeCountPrimeNodes_rec( Dsd_Node_t * pNode );
static int  Dsd_TreeCollectDecomposableVars_rec( DdManager * dd, Dsd_Node_t * pNode, int * pVars, int * nVars );
static void Dsd_TreeCollectNodesDfs_rec( Dsd_Node_t * pNode, Dsd_Node_t * ppNodes[], int * pnNodes );
static void Dsd_TreePrint_rec( FILE * pFile, Dsd_Node_t * pNode, int fCcmp, char * pInputNames[], char * pOutputName, int nOffset, int * pSigCounter, int fShortNames );


////////////////////////////////////////////////////////////////////////
///                      STATIC VARIABLES                            ///
////////////////////////////////////////////////////////////////////////

static int s_DepthMax;
static int s_GateSizeMax;

static int s_CounterBlocks;
static int s_CounterPos;
static int s_CounterNeg;
static int s_CounterNo;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create the DSD node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dsd_Node_t * Dsd_TreeNodeCreate( int Type, int nDecs, int BlockNum )
{
	// allocate memory for this node 
	Dsd_Node_t * p = (Dsd_Node_t *) malloc( sizeof(Dsd_Node_t) );
	memset( p, 0, sizeof(Dsd_Node_t) );
	p->Type       = Type;       // the type of this block
	p->nDecs      = nDecs;      // the number of decompositions
	if ( p->nDecs )
	{
		p->pDecs      = (Dsd_Node_t **) malloc( p->nDecs * sizeof(Dsd_Node_t *) );
		p->pDecs[0]   = NULL;
	}
	return p;
}

/**Function*************************************************************

  Synopsis    [Frees the DSD node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeNodeDelete( DdManager * dd, Dsd_Node_t * pNode )
{
	if ( pNode->G )  Cudd_RecursiveDeref( dd, pNode->G );
	if ( pNode->S )  Cudd_RecursiveDeref( dd, pNode->S );
	FREE( pNode->pDecs );
	FREE( pNode );
}

/**Function*************************************************************

  Synopsis    [Unmarks the decomposition tree.]

  Description [This function assumes that originally pNode->nVisits are 
  set to zero!]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeUnmark( Dsd_Manager_t * pDsdMan )
{
	int i;
	for ( i = 0; i < pDsdMan->nRoots; i++ )
		Dsd_TreeUnmark_rec( Dsd_Regular( pDsdMan->pRoots[i] ) );
}


/**Function*************************************************************

  Synopsis    [Recursive unmarking.]

  Description [This function should be called with a non-complemented 
  pointer.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeUnmark_rec( Dsd_Node_t * pNode )
{
	int i;

	assert( pNode );
	assert( !Dsd_IsComplement( pNode ) );
	assert( pNode->nVisits > 0 );

	if ( --pNode->nVisits ) // if this is not the last visit, return
		return;

	// upon the last visit, go through the list of successors and call recursively 
	if ( pNode->Type != DSD_NODE_BUF && pNode->Type != DSD_NODE_CONST1 )
	for ( i = 0; i < pNode->nDecs; i++ )
		Dsd_TreeUnmark_rec( Dsd_Regular(pNode->pDecs[i]) );
}

/**Function*************************************************************

  Synopsis    [Getting information about the node.]

  Description [This function computes the max depth and the max gate size 
  of the tree rooted at the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeNodeGetInfo( Dsd_Manager_t * pDsdMan, int * DepthMax, int * GateSizeMax )
{
	int i;
	s_DepthMax    = 0;
	s_GateSizeMax = 0;

	for ( i = 0; i < pDsdMan->nRoots; i++ )
    	Dsd_TreeGetInfo_rec( Dsd_Regular( pDsdMan->pRoots[i] ), 0 );

	if ( DepthMax ) 
		*DepthMax     = s_DepthMax;
	if ( GateSizeMax ) 
		*GateSizeMax  = s_GateSizeMax;
}

/**Function*************************************************************

  Synopsis    [Getting information about the node.]

  Description [This function computes the max depth and the max gate size 
  of the tree rooted at the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeNodeGetInfoOne( Dsd_Node_t * pNode, int * DepthMax, int * GateSizeMax )
{
	s_DepthMax    = 0;
	s_GateSizeMax = 0;

	Dsd_TreeGetInfo_rec( Dsd_Regular(pNode), 0 );

	if ( DepthMax ) 
		*DepthMax     = s_DepthMax;
	if ( GateSizeMax ) 
		*GateSizeMax  = s_GateSizeMax;
}


/**Function*************************************************************

  Synopsis    [Performs the recursive step of Dsd_TreeNodeGetInfo().]

  Description [pNode is the node, for the tree rooted in which we are 
  determining info. RankCur is the current rank to assign to the node.
  fSetRank is the flag saying whether the rank will be written in the 
  node. s_DepthMax is the maximum depths of the tree. s_GateSizeMax is 
  the maximum gate size.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeGetInfo_rec( Dsd_Node_t * pNode, int RankCur )
{
	int i;
	int GateSize;

	assert( pNode );
	assert( !Dsd_IsComplement( pNode ) );
	assert( pNode->nVisits >= 0 );

	// we don't want the two-input gates to count for non-decomposable blocks
	if ( pNode->Type == DSD_NODE_OR  || 
		 pNode->Type == DSD_NODE_EXOR )
		GateSize = 2;
	else
		GateSize = pNode->nDecs;

	// update the max size of the node
	if ( s_GateSizeMax < GateSize )
		 s_GateSizeMax = GateSize;

	if ( pNode->nDecs < 2 )
		return;

	// update the max rank
	if ( s_DepthMax < RankCur+1 )
		 s_DepthMax = RankCur+1;

	// call recursively
	for ( i = 0; i < pNode->nDecs; i++ )
		Dsd_TreeGetInfo_rec( Dsd_Regular(pNode->pDecs[i]), RankCur+1 );
}

/**Function*************************************************************

  Synopsis    [Counts non-terminal nodes of the DSD tree.]

  Description [Nonterminal nodes include all the nodes with the
  support more than 1. These are OR, EXOR, and PRIME nodes. They
  do not include the elementary variable nodes and the constant 1
  node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountNonTerminalNodes( Dsd_Manager_t * pDsdMan )
{
	int Counter, i;
    Counter = 0;
	for ( i = 0; i < pDsdMan->nRoots; i++ )
		Counter += Dsd_TreeCountNonTerminalNodes_rec( Dsd_Regular( pDsdMan->pRoots[i] ) );
	Dsd_TreeUnmark( pDsdMan );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountNonTerminalNodesOne( Dsd_Node_t * pRoot )
{
	int Counter = 0;

	// go through the list of successors and call recursively 
	Counter = Dsd_TreeCountNonTerminalNodes_rec( Dsd_Regular(pRoot) );

	Dsd_TreeUnmark_rec( Dsd_Regular(pRoot) );
	return Counter;
}


/**Function*************************************************************

  Synopsis    [Counts non-terminal nodes for one root.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountNonTerminalNodes_rec( Dsd_Node_t * pNode )
{
	int i;
	int Counter = 0;

	assert( pNode );
	assert( !Dsd_IsComplement( pNode ) );
	assert( pNode->nVisits >= 0 );

	if ( pNode->nVisits++ ) // if this is not the first visit, return zero
		return 0;

	if ( pNode->nDecs <= 1 )
		return 0;

	// upon the first visit, go through the list of successors and call recursively 
	for ( i = 0; i < pNode->nDecs; i++ )
		Counter += Dsd_TreeCountNonTerminalNodes_rec( Dsd_Regular(pNode->pDecs[i]) );

	return Counter + 1;
}


/**Function*************************************************************

  Synopsis    [Counts prime nodes of the DSD tree.]

  Description [Prime nodes are nodes with the support more than 2,
  that is not an OR or EXOR gate.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountPrimeNodes( Dsd_Manager_t * pDsdMan )
{
	int Counter, i;
    Counter = 0;
	for ( i = 0; i < pDsdMan->nRoots; i++ )
		Counter += Dsd_TreeCountPrimeNodes_rec( Dsd_Regular( pDsdMan->pRoots[i] ) );
	Dsd_TreeUnmark( pDsdMan );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts prime nodes for one root.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountPrimeNodesOne( Dsd_Node_t * pRoot )
{
	int Counter = 0;

	// go through the list of successors and call recursively 
	Counter = Dsd_TreeCountPrimeNodes_rec( Dsd_Regular(pRoot) );

	Dsd_TreeUnmark_rec( Dsd_Regular(pRoot) );
	return Counter;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCountPrimeNodes_rec( Dsd_Node_t * pNode )
{
	int i;
	int Counter = 0;

	assert( pNode );
	assert( !Dsd_IsComplement( pNode ) );
	assert( pNode->nVisits >= 0 );

	if ( pNode->nVisits++ ) // if this is not the first visit, return zero
		return 0;

	if ( pNode->nDecs <= 1 )
		return 0;

	// upon the first visit, go through the list of successors and call recursively 
	for ( i = 0; i < pNode->nDecs; i++ )
		Counter += Dsd_TreeCountPrimeNodes_rec( Dsd_Regular(pNode->pDecs[i]) );

	if ( pNode->Type == DSD_NODE_PRIME )
		Counter++;

	return Counter;
}


/**Function*************************************************************

  Synopsis    [Collects the decomposable vars on the PI side.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCollectDecomposableVars( Dsd_Manager_t * pDsdMan, int * pVars )
{
	int nVars;

	// set the vars collected to 0
	nVars = 0;
	Dsd_TreeCollectDecomposableVars_rec( pDsdMan->dd, Dsd_Regular(pDsdMan->pRoots[0]), pVars, &nVars );
	// return the number of collected vars
	return nVars;
}

/**Function*************************************************************

  Synopsis    [Implements the recursive part of Dsd_TreeCollectDecomposableVars().]

  Description [Adds decomposable variables as they are found to pVars and increments 
  nVars. Returns 1 if a non-dec node with more than 4 inputs was encountered 
  in the processed subtree. Returns 0, otherwise. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dsd_TreeCollectDecomposableVars_rec( DdManager * dd, Dsd_Node_t * pNode, int * pVars, int * nVars )
{
	int fSkipThisNode, i;
	Dsd_Node_t * pTemp;
	int fVerbose = 0;

	assert( pNode );
	assert( !Dsd_IsComplement( pNode ) );

	if ( pNode->nDecs <= 1 )
		return 0;

	// go through the list of successors and call recursively 
	fSkipThisNode = 0;
	for ( i = 0; i < pNode->nDecs; i++ )
		if ( Dsd_TreeCollectDecomposableVars_rec(dd, Dsd_Regular(pNode->pDecs[i]), pVars, nVars) )
			fSkipThisNode = 1;

	if ( !fSkipThisNode && (pNode->Type == DSD_NODE_OR || pNode->Type == DSD_NODE_EXOR || pNode->nDecs <= 4) )
	{
if ( fVerbose )
printf( "Node of type <%d> (OR=6,EXOR=8,RAND=1): ", pNode->Type );

		for ( i = 0; i < pNode->nDecs; i++ )
		{
			pTemp = Dsd_Regular(pNode->pDecs[i]);
			if ( pTemp->Type == DSD_NODE_BUF )
			{
				if ( pVars )
					pVars[ (*nVars)++ ] = pTemp->S->index;
				else
					(*nVars)++;
					
if ( fVerbose )
printf( "%d ", pTemp->S->index );
			}
		}
if ( fVerbose )
printf( "\n" );
	}
	else
		fSkipThisNode = 1;


	return fSkipThisNode;
}


/**Function*************************************************************

  Synopsis    [Creates the DFS ordered array of DSD nodes in the tree.]

  Description [The collected nodes do not include the terminal nodes
  and the constant 1 node. The array of nodes is returned. The number
  of entries in the array is returned in the variale pnNodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dsd_Node_t ** Dsd_TreeCollectNodesDfs( Dsd_Manager_t * pDsdMan, int * pnNodes )
{
	Dsd_Node_t ** ppNodes;
	int nNodes, nNodesAlloc;
	int i;

    nNodesAlloc = Dsd_TreeCountNonTerminalNodes(pDsdMan);
	nNodes  = 0;
	ppNodes = ALLOC( Dsd_Node_t *, nNodesAlloc );
	for ( i = 0; i < pDsdMan->nRoots; i++ )
		Dsd_TreeCollectNodesDfs_rec( Dsd_Regular(pDsdMan->pRoots[i]), ppNodes, &nNodes );
	Dsd_TreeUnmark( pDsdMan );
    assert( nNodesAlloc == nNodes );
	*pnNodes = nNodes;
	return ppNodes;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreeCollectNodesDfs_rec( Dsd_Node_t * pNode, Dsd_Node_t * ppNodes[], int * pnNodes )
{
	int i;
	assert( pNode );
	assert( !Dsd_IsComplement(pNode) );
	assert( pNode->nVisits >= 0 );

	if ( pNode->nVisits++ ) // if this is not the first visit, return zero
		return;
	if ( pNode->nDecs <= 1 )
		return;

	// upon the first visit, go through the list of successors and call recursively 
	for ( i = 0; i < pNode->nDecs; i++ )
		Dsd_TreeCollectNodesDfs_rec( Dsd_Regular(pNode->pDecs[i]), ppNodes, pnNodes );

	ppNodes[ (*pnNodes)++ ] = pNode;
}

/**Function*************************************************************

  Synopsis    [Prints the decompostion tree into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreePrint( FILE * pFile, Dsd_Manager_t * pDsdMan, char * pInputNames[], char * pOutputNames[], int fShortNames, int Output )
{
	Dsd_Node_t * pNode;
	int SigCounter;
	int i;
	SigCounter = 1;

	if ( Output == -1 )
	{
		for ( i = 0; i < pDsdMan->nRoots; i++ )
		{
			pNode = Dsd_Regular( pDsdMan->pRoots[i] );
			Dsd_TreePrint_rec( pFile, pNode, (pNode != pDsdMan->pRoots[i]), pInputNames, pOutputNames[i], 0, &SigCounter, fShortNames );
		}
	}
	else
	{
		assert( Output >= 0 && Output < pDsdMan->nRoots );
		pNode = Dsd_Regular( pDsdMan->pRoots[Output] );
		Dsd_TreePrint_rec( pFile, pNode, (pNode != pDsdMan->pRoots[Output]), pInputNames, pOutputNames[Output], 0, &SigCounter, fShortNames );
	}
}

/**Function*************************************************************

  Synopsis    [Prints the decompostion tree into file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dsd_TreePrint_rec( FILE * pFile, Dsd_Node_t * pNode, int fComp, char * pInputNames[], char * pOutputName, int nOffset, int * pSigCounter, int fShortNames )
{
	char Buffer[100];
	Dsd_Node_t * pInput;
	int * pInputNums;
	int fCompNew, i;

	assert( pNode->Type == DSD_NODE_BUF || pNode->Type == DSD_NODE_CONST1 || 
        pNode->Type == DSD_NODE_PRIME || pNode->Type == DSD_NODE_OR || pNode->Type == DSD_NODE_EXOR ); 

	Extra_PrintSymbols( pFile, ' ', nOffset, 0 );
	fprintf( pFile, "%s: ", pOutputName );
	pInputNums = ALLOC( int, pNode->nDecs );
	if ( pNode->Type == DSD_NODE_CONST1 )
	{
		if ( fComp )
			fprintf( pFile, " Constant 0.\n" );
		else
			fprintf( pFile, " Constant 1.\n" );
	}
	else if ( pNode->Type == DSD_NODE_BUF )
	{
		if ( fComp )
			fprintf( pFile, " NOT(" );
		else
			fprintf( pFile, " " );
		if ( fShortNames )
			fprintf( pFile, "%d", pNode->S->index );
		else
			fprintf( pFile, "%s", pInputNames[pNode->S->index] );
		if ( fComp )
			fprintf( pFile, ")" );
		fprintf( pFile, "\n" );
	}
	else if ( pNode->Type == DSD_NODE_PRIME )
	{
		// print the line
		fprintf( pFile, "PRIME(" );
		for ( i = 0; i < pNode->nDecs; i++ )
		{
			pInput   = Dsd_Regular( pNode->pDecs[i] );
			fCompNew = (int)( pInput != pNode->pDecs[i] );
			if ( i )
				fprintf( pFile, "," );
			if ( pInput->Type == DSD_NODE_BUF )
			{
				pInputNums[i] = 0;
				if ( fCompNew )
					fprintf( pFile, " NOT(" );
				else
					fprintf( pFile, " " );
				if ( fShortNames )
					fprintf( pFile, "%d", pInput->S->index );
				else
					fprintf( pFile, "%s", pInputNames[pInput->S->index] );
				if ( fCompNew )
					fprintf( pFile, ")" );
			}
			else
			{
				pInputNums[i] = (*pSigCounter)++;
				fprintf( pFile, " <%d>", pInputNums[i] );
			}
		}
		fprintf( pFile, " )\n" );
		// call recursively for the following blocks
		for ( i = 0; i < pNode->nDecs; i++ )
			if ( pInputNums[i] )
			{
				pInput   = Dsd_Regular( pNode->pDecs[i] );
				fCompNew = (int)( pInput != pNode->pDecs[i] );
				sprintf( Buffer, "<%d>", pInputNums[i] );
				Dsd_TreePrint_rec( pFile, Dsd_Regular( pNode->pDecs[i] ), fCompNew, pInputNames, Buffer, nOffset + 6, pSigCounter, fShortNames );
			}
	}
	else if ( pNode->Type == DSD_NODE_OR )
	{
		// print the line
		if ( fComp )
			fprintf( pFile, "AND(" );
		else
			fprintf( pFile, "OR(" );
		for ( i = 0; i < pNode->nDecs; i++ )
		{
			pInput = Dsd_Regular( pNode->pDecs[i] );
			fCompNew  = (int)( pInput != pNode->pDecs[i] );
			if ( i )
				fprintf( pFile, "," );
			if ( pInput->Type == DSD_NODE_BUF )
			{
				pInputNums[i] = 0;
				if ( fCompNew )
					fprintf( pFile, " NOT(" );
				else
					fprintf( pFile, " " );
				if ( fShortNames )
					fprintf( pFile, "%d", pInput->S->index );
				else
					fprintf( pFile, "%s", pInputNames[pInput->S->index] );
				if ( fCompNew )
					fprintf( pFile, ")" );
			}
			else
			{
				pInputNums[i] = (*pSigCounter)++;
				fprintf( pFile, " <%d>", pInputNums[i] );
			}
		}
		fprintf( pFile, " )\n" );
		// call recursively for the following blocks
		for ( i = 0; i < pNode->nDecs; i++ )
			if ( pInputNums[i] )
			{
				pInput = Dsd_Regular( pNode->pDecs[i] );
				fCompNew  = (int)( pInput != pNode->pDecs[i] );
				sprintf( Buffer, "<%d>", pInputNums[i] );
				Dsd_TreePrint_rec( pFile, Dsd_Regular( pNode->pDecs[i] ), fComp ^ fCompNew, pInputNames, Buffer, nOffset + 6, pSigCounter, fShortNames );
			}
	}
	else if ( pNode->Type == DSD_NODE_EXOR )
	{
		// print the line
		if ( fComp )
			fprintf( pFile, "NEXOR(" );
		else
			fprintf( pFile, "EXOR(" );
		for ( i = 0; i < pNode->nDecs; i++ )
		{
			pInput = Dsd_Regular( pNode->pDecs[i] );
			fCompNew  = (int)( pInput != pNode->pDecs[i] );
			if ( i )
				fprintf( pFile, "," );
			if ( pInput->Type == DSD_NODE_BUF )
			{
				pInputNums[i] = 0;
				if ( fCompNew )
					fprintf( pFile, " NOT(" );
				else
					fprintf( pFile, " " );
				if ( fShortNames )
					fprintf( pFile, "%d", pInput->S->index );
				else
					fprintf( pFile, "%s", pInputNames[pInput->S->index] );
				if ( fCompNew )
					fprintf( pFile, ")" );
			}
			else
			{
				pInputNums[i] = (*pSigCounter)++;
				fprintf( pFile, " <%d>", pInputNums[i] );
			}
		}
		fprintf( pFile, " )\n" );
		// call recursively for the following blocks
		for ( i = 0; i < pNode->nDecs; i++ )
			if ( pInputNums[i] )
			{
				pInput = Dsd_Regular( pNode->pDecs[i] );
				fCompNew  = (int)( pInput != pNode->pDecs[i] );
				sprintf( Buffer, "<%d>", pInputNums[i] );
				Dsd_TreePrint_rec( pFile, Dsd_Regular( pNode->pDecs[i] ), fCompNew, pInputNames, Buffer, nOffset + 6, pSigCounter, fShortNames );
			}
	}
	free( pInputNums );
}


/**Function*************************************************************

  Synopsis    [Retuns the function of one node of the decomposition tree.]

  Description [This is the old procedure. It is now superceded by the
  procedure Dsd_TreeGetPrimeFunction() found in "dsdLocal.c".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dsd_TreeGetPrimeFunctionOld( DdManager * dd, Dsd_Node_t * pNode, int fRemap ) 
{
	DdNode * bCof0,  * bCof1, * bCube0, * bCube1, * bNewFunc, * bTemp;
	int i;
	int fAllBuffs = 1;
	static int Permute[MAXINPUTS];

	assert( pNode );
	assert( !Dsd_IsComplement( pNode ) );
	assert( pNode->Type == DSD_NODE_PRIME );

	// transform the function of this block to depend on inputs
	// corresponding to the formal inputs

	// first, substitute those inputs that have some blocks associated with them
	// second, remap the inputs to the top of the manager (then, it is easy to output them)

	// start the function
	bNewFunc = pNode->G;  Cudd_Ref( bNewFunc );
	// go over all primary inputs
	for ( i = 0; i < pNode->nDecs; i++ )
	if ( pNode->pDecs[i]->Type != DSD_NODE_BUF ) // remap only if it is not the buffer
	{
		bCube0 = Extra_bddFindOneCube( dd, Cudd_Not(pNode->pDecs[i]->G) );  Cudd_Ref( bCube0 );
		bCof0 = Cudd_Cofactor( dd, bNewFunc, bCube0 );                     Cudd_Ref( bCof0 );
		Cudd_RecursiveDeref( dd, bCube0 );

		bCube1 = Extra_bddFindOneCube( dd,          pNode->pDecs[i]->G  );  Cudd_Ref( bCube1 );
		bCof1 = Cudd_Cofactor( dd, bNewFunc, bCube1 );                     Cudd_Ref( bCof1 );
		Cudd_RecursiveDeref( dd, bCube1 );

		Cudd_RecursiveDeref( dd, bNewFunc );

		// use the variable in the i-th level of the manager
//		bNewFunc = Cudd_bddIte( dd, dd->vars[dd->invperm[i]],bCof1,bCof0 );     Cudd_Ref( bNewFunc );
		// use the first variale in the support of the component
		bNewFunc = Cudd_bddIte( dd, dd->vars[pNode->pDecs[i]->S->index],bCof1,bCof0 );     Cudd_Ref( bNewFunc );
		Cudd_RecursiveDeref( dd, bCof0 );
		Cudd_RecursiveDeref( dd, bCof1 );
	}

	if ( fRemap )
	{
		// remap the function to the top of the manager
		// remap the function to the first variables of the manager
		for ( i = 0; i < pNode->nDecs; i++ )
	//		Permute[ pNode->pDecs[i]->S->index ] = dd->invperm[i];
			Permute[ pNode->pDecs[i]->S->index ] = i;

		bNewFunc = Cudd_bddPermute( dd, bTemp = bNewFunc, Permute );   Cudd_Ref( bNewFunc );
		Cudd_RecursiveDeref( dd, bTemp );
	}

	Cudd_Deref( bNewFunc );
	return bNewFunc;
}


////////////////////////////////////////////////////////////////////////
///                           END OF FILE                            ///
////////////////////////////////////////////////////////////////////////
