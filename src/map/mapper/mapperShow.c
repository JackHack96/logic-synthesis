/**CFile****************************************************************

  FileName    [mapperShow.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mapperShow.c,v 1.6 2004/10/28 17:36:10 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Map_ManPrintDot( Map_Man_t * pMan, char * pFileName );

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
void Map_MappingShow( Map_Man_t * pMan, char * pFileName )
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
    sprintf( FileNameDot, "%s.dot", Extra_FileNameAppend(FileGeneric, "_map") ); 
    sprintf( FileNamePs,  "%s.ps",  Extra_FileNameAppend(FileGeneric, "_map") ); 
    FREE( FileGeneric );

    Map_ManPrintDot( pMan, FileNameDot );

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
void Map_ManPrintDot( Map_Man_t * pMan, char * pFileName )
{
    FILE * pFile;
    Map_Node_t * pNode;
    Map_NodeVec_t * vNodes;
    int nNodes, nLevels, Level;
    int i;

    // collect the nodes in the DFS order
    vNodes = Map_MappingDfs( pMan, 1 );
    nNodes = vNodes->nSize;
    if ( nNodes > 200 )
    {
        printf( "The network has more than %d nodes. DOT file is not written.\n", 100 );
        return;
    }

    // change this because levels are computed by NodeCreate() now!
    assert( 0 );
    // perform the reachability analysis and set the level of the states
    nLevels = Map_MappingCountLevels( pMan );
    
    for ( i = 0; i < nNodes; i++ )
        vNodes->pArray[i]->pData0 = (char *)1;
    for ( i = 0; i < pMan->nInputs; i++ )
        pMan->pInputs[i]->pData0 = (char *)0;
    for ( i = 0; i < pMan->nOutputs; i++ )
        Map_Regular(pMan->pOutputs[i])->pData0 = (char *)2;


    // start the stream
    pFile = fopen( pFileName, "w" );

    // write the DOT header
    fprintf( pFile, "# %s\n",  "And-Inv graph generated by Map_ManPrintDot() procedure in MVSIS 2.0." );
    fprintf( pFile, "\n" );
    fprintf( pFile, "digraph AIG {\n" );
    fprintf( pFile, "size = \"7.5,10\";\n" );
//  fprintf( pFile, "ranksep = 0.5;\n" );
//  fprintf( pFile, "nodesep = 0.5;\n" );
    fprintf( pFile, "center = true;\n" );
//  fprintf( pFile, "edge [fontsize = 10];\n" );
//  fprintf( pFile, "edge [dir = none];\n" );
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
//          fprintf( pFile, "%d", Level+1 );
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
//  fprintf( pFile, "%d products",       Aut_AutoCountProducts(pAut) );
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
//      fprintf( pFile, "  node [shape=record,width=1,height=0.75];\n" );
//      fprintf( pFile, "  node [shape = circle];\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  Level );

        for ( i = 0; i < nNodes; i++ )
        {
            pNode = vNodes->pArray[i];
            if ( pNode->Level == (unsigned)(nLevels-Level) )
            {
                fprintf( pFile, "  Node%d [label = \"%d\"", pNode->Num, pNode->Num );
                if ( pNode->pData0 == 0 )
                    fprintf( pFile, ", shape = triangle" );
                else
                    fprintf( pFile, ", shape = ellipse" );
                if ( pNode->pData0 == (char *)2 )
                    fprintf( pFile, ", color = coral, fillcolor = coral" );
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
//      fprintf( pFile, "  node [shape = circle];\n" );
        fprintf( pFile, "  node [hight = 0.7];\n" );
        fprintf( pFile, "  node [width = 1.0];\n" );
//      fprintf( pFile, "  node [style = bold];\n" );
        // print the nodes
//        s = 0;
//        for ( s = 0; s < pAut->nStates; s++ )
        Map_NetworkForEachNodeSpecial( pMan, pNode )
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
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        pNode = pMan->pOutputs[i];
        fprintf( pFile, "title2 -> Node%d", Map_Regular(pNode)->Num );
        fprintf( pFile, " [style = %s, color = coral]", Map_IsComplement(pNode)? "dashed" : "bold" );
        fprintf( pFile, ";\n" );
    }

    // generate edges
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = vNodes->pArray[i];
        if ( !Map_NodeIsAnd(pNode) )
            continue;
        // generate the edge from this node to the next (or input variable)
        fprintf( pFile, "Node%d",  pNode->Num );
        fprintf( pFile, " -> " );
        fprintf( pFile, "Node%d",  Map_Regular(pNode->p1)->Num );
        fprintf( pFile, " [style = %s]", Map_IsComplement(pNode->p1)? "dashed" : "bold" );
        fprintf( pFile, ";\n" );
        // generate the edge from this node to the next (or input variable)
        fprintf( pFile, "Node%d",  pNode->Num );
        fprintf( pFile, " -> " );
        fprintf( pFile, "Node%d",  Map_Regular(pNode->p2)->Num );
        fprintf( pFile, " [style = %s]", Map_IsComplement(pNode->p2)? "dashed" : "bold" );
        fprintf( pFile, ";\n" );
    }

    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );

    for ( i = 0; i < nNodes; i++ )
        vNodes->pArray[i]->pData0 = 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



