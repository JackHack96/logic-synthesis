/**CFile****************************************************************

  FileName    [ntkiSat.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiSat.c,v 1.5 2003/11/18 18:54:56 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ntk_NetworkCreateClauses( Ntk_Network_t * pNet );
static int Ntk_NetworkDeleteClauses( Ntk_Network_t * pNet );
static int Ntk_NodeGenerateClauses( Ntk_Node_t * pNet );
static void Ntk_NodeWriteClauses( FILE * pFile, Ntk_Node_t * pNode, char * sPrefix );
static void Ntk_NodeWriteClausesBuffer( FILE * pFile, Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, 
    char * pPref1, char * pPref2, int Phase );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkWriteClauses( Ntk_Network_t * pNet, char * pFileName )
{
    Ntk_Node_t * pNode, * pFanout;
    Ntk_Pin_t * pPin;
    FILE * pFile;
    int nClauses;

    // for each internal node, get the set of clauses
    // the clauses are pset_families attached to the pNode->pCopy field
    nClauses = Ntk_NetworkCreateClauses( pNet );

    // start the file
    pFile = fopen( pFileName, "w" );

    // comment
    fprintf( pFile, "# MV SAT clauses generated for circuit \"%s\".\n", pNet->pName );
    fprintf( pFile, "# Generation date: %s  %s\n", __DATE__, __TIME__);

    // write inputs
    fprintf( pFile, ".i %d\n", Ntk_NetworkReadNodeWritableNum(pNet) );

    // write the PIs
    fprintf( pFile, ".inputs" );
    Ntk_NetworkForEachCi( pNet, pNode )
        fprintf( pFile, " %s", Ntk_NodeGetNameLong(pNode) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs" );
    Ntk_NetworkForEachCo( pNet, pNode )
        fprintf( pFile, " %s", Ntk_NodeGetNameLong(pNode) );
    fprintf( pFile, "\n" );

    // write mv lines
    Ntk_NetworkForEachCi( pNet, pNode )
        fprintf( pFile, ".mv %s %d\n", Ntk_NodeGetNameLong(pNode), pNode->nValues );
    Ntk_NetworkForEachCo( pNet, pNode )
        fprintf( pFile, ".mv %s %d\n", Ntk_NodeGetNameLong(pNode), pNode->nValues );
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        // if an internal node has a single CO fanout, 
        // the name of this node is the same as CO name
        // and the node's .mv is already written above as the PO's .mv
        if ( Ntk_NodeHasCoName(pNode) )
            continue;
        // otherwise, write the .mv directive if it is non-binary
        fprintf( pFile, ".mv %s %d\n", Ntk_NodeGetNameLong(pNode), pNode->nValues );
    }

    // write the number of clauses
    fprintf( pFile, ".c %d\n", nClauses );

    // write clauses
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        // write the internal node
        Ntk_NodeWriteClauses( pFile, pNode, NULL );
        // if an internal node has only one fanout, which is a PO,
        // the PO name is used for the internal node, and in this way, 
        // the PO is written into the file

        // however, if an internal node has more than one fanout
        // it is written as an intenal node with its own name
        // in this case, if among the fanouts of the node there are POs,
        // they should be written with their names, as MV buffers
        // driven by the given internal node
        if ( Ntk_NodeReadFanoutNum( pNode ) > 1 )
        {
            Ntk_NodeForEachFanout( pNode, pPin, pFanout ) 
                if ( Ntk_NodeIsCo(pFanout) )
                    Ntk_NodeWriteClausesBuffer( pFile, pNode, pFanout, NULL, NULL, 1 ); 
        }
    }

    // finish off
    fprintf( pFile, ".e\n" );
    fclose( pFile );

    // remove the clauses
    Ntk_NetworkDeleteClauses( pNet );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NetworkWriteSat( Ntk_Network_t * pNet1, Ntk_Network_t * pNet2, char * pFileName )
{
    FILE * pFile;
    char * pPref1 = "1_", * pPref2 = "2_";
    Ntk_Node_t * pNode1, * pNode2, * pFanout;
    Ntk_Pin_t * pPin;
    int nValuesCiTotal, nValuesCoTotal;
    int nClauses1, nClauses2;

    // generate clauses for both networks
    nClauses1 = Ntk_NetworkCreateClauses( pNet1 );
    nClauses2 = Ntk_NetworkCreateClauses( pNet2 );

    // check that the number of PI values is the same
    nValuesCiTotal = 0;
    Ntk_NetworkForEachCi( pNet1, pNode1 )
    {
        pNode2 = Ntk_NetworkFindNodeByName( pNet2, pNode1->pName );
        assert( pNode1->nValues == pNode2->nValues );
        nValuesCiTotal += pNode1->nValues;
    }
    nValuesCoTotal = 0;
    Ntk_NetworkForEachCo( pNet1, pNode1 )
    {
        pNode2 = Ntk_NetworkFindNodeByName( pNet2, pNode1->pName );
        assert( pNode1->nValues == pNode2->nValues );
        nValuesCoTotal += pNode1->nValues;
    }

    // start the file
    pFile = fopen( pFileName, "w" );

    // write the header
    // comment
    fprintf( pFile, "# MV SAT benchmark derived by asserting functional equivalence\n" );
    fprintf( pFile, "# of circuits \"%s\" (prefix \"%s\") and \"%s\" (prefix \"%s\").\n", 
        pNet1->pName, pPref1, pNet2->pName, pPref2 );
    fprintf( pFile, "# Generation date: %s  %s\n", __DATE__, __TIME__);

    // write inputs
    fprintf( pFile, ".i %d\n", Ntk_NetworkReadNodeWritableNum(pNet1) + Ntk_NetworkReadNodeWritableNum(pNet2) );
    // write mv lines
    // combinational inputs
    Ntk_NetworkForEachCi( pNet1, pNode1 )
        fprintf( pFile, ".mv %s%s %d\n", pPref1, Ntk_NodeGetNameLong(pNode1), pNode1->nValues );
    Ntk_NetworkForEachCi( pNet2, pNode2 )
        fprintf( pFile, ".mv %s%s %d\n", pPref2, Ntk_NodeGetNameLong(pNode2), pNode2->nValues );
    // combinational outputs
    Ntk_NetworkForEachCo( pNet1, pNode1 )
        fprintf( pFile, ".mv %s%s %d\n", pPref1, Ntk_NodeGetNameLong(pNode1), pNode1->nValues );
    Ntk_NetworkForEachCo( pNet2, pNode2 )
        fprintf( pFile, ".mv %s%s %d\n", pPref2, Ntk_NodeGetNameLong(pNode2), pNode2->nValues );
    // internal nodes
    Ntk_NetworkForEachNode( pNet1, pNode1 )
    {
        // if an internal node has a single CO fanout, 
        // the name of this node is the same as CO name
        // and the node's .mv is already written above as the PO's .mv
        if ( Ntk_NodeHasCoName(pNode1) )
            continue;
        // otherwise, write the .mv directive if it is non-binary
        fprintf( pFile, ".mv %s%s %d\n", pPref1, Ntk_NodeGetNameLong(pNode1), pNode1->nValues );
    }
    Ntk_NetworkForEachNode( pNet2, pNode2 )
    {
        // if an internal node has a single CO fanout, 
        // the name of this node is the same as CO name
        // and the node's .mv is already written above as the PO's .mv
        if ( Ntk_NodeHasCoName(pNode2) )
            continue;
        // otherwise, write the .mv directive if it is non-binary
        fprintf( pFile, ".mv %s%s %d\n", pPref2, Ntk_NodeGetNameLong(pNode2), pNode2->nValues );
    }

    // write the number of clauses
    fprintf( pFile, ".c %d\n", nClauses1 + nClauses2 + nValuesCiTotal + nValuesCoTotal );

    // write the clauses to assert the indentity of PIs
    Ntk_NetworkForEachCi( pNet1, pNode1 )
        Ntk_NodeWriteClausesBuffer( pFile, pNode1, pNode1, pPref1, pPref2, 1 ); 
    // write the clauses to assert the non-indentity of POs
    Ntk_NetworkForEachCo( pNet1, pNode1 )
        Ntk_NodeWriteClausesBuffer( pFile, pNode1, pNode1, pPref1, pPref2, 0 ); 

    // write other clauses of the networks
    Ntk_NetworkForEachNode( pNet1, pNode1 )
    {
        // write the internal node
        Ntk_NodeWriteClauses( pFile, pNode1, pPref1 );
        // if an internal node has only one fanout, which is a PO,
        // the PO name is used for the internal node, and in this way, 
        // the PO is written into the file

        // however, if an internal node has more than one fanout
        // it is written as an intenal node with its own name
        // in this case, if among the fanouts of the node there are POs,
        // they should be written with their names, as MV buffers
        // driven by the given internal node
        if ( Ntk_NodeReadFanoutNum( pNode1 ) > 1 )
        {
            Ntk_NodeForEachFanout( pNode1, pPin, pFanout ) 
                if ( Ntk_NodeIsCo(pFanout) )
                    Ntk_NodeWriteClausesBuffer( pFile, pNode1, pFanout, pPref1, pPref1, 1 ); 
        }
    }
    Ntk_NetworkForEachNode( pNet2, pNode2 )
    {
        // write the internal node
        Ntk_NodeWriteClauses( pFile, pNode2, pPref2 );
        // if an internal node has only one fanout, which is a PO,
        // the PO name is used for the internal node, and in this way, 
        // the PO is written into the file

        // however, if an internal node has more than one fanout
        // it is written as an intenal node with its own name
        // in this case, if among the fanouts of the node there are POs,
        // they should be written with their names, as MV buffers
        // driven by the given internal node
        if ( Ntk_NodeReadFanoutNum( pNode2 ) > 1 )
        {
            Ntk_NodeForEachFanout( pNode2, pPin, pFanout ) 
                if ( Ntk_NodeIsCo(pFanout) )
                    Ntk_NodeWriteClausesBuffer( pFile, pNode2, pFanout, pPref2, pPref2, 1 ); 
        }
    }

    // finish off
    fprintf( pFile, ".e\n" );
    fclose( pFile );

    // remove the clauses
    Ntk_NetworkDeleteClauses( pNet1 );
    Ntk_NetworkDeleteClauses( pNet2 );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkCreateClauses( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    int nClauses;
    // for each internal node, get the set of clauses
    // the clauses are pset_families attached to the pNode->pCopy field
    nClauses = 0;
    Ntk_NetworkForEachNode( pNet, pNode )
        nClauses += Ntk_NodeGenerateClauses( pNode );
    return nClauses;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkDeleteClauses( Ntk_Network_t * pNet )
{
    Ntk_Node_t * pNode;
    Ntk_NetworkForEachNode( pNet, pNode )
        Mvc_CoverFree( (Mvc_Cover_t *)pNode->pCopy );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeGenerateClauses( Ntk_Node_t * pNode )
{
    Mvr_Relation_t * pMvr;
    DdNode * bRel;
    Vm_VarMap_t * pVm;
    Mvc_Cover_t * pCover;
    int nVars;

    // get the relation
    pMvr  = Ntk_NodeGetFuncMvr( pNode );
    pVm   = Ntk_NodeReadFuncVm( pNode );
    nVars = Vm_VarMapReadVarsNum( pVm );
    bRel  = Mvr_RelationReadRel( pMvr );  // comes already referenced
    // derife the cover
    pCover = Fnc_FunctionDeriveSopFromMdd( Ntk_NodeReadManMvc(pNode), pMvr, Cudd_Not(bRel), Cudd_Not(bRel), nVars );
//Ntk_NodePrintMvr( stdout, pNode );
//Ntk_NodePrintCvrPositional( stdout, pVm, pCover, Ntk_NodeReadFaninNum(pNode) + 1 );
    // attach the cover to the node
    pNode->pCopy = (Ntk_Node_t *)pCover;
    return Mvc_CoverReadCubeNum( pCover );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeWriteClauses( FILE * pFile, Ntk_Node_t * pNode, char * sPrefix )
{
    Vm_VarMap_t * pVm;
    Mvc_Cover_t * Cover;
    Mvc_Cube_t * pCube;
    bool fFirstLit, fFirstValue;
    Ntk_Node_t ** pFanins;
    int nFanins;
    int i, v;
    int nVars;
    int * pValuesFirst;


    // get the fanins
    nFanins = Ntk_NodeReadFaninNum(pNode);
    pFanins = pNode->pNet->pArray1;
    Ntk_NodeReadFanins( pNode, pFanins );
    // add the output variable just in case
    pFanins[nFanins] = pNode;
    // get the cover and the variable map to use with the cover
    Cover = (Mvc_Cover_t *)pNode->pCopy;
    pVm   = Ntk_NodeReadFuncVm( pNode );
    nVars = Vm_VarMapReadVarsNum( pVm );
    pValuesFirst = Vm_VarMapReadValuesArray(pVm);
    assert( Mvc_CoverReadBitNum(Cover) == Vm_VarMapReadValuesNum(pVm) );

    // iterate through the cubes (1 cube = 1 clause)
    Mvc_CoverForEachCube( Cover, pCube )
    {
        fFirstLit = 1;
        for ( i = 0; i < nVars; i++ )
        {
            // check if this literal is full
            for ( v = pValuesFirst[i]; v < pValuesFirst[i+1]; v++ )
                if ( !Mvc_CubeBitValue( pCube,  v ) )
                    break;
            if ( v == pValuesFirst[i+1] )
                continue;

            if ( fFirstLit )
                fFirstLit = 0;
            else
                fprintf( pFile, " " );

            // print this literal
            fFirstValue = 1;
            fprintf( pFile, "%s%s{", (sPrefix? sPrefix: ""), Ntk_NodeGetNameLong(pFanins[i]) );
            for ( v = pValuesFirst[i]; v < pValuesFirst[i+1]; v++ )
                if ( !Mvc_CubeBitValue( pCube,  v ) ) // complementation is done here!
                {
                    if ( fFirstValue )
                        fFirstValue = 0;
                    else
                        fprintf( pFile, "," );
                    fprintf( pFile, "%d", v - pValuesFirst[i] );
                }
            fprintf( pFile, "}" );
        }
        fprintf( pFile, "\n" );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_NodeWriteClausesBuffer( FILE * pFile, 
    Ntk_Node_t * pNode1, Ntk_Node_t * pNode2, 
    char * pPref1, char * pPref2, 
    int Phase )
{
    int v, i, nValues;
    bool fFirst;
    assert( pNode1->nValues == pNode2->nValues );

    nValues = pNode1->nValues;
    for ( v = 0; v < nValues; v++ )
    {
        fprintf( pFile, "%s%s", (pPref1? pPref1: ""), Ntk_NodeGetNameLong(pNode1) );
        fprintf( pFile, "{" );
        if ( Phase ) // write the buffer (pNode1 == pNode1)
            fprintf( pFile, "%d", v );
        else // write the complement of the buffer (pNode1 != pNode1)
        {
            fFirst = 1;
            for ( i = 0; i < nValues; i++ )
                if ( i != v )
                {
                    if ( fFirst )
                        fFirst = 0;
                    else
                        fprintf( pFile, "," );
                    fprintf( pFile, "%d", i );
                }
        }
        fprintf( pFile, "} " );

        fprintf( pFile, "%s%s", (pPref2? pPref2: ""), Ntk_NodeGetNameLong(pNode2) );
        fprintf( pFile, "{" );
        fFirst = 1;
        for ( i = 0; i < nValues; i++ )
            if ( i != v )
            {
                if ( fFirst )
                    fFirst = 0;
                else
                    fprintf( pFile, "," );
                fprintf( pFile, "%d", i );
            }
        fprintf( pFile, "}\n" );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


