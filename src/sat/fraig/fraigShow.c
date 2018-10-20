/**CFile****************************************************************

  FileName    [fraigShow.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Procedures to visualize the AIGs.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.1. Started - October 1, 2004]

  Revision    [$Id: fraigShow.c,v 1.7 2005/07/08 01:01:32 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Fraig_ManPrintDot( Fraig_Man_t * pMan, Fraig_Node_t ** ppRoots, int nRoots, char * pFileName );

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
void Fraig_MappingShow( Fraig_Man_t * pMan, char * pFileName )
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

    Fraig_ManPrintDot( pMan, NULL, -1, FileNameDot );

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
void Fraig_MappingShowNodes( Fraig_Man_t * pMan, Fraig_Node_t ** ppRoots, int nRoots, char * pFileName )
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

    Fraig_ManPrintDot( pMan, ppRoots, nRoots, FileNameDot );

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
void Fraig_ManPrintDot( Fraig_Man_t * pMan, Fraig_Node_t ** ppRoots, int nRoots, char * pFileName )
{
    FILE * pFile;
    Fraig_Node_t * pNode, * pTemp, * pPrev;
    Fraig_NodeVec_t * vNodes;
    int nNodes, nLevels, Level, i, fArrayAlloc;

    // collect the nodes in the DFS order
//    vNodes = Fraig_Dfs( pMan );
    if ( ppRoots == NULL )
    {
        vNodes = pMan->vNodes;
        fArrayAlloc = 0;
    }
    else
    {
        vNodes = Fraig_DfsNodes( pMan, ppRoots, nRoots, 1 );
        fArrayAlloc = 1;
    }

    nNodes = vNodes->nSize;
    if ( nNodes > 200 )
    {
        printf( "The network has more than %d nodes. DOT file is not written.\n", 100 );
        return;
    }

    // perform the reachability analysis and set the level of the states
    if ( ppRoots == NULL )
    {
//        nLevels = Fraig_CountLevels( pMan );    
        nLevels = Fraig_GetMaxLevel( pMan );
    }
    else
    {
//        nLevels = Fraig_CountLevelsNodes( pMan, ppRoots, nRoots );
        // get the max level of all nodes
        nLevels = 0;
        for ( i = 0; i < nRoots; i++ )
            nLevels = FRAIG_MAX( nLevels, Fraig_Regular(ppRoots[i])->Level );
    }

    for ( i = 0; i < nNodes; i++ )
        vNodes->pArray[i]->pData0 = (Fraig_Node_t *)1;
    for ( i = 0; i < pMan->vInputs->nSize; i++ )
        pMan->vInputs->pArray[i]->pData0 = (Fraig_Node_t *)0;
    for ( i = 0; i < pMan->vOutputs->nSize; i++ )
    {
        if ( pMan->vOutputs->pArray[i] == NULL )
            continue;
        Fraig_Regular(pMan->vOutputs->pArray[i])->pData0 = (Fraig_Node_t *)2;
    }


    // start the stream
    pFile = fopen( pFileName, "w" );

    // write the DOT header
    fprintf( pFile, "# %s\n",  "And-Inv graph generated by Fraig_ManPrintDot() procedure in MVSIS 2.0." );
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
            if ( pNode->Level == (nLevels-Level) )
            {
                fprintf( pFile, "  Node%d [label = \"%d\"", pNode->Num, pNode->Num );
                if ( pNode->pData0 == 0 )
                    fprintf( pFile, ", shape = triangle" );
                else
                    fprintf( pFile, ", shape = ellipse" );
                if ( pNode->pData0 == (Fraig_Node_t *)2 )
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
        Fraig_NetworkForEachNodeSpecial( pMan, pNode )
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
//    for ( i = 0; i < pMan->vOutputs->nSize; i++ )
    for ( i = 0; i < nRoots; i++ )
    {
//        if ( pMan->vOutputs->pArray[i] == NULL )
//            continue;
//        pNode = pMan->vOutputs->pArray[i];
        pNode = ppRoots[i];
        fprintf( pFile, "title2 -> Node%d", Fraig_Regular(pNode)->Num );
        fprintf( pFile, " [style = %s, color = coral]", Fraig_IsComplement(pNode)? "dashed" : "bold" );
        fprintf( pFile, ";\n" );
    }

    // generate edges
    for ( i = 0; i < nNodes; i++ )
    {
        pNode = vNodes->pArray[i];
        if ( !Fraig_NodeIsAnd(pNode) )
            continue;
        // generate the edge from this node to the next (or input variable)
        fprintf( pFile, "Node%d",  pNode->Num );
        fprintf( pFile, " -> " );
        fprintf( pFile, "Node%d",  Fraig_Regular(pNode->p1)->Num );
        fprintf( pFile, " [style = %s]", Fraig_IsComplement(pNode->p1)? "dashed" : "bold" );
        fprintf( pFile, ";\n" );
        // generate the edge from this node to the next (or input variable)
        fprintf( pFile, "Node%d",  pNode->Num );
        fprintf( pFile, " -> " );
        fprintf( pFile, "Node%d",  Fraig_Regular(pNode->p2)->Num );
        fprintf( pFile, " [style = %s]", Fraig_IsComplement(pNode->p2)? "dashed" : "bold" );
        fprintf( pFile, ";\n" );
        // generate the edges between the equivalent nodes
        pPrev = pNode;
        for ( pTemp = pNode->pNextE; pTemp; pTemp = pTemp->pNextE )
        {
            fprintf( pFile, "Node%d",  pPrev->Num );
            fprintf( pFile, " -> " );
            fprintf( pFile, "Node%d",  pTemp->Num );
            fprintf( pFile, ";\n" );
            pPrev = pTemp;
        }
    }

    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );

    for ( i = 0; i < nNodes; i++ )
        vNodes->pArray[i]->pData0 = 0;
    if ( fArrayAlloc )
        Fraig_NodeVecFree( vNodes );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



