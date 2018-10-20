/**CFile****************************************************************

  FileName    [shTravId.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Manipulating trav IDs and special lists of strashed nodes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shUtils.c,v 1.6 2004/05/12 04:30:11 alanmi Exp $]

***********************************************************************/

#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "process.h" 
#endif

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeShow( Sh_Manager_t * pMan, Sh_Node_t * pNode, char * pFileName )
{
    Sh_Network_t * pNetwork;
    pNetwork = Sh_NetworkCreate( pMan, pMan->nVars, 1 );
    pNetwork->ppOutputs[0] = pNode;   shRef( pNode );
    Sh_NetworkShow( pNetwork, pFileName );
    Sh_NetworkFree( pNetwork );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeShowArray( Sh_Manager_t * pMan, Sh_Node_t * pNodes[], int nNodes, char * pFileName )
{
    Sh_Network_t * pNetwork;
    int i;
    pNetwork = Sh_NetworkCreate( pMan, pMan->nVars, nNodes );
    for ( i = 0; i < nNodes; i++ )
    {
        pNetwork->ppOutputs[i] = pNodes[i];  shRef( pNodes[i] );
    }
    Sh_NetworkShow( pNetwork, pFileName );
    Sh_NetworkFree( pNetwork );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkShow( Sh_Network_t * pShNet, char * pFileName )
{
    FILE * pFile;
    char * FileNameIn;
    char * FileNameOut;
    char * FileGeneric;
    char FileNameDot[200];
    char FileNamePs[200];
    char CommandDot[1000];
#ifndef WIN32
    char CommandPs[1000];
#endif
//    int fShowCond;
    char * pProgDotName;
    char * pProgGsViewName;
//    int fVerbose;
//    int c;
//    int nStatesMax;

#ifdef WIN32
    pProgDotName = "dot.exe";
    pProgGsViewName = NULL;
#else
    pProgDotName = "dot";
    pProgGsViewName = "gv";
#endif

    FileNameIn  = pFileName;
    FileNameOut = NULL;

    // get the generic file name
    FileGeneric = Extra_FileNameGeneric( FileNameIn );
    sprintf( FileNameDot, "%s.dot", Extra_FileNameAppend(FileGeneric, "_aig") ); 
    sprintf( FileNamePs,  "%s.ps",  Extra_FileNameAppend(FileGeneric, "_aig") ); 
    FREE( FileGeneric );

    Sh_NetworkPrintDot( pShNet, FileNameDot );

    // check that the input file is okay
    if ( (pFile = fopen( FileNameDot, "r" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        goto usage;
    }
    fclose( pFile );

    // generate the DOT file
    sprintf( CommandDot,  "%s -Tps -o %s %s", pProgDotName, FileNamePs, FileNameDot ); 
    if ( system( CommandDot ) == -1 )
    {
#ifdef WIN32
        _unlink( FileNameDot );
#else
        unlink( FileNameDot );
#endif

        fprintf( stdout, "Cannot find \"%s\".\n", pProgDotName );
        goto usage;
    }
#ifdef WIN32
    _unlink( FileNameDot );
#else
    unlink( FileNameDot );
#endif

    // check that the input file is okay
    if ( (pFile = fopen( FileNamePs, "r" )) == NULL )
    {
        fprintf( stdout, "Cannot open intermediate file \"%s\".\n", FileNamePs );
        goto usage;
    }
    fclose( pFile ); 

#ifdef WIN32
    if ( _spawnl( _P_NOWAIT, "gsview32.exe", "gsview32.exe", FileNamePs, NULL ) == -1 )
        if ( _spawnl( _P_NOWAIT, "C:\\Program Files\\Ghostgum\\gsview\\gsview32.exe", 
            "C:\\Program Files\\Ghostgum\\gsview\\gsview32.exe", FileNamePs, NULL ) == -1 )
        {
            fprintf( stdout, "Cannot find \"%s\".\n", "gsview32.exe" );
            goto usage;
        }
#else
    sprintf( CommandPs,  "%s %s &", pProgGsViewName, FileNamePs ); 
    if ( system( CommandPs ) == -1 )
    {
        fprintf( stdout, "Cannot execute \"%s\".\n", FileNamePs );
        goto usage;
    }
#endif
usage:
    {}
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkPrintDot( Sh_Network_t * pShNet, char * pFileName )
{
    FILE * pFile;
    Sh_Node_t * pNode;
    int nNodes, nLevels, Level;
    int i;

    // collect the nodes in the DFS order
    nNodes = Sh_NetworkDfs( pShNet );
    if ( nNodes > 200 )
    {
        printf( "The network has more than %d nodes. DOT file is not written.\n", 100 );
        return;
    }

    Sh_NetworkForEachNodeSpecial( pShNet, pNode )
        pNode->pData = shNodeIsAnd(pNode);
    for ( i = 0; i < pShNet->nOutputs; i++ )
        Sh_Regular(pShNet->ppOutputs[i])->pData = 2;


    // perform the reachability analysis and set the level of the states
    nLevels = Sh_NetworkGetNumLevels( pShNet );

    // set the node numbers 
//    Sh_NetworkSetNumbers( pShNet );

    // start the stream
	pFile = fopen( pFileName, "w" );

	// write the DOT header
	fprintf( pFile, "# %s\n",  "And-Inv graph generated by Sh_NetworkPrintDot() procedure in MVSIS 2.0." );
	fprintf( pFile, "\n" );
	fprintf( pFile, "digraph AIG {\n" );
	fprintf( pFile, "size = \"7.5,10\";\n" );
//	fprintf( pFile, "ranksep = 0.5;\n" );
//	fprintf( pFile, "nodesep = 0.5;\n" );
	fprintf( pFile, "center = true;\n" );
//  fprintf( pFile, "edge [fontsize = 10];\n" );
//	fprintf( pFile, "edge [dir = none];\n" );
	fprintf( pFile, "\n" );

	// labels on the left of the picture
	fprintf( pFile, "{\n" );
	fprintf( pFile, "  node [shape = plaintext];\n" );
	fprintf( pFile, "  edge [style = invis];\n" );
	fprintf( pFile, "  LevelTitle1 [label=\"\"];\n" );
	fprintf( pFile, "  LevelTitle2 [label=\"\"];\n" );
	// generate node names with labels
	for ( Level = 0; Level <= nLevels; Level++ )
	{
		// the visible node name
		fprintf( pFile, "  Level%d", Level );
		fprintf( pFile, " [label = " );
		// label name
		fprintf( pFile, "\"" );
//        if ( Level != nLevels )
//		    fprintf( pFile, "%d", Level+1 );
		fprintf( pFile, "\"" );
		fprintf( pFile, "];\n" );
	}

	fprintf( pFile, "  LevelTitle1 ->  LevelTitle2 ->" );
	// genetate the sequence of visible/invisible nodes to mark levels
	for ( Level = 0; Level <= nLevels; Level++ )
	{
		// the visible node name
		fprintf( pFile, "  Level%d",  Level );
		// the connector
		if ( Level != nLevels )
			fprintf( pFile, " ->" );
		else
			fprintf( pFile, ";" );
	}
	fprintf( pFile, "\n" );
	fprintf( pFile, "}" );
	fprintf( pFile, "\n" );
	fprintf( pFile, "\n" );

	// generate title box on top
	fprintf( pFile, "{\n" );
	fprintf( pFile, "  rank = same;\n" );
	fprintf( pFile, "  LevelTitle1;\n" );
	fprintf( pFile, "  title1 [shape=plaintext,\n" );
	fprintf( pFile, "          fontsize=20,\n" );
	fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
	fprintf( pFile, "          label=\"" );
	fprintf( pFile, "%s", "And-Inv graph generated by MVSIS 2.0." );
	fprintf( pFile, "\\n" );
	fprintf( pFile, "Benchmark \\\"%s\\\". ", "unknown" );
	fprintf( pFile, "Time was %s. ",  Extra_TimeStamp() );
	fprintf( pFile, "\"\n" );
	fprintf( pFile, "         ];\n" );
	fprintf( pFile, "}" );
	fprintf( pFile, "\n" );
	fprintf( pFile, "\n" );

	// generate statistics box
	fprintf( pFile, "{\n" );
	fprintf( pFile, "  rank = same;\n" );
	fprintf( pFile, "  LevelTitle2;\n" );
	fprintf( pFile, "  title2 [shape=plaintext,\n" );
	fprintf( pFile, "          fontsize=18,\n" );
	fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
	fprintf( pFile, "          label=\"" );
	fprintf( pFile, "The graph has %d nodes and %d levels.", nNodes, nLevels );
    fprintf( pFile, "\\n" );
/*
    fprintf( pFile, "%d inputs  ",       pAut->nVars );
	fprintf( pFile, "%d states  ",       pAut->nStates );
	fprintf( pFile, "%d transitions",    Aut_AutoCountTransitions(pAut) );
//	fprintf( pFile, "%d products",       Aut_AutoCountProducts(pAut) );
	fprintf( pFile, "\\n" );
*/
/*
    fprintf( pFile, "Inputs = {" );
    for ( s = 0; s < pAut->nVars; s++ )
    {
        fprintf( pFile, "%s %s", ((s==0)? "": ","),  pAut->pVars[s]->pName );
        if ( pAut->pVars[s]->nValues > 2 )
            fprintf( pFile, "(%d)", pAut->pVars[s]->nValues );
    }
    fprintf( pFile, " }" );
	fprintf( pFile, "\\n" );
*/

	fprintf( pFile, "\"\n" );
	fprintf( pFile, "         ];\n" );
	fprintf( pFile, "}" );
	fprintf( pFile, "\n" );
	fprintf( pFile, "\n" );
/*
	// generate the square node on top
	fprintf( pFile, "{\n" );
	fprintf( pFile, "  rank = same;\n" );
	fprintf( pFile, "  node [shape=polygon, sides=7, peripheries=3];\n" );
	fprintf( pFile, "  edge [style = invis];\n" );
	for ( out = 0; out < pNet->nRoots; out++ )
		fprintf( pFile, "  title3_%d [label=\"%s\"];\n", out, pOutputs[out] );
	fprintf( pFile, "}" );
	fprintf( pFile, "\n" );
	fprintf( pFile, "\n" );
*/

	// generate nodes of each rank
	for ( Level = 0; Level <= nLevels; Level++ )
	{
	    fprintf( pFile, "{\n" );
	    fprintf( pFile, "  rank = same;\n" );
//		fprintf( pFile, "  node [shape=record,width=1,height=0.75];\n" );
//		fprintf( pFile, "  node [shape = circle];\n" );
		// the labeling node of this level
		fprintf( pFile, "  Level%d;\n",  Level );

        Sh_NetworkForEachNodeSpecial( pShNet, pNode )
            if ( pNode->pData2 == (unsigned)(nLevels-Level) )
            {
	            fprintf( pFile, "  Node%d [label = \"%d\"", pNode->index, Sh_NodeReadIndex(pNode) );
                if ( pNode->pData == 0 )
                    fprintf( pFile, ", shape = triangle" );
                else
                    fprintf( pFile, ", shape = ellipse" );
                if ( pNode->pData == 2 )
                    fprintf( pFile, ", color = coral, fillcolor = coral" );
//                else
//                    fprintf( pFile, ", color = \".3 .7 1.0\", fillcolor = \".3 .7 1.0\"" );

    //            fprintf( pFile, "%s", (s == 0? ", shape = octagon": "") );
    //            fprintf( pFile, "%s", (pAut->pStates[s]->fAcc? ", style = bold": "") );
    //            fprintf( pFile, "%s", (pState->fAcc? ", color = \".3 .7 1.0\", fillcolor = \".3 .7 1.0\"": ", color = coral, fillcolor = coral") );
	            fprintf( pFile, "];\n" );
            }
		fprintf( pFile, "}" );
		fprintf( pFile, "\n" );
		fprintf( pFile, "\n" );
	}

/*
    {
		fprintf( pFile, "{\n" );
   	    fprintf( pFile, "  node [fixedsize = true];\n" );
	    fprintf( pFile, "  node [fontsize = 14];\n" );
//		fprintf( pFile, "  node [shape = circle];\n" );
		fprintf( pFile, "  node [hight = 0.7];\n" );
		fprintf( pFile, "  node [width = 1.0];\n" );
//		fprintf( pFile, "  node [style = bold];\n" );
        // print the nodes
//        s = 0;
//        for ( s = 0; s < pAut->nStates; s++ )
        Sh_NetworkForEachNodeSpecial( pShNet, pNode )
        {
	        fprintf( pFile, "  Node%d [style=filled, label = \"%d\"", pNode->Num, pNode->Num );
//            fprintf( pFile, "%s", (s == 0? ", shape = octagon": "") );
//            fprintf( pFile, "%s", (pAut->pStates[s]->fAcc? ", style = bold": "") );
//            fprintf( pFile, "%s", (pState->fAcc? ", color = \".3 .7 1.0\", fillcolor = \".3 .7 1.0\"": ", color = coral, fillcolor = coral") );
	        fprintf( pFile, "];\n" );
//            pNode-->uData = s;
//            s++;
        }
		fprintf( pFile, "}" );
		fprintf( pFile, "\n" );
		fprintf( pFile, "\n" );
    }
*/
    
	// generate invisible edges from the square down
	fprintf( pFile, "title1 -> title2 [style = invis];\n" );

    // create edges to the PO nodes
    for ( i = 0; i < pShNet->nOutputs; i++ )
    {
        pNode = pShNet->ppOutputs[i];
        fprintf( pFile, "title2 -> Node%d", Sh_Regular(pNode)->index );
        fprintf( pFile, " [style = %s, color = coral]", Sh_IsComplement(pNode)? "dashed" : "bold" );
        fprintf( pFile, ";\n" );
    }

	// generate edges
    Sh_NetworkForEachNodeSpecial( pShNet, pNode )
    {
        if ( !shNodeIsAnd(pNode) )
            continue;
    	// generate the edge from this node to the next (or input variable)
		fprintf( pFile, "Node%d",  pNode->index );
		fprintf( pFile, " -> " );
		fprintf( pFile, "Node%d",  Sh_Regular(pNode->pOne)->index );
        fprintf( pFile, " [style = %s]", Sh_IsComplement(pNode->pOne)? "dashed" : "bold" );
		fprintf( pFile, ";\n" );
    	// generate the edge from this node to the next (or input variable)
		fprintf( pFile, "Node%d",  pNode->index );
		fprintf( pFile, " -> " );
		fprintf( pFile, "Node%d",  Sh_Regular(pNode->pTwo)->index );
        fprintf( pFile, " [style = %s]", Sh_IsComplement(pNode->pTwo)? "dashed" : "bold" );
		fprintf( pFile, ";\n" );
    }

	fprintf( pFile, "}" );
	fprintf( pFile, "\n" );
	fprintf( pFile, "\n" );
	fclose( pFile );
}



static DdNode * Sh_NodeComputeBdd_rec( DdManager * dd, Sh_Manager_t * pMan, Sh_Node_t * pNode );
static void Sh_NodeDerefBdd_rec( DdManager * dd, Sh_Manager_t * pMan, Sh_Node_t * pNode );

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodePrintFunction( Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    DdManager * dd;
    DdNode * bFunc;
    int i;
    // start the manager
    dd = Cudd_Init( pMan->nVars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // assign the elementary variables
    for ( i = 0; i < pMan->nVars; i++ )
        pMan->pVars[i]->pData = (unsigned)dd->vars[i];
    // build the BDD
    Sh_ManagerIncrementTravId( pMan );
    bFunc = Sh_NodeComputeBdd_rec( dd, pMan, Sh_Regular(pNode) );  
    if ( Sh_IsComplement(pNode) )
        bFunc = Cudd_Not( bFunc );
PRB( dd, bFunc );
    Sh_ManagerIncrementTravId( pMan );
    Sh_NodeDerefBdd_rec( dd, pMan, Sh_Regular(pNode) );
    // quit the manager
    Extra_StopManager( dd );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Sh_NodeDeriveBdd( DdManager * dd, Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    DdNode * bFunc;
    // build the BDD
    Sh_ManagerIncrementTravId( pMan );
    bFunc = Sh_NodeComputeBdd_rec( dd, pMan, Sh_Regular(pNode) );  Cudd_Ref( bFunc );
    if ( Sh_IsComplement(pNode) )
        bFunc = Cudd_Not( bFunc );
    Sh_ManagerIncrementTravId( pMan );
    Sh_NodeDerefBdd_rec( dd, pMan, Sh_Regular(pNode) );
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Replaces a set of variables by a set of variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Sh_NodeComputeBdd_rec( DdManager * dd, Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    DdNode * bCof0, * bCof1, * bRes;
    assert( !Sh_IsComplement(pNode) );
    if ( shNodeIsConst(pNode) )
        return dd->one;
    if ( shNodeIsVar(pNode) )
        return dd->vars[Sh_NodeReadIndex(pNode)];
    // check if the result is already computed
    if ( Sh_NodeIsTravIdCurrent(pMan, pNode) )
        return (DdNode *)pNode->pData; 
    Sh_NodeSetTravIdCurrent( pMan, pNode );

    // solve the problem for the cofactors
    bCof0 = Sh_NodeComputeBdd_rec( dd, pMan, Sh_Regular(pNode->pOne) );
    bCof1 = Sh_NodeComputeBdd_rec( dd, pMan, Sh_Regular(pNode->pTwo) );
    if ( Sh_IsComplement(pNode->pOne) )
        bCof0 = Cudd_Not(bCof0);
    if ( Sh_IsComplement(pNode->pTwo) )
        bCof1 = Cudd_Not(bCof1);
    bRes = Cudd_bddAnd( dd, bCof0, bCof1 );   Cudd_Ref( bRes );
    // set the computed result
    pNode->pData = (unsigned)bRes;
    return bRes;
}

/**Function*************************************************************

  Synopsis    [Replaces a set of variables by a set of variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeDerefBdd_rec( DdManager * dd, Sh_Manager_t * pMan, Sh_Node_t * pNode )
{
    assert( !Sh_IsComplement(pNode) );
    if ( shNodeIsConst(pNode) )
        return;
    if ( shNodeIsVar(pNode) )
        return;
    // check if the result is already computed
    if ( Sh_NodeIsTravIdCurrent(pMan, pNode) )
        return; 
    Sh_NodeSetTravIdCurrent( pMan, pNode );
    Cudd_RecursiveDeref( dd, (DdNode *)pNode->pData );
    Sh_NodeDerefBdd_rec( dd, pMan, Sh_Regular(pNode->pOne) );
    Sh_NodeDerefBdd_rec( dd, pMan, Sh_Regular(pNode->pTwo) );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


