/**CFile****************************************************************

  FileName    [ntkiShow.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Shows the topological structure of the network using DOT software.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiShow.c,v 1.3 2005/03/31 01:29:40 casem Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Ntk_NetworkPrintDot( Ntk_Network_t * pNet,
                                 Ntk_Node_t ** ppRoots,
                                 int nRoots,
                                 bool fTfiOnly,
                                 char * pFileName );

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
void Ntk_NetworkShow( Ntk_Network_t * pNet, Ntk_Node_t ** ppNodes, int nNodes, bool fTfiOnly )
{
    FILE * pFile;
    char * FileNameIn;
    char * FileNameOut;
    char * FileGeneric;
    char * pAddition;
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

    FileNameIn  = NULL;
    FileNameOut = NULL;

    // get the generic file name
    FileGeneric = Extra_FileNameGeneric( pNet->pName );
    pAddition   = ppNodes ? Ntk_NodeGetNamePrintable(ppNodes[0]) : "";
    sprintf( FileNameDot, "%s.dot", Extra_FileNameAppend(FileGeneric, pAddition) ); 
    sprintf( FileNamePs,  "%s.ps",  Extra_FileNameAppend(FileGeneric, pAddition) ); 
    FREE( FileGeneric );

    Ntk_NetworkPrintDot( pNet, ppNodes, nNodes, fTfiOnly, FileNameDot );

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
void Ntk_NetworkPrintDot( Ntk_Network_t * pNet,
                          Ntk_Node_t ** ppRoots,
                          int nRoots,
                          bool fTfiOnly,
                          char * pFileName )
{
    FILE * pFile;
    Ntk_Node_t * pNode;
    int nNodes, nLevels, Level;
    int i,j;
    Ntk_Node_t ** pFanins;
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t ** pCovers;
    int nInputs;
    Mvc_Cube_t * pCube;
    Vm_VarMap_t * pVm;
    int * pValuesFirst;

    // collect the nodes in the DFS order
    if ( ppRoots && fTfiOnly )
        Ntk_NetworkDfsNodes( pNet, ppRoots, nRoots, 1 );
    else
        Ntk_NetworkDfs( pNet, 1 );
    nNodes = Ntk_NetworkCountSpecial( pNet );
    if ( nNodes > 200 )
    {
        printf( "The network has more than %d nodes. DOT file is not written.\n", 100 );
        return;
    }

    // set the level of the nodes
    nLevels = Ntk_NetworkGetNumLevels( pNet );
    if ( ppRoots && fTfiOnly ) {
      nLevels = -1;
      for (i = 0; i < nRoots; i++) {
        if ((nLevels < ppRoots[i]->Level) || (nLevels == -1)) {
          nLevels = ppRoots[i]->Level;
        }
      }
    }

    // start the stream
	pFile = fopen( pFileName, "w" );

	// write the DOT header
	fprintf( pFile, "# %s\n",  "Network structure generated by Ntk_NetworkPrintDot() procedure in MVSIS 2.0." );
	fprintf( pFile, "\n" );
	fprintf( pFile, "digraph network {\n" );
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
	fprintf( pFile, "%s", "Network structure generated by MVSIS 2.0." );
	fprintf( pFile, "\\n" );
	fprintf( pFile, "Benchmark \\\"%s\\\". ", pNet->pName );
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
//	fprintf( pFile, "The graph has %d nodes and %d levels.", nNodes, nLevels );
//   fprintf( pFile, "\\n" );

    fprintf( pFile, "%d CIs    ",    Ntk_NetworkReadCiNum(pNet) );
	fprintf( pFile, "%d COs    ",    Ntk_NetworkReadCoNum(pNet) );
	fprintf( pFile, "%d latches",  Ntk_NetworkReadLatchNum(pNet) );
	fprintf( pFile, "\\n" );

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

        Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        {
            if ( pNode->Type == MV_NODE_CO )
                continue;
            if ( pNode->Level == nLevels-Level )
            {
	            fprintf( pFile, "  Node%d [label = \"%s\"", pNode->Id, Ntk_NodeGetNamePrintable(pNode) );
                if ( pNode->Type == MV_NODE_CI )
                    fprintf( pFile, ", shape = triangle" );
                else
                    fprintf( pFile, ", shape = ellipse" );
                //                if ( pNode == pRoot ) //Ntk_NodeIsCoDriver(pNode) )
                //                    fprintf( pFile, ", color = coral, fillcolor = coral" );
//                else
//                    fprintf( pFile, ", color = \".3 .7 1.0\", fillcolor = \".3 .7 1.0\"" );

    //            fprintf( pFile, "%s", (s == 0? ", shape = octagon": "") );
    //            fprintf( pFile, "%s", (pAut->pStates[s]->fAcc? ", style = bold": "") );
    //            fprintf( pFile, "%s", (pState->fAcc? ", color = \".3 .7 1.0\", fillcolor = \".3 .7 1.0\"": ", color = coral, fillcolor = coral") );
	            fprintf( pFile, "];\n" );
            }
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
        Ntk_NetworkForEachNodeSpecial( pNet, pNode )
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

    if ( ppRoots && fTfiOnly )
    {
      for (i = 0; i < nRoots; i++) {
        fprintf( pFile, "Node%d -> title2", ppRoots[i]->Id );
        fprintf( pFile, " [color = coral]" );
        fprintf( pFile, ";\n" );
      }
    }
    else
    {
        Ntk_NetworkForEachCo( pNet, pNode )
        {
          //            fprintf( pFile, "Node%d -> title2",
          //            Ntk_NodeReadFaninNode(pNode,0)->Id );
            fprintf( pFile, "Node%d -> title2", pNode->Id );
            fprintf( pFile, " [color = coral]" );
            fprintf( pFile, ";\n" );
        }
    }

	// generate edges
    //    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    //    {
    //        Ntk_Node_t * pFanin;
    //        Ntk_Pin_t * pPin;
    //        if ( pNode->Type != MV_NODE_INT )
    //            continue;
    //        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
    //        {
    //    	    // generate the edge from this node to the next (or input variable)
    //		    fprintf( pFile, "Node%d",  pNode->Id );
    //		    fprintf( pFile, " -> " );
    //		    fprintf( pFile, "Node%d",  pFanin->Id );
    //            fprintf( pFile, " [style = %s]", Ntk_IsComplement(pNode->pOne)? "dashed" : "bold" );
    //		    fprintf( pFile, ";\n" );
    //        }
    //    }

	// generate edges
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        if ( pNode->Type != MV_NODE_INT )
            continue;
        // get the cover
        pVm  = Ntk_NodeReadFuncVm(pNode);
        pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);
        pCvr = Ntk_NodeGetFuncCvr(pNode);
        pCovers = Cvr_CoverReadIsets( pCvr );
        nInputs = Ntk_NodeReadFaninNum(pNode);
        // get the fanins
        pFanins = pNode->pNet->pArray1;
        Ntk_NodeReadFanins( pNode, pFanins );
        // iterate through the i-sets
        for ( i = 0; i < pNode->nValues; i++ ) {
          if ( pCovers[i] ) {
            Mvc_CoverForEachCube( pCovers[i], pCube ) {
              for (j=0; j < nInputs; j++) {
                // generate the edge from this node to the next (or input variable)
                fprintf( pFile, "Node%d",  pFanins[j]->Id );
                fprintf( pFile, " -> " );
                fprintf( pFile, "Node%d",  pNode->Id );
                fprintf( pFile,
                         " [style = %s]",
                         // is this fanin complemented in the cover?
                         Mvc_CubeBitValue( pCube,  pValuesFirst[j]) 
                           ? "dotted"
                           : "bold" );
                fprintf( pFile, ";\n" );
              }
            }
          }
        }
    }

	fprintf( pFile, "}" );
	fprintf( pFile, "\n" );
	fprintf( pFile, "\n" );
	fclose( pFile );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintGml( Ntk_Network_t * pNet, Ntk_Node_t * pRoot, bool fTfiOnly, char * pFileName )
{
    FILE * pFile;
    Ntk_Node_t * pNode, * pFanin;
    Ntk_Pin_t * pPin;
    Ntk_Latch_t * pLatch;

    // start the stream
	pFile = fopen( pFileName, "w" );
	fprintf( pFile, "graph [\n" );

    // output the POs
    fprintf( pFile, "\n" );
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        fprintf( pFile, "    node [ id %5d label \"%s\"\n", pNode->Id, Ntk_NodeGetNamePrintable(pNode) );
        fprintf( pFile, "        graphics [ type \"triangle\" fill \"#00FFFF\" ]\n" );   // blue
        fprintf( pFile, "    ]\n" );
    }
    // output the PIs
    fprintf( pFile, "\n" );
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        fprintf( pFile, "    node [ id %5d label \"%s\"\n", pNode->Id, Ntk_NodeGetNamePrintable(pNode) );
        fprintf( pFile, "        graphics [ type \"triangle\" fill \"#00FF00\" ]\n" );   // green
        fprintf( pFile, "    ]\n" );
    }
    // output the latches
    fprintf( pFile, "\n" );
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        pNode = pLatch->pOutput;
        fprintf( pFile, "    node [ id %5d label \"%s\"\n", pNode->Id + 1000000, Ntk_NodeGetNamePrintable(pNode) );
        fprintf( pFile, "        graphics [ type \"rectangle\" fill \"#FF0000\" ]\n" );   // red
        fprintf( pFile, "    ]\n" );
    }
    // output the nodes
    fprintf( pFile, "\n" );
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        fprintf( pFile, "    node [ id %5d label \"%s\"\n", pNode->Id, Ntk_NodeGetNamePrintable(pNode) );
        fprintf( pFile, "        graphics [ type \"ellipse\" fill \"#CCCCFF\" ]\n" );     // grey
        fprintf( pFile, "    ]\n" );
    }

    // output the edges
    fprintf( pFile, "\n" );
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        Ntk_NodeForEachFanin( pNode, pPin, pFanin )
        {
            if ( pFanin->Type == MV_NODE_CI && pFanin->Subtype == MV_BELONGS_TO_LATCH )
            fprintf( pFile, "    edge [ source %5d   target %5d\n", pNode->Id, pFanin->Id + 1000000 );
            else
            fprintf( pFile, "    edge [ source %5d   target %5d\n", pNode->Id, pFanin->Id );
            fprintf( pFile, "        graphics [ type \"line\" arrow \"first\" ]\n" );
            fprintf( pFile, "    ]\n" );
        }
    }
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        pFanin = Ntk_NodeReadFaninNode(pNode,0);
        if ( pFanin->Type == MV_NODE_CI && pFanin->Subtype == MV_BELONGS_TO_LATCH )
        fprintf( pFile, "    edge [ source %5d   target %5d\n", pNode->Id, pFanin->Id + 1000000 );
        else
        fprintf( pFile, "    edge [ source %5d   target %5d\n", pNode->Id, pFanin->Id );
        fprintf( pFile, "        graphics [ type \"line\" arrow \"first\" ]\n" );
        fprintf( pFile, "    ]\n" );
    }
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        pNode = pLatch->pOutput;
        pFanin = Ntk_NodeReadFaninNode(pLatch->pInput,0);
        fprintf( pFile, "    edge [ source %5d   target %5d\n", pNode->Id + 1000000, pFanin->Id );
        fprintf( pFile, "        graphics [ type \"line\" arrow \"first\" ]\n" );
        fprintf( pFile, "    ]\n" );
    }

	fprintf( pFile, "]\n" );
	fprintf( pFile, "\n" );
	fclose( pFile );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


