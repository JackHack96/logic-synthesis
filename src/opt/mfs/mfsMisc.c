/**CFile****************************************************************

  FileName    [mfsMisc.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Performs simplification of MV networks using complete flexiblitity.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mfsMisc.c,v 1.5 2003/11/18 18:55:13 alanmi Exp $]

***********************************************************************/

#include "mfsInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

int timeGlobalFirst;
int timeGlobal;
int timeContain;
int timeImage;
int timeImagePrep;
int timeStrash;
int timeOther;
int timeSopMin;
int timeResub;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes some parameters that influence the performance.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_NetworkMfsPreprocess( Mfs_Params_t * p, Ntk_Network_t * pNet )
{
    FILE * pFile;

    // check if the network is binary
    // this check overlooks the fact the some nodes may be ISFs
    p->fBinary = Ntk_NetworkIsBinary( pNet );
    p->fNonDet = Ntk_NetworkIsND( pNet );

    // make adjustments:
    // if the network is ND, the binary transformations cannot be used
    // also when the user requested ND processing, binary cannot be used
    if ( p->fNonDet || p->fUseMv )
        p->fBinary = 0; 

    // if the network is ND, we need to use the set outputs
    if ( p->fNonDet )
        p->fSetOutputs = 1;

    // if the circuit is ND, the user want to use the external spec 
    // but it is not avaiable, report
    if ( p->fNonDet && p->fUseSpec && pNet->pSpec == NULL )
        printf( "Don't know what external spec file to use; using the current network as it is own spec.\n" );

    // if the circuit is deterministic, yet spec is required
//    if ( !p->fNonDet && p->fUseSpec )
//    {
//        printf( "The current network is deterministic; using it as its own spec.\n" );
//        p->fUseSpec = 0;
//        pNet->pSpec = NULL;
//    }

    // if the spec file name is avaiable but the file cannot be opened
    if ( p->fUseSpec && pNet->pSpec )
    {
        pFile = fopen( pNet->pSpec, "r" );
        if ( pFile == NULL )
        {
            printf( "Cannot open the external spec file \"%s\"; using the current network as it is own spec.\n", pNet->pSpec );
            p->fUseSpec = 0;
        }
        else
            fclose( pFile );
    }
}

/**Function*************************************************************

  Synopsis    [Simplifies the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mfs_NetworkMfsNodeMinimize( Mfs_Params_t * p, Ntk_Node_t * pNode, Mvr_Relation_t * pFlex )
{
    FILE * pOutput;
    Cvr_Cover_t * pCvr, * pCvrMin;
    double Ratio;
    int fChanges;
    int clk;

    // get the MVSIS output stream
    pOutput = Ntk_NodeReadMvsisOut( pNode );

    // read the current cover if present
    pCvr = Ntk_NodeReadFuncCvr( pNode );

clk = clock();
    // minimize the flexibility with Espresso
    pCvrMin = Fnc_FunctionMinimizeCvr( Ntk_NodeReadManMvc(pNode), pFlex, 0 ); // espresso
    // if failed, try minimizing using ISOP
    if ( pCvrMin == NULL )
        pCvrMin = Fnc_FunctionMinimizeCvr( Ntk_NodeReadManMvc(pNode), pFlex, 1 ); // isop
timeSopMin += clock() - clk;

    // print statistics about resubstitution
    if ( p->fVerbose )
    {
        pOutput = Ntk_NodeReadMvsisOut(pNode);
        fprintf( pOutput, "%5s : ", Ntk_NodeGetNamePrintable(pNode) );

        Ratio = Mvr_RelationFlexRatio( pFlex );
//        Ratio = 0.0;
        if ( pCvr )
            fprintf( pOutput, "CUR  cb= %3d lit= %3d  ", Cvr_CoverReadCubeNum(pCvr), Cvr_CoverReadLitFacNum(pCvr) );
        else
            fprintf( pOutput, "CUR  cb= %3s lit= %3s  ", "-", "-" );

        if ( pCvrMin )
            fprintf( pOutput, "MFS   dc   = %5.1f%%  cb= %3d lit= %3d   ", Ratio, Cvr_CoverReadCubeNum(pCvrMin), Cvr_CoverReadLitFacNum(pCvrMin) );
        else
            fprintf( pOutput, "MFS   dc   = %5.1f%%  cb= %3s lit= %3s   ", Ratio, "-", "-" );
//        fprintf( pOutput, "\n" );
    }

    // consider four situations depending on the present/absence of Cvr and CvrMin
    if ( pCvr && pCvrMin )
    { // both Cvrs are avaible - choose the best one

        // compare the minimized relation with the original one
        if ( Ntk_NodeTestAcceptCvr( pCvr, pCvrMin, p->AcceptType, 0 ) )
        { // accept the minimized node
            if ( p->fVerbose )
                fprintf( pOutput, "Accept\n" );
            // this will remove Mvr and old Cvr
            Ntk_NodeSetFuncCvr( pNode, pCvrMin );
//            Ntk_NodeMakeMinimumBase( pNode );
            fChanges = 1;
        }
        else
        { // reject the minimized node
            if ( p->fVerbose )
                fprintf( pOutput, "\n" );
            Cvr_CoverFree( pCvrMin );
            fChanges = 0;
        }
    }
    else if ( pCvr )
    { // only the old Cvr is available - leave it as it is
        if ( p->fVerbose )
            fprintf( pOutput, "\n" );
        fChanges = 0;
    }
    else if ( pCvrMin )
    { // only the new Cvr is available - set it
        if ( p->fVerbose )
            fprintf( pOutput, "Accept\n" );
        Ntk_NodeSetFuncCvr( pNode, pCvrMin );
//        Ntk_NodeMakeMinimumBase( pNode );
        fChanges = 1;
    }
    else
    { // none of them is available
        if ( p->fVerbose )
            fprintf( pOutput, "\n" );
        fprintf( pOutput, "Ntk_NetworkMfs(): Failed to minimize MV SOP of node \"%s\".\n", 
            Ntk_NodeGetNamePrintable(pNode) );
        fprintf( pOutput, "Currently the node does not have MV SOP, only MV relation.\n" );
        fprintf( pOutput, "The relation is the original one. Flexibility is not used.\n" );
        fChanges = 0;
    }

    // now, try resubstitution
clk = clock();
    if ( !Ntk_NetworkResubNode( pNode, pFlex, p->AcceptType, p->nCands, p->fVerbose ) )
    {
        // if the node is not resubstituted and it has changed, mininize its support
        if ( fChanges )
            Ntk_NodeMakeMinimumBase( pNode );
    }
    else // the node is resubstituted; there is no need to minimize the support
        fChanges = 1;
timeResub += clock() - clk;
//    Ntk_NodeMakeMinimumBase( pNode );
    return fChanges;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


