/**CFile****************************************************************

  FileName    [shResynthesis.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Resynthesis of AND-INV graphs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: shResyn.c,v 1.4 2004/05/12 04:30:11 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "shInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Sh_Network_t * Ver_NetworkStrash( Sh_Manager_t * pMan, Ntk_Network_t * pNet );
extern Sh_Node_t *    Ver_NetworkStrashBdd( Sh_Manager_t * pMan, DdManager * dd, DdNode * bFunc );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs resynthesis of AND-INV graphs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeCollapseBdd( DdManager * dd, Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    DdNode * bRes;
    Sh_Node_t * gRes;
    bRes = Sh_NodeDeriveBdd( dd, pMan, gNode );     Cudd_Ref( bRes );
    gRes = Ver_NetworkStrashBdd( pMan, dd, bRes );  Sh_Ref( gRes );
    Cudd_RecursiveDeref( dd, bRes );
    Sh_Deref( gRes );
    return gRes;
}

/**Function*************************************************************

  Synopsis    [Performs resynthesis of AND-INV graphs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeCollapse( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    Sh_Node_t * pRes;
    // write the node into the BLIF file
    Sh_NodeWriteBlif( pMan, gNode, "temp_aig_inc.blif" );
    // call the resynthesis script
    system( "mvsis -f collapse.script" );
    // read the circuit back
    pRes = Sh_NodeReadBlif( pMan, "temp_aig_outc.blif" );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Performs resynthesis of AND-INV graphs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeResynthesize( Sh_Manager_t * pMan, Sh_Node_t * gNode )
{
    Sh_Node_t * pRes;
    // write the node into the BLIF file
    Sh_NodeWriteBlif( pMan, gNode, "temp_aig_in.blif" );
    // call the resynthesis script
    system( "mvsis -f resyn.script" );
    // read the circuit back
    pRes = Sh_NodeReadBlif( pMan, "temp_aig_out.blif" );
    return pRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NodeWriteBlif( Sh_Manager_t * pMan, Sh_Node_t * gNode, char * pFileName )
{
    Sh_Network_t * pNetwork;
    pNetwork = Sh_NetworkCreate( pMan, pMan->nVars, 1 );
    pNetwork->ppOutputs[0] = gNode;   shRef( gNode );
    Sh_NetworkWriteBlif( pNetwork, NULL, NULL, pFileName );
    Sh_NetworkFree( pNetwork );
}

/**Function*************************************************************

  Synopsis    [Replaces a set of variables by a set of variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sh_NetworkWriteBlif( Sh_Network_t * p, char * sNamesIn[], char * sNamesOut[], char * sNameFile )
{
    Sh_Node_t * gNode, * gNodeIn;
    FILE * pFile;
    int i;

    pFile = fopen( sNameFile, "w" );
    fprintf( pFile, ".model %s\n", Extra_FileNameGeneric(sNameFile) );

    fprintf( pFile, ".inputs" );
    if ( sNamesIn == NULL )
        for ( i = 0; i < p->nInputs; i++ )
            fprintf( pFile, " x%d", i );
    else
        for ( i = 0; i < p->nInputs; i++ )
            fprintf( pFile, " %s", sNamesIn[i] );
    fprintf( pFile, "\n" );

    fprintf( pFile, ".outputs" );
    if ( sNamesOut == NULL )
        for ( i = 0; i < p->nOutputs; i++ )
            fprintf( pFile, " z%d", i );
    else
        for ( i = 0; i < p->nOutputs; i++ )
            fprintf( pFile, " %s", sNamesOut[i] );
    fprintf( pFile, "\n" );

    // output buffers
    for ( i = 0; i < p->nOutputs; i++ )
    {
        gNode = Sh_Regular(p->ppOutputs[i]);
        if ( gNode == p->pMan->pConst1 )
        {
            if ( sNamesOut == NULL )
                fprintf( pFile, ".names z%d\n", i );
            else
                fprintf( pFile, ".names %s\n", sNamesOut[i] );
            if ( !Sh_IsComplement(p->ppOutputs[i]) )
                fprintf( pFile, " 1\n" );
        }
        else if ( shNodeIsVar(gNode) )
        {
            if ( sNamesOut == NULL )
            {
                if ( sNamesIn == NULL )
                    fprintf( pFile, ".names x%d z%d\n", Sh_NodeReadIndex(gNode), i );
                else
                    fprintf( pFile, ".names %s z%d\n", sNamesOut[Sh_NodeReadIndex(gNode)], i );
            }
            else
            {
                if ( sNamesIn == NULL )
                    fprintf( pFile, ".names x%d %s\n", Sh_NodeReadIndex(gNode), sNamesOut[i] );
                else
                    fprintf( pFile, ".names %s %s\n", sNamesOut[Sh_NodeReadIndex(gNode)], sNamesOut[i] );
            }
            if ( !Sh_IsComplement(p->ppOutputs[i]) )
                fprintf( pFile, "1 1\n" );
            else
                fprintf( pFile, "0 1\n" );
        }
        else
        {
            if ( sNamesOut == NULL )
                fprintf( pFile, ".names [%d] z%d\n", Sh_NodeReadIndex(gNode), i );
            else
                fprintf( pFile, ".names [%d] %s\n", Sh_NodeReadIndex(gNode), sNamesOut[i] );
            if ( !Sh_IsComplement(p->ppOutputs[i]) )
                fprintf( pFile, "1 1\n" );
            else
                fprintf( pFile, "0 1\n" );
        }
    }

    // write the internal nodes
    // collect the nodes in the DFS order
    Sh_NetworkDfs( p );
    Sh_NetworkForEachNodeSpecial( p, gNode )
    {
        if ( !shNodeIsAnd(gNode) )
        {
            if ( shNodeIsConst(gNode) )
            {
//                fprintf( pFile, ".names [%d]\n", Sh_NodeReadIndex(gNode) );
//                fprintf( pFile, " 1\n" );
            }
            continue;
        }
    	// generate the edge from this node to the next (or input variable)
        fprintf( pFile, ".names", gNode );

        gNodeIn = Sh_Regular(gNode->pOne);
        if ( shNodeIsVar(gNodeIn) )
        {
            if ( sNamesIn == NULL )
                fprintf( pFile, " x%d", Sh_NodeReadIndex(gNodeIn) );
            else
                fprintf( pFile, " %s", sNamesOut[i] );
        }
        else
            fprintf( pFile, " [%d]", Sh_NodeReadIndex(gNodeIn) );

        gNodeIn = Sh_Regular(gNode->pTwo);
        if ( shNodeIsVar(gNodeIn) )
        {
            if ( sNamesIn == NULL )
                fprintf( pFile, " x%d", Sh_NodeReadIndex(gNodeIn) );
            else
                fprintf( pFile, " %s", sNamesOut[i] );
        }
        else
            fprintf( pFile, " [%d]", Sh_NodeReadIndex(gNodeIn) );
        fprintf( pFile, " [%d]\n", Sh_NodeReadIndex(gNode) );
        fprintf( pFile, "%d%d 1\n", !Sh_IsComplement(gNode->pOne), !Sh_IsComplement(gNode->pTwo) );
    }

    fprintf( pFile, ".end\n" );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    [Reads the AND-INV graph from the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Node_t * Sh_NodeReadBlif( Sh_Manager_t * pMan, char * pFileName )
{
    Sh_Network_t * pShNet;
    Sh_Node_t * gNode;
    pShNet = Sh_NetworkReadBlif( pMan, pFileName );
    assert( pShNet->nOutputs == 1 );
    gNode = pShNet->ppOutputs[0];   shRef( gNode );
    Sh_NetworkFree( pShNet );
    shDeref( gNode );
    return gNode;
}

/**Function*************************************************************

  Synopsis    [Reads the AND-INV graph from the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sh_Network_t * Sh_NetworkReadBlif( Sh_Manager_t * pMan, char * pFileName )
{
    Sh_Network_t * pShNet;
    Ntk_Network_t * pNet;
    Mv_Frame_t * pMvsis = Mv_FrameGetGlobalFrame();
    // read the second network using the same functionality manager
    pNet = Io_ReadNetwork( pMvsis, pFileName );
    if ( pNet == NULL )
    {
        printf( "Sh_NetworkReadBlif(): Reading file \"%s\" has failed.\n", pFileName );
        return NULL;
    }
    pShNet = Ver_NetworkStrash( pMan, pNet );
    Ntk_NetworkDelete( pNet );
    return pShNet;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


