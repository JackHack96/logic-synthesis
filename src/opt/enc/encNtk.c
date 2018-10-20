/**CFile****************************************************************

  FileName    [encNtk.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: encNtk.c,v 1.4 2003/05/27 23:15:31 alanmi Exp $]

***********************************************************************/


#include "encInt.h"


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static void EncNetworkEncodeNode( Ntk_Network_t *pNet, Ntk_Node_t *pNode, Enc_Info_t *pInfo); 


/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void 
Enc_NetworkEncode( 
    Ntk_Network_t * pNet, 
    int nNodes, 
    Enc_Info_t *pInfo)
{
    Ntk_Node_t ** ppNodes;
    Ntk_Node_t * pNode;
    int i;


    // collect nodes into the list
    if (nNodes == 0) {  
        // encode all internal nodes
	nNodes = Ntk_NetworkReadNodeIntNum(pNet);
        Ntk_NetworkDfs(pNet, pInfo->fFromOutputs);
    }
    if (nNodes == 0)
        ppNodes = NULL;
    else
        ppNodes = ALLOC( Ntk_Node_t *, nNodes );
    i = 0;
    Ntk_NetworkForEachNodeSpecial( pNet, pNode ) {
        if ( !Ntk_NodeIsInternal(pNode) )
	    continue;
        if ( Ntk_NodeReadValueNum(pNode) > 2) {
	    (void) Ntk_NodeGetFuncMvr(pNode);  // enforce mvr to exist
            ppNodes[i++] = pNode;
	}
	else {
	    nNodes--;
	}
    }
    assert( i == nNodes );
    
    
    // encode nodes one by one
    for ( i = 0; i < nNodes; i++ ) {
        if ( !Enc_NodeCanBeEncoded( ppNodes[i] ) ) {
            printf( "Ntk_NetworkEncode(): Cannot encode node \"%s\".\n", 
                    Ntk_NodeGetNamePrintable(ppNodes[i]) );
            continue;
        }
	
        EncNetworkEncodeNode( pNet, ppNodes[i], pInfo );
    }
    if( nNodes )
        FREE( ppNodes );

    if ( !Ntk_NetworkCheck( pNet ) )
        printf( "Ntk_NetworkEncode(): Network check has failed.\n" );
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
static void
EncNetworkEncodeNode(
    Ntk_Network_t *pNet, 
    Ntk_Node_t *pNode,
    Enc_Info_t *pInfo) 
{
    int i;
    Enc_Code_t *  pCode;
    Ntk_Pin_t  *  pPin, * pPin2;
    Ntk_Node_t *  pFanout, * pDecoder;
    Ntk_Node_t ** ppBinNodes;
    

    if(Ntk_NodeReadValueNum(pNode) <= 2)
      return;
        
    // get coding
    pCode = Enc_NodeComputeCode(pNet, pNode, pInfo);

    if(pInfo->verbose) {
        for(i=0;i<pCode->numValues;i++){
            int j;
            printf("value %d:  ",i);
            for(j=0;j<pCode->codeLength;j++)
	        printf("%d ",pCode->codeArray[i*pCode->codeLength+j]);
            printf("\n");
        }
        for(i=0;i<pCode->numCodes;i++)
            printf("(%d,%d)\t",i,pCode->assignArray[i]);
        printf("\n");
    }

 
    // create new binary nodes
    ppBinNodes = ALLOC(Ntk_Node_t *, pCode->codeLength);
    Ntk_NodeCreateEncoded(pNode, ppBinNodes, pCode->codeLength,
                          pCode->assignArray, pCode->numCodes,
                          pCode->numValues);
    
    // add new binary nodes to the network
    for (i = 0; i < pCode->codeLength; i++)
        Ntk_NetworkAddNode(pNet, ppBinNodes[i], TRUE);

    // create decoder
    pDecoder = Ntk_NodeCreateDecoderGeneral( ppBinNodes, 
                                             pCode->codeLength,
                                             pCode->assignArray, 
                                             pCode->numCodes,
					     pCode->numValues );
    Ntk_NodeMakeMinimumBase(pDecoder);

    // replace the original node
    Ntk_NodeReplace(pNode, pDecoder); // disposes of pDecoder


    //update fanouts (keep CIs and COs multi-valued)
    Ntk_NodeForEachFanoutSafe(pNode, pPin, pPin2, pFanout){
        if (!Ntk_NodeIsCo(pFanout))
            Ntk_NetworkCollapseNodes(pFanout, pNode);     
    }

    if (Ntk_NodeReadFanoutNum(pNode) == 0)
        Ntk_NetworkDeleteNode( pNet, pNode, 1, 1 );

    Enc_CodeFree(pCode);
    FREE(ppBinNodes);
}













