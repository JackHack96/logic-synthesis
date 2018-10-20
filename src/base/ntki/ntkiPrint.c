/**CFile****************************************************************

  FileName    [ntkiPrint.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [All the printing procedures for stats/ios/values, etc.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiPrint.c,v 1.7 2004/05/12 04:30:10 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static char * Ntk_NodePrintLiteral( Mvc_Cube_t * pCube, int iLit, Ntk_Node_t ** ppFanins, int * pValuesFirst );
static void Ntk_NodePrintUpdatePos( FILE * pFile, int * pPos, char * pName );
static char * Ntk_NodeGetNamePrintableInt( pNode );
static int Ntk_NodePrintReconv( FILE * pFile, Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, 
    int fFanout, Ntk_Node_t ** ppResult );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Print stats command.]

  Description [Reports the vital statistics about the network:
  (1) the number of PI/PO nodes; (2) the number of internal nodes;
  (3) the number of inputs in all nodes; (4) the number of BDD nodes
  in all local functions.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintStats( FILE * pFile, Ntk_Network_t * pNet, bool fComputeCvr, bool fComputeMvr, bool fShort )
{
    Ntk_Node_t * pNode;
    Cvr_Cover_t * pCvr;
    Mvr_Relation_t * pMvr;
    int nTotalFanins;
    int nTotalValues;
    int nTotalCubes;
    int nTotalLitsSop;
    int nTotalLitsFt;
    int nTotalBddNodes;
    int nNodesWithCvr;
    int nNodesWithMvr;
    int nNodesInternal;
    double RatioWithCvr;
    double RatioWithMvr;

    // statistics
    nTotalFanins   = 0;
    nTotalValues   = 0;
    nTotalCubes    = 0;
    nTotalLitsSop  = 0;
    nTotalLitsFt   = 0;
    nTotalBddNodes = 0;

    // nodes
    nNodesWithCvr  = 0;
    nNodesWithMvr  = 0;
    nNodesInternal = Ntk_NetworkReadNodeIntNum(pNet);

    // go through all the nodes
    Ntk_NetworkForEachNode( pNet, pNode ) 
    { 
        // count the number of fanins and values
        nTotalFanins += Ntk_NodeReadFaninNum(pNode);
        nTotalValues += pNode->nValues;

        // count the Cvr based parameters
        if ( fComputeCvr )
            pCvr = Ntk_NodeGetFuncCvr( pNode );
        else
            pCvr = Ntk_NodeReadFuncCvr( pNode );
        if ( pCvr )
        {
            nNodesWithCvr++;
            nTotalLitsSop += Cvr_CoverReadLitSopNum( pCvr );
            nTotalLitsFt += Cvr_CoverReadLitFacNum( pCvr ); // use CST cover
            nTotalCubes += Cvr_CoverReadCubeNum( pCvr );
        }

        // count the Mvr based parameters
        if ( fComputeMvr )
            pMvr = Ntk_NodeGetFuncMvr( pNode );
        else
            pMvr = Ntk_NodeReadFuncMvr( pNode );
        if ( pMvr )
        {
            nNodesWithMvr++;
            nTotalBddNodes += Mvr_RelationGetNodes(pMvr);
        }
    }

    RatioWithCvr = 100.0 * nNodesWithCvr / nNodesInternal;
    RatioWithMvr = 100.0 * nNodesWithMvr / nNodesInternal;

    // report the statistics
    if ( fShort )
    { // use the short form
        fprintf( pFile, "%s:",       pNet->pName );
        fprintf( pFile, "  pi/po = %d/%d", Ntk_NetworkReadCiNum(pNet) - Ntk_NetworkReadLatchNum(pNet), Ntk_NetworkReadCoNum(pNet) - Ntk_NetworkReadLatchNum(pNet) );
        fprintf( pFile, "  lat = %d", Ntk_NetworkReadLatchNum(pNet) );
        fprintf( pFile, "  nd = %d", Ntk_NetworkReadNodeIntNum(pNet) );
        fprintf( pFile, "  cube = %d",  nTotalCubes );
        fprintf( pFile, "  ff-lit = %d",  nTotalLitsFt );
        fprintf( pFile, "  lev = %d", Ntk_NetworkGetNumLevels(pNet) );
        fprintf( pFile, "\n" );
    }
    else
    {
        fprintf( pFile, "%-4s:",       pNet->pName );
        fprintf( pFile, "  pi = %d",   Ntk_NetworkReadCiNum(pNet) - Ntk_NetworkReadLatchNum(pNet) );
        fprintf( pFile, "  po = %d",   Ntk_NetworkReadCoNum(pNet) - Ntk_NetworkReadLatchNum(pNet) );
        fprintf( pFile, "  latch = %d", Ntk_NetworkReadLatchNum(pNet) );
        fprintf( pFile, "  node = %d", Ntk_NetworkReadNodeIntNum(pNet) );
        fprintf( pFile, "  level = %d", Ntk_NetworkGetNumLevels(pNet) );
        fprintf( pFile, "\n" );
        fprintf( pFile, "SOP (%5.1f%%):",  RatioWithCvr );
        fprintf( pFile, "  cubes = %d",  nTotalCubes );
        fprintf( pFile, "  lits = %d",  nTotalLitsSop );
        fprintf( pFile, "  lits(ff) = %d",  nTotalLitsFt );
        fprintf( pFile, "\n" );
        fprintf( pFile, "MDD (%5.1f%%):",  RatioWithMvr );
        fprintf( pFile, "  nodes(bdd) = %d", nTotalBddNodes );
        fprintf( pFile, "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Print stats command for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintStats(FILE *fp, Ntk_Node_t *node, bool cvr, bool mvr)
{
    Cvr_Cover_t * pCvr;
    Mvr_Relation_t * pMvr;

    fprintf(fp, "%s\t", Ntk_NodeGetNamePrintableInt(node));
    switch ( Ntk_NodeReadType(node) ) 
    {
	    case MV_NODE_CI:
	        fprintf(fp, "(combinational input)\n");
	        break;
	    case MV_NODE_CO:
	        fprintf(fp, "(combinational output)\n");
	        break;
	    case MV_NODE_INT:
	        fprintf(fp, "%3d inputs", Ntk_NodeReadFaninNum(node));
	        fprintf(fp, "%3d vals", node->nValues);

	        if (cvr) 
                pCvr = Ntk_NodeGetFuncCvr(node);
	        else     
                pCvr = Ntk_NodeReadFuncCvr(node);
	        if (pCvr) 
            {
		        fprintf(fp, "%5d cubes",    Cvr_CoverReadCubeNum(pCvr));
		        fprintf(fp, "%6d lits",     Cvr_CoverReadLitSopNum(pCvr));
		        fprintf(fp, "%6d ff-lits", Cvr_CoverReadLitFacNum(pCvr));
	        }
		    
	        if (mvr) 
                pMvr = Ntk_NodeGetFuncMvr(node);
	        else     
                pMvr = Ntk_NodeReadFuncMvr(node);
	        if (pMvr) 
    		    fprintf(fp, "%5d BDD nodes", Mvr_RelationGetNodes(pMvr));
	        fprintf(fp, "\n");

	        break;
	    case MV_NODE_NONE:
	        abort();
    }
}

/**Function*************************************************************

  Synopsis    [print_io command for the network and the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintIo( FILE * pFile, Ntk_Network_t * pNet, int nNodes )
{
    Ntk_Node_t * pNode, * pFanin, * pFanout;
    Ntk_Pin_t * pPin;
    int i;

    if ( nNodes == 0 )
    {
        // this code will work only if the new CIs/ COs are not added
        // during the network processing

        fprintf( pFile, "Primary inputs: " );    
        for ( i = 1; i < pNet->nIds; i++ )
        {
            pNode = pNet->pId2Node[i];
            if ( pNode == NULL )
                continue;
            if ( pNode->Type == MV_NODE_CI && pNode->Subtype == MV_BELONGS_TO_NET )
                fprintf( pFile, " %s", Ntk_NodeGetNamePrintableInt(pNode) );
        }
        fprintf( pFile, "\n" );   
        
        fprintf( pFile, "Latch outputs:  " );    
        for ( i = 1; i < pNet->nIds; i++ )
        {
            pNode = pNet->pId2Node[i];
            if ( pNode == NULL )
                continue;
            if ( pNode->Type == MV_NODE_CI && pNode->Subtype != MV_BELONGS_TO_NET )
                fprintf( pFile, " %s", Ntk_NodeGetNamePrintableInt(pNode) );
        }
        fprintf( pFile, "\n" );   
                
        fprintf( pFile, "Latch inputs:   " );    
        for ( i = 1; i < pNet->nIds; i++ )
        {
            pNode = pNet->pId2Node[i];
            if ( pNode == NULL )
                continue;
            if ( pNode->Type == MV_NODE_CO && pNode->Subtype != MV_BELONGS_TO_NET )
                fprintf( pFile, " %s", Ntk_NodeGetNamePrintableInt(pNode) );
        }
        fprintf( pFile, "\n" );    
        
        fprintf( pFile, "Primary outputs:" );    
        for ( i = 1; i < pNet->nIds; i++ )
        {
            pNode = pNet->pId2Node[i];
            if ( pNode == NULL )
                continue;
            if ( pNode->Type == MV_NODE_CO && pNode->Subtype == MV_BELONGS_TO_NET )
                fprintf( pFile, " %s", Ntk_NodeGetNamePrintableInt(pNode) );
        }
        fprintf( pFile, "\n" );    
    }
    else
    {
        Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        {
            fprintf( pFile, "%s:\n", Ntk_NodeGetNamePrintableInt(pNode) ); 
            fprintf( pFile, "    Fanins: ", Ntk_NodeGetNamePrintableInt(pNode) ); 
            Ntk_NodeForEachFanin( pNode, pPin, pFanin )
                fprintf( pFile, " %s", Ntk_NodeGetNamePrintableInt(pFanin) ); 
            fprintf( pFile, "\n" ); 
            fprintf( pFile, "    Fanouts:" ); 
            Ntk_NodeForEachFanout( pNode, pPin, pFanout )
                if ( !Ntk_NodeIsCo(pFanout) )
                    fprintf( pFile, " %s", Ntk_NodeGetNamePrintableInt(pFanout) ); 
            fprintf( pFile, "\n" ); 
        }
    }
}

/**Function*************************************************************

  Synopsis    [print_io command for the network and the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintLatch( FILE * pFile, Ntk_Network_t * pNet, int nNodes )
{
    Ntk_Latch_t * pLatch;
    int i;
    i = 0;
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        printf( "%3d : (%20s, %20s)   %2d values", 
            i++, pLatch->pInput->pName, pLatch->pOutput->pName, 
            pLatch->pInput->nValues );
        if ( pLatch->pNode == NULL )
            printf( "   reset = %2d\n", pLatch->Reset );
        else
            printf( "   reset = node\n" );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintValue( FILE * pFile, Ntk_Network_t * pNet, int nNodes )
{
    Ntk_Node_t * pNode;
    int LenghtMax, LenghtCur;
    int nColumns, iNode;

    if ( nNodes == 1 )
    {
        Ntk_NodePrintValue( pFile, pNet->pOrder );
        return;
    }
    
    // get the longest name
    LenghtMax = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( !Ntk_NodeIsCo(pNode) && !Ntk_NodeIsCoFanin(pNode) )
        {
            LenghtCur = strlen( Ntk_NodeGetNamePrintableInt(pNode) );
            if ( LenghtMax < LenghtCur )
                LenghtMax = LenghtCur;
        }

    // decide how many columns to print
    nColumns = 80 / (LenghtMax + 9);

    // print the node names
    iNode = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
        if ( !Ntk_NodeIsCo(pNode) && !Ntk_NodeIsCoFanin(pNode) )
        {
            fprintf( pFile, "%*s:%5d", LenghtMax,
                Ntk_NodeGetNamePrintableInt(pNode),
                Ntk_NetworkGetNodeValueSop(pNode) );
            if ( ++iNode % nColumns == 0 )
                fprintf( pFile, "\n" ); 
            else
                fprintf( pFile, "   " ); 
        }
    if ( iNode % nColumns )
        fprintf( pFile, "\n" ); 
}

/**Function*************************************************************

  Synopsis    [Prints the factored form of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintValue( FILE * pFile, Ntk_Node_t * pNode )
{
    char * pName;
    if ( pNode == NULL )
        return;
    // check the situation when the node's value is trivial
    pName = Ntk_NodeGetNamePrintableInt(pNode);
    if ( Ntk_NodeIsCi(pNode) )
    {
        fprintf( pFile, "%10s:   combinational input\n", pName );
        return;
    }
    if ( Ntk_NodeIsCo(pNode) || Ntk_NodeIsCoFanin(pNode) )
    {
        fprintf( pFile, "%10s:   combinational output\n", pName );
        return;
    }
    // print
    fprintf( pFile, "%10s: %3d\n", pName, Ntk_NetworkGetNodeValueSop(pNode) );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintRange( FILE * pFile, Ntk_Network_t * pNet, int nNodes )
{
    Ntk_Node_t * pNode;
    int LenghtMax, LenghtCur;
    int nColumns, iNode;

    if ( nNodes == 1 )
    {
        Ntk_NodePrintRange( pFile, pNet->pOrder );
        return;
    }
    
    // get the longest name
    LenghtMax = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        LenghtCur = strlen( Ntk_NodeGetNamePrintableInt(pNode) );
        if ( LenghtMax < LenghtCur )
            LenghtMax = LenghtCur;
    }

    // decide how many columns to print
    nColumns = 80 / (LenghtMax + 7);

    // print the node names
    iNode = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode )
    {
        fprintf( pFile, "%*s:%3d", LenghtMax,
            Ntk_NodeGetNamePrintableInt(pNode),
            pNode->nValues );
        if ( ++iNode % nColumns == 0 )
            fprintf( pFile, "\n" ); 
        else
            fprintf( pFile, "   " ); 
    }
    if ( iNode % nColumns )
        fprintf( pFile, "\n" ); 
}

/**Function*************************************************************

  Synopsis    [Prints the factored form of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintRange( FILE * pFile, Ntk_Node_t * pNode )
{
    char * pName;
    if ( pNode == NULL )
        return;
    // check the situation when the node's value is trivial
    pName = Ntk_NodeGetNamePrintableInt(pNode);
    // print
    fprintf( pFile, "%10s: %3d", pName, pNode->nValues );
    if ( Ntk_NodeIsCi(pNode) )
        fprintf( pFile, "  <combinational input>\n", pName );
    if ( Ntk_NodeIsCo(pNode) || Ntk_NodeIsCoFanin(pNode) )
        fprintf( pFile, "  <combinational output>\n", pName );
    fprintf( pFile, "\n" );

}

/**Function*************************************************************

  Synopsis    [Prints the factored form of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintFactor( FILE * pFile, Ntk_Node_t * pNode )
{
    Ntk_Node_t * pFanin;
    Ft_Tree_t * pTree;
    Vm_VarMap_t * pVm;
    char ** pNamesIn;
    int nVarsIn, i;

    // the node may be CO, if its fanin is a CI or an internal node without CO name
    if ( pNode->Type == MV_NODE_CO )
    {
        for ( i = 1; i < pNode->nValues; i++ )
        {
            Ntk_NodePrintOutputName( pFile, Ntk_NodeGetNamePrintableInt(pNode), pNode->nValues, i );
            fprintf( pFile, "%s", Ntk_NodeGetNamePrintableInt( Ntk_NodeReadFaninNode(pNode,0) ) );
            if ( pNode->nValues > 2 )
                fprintf( pFile, "{%d}", i );
            fprintf( pFile, "\n" );
        }
        return;
    }

    pVm = Ntk_NodeReadFuncVm( pNode );
    nVarsIn = Vm_VarMapReadVarsInNum( pVm );

    // get the names of the input variables
    pNamesIn = ALLOC( char *, nVarsIn );
    for ( i = 0; i < nVarsIn; i++ )
    {
        pFanin = Ntk_NodeReadFaninNode( pNode, i );
        pNamesIn[i] = util_strsav( Ntk_NodeGetNamePrintableInt(pFanin) );
    }

    pTree = Cvr_CoverFactor( Ntk_NodeGetFuncCvr( pNode ) );
    Ft_TreePrint( pFile, pTree, pNamesIn, Ntk_NodeGetNamePrintableInt(pNode) );

    for ( i = 0; i < nVarsIn; i++ )
        FREE( pNamesIn[i] );
    FREE( pNamesIn );
}


/**Function*************************************************************

  Synopsis    [Prints the nodes by levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintLevel( FILE * pFile, Ntk_Network_t * pNet, int nNodes, int fFromOutputs )
{
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    int nLevels, Pos, i;
    int LevelStart, LevelStop;
    char * pName;
    
    if ( nNodes == 0 )
        nLevels = Ntk_NetworkLevelize( pNet, fFromOutputs );
    else
    {
        // collect the nodes into the array
        i = 0;
        ppNodes = ALLOC( Ntk_Node_t *, nNodes );
        Ntk_NetworkForEachNodeSpecial( pNet, pNode )
            ppNodes[i++] = pNode;
        // levelize starting from the nodes
        nLevels = Ntk_NetworkLevelizeNodes( ppNodes, nNodes, fFromOutputs );
        FREE( ppNodes );
    }

    // print nodes by level
    LevelStart = fFromOutputs? 0: 1;
    LevelStop  = fFromOutputs? nLevels + 1: nLevels + 2;
    for ( i = LevelStart; i < LevelStop; i++ )
    {
        if ( Ntk_NetworkReadOrderByLevel( pNet, i ) == NULL )
            continue;
        fprintf( pFile, "    %2d: ", i + fFromOutputs - 1 );
        Pos = 8;
        Ntk_NetworkForEachNodeSpecialByLevel( pNet, i, pNode )
        {
            if ( Ntk_NodeIsCo(pNode) )
                continue;
            pName = Ntk_NodeGetNamePrintableInt( pNode );
            if ( Pos + strlen(pName) > 78 )
            {
                fprintf( pFile, "\n" );
                fprintf( pFile, "        " );
                Pos = 8;
            }
            fprintf( pFile, "%s ", pName );
            Pos += strlen(pName) + 1;
        }
        fprintf( pFile, "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the reconvergence of the nodes in the network.]

  Description [For each node prints the nodes which have paths reconverging
  at the given node. Consider node N. Consider pairs of fanins of N. Let
  one pair be (F1, F2). Find the intersection of the TFI cones of (F1, F2).
  Find the nodes that do not fanouts into the nodes in the intersection.
  Print these nodes as containing reconvergence for the given node.
  Do this for all possible fanin pairs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintReconv( FILE * pFile, Ntk_Network_t * pNet, int nNodes, bool fFanout, bool fVerbose )
{
    Ntk_Node_t * pNode;
    Ntk_Node_t ** ppFans;
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t ** ppReconvs;
    int nReconvs, nTotal;
    int nFans, n, i, k;

    fprintf( pFile, "Reconvergent nodes on the %s side:\n", fFanout? "fanout": "fanin" );

    // collect the nodes into the given storage
    ppNodes = ALLOC( Ntk_Node_t *, nNodes );
    Ntk_NetworkCreateArrayFromSpecial( pNet, ppNodes );

    // allocate room for the reconvergent nodes
    ppReconvs = ALLOC( Ntk_Node_t *, 100000 );
    ppFans = ALLOC( Ntk_Node_t *, 1000 );

    // go through the internal nodes
    nTotal = 0;
    for ( n = 0; n < nNodes; n++ )
    {
        pNode = ppNodes[n];
        if ( pNode->Type != MV_NODE_INT )
            continue;

        if ( fFanout )
            nFans = Ntk_NodeReadFanoutNum(pNode);
        else
            nFans = Ntk_NodeReadFaninNum(pNode);

        // get the fanins
        if ( fFanout )
            Ntk_NodeReadFanouts( pNode, ppFans );
        else
            Ntk_NodeReadFanins( pNode, ppFans );

        // start collecting for this node
        nReconvs = 0;
        // go through the fanins pairs
        for ( i = 0; i < nFans; i++ )
            for ( k = i + 1; k < nFans; k++ )
                nReconvs += Ntk_NodePrintReconv( pFile, 
                    ppFans[i], ppFans[k], fFanout, ppReconvs + nReconvs );

        if ( nReconvs > 0 && fVerbose )
        {
            fprintf( pFile, "%10s ", Ntk_NodeGetNamePrintable(pNode) );
            fprintf( pFile, "(%d) : ", nFans );
            for ( i = 0; i < nReconvs; i++ )
                fprintf( pFile, " %s", Ntk_NodeGetNamePrintable(ppReconvs[i]) );
            fprintf( pFile, "\n" );
        }
        nTotal += nReconvs;
    }
    fprintf( pFile, "Total reconvergences = %d. ", nTotal );
    fprintf( pFile, "Reconvergences per node = %.3f.", 
        ((float)nTotal)/Ntk_NetworkReadNodeIntNum(pNet) );
    fprintf( pFile, "\n" );

    FREE( ppNodes );
    FREE( ppReconvs );
    FREE( ppFans );
}

/**Function*************************************************************

  Synopsis    [Prints the reconvergent nodes of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodePrintReconv( FILE * pFile, Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, 
    int fFanout, Ntk_Node_t ** ppResult )
{
    st_table * tNodes1, * tNodes12;
    Ntk_Node_t * pNode, * pFan;
    Ntk_Pin_t * pPin;
    st_generator * gen;
    int fFound;
    int nResult;

    // compute the TFI of the first node (skip CIs)
    if ( fFanout )
        Ntk_NetworkComputeNodeTfo( pNode1->pNet, &pNode1, 1, 10000, 0, 1 );
    else
        Ntk_NetworkComputeNodeTfi( pNode1->pNet, &pNode1, 1, 10000, 0, 1 );
    // put the nodes in the table
    tNodes1 = Ntk_NetworkCreateTableFromSpecial( pNode1->pNet );
    // compute the TFI of the second node (skip CIs)
    if ( fFanout )
        Ntk_NetworkComputeNodeTfo( pNode2->pNet, &pNode2, 1, 10000, 0, 1 );
    else
        Ntk_NetworkComputeNodeTfi( pNode2->pNet, &pNode2, 1, 10000, 0, 1 );

    // compute the intersection of the two sets of nodes
    tNodes12 = st_init_table(st_ptrcmp, st_ptrhash);
    Ntk_NetworkForEachNodeSpecial( pNode1->pNet, pNode )
        if ( st_is_member( tNodes1, (char *)pNode ) )
            st_insert( tNodes12, (char *)pNode, (char *)NULL );

    // print out the nodes that do not have fanouts in the intersection
    nResult = 0;
    st_foreach_item( tNodes12, gen, (char **)&pNode, NULL )
    {
        fFound = 0;
        if ( fFanout )
        {
            Ntk_NodeForEachFanin( pNode, pPin, pFan )
                if ( st_is_member( tNodes12, (char *)pFan ) )
                {
                    fFound = 1;
                    break;
                }
        }
        else
        {
            Ntk_NodeForEachFanout( pNode, pPin, pFan )
                if ( st_is_member( tNodes12, (char *)pFan ) )
                {
                    fFound = 1;
                    break;
                }
        }
        if ( fFound )
            continue;
        // print this node
//        fprintf( pFile, "%s ", Ntk_NodeGetNamePrintable(pNode) );
        ppResult[ nResult++ ] = pNode;
    }
    st_free_table( tNodes1 );
    st_free_table( tNodes12 );
    return nResult;
}

/**Function*************************************************************

  Synopsis    [Prints the list of ND nodes of the current network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkPrintNd( FILE * pFile, Ntk_Network_t * pNet, int fVerbose )
{
    Mvc_Data_t * pData;
    Cvr_Cover_t * pCvr;
    Ntk_Node_t * pNode;
    int nNodes, Default;
    Mvc_Cover_t ** ppIsets, * pMvcTemp;

    // count the ND nodes
    nNodes = 0;
    Ntk_NetworkStartSpecial( pNet );
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        pCvr = Ntk_NodeReadFuncCvr( pNode );
        if ( pCvr )
        {
            // get any cover of pCvrFshift
            Default  = Cvr_CoverReadDefault(pCvr);
            ppIsets  = Cvr_CoverReadIsets(pCvr);
            pMvcTemp = (Default != 0)? ppIsets[0]: ppIsets[1];

            // get the MV data for the new man
            pData = Mvc_CoverDataAlloc( Ntk_NodeReadFuncVm(pNode), pMvcTemp );
            if ( Cvr_CoverIsND( pData, pCvr ) )
            {
                Ntk_NetworkAddToSpecial( pNode );
                nNodes++;
            }
            Mvc_CoverDataFree( pData, pMvcTemp );
        }
        else
        {
            if ( Mvr_RelationIsND( Ntk_NodeGetFuncMvr(pNode) ) )
            {
                Ntk_NetworkAddToSpecial( pNode );
                nNodes++;
            }
        }
    }
    Ntk_NetworkStopSpecial( pNet );
 
    if ( nNodes == 0 )
        fprintf( pFile, "There are no ND nodes in the current network.\n" );
    else
    {
        fprintf( pFile, "There are %d ND nodes in the current network.\n", nNodes );
        if ( fVerbose )
        {
            Ntk_NetworkForEachNodeSpecial( pNet, pNode ) 
                fprintf( pFile, "  %s", Ntk_NodeGetNamePrintable(pNode) );
            fprintf( pFile, "\n" );
        }
    }
    Ntk_NetworkResetSpecial( pNet );
}

/**Function*************************************************************

  Synopsis    [Prints the node MV relation as K-map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintMvr( FILE * pFile, Ntk_Node_t * pNode )
{
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pFanin;
    int nFanins;
    char ** pInputNames;
    int i;

    if ( pNode->Type == MV_NODE_CI )
        return;
    if ( pNode->Type == MV_NODE_CO )
    {
        pNode = Ntk_NodeReadFaninNode( pNode, 0 );
        // currently, cannot visualize the MV buffer
        // when the CI feed directly into a PO
        if ( pNode->Type == MV_NODE_CI )
            return;
    }

    // get the input variable names
    nFanins = Ntk_NodeReadFaninNum(pNode);
    pInputNames = ALLOC( char *, nFanins + 1 );
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
        pInputNames[i] = util_strsav( Ntk_NodeGetNamePrintableInt( pFanin ) );
    pInputNames[nFanins] = util_strsav( Ntk_NodeGetNamePrintableInt( pNode ) );

    // print the K-map
    Mvr_RelationPrintKmap( pFile, Ntk_NodeGetFuncMvr(pNode), pInputNames );

    for ( i = 0; i <= nFanins; i++ )
        FREE( pInputNames[i] );
    FREE( pInputNames );
}


/**Function*************************************************************

  Synopsis    [Prints the node MV relation as K-map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintFlex( FILE * pFile, Ntk_Node_t * pNode, Mvr_Relation_t * pFlex )
{
    Ntk_Pin_t * pPin;
    Ntk_Node_t * pFanin;
    int nFanins;
    char ** pInputNames;
    int i;

    if ( pNode->Type != MV_NODE_INT )
        return;

    // get the input variable names
    nFanins = Ntk_NodeReadFaninNum(pNode);
    pInputNames = ALLOC( char *, nFanins + 1 );
    Ntk_NodeForEachFaninWithIndex( pNode, pPin, pFanin, i )
        pInputNames[i] = util_strsav( Ntk_NodeGetNamePrintableInt( pFanin ) );
    pInputNames[nFanins] = util_strsav( Ntk_NodeGetNamePrintableInt( pNode ) );

    // print the K-map
    Mvr_RelationPrintKmap( pFile, pFlex, pInputNames );

    for ( i = 0; i <= nFanins; i++ )
        FREE( pInputNames[i] );
    FREE( pInputNames );
}


/**Function*************************************************************

  Synopsis    [Prints the flexibility of the node as K-map.]

  Description [The flexibility is expressed using the same MV relation
  parameters as the node's MV relation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintMvrFlex( FILE * pFile, Ntk_Node_t * pNode, DdNode * bFlex )
{
    Mvr_Relation_t * pMvr;
    DdNode * bRel;

    // get the relation
    pMvr = Ntk_NodeReadFuncMvr( pNode );
    // set the flexibility
    bRel = Mvr_RelationReadRel( pMvr );
    Mvr_RelationWriteRel( pMvr, bFlex );
    Ntk_NodePrintMvr( pFile, pNode );
    Mvr_RelationWriteRel( pMvr, bRel );
}

/**Function*************************************************************

  Synopsis    [Prints the node cover in positional notation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintCvr( FILE * pFile, Ntk_Node_t * pNode, bool fPrintDefault, bool fPrintPositional )
{
    Ntk_Node_t ** pFanins;
    Cvr_Cover_t * pCvr;
    Vm_VarMap_t * pVm;
    char * pNameOut;
    Mvc_Cover_t ** pCovers;
    int nInputs, Pos, i;

    // get the output name
    pNameOut = Ntk_NodeGetNamePrintableInt( pNode );

    // the node may be CO, if its fanin is a CI or an internal node without CO name
    if ( pNode->Type == MV_NODE_CO )
    {
        if ( fPrintPositional )
            return;
        for ( i = 1; i < pNode->nValues; i++ )
        {
            Ntk_NodePrintOutputName( pFile, pNameOut, pNode->nValues, i );
            fprintf( pFile, "%s", Ntk_NodeGetNamePrintableInt( Ntk_NodeReadFaninNode(pNode,0) ) );
            if ( pNode->nValues > 2 )
                fprintf( pFile, "{%d}", i );
            fprintf( pFile, "\n" );
        }
        return;
    }

    // get the cover
    pVm  = Ntk_NodeReadFuncVm(pNode);
    pCvr = Ntk_NodeGetFuncCvr(pNode);
    pCovers = Cvr_CoverReadIsets( pCvr );
    nInputs = Ntk_NodeReadFaninNum(pNode);
    // save the output name
    pNameOut = util_strsav( pNameOut );
    // get the fanins
    pFanins = pNode->pNet->pArray1;
    Ntk_NodeReadFanins( pNode, pFanins );

    // iterate through the i-sets
    for ( i = 0; i < pNode->nValues; i++ )
        if ( pCovers[i] )
        {
            Pos = Ntk_NodePrintOutputName( pFile, pNameOut, pNode->nValues, i );
            if ( fPrintPositional )
                Ntk_NodePrintCvrPositional( pFile, pVm, pCovers[i], nInputs );
            else
                Ntk_NodePrintCvrLiterals( pFile, pVm, pFanins, pCovers[i], nInputs, &Pos );

            fprintf( pFile, "\n" );
        }
        else if ( fPrintDefault )
        {
            Pos = Ntk_NodePrintOutputName( pFile, pNameOut, pNode->nValues, i );
            fprintf( pFile, "<default> " );
            Pos += 10;
            
            pCovers[i] = Ntk_NodeComputeDefault( pNode );

            if ( fPrintPositional )
                Ntk_NodePrintCvrPositional( pFile, pVm, pCovers[i], nInputs );
            else
                Ntk_NodePrintCvrLiterals( pFile, pVm, pFanins, pCovers[i], nInputs, &Pos );
            fprintf( pFile, "\n" );
            
            Mvc_CoverFree( pCovers[i] );
            pCovers[i] = NULL;
        }
    FREE( pNameOut );
}

/**Function*************************************************************

  Synopsis    [Prints the node cover in positional notation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintCvrPositional( FILE * pFile, Vm_VarMap_t * pVm, Mvc_Cover_t * Cover, int nVars )
{
    Mvc_Cube_t * pCube;
    int * pValuesFirst;
    int FirstCube;
    int i, v;

    if ( Mvc_CoverIsEmpty(Cover) )
    {
        for ( i = 0; i < 15; i++ )
            fprintf( pFile, " " );
        fprintf( pFile, "Constant 0" );
        return;
    }
    if ( Mvc_CoverIsTautology(Cover) )
    {
        for ( i = 0; i < 15; i++ )
            fprintf( pFile, " " );
        fprintf( pFile, "Constant 1" );
        return;
    }

    pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);

    FirstCube = 1;
    Mvc_CoverForEachCube( Cover, pCube )
    {
        fprintf( pFile, "\n" );
        for ( i = 0; i < 15; i++ )
            fprintf( pFile, " " );
        for ( i = 0; i < nVars; i++ )
        {
            // print this literal
            fprintf( pFile, " " );
            for ( v = pValuesFirst[i]; v < pValuesFirst[i+1]; v++ )
                if ( Mvc_CubeBitValue( pCube,  v ) )
                    fprintf( pFile, "1" );
                else
                    fprintf( pFile, "0" );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Prints the node cover in positional notation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintCvrLiterals( FILE * pFile, Vm_VarMap_t * pVm, 
    Ntk_Node_t * pFanins[], Mvc_Cover_t * Cover, int nVars, int * pPos )
{
    Mvc_Cube_t * pCube;
    bool fFirstCube, fFirstLit;
    int * pValuesFirst;
    char * pName;
    int i, v;

    if ( Mvc_CoverIsEmpty(Cover) )
    {
        fprintf( pFile, " Constant 0" );
        return;
    }
    if ( Mvc_CoverIsTautology(Cover) )
    {
        fprintf( pFile, " Constant 1" );
        return;
    }

    pValuesFirst = Vm_VarMapReadValuesFirstArray(pVm);

    fFirstCube = 1;
    Mvc_CoverForEachCube( Cover, pCube )
    {
        if ( fFirstCube )
            fFirstCube = 0;
        else
        {
            fprintf( pFile, " + " );
            (*pPos) += 3;
        }

        fFirstLit = 1;
        for ( i = 0; i < nVars; i++ )
        {
            // check if this literal is full
            for ( v = pValuesFirst[i]; v < pValuesFirst[i+1]; v++ )
                if ( !Mvc_CubeBitValue( pCube,  v ) )
                    break;
            if ( v == pValuesFirst[i+1] )
                continue;

            // print space between literals
            if ( fFirstLit )
                fFirstLit = 0;
            else
            {
                fprintf( pFile, " " );
                (*pPos) += 1;
            }

            // print this literal
            pName = Ntk_NodePrintLiteral( pCube, i, pFanins, pValuesFirst );
            Ntk_NodePrintUpdatePos( pFile, pPos, pName );
            // print
            fprintf( pFile, "%s", pName );
            (*pPos) += strlen(pName);
        }
    }
}

/**Function*************************************************************

  Synopsis    [Prints one literal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ntk_NodePrintLiteral( Mvc_Cube_t * pCube, int iLit, Ntk_Node_t ** ppFanins, int * pValuesFirst )
{
    static char Buffer[500];
    char Value[10];
    bool fFirstValue;
    int Val0, Val1;
    int v;

    fFirstValue = 1;
    if ( pValuesFirst[iLit+1] - pValuesFirst[iLit] == 2 )
    {
        Val0 = Mvc_CubeBitValue( pCube,  pValuesFirst[iLit]   );
        Val1 = Mvc_CubeBitValue( pCube,  pValuesFirst[iLit]+1 );
        assert( (Val0 ^ Val1) == 1 );
        sprintf( Buffer, "%s", Ntk_NodeGetNamePrintableInt(ppFanins[iLit]) );
        if ( Val0 )
            strcat( Buffer, "\'" );
    }
    else
    {
        sprintf( Buffer, "%s{", Ntk_NodeGetNamePrintableInt(ppFanins[iLit]) );
        for ( v = pValuesFirst[iLit]; v < pValuesFirst[iLit+1]; v++ )
            if ( Mvc_CubeBitValue( pCube,  v ) )
            {
                if ( fFirstValue )
                    fFirstValue = 0;
                else
                    strcat( Buffer, "," );
                sprintf( Value, "%d", v - pValuesFirst[iLit] );
                strcat( Buffer, Value );
            }
        strcat( Buffer, "}" );
    }
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodePrintUpdatePos( FILE * pFile, int * pPos, char * pName )
{
    int i;
    if ( *pPos + strlen(pName) < 76 )
        return;
    fprintf( pFile, "\n" );
    for ( i = 0; i < 10; i++ )
        fprintf( pFile, " " );
    *pPos = 10;
}

/**Function*************************************************************

  Synopsis    [Wraps the output names into braces {} like in SIS.]

  Description [The braces do not counts as characters in the name.
  They are ohly used to distinquish the CO node names from all other 
  names in the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ntk_NodeGetNamePrintableInt( Ntk_Node_t * pNode )
{
    static char NameInt[500];
    Ntk_Node_t * pNodeCo;
    if ( Ntk_NodeIsCo(pNode) )
    {
        sprintf( NameInt, "{%s}", Ntk_NodeGetNamePrintable(pNode) );
        return NameInt;
    }
    if ( pNodeCo = Ntk_NodeHasCoName(pNode) )
    {
        sprintf( NameInt, "{%s}", Ntk_NodeGetNamePrintable(pNodeCo) );
        return NameInt;
    }
    return Ntk_NodeGetNamePrintable(pNode);
}

/**Function*************************************************************

  Synopsis    [Starts the printout for a factored form or cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodePrintOutputName( FILE * pFile, char * pNameOut, int nValuesOut, int iSet )
{
    char Buffer[500];
    char Value[10];

    sprintf( Buffer, "%s", pNameOut? pNameOut: "?" );
    if ( nValuesOut > 2 )
    {
        sprintf( Value, "{%d} ", iSet );
        strcat( Buffer, Value );
    }
    else
    {
        sprintf( Value, "%s", (iSet==0)? "\'": " " );
        strcat( Buffer, Value );
    }
    fprintf( pFile, "%8s: ", Buffer );
    return 10;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


