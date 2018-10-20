/**CFile****************************************************************

  FileName    [encCmd.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: encCmd.c,v 1.7 2005/06/02 03:34:11 alanmi Exp $]

***********************************************************************/


#include "encInt.h"
#include "cmd.h"


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static int  EncCommandEncode(Mv_Frame_t * pMvsis, int argc, char ** argv);
static Enc_Info_t * Enc_InfoInit();
static void Enc_InfoEnd(Enc_Info_t * pInfo);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Initialize the ENC package.]

  Description [Initialize the ENC package. Called by mvsisInit() one ONCE.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
Enc_Init( Mv_Frame_t *pMvsis )
{
#ifndef BALM
    Cmd_CommandAdd(pMvsis, "Combinational synthesis", "encode",  EncCommandEncode, 1);
#endif
    return;
}

/**Function********************************************************************

  Synopsis    [End the ENC package.]

  Description [End the ENC package. Called by mvsisInit() one ONCE, when
  quitting.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
Enc_End( Mv_Frame_t *pMvsis )
{
    return;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Encode each node of the given network to binary.]

  CommandName [enc_net]

  CommandSynopsis [Encode each node of the network to binary.]

  CommandArguments [\[-h\]]

  CommandDescription [

  Command options:<p>  

  <dl>
  <dt> -v
  <dd> Print out verbose information.<p> 
  </dl>

  ]
  
  SideEffects []

******************************************************************************/
static int
EncCommandEncode(
    Mv_Frame_t *pMvsis,
    int     argc,
    char ** argv)
{
    FILE          *pErr;
    char          c;
    int           nNodes;
    Ntk_Network_t *pNet;
    Ntk_Node_t    *pNode;
    Enc_Info_t    *pInfo = NULL;
  
  
    pNet = Mv_FrameReadNet(pMvsis);
    pErr = Mv_FrameReadErr(pMvsis);
  
  
    /* set up enc info structure */
    pInfo = Enc_InfoInit( pMvsis, pNet );
  
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "dhvm:")) != EOF) {
        switch(c) {
            case 'd':
	        pInfo->fFromOutputs = FALSE;
	        break;
            case 'v':
                pInfo->verbose = TRUE;
                break;
            case 'm':
                if (strcmp(util_optarg, "inout") == 0) {
                    pInfo->method = 0;
                } else if (strcmp(util_optarg, "in") == 0) {
                    pInfo->method = 1;
                } else if (strcmp(util_optarg, "out") == 0) {
                    pInfo->method = 2;
                } else {
                    goto usage;
                }
                break; 
            default:
                goto usage;
        }
    }

    if (pNet == NULL) {
        Enc_InfoEnd(pInfo);
        fprintf( pErr, "Empty network.\n" );
        return 1;
    }

    // safety checks 
    if (!Ntk_NetworkCheck(pNet)) {
        Enc_InfoEnd(pInfo);
        printf("encode aborted\n");
        return 1;
    }

    // determinize the node
    Ntk_NetworkForEachNode( pNet, pNode ) {
        if(Ntk_NodeDeterminize(pNode))
            if(pInfo->verbose)
	        printf("Node %s is determinized.\n", 
                       Ntk_NodeGetNamePrintable(pNode));
    }


    // eliminate constant and dangling nodes
    Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );
  
    // collect the nodes to be encoded
    nNodes = 0;
    //nNodes = Cmd_CommandGetNodes( pMvsis, 
    //                              argc-util_optind+1, 
    //                              argv+util_optind-1, 0 ); 

    // encode each internal node on the list
    Enc_NetworkEncode(pNet, nNodes, pInfo);

    // eliminate constant and dangling nodes
    Ntk_NetworkSweep( pNet, 1, 1, 1, 0 );
    //Ntk_NetworkEliminate( pNet, 100000, 50, 20, 1, 0, 0);


    Enc_InfoEnd(pInfo);

    return 0;

    usage:
    
    //fprintf(pErr, "Usage: encode [-dv] [-m method] <node_list>\n");
    fprintf(pErr, "Usage: encode [-dv] [-m method]\n");
    fprintf(pErr, "\t-d        : reverse encoding direction from inputs to outputs\n");
    fprintf(pErr, "\t-m method : encoding methods [default=inout]\n");
    fprintf(pErr, "\t            in|out|inout\n");
    fprintf(pErr, "\t-v        : verbose for debugging\n");
    //fprintf(pErr, "\t<node_list> : if empty, encode all internal nodes in the network\n" );
  
    Enc_InfoEnd(pInfo);

    return 1;
}


/**Function********************************************************************

  Synopsis    [Initialize the generic data structure for the ENC package]

  Description [Initialize the generic data structure for the ENC package.
  Assign default values for all the options.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
static Enc_Info_t *
Enc_InfoInit() 
{
    Enc_Info_t *pInfo;
    
    pInfo = ALLOC(Enc_Info_t, 1);
    memset(pInfo, 0, sizeof(Enc_Info_t));
    
    pInfo->verbose      = FALSE;
    pInfo->fFromOutputs = TRUE;
    pInfo->method = ENC_INOUT;
    pInfo->simp = SIMP_NOCOMP;
    
    return pInfo;
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
static void
Enc_InfoEnd(
    Enc_Info_t * pInfo)
{
    if ( pInfo ) 
        FREE(pInfo);
}














