/**CFile****************************************************************

  FileName    [ntkiAttach.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Attaches the gates from the library to the current nodes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: ntkiAttach.c,v 1.2 2005/02/28 05:34:25 alanmi Exp $]

***********************************************************************/

#include "ntkiInt.h"
#include "mio.h"
#include "mapper.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define    ATTACH_FULL             (~((unsigned)0))
#define    ATTACH_MASK(n)         ((~((unsigned)0)) >> (32-(n)))

static void Ntk_AttachSetupTruthTables( unsigned uTruths[][2] );
static void Ntk_AttachComputeTruth( Mvc_Cover_t * pCover, unsigned uTruthsIn[][2], unsigned * uTruthNode );
static Mio_Gate_t * Ntk_AttachFind( Mio_Gate_t ** ppGates, unsigned ** puTruthGates, int nGates, unsigned * uTruthNode, int * Perm );
static int Ntk_AttachCompare( unsigned ** puTruthGates, int nGates, unsigned * uTruthNode );
static int Ntk_NodeAttach( Ntk_Node_t * pNode, Mio_Gate_t ** ppGates, unsigned ** puTruthGates, int nGates, unsigned uTruths[][2] );
static void Ntk_TruthPermute( char * pPerm, int nVars, unsigned * uTruthNode, unsigned * uTruthPerm );

static char ** s_pPerms = NULL;
static int s_nPerms;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NetworkAttach( Ntk_Network_t * pNet )
{
    extern Mio_Library_t * s_pLib;
    unsigned uTruths[6][2];
    unsigned ** puTruthGates;
    Ntk_Node_t * pNode;
    Mio_Gate_t ** ppGates;
    Mio_Gate_t * pGateInv;
    int nGates, i;

    // quit if the network is not binary
    if ( !Ntk_NetworkIsBinary(pNet) )
    {
        printf( "Currently can only map binary networks.\n" );
        return 0;
    }

    // check that the library is available
    if ( s_pLib == NULL )
    {
        printf( "The current library is not available.\n" );
        return 0;
    }

    // start the truth tables
    Ntk_AttachSetupTruthTables( uTruths );
    
    // collect all the gates
    ppGates = Mio_CollectRoots( s_pLib, 6, (float)1.0e+20, 1, &nGates );

    // derive the gate truth tables
    puTruthGates    = ALLOC( unsigned *, nGates );
    puTruthGates[0] = ALLOC( unsigned, 2 * nGates );
    for ( i = 1; i < nGates; i++ )
        puTruthGates[i] = puTruthGates[i-1] + 2;
    for ( i = 0; i < nGates; i++ )
        Mio_DeriveTruthTable( ppGates[i], uTruths, Mio_GateReadInputs(ppGates[i]), 6, puTruthGates[i] );

    // assign the gates
    pGateInv = Mio_LibraryReadInv( s_pLib );
    Ntk_NetworkForEachNode( pNet, pNode )
    {
        if ( Ntk_NodeReadFaninNum(pNode) == 1 )
        {
            if ( !Ntk_NodeIsBinaryBuffer( pNode ) )
                Ntk_NodeSetMap( pNode, Ntk_NodeBindingCreate((char *)pGateInv, pNode, 0.0) );
            continue;
        }
        if ( Ntk_NodeReadFaninNum(pNode) > 6 )
        {
            printf( "Cannot attach gate with more than 6 inputs to node %s.\n", Ntk_NodeGetNamePrintable(pNode) );
            free( puTruthGates[0] );
            free( puTruthGates );
            free( ppGates );
            return 0;
        }
        if ( !Ntk_NodeAttach( pNode, ppGates, puTruthGates, nGates, uTruths ) )
        {
            printf( "Could not attach the library gate to node %s.\n", Ntk_NodeGetNamePrintable(pNode) );
            free( puTruthGates[0] );
            free( puTruthGates );
            free( ppGates );
            return 0;
        }
    }
    printf( "Library gates are successfully attached to the nodes.\n" );
    free( puTruthGates[0] );
    free( puTruthGates );
    free( ppGates );
    FREE( s_pPerms );

    // write out for verification
//    Io_NetworkWriteMappedBlif( pNet, "testmap.blif" ); 
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_NodeAttach( Ntk_Node_t * pNode, Mio_Gate_t ** ppGates, unsigned ** puTruthGates, int nGates, unsigned uTruths[][2] )
{
    int Perm[10];
    Ntk_Node_t * pNodesTemp[10];
    unsigned uTruthNode[2];
    Ntk_NodeBinding_t * pBinding;
    Ntk_Node_t ** pArray;
    Mio_Gate_t * pGate;
    int nFanins, i;
    Cvr_Cover_t * pCvr;
    Mvc_Cover_t * pMvc;

    // get hold of nodes function
    pCvr = Ntk_NodeGetFuncCvr( pNode );
    if ( pCvr == NULL )
        return 0;
    pMvc = Cvr_CoverReadIsets( pCvr )[1];
    if ( pMvc == NULL )
        return 0;
    // compute the node's truth table
    Ntk_AttachComputeTruth( pMvc, uTruths, uTruthNode );
    // find the matching gate and permutation
    pGate = Ntk_AttachFind( ppGates, puTruthGates, nGates, uTruthNode, Perm );
    if ( pGate == NULL )
        return 0;
    // create the binding
    pBinding = Ntk_NodeBindingCreate( (char *)pGate, pNode, 0.0 );
    pArray = Ntk_NodeBindingReadFanInArray( pBinding ); 
    // save the array
    nFanins = Ntk_NodeReadFaninNum( pNode );
    for ( i = 0; i < nFanins; i++ )
        pNodesTemp[i] = pArray[i];
    // permute the array
    for ( i = 0; i < nFanins; i++ )
        pArray[Perm[i]] = pNodesTemp[i];
    // set the binding
    Ntk_NodeSetMap( pNode, pBinding );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sets up the truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_AttachSetupTruthTables( unsigned uTruths[][2] )
{
    int m, v;
    for ( v = 0; v < 5; v++ )
        uTruths[v][0] = 0;
    // set up the truth tables
    for ( m = 0; m < 32; m++ )
        for ( v = 0; v < 5; v++ )
            if ( m & (1 << v) )
                uTruths[v][0] |= (1 << m);
    // make adjustments for the case of 6 variables
    for ( v = 0; v < 5; v++ )
        uTruths[v][1] = uTruths[v][0];
    uTruths[5][0] = 0;
    uTruths[5][1] = ATTACH_FULL;
}

/**Function*************************************************************

  Synopsis    [Compute the truth table of the node's cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_AttachComputeTruth( Mvc_Cover_t * pCover, unsigned uTruthsIn[][2], unsigned * uTruthRes )
{
    Mvc_Cube_t * pCube;
    unsigned uSignCube[2];
    int Var, Value;
//    int nInputs = pCover->nBits/2;
    int nInputs = 6;

    // make sure that the number of input truth tables in equal to the number of gate inputs
    assert( pCover->nBits/2 < 7 );

    // clean the resulting truth table
    uTruthRes[0] = 0;
    uTruthRes[1] = 0;
    if ( nInputs < 6 )
    {
        // consider the case when only one unsigned can be used
        Mvc_CoverForEachCube( pCover, pCube )
        {
            uSignCube[0] = ATTACH_FULL;
            Mvc_CubeForEachVarValue( pCover, pCube, Var, Value )
            {
                assert( Value );
                if ( Value == 1 )
                    uSignCube[0] &= ~uTruthsIn[Var][0];
                else if ( Value == 2 )
                    uSignCube[0] &=  uTruthsIn[Var][0];
            }
            uTruthRes[0] |= uSignCube[0];
        }
        if ( nInputs < 5 )
            uTruthRes[0] &= ATTACH_MASK(1<<nInputs);
    }
    else
    {
        // consider the case when two unsigneds should be used
        Mvc_CoverForEachCube( pCover, pCube )
        {
            uSignCube[0] = ATTACH_FULL;
            uSignCube[1] = ATTACH_FULL;
            Mvc_CubeForEachVarValue( pCover, pCube, Var, Value )
            {
                assert( Value );
                if ( Value == 1 )
                {
                    uSignCube[0] &= ~uTruthsIn[Var][0];
                    uSignCube[1] &= ~uTruthsIn[Var][1];
                }
                else if ( Value == 2 )
                {
                    uSignCube[0] &=  uTruthsIn[Var][0];
                    uSignCube[1] &=  uTruthsIn[Var][1];
                }
            }
            uTruthRes[0] |= uSignCube[0];
            uTruthRes[1] |= uSignCube[1];
        }
    }
}

/**Function*************************************************************

  Synopsis    [Find the gate by truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Gate_t * Ntk_AttachFind( Mio_Gate_t ** ppGates, unsigned ** puTruthGates, int nGates, unsigned * uTruthNode, int * Perm )
{
    unsigned uTruthPerm[2];
    int i, v, iNum;

    // try the gates without permutation
    if ( (iNum = Ntk_AttachCompare( puTruthGates, nGates, uTruthNode )) >= 0 )
    {
        for ( v = 0; v < 6; v++ )
            Perm[v] = v;
        return ppGates[iNum];
    }
    // get permutations
    if ( s_pPerms == NULL )
    {
        s_pPerms = Extra_Permutations( 6 );
        s_nPerms = Extra_Factorial( 6 );
    }
    // try permutations
    for ( i = 0; i < s_nPerms; i++ )
    {
        Ntk_TruthPermute( s_pPerms[i], 6, uTruthNode, uTruthPerm );
        if ( (iNum = Ntk_AttachCompare( puTruthGates, nGates, uTruthPerm )) >= 0 )
        {
            for ( v = 0; v < 6; v++ )
                Perm[v] = (int)s_pPerms[i][v];
            return ppGates[iNum];
        }
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Find the gate by truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ntk_AttachCompare( unsigned ** puTruthGates, int nGates, unsigned * uTruthNode )
{
    int i;
    for ( i = 0; i < nGates; i++ )
        if ( puTruthGates[i][0] == uTruthNode[0] && puTruthGates[i][1] == uTruthNode[1] )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Permutes the 6-input truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ntk_TruthPermute( char * pPerm, int nVars, unsigned * uTruthNode, unsigned * uTruthPerm )
{
    int nMints, iMintPerm, iMint, v;
    uTruthPerm[0] = uTruthPerm[1] = 0;
    nMints = (1 << nVars);
    for ( iMint = 0; iMint < nMints; iMint++ )
    {
        if ( (uTruthNode[iMint/32] & (1 << (iMint%32))) == 0 )
            continue;
        iMintPerm = 0;
        for ( v = 0; v < nVars; v++ )
            if ( iMint & (1 << v) )
                iMintPerm |= (1 << pPerm[v]);
        uTruthPerm[iMintPerm/32] |= (1 << (iMintPerm%32));     
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


