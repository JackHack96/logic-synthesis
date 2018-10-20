/**CFile****************************************************************

  FileName    [simpCmd.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [MV network simplification commands]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpCmd.c,v 1.34 2005/06/02 03:34:12 alanmi Exp $]

***********************************************************************/

#include "espresso.h"
#include "simpInt.h"
#include "cmd.h"
#include <time.h>


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static int  SimpCommandFullsimp(Mv_Frame_t * pMvsis, int argc, char ** argv);
static int  SimpCommandSimplify(Mv_Frame_t * pMvsis, int argc, char ** argv);
static int  SimpNetworkReadCost( Simp_Info_t *pInfo, Ntk_Network_t *pNet );

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Initialize the generic data structure for the simp package]

  Description [Initialize the generic data structure for the simp package.
  Assign default values for all the options.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
Simp_Info_t *
Simp_InfoInit(
    Mv_Frame_t *pMvsis,
    Ntk_Network_t *net) 
{
    char *sValue;
    int   iCost;
    Simp_Info_t *pInfo;
    
    pInfo = ALLOC(Simp_Info_t, 1);
    memset(pInfo, 0, sizeof(Simp_Info_t));
    
    pInfo->timeout_net   = 5000;
    pInfo->timeout_node  = 10;
    pInfo->fanin_max     = 30;
    
    pInfo->timeout_occur = FALSE;
    pInfo->verbose       = FALSE;
    pInfo->method        = SIMP_SNOCOMP;
    pInfo->use_bres      = TRUE;
    pInfo->fConser       = TRUE;
    pInfo->fProgrs       = TRUE;
    
    /* global variable used by 'set' command */
    sValue = Cmd_FlagReadByName( pMvsis, "cost" );
    if ( sValue ) {
        iCost = atoi( sValue );
    }
    else {
        iCost = 2;
    }
    
    if ( iCost == 0 ) {
        pInfo->accept = SIMP_CUBE;
    }
    else if ( iCost == 1 ) {
        pInfo->accept = SIMP_SOP_LIT;
    }
    else {
        pInfo->accept = SIMP_FCT_LIT;
    }
    
    return pInfo;
}


/**Function********************************************************************

  Synopsis    [Initialize the simp package.]

  Description [Initialize the simp package. Called by mvsisInit() one ONCE.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
Simp_Init(
    Mv_Frame_t *pMvsis)
{
#ifndef BALM
    Cmd_CommandAdd(pMvsis, "Combinational synthesis", "simplify", SimpCommandSimplify, 1);
    Cmd_CommandAdd(pMvsis, "Combinational synthesis", "fullsimp", SimpCommandFullsimp, 1);
#endif
    return;
}

/**Function********************************************************************

  Synopsis    [End the simp package.]

  Description [End the simp package. Called by mvsisInit() one ONCE, when
  quitting.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
Simp_End(
    void)
{
    /* clean up memory allocated by Cvr_EspressoSetUp() */
    sf_cleanup();
    Cvr_CubeCleanUp ();
    return;
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Simplify each node with mv CODC's]

  CommandName [fullsimp]

  CommandSynopsis [Simplify each node with mv CODC's]

  CommandArguments [\[-h\]]

  CommandDescription [Simplify each node in the network using Compatible
  Observability Don't Cares (CODC). The network is traversed in a reverse
  topological order. At each node, the CODC set is computed for each fanout
  edge. These are intersected and represented in the global BDD domain. Then
  image computation maps these don't cares into the local space of the
  node, which includes the satisfiability don't cares (SDC) for the nodes
  in the transitive fanin cone. By default, the SDC's for the nodes with a
  subset of the immediate fanins is also computed, which leads to possible
  Boolean resubstitution. Finally, the node is minimized by ESPRESSO-MV using
  the don't cares. The simplified result replaces the old one if it has a
  lower cost (cube count, literal count, or literal count in the factored
  forms). 

  Command options:<p>  
  ]
  
  SideEffects []

******************************************************************************/
static int
SimpCommandFullsimp(
    Mv_Frame_t *pMvsis,
    int     argc,
    char ** argv)
{
  FILE *pErr;
  char         c;
  int          fChanges, nIter, nTimeout, nNodes, nCost, nTemp;
  bool         fTimeout=FALSE;
  Ntk_Network_t *pNet;
  Simp_Info_t *pInfo=NULL;
  
  nIter = 15;
  nTimeout = 0;
  
  pNet = Mv_FrameReadNet(pMvsis);
  pErr = Mv_FrameReadErr(pMvsis);
  
  /* set up simp info structure */
  pInfo = Simp_InfoInit( pMvsis, pNet );
  
  util_getopt_reset();
  while ((c = util_getopt(argc, argv, "bcfnpsxrvd:m:i:t:T:")) != EOF) {
    switch(c) {
        case 'v':
            pInfo->verbose = TRUE;
            break;
        case 'b':
            pInfo->fProgrs = FALSE;
            break;
        case 'c':
            pInfo->fConser = FALSE;
            break;
        case 'f':
            pInfo->fFilter = TRUE;
            break;
        case 'n':
            pInfo->fRelatn = TRUE;
            break;
        case 'p':
            pInfo->fPhase = TRUE;
            break;
        case 's':
            pInfo->fSparse = TRUE;
            break;
        case 'r':
            pInfo->use_bres = FALSE;
            break;
            
        case 'd':
            if (strcmp(util_optarg, "codc") == 0) {
                pInfo->dc_type = 0;
            }
            else if (strcmp(util_optarg, "sdc") == 0) {
                pInfo->dc_type = 1;
            }
            else if (strcmp(util_optarg, "modc") == 0) {
                pInfo->dc_type = 2;
            }
            else if (strcmp(util_optarg, "complete") == 0) {
                pInfo->dc_type = 3;
            }
            else {
                goto usage;
            }
            break; 
            
        case 'i':
            nIter = atoi(util_optarg);
            if (nIter<0) goto usage;
            break;
        case 't':
            pInfo->timeout_node = atoi(util_optarg);
            if (pInfo->timeout_node < 0) goto usage;
            break;
        case 'T':
            pInfo->timeout_net = atoi(util_optarg);
            if (pInfo->timeout_net < 0) goto usage;
            break;
            
        case 'm':
            if (strcmp(util_optarg, "nocomp") == 0) {
                pInfo->method = SIMP_NOCOMP;
            }
            else if (strcmp(util_optarg, "snocomp") == 0) {
                pInfo->method = SIMP_SNOCOMP;
            }
            else if (strcmp(util_optarg, "dcsimp") == 0) {
                pInfo->method = SIMP_DCSIMP;
            }
            else if (strcmp(util_optarg, "espresso") == 0) {
                pInfo->method = SIMP_ESPRESSO;
            }
            else if (strcmp(util_optarg, "exact") == 0) {
                pInfo->method = SIMP_EXACT;
            }
            else if (strcmp(util_optarg, "exactlit") == 0) {
                pInfo->method = SIMP_EXACT_LITS;
            }
            else {
                goto usage;
            }
            break;
        default:
            goto usage;
    }
  }
  
  /* safty checks */
  if ( pNet == NULL ) {
      fprintf( Mv_FrameReadErr(pMvsis), "Empty network\n" );
      FREE( pInfo );
      return 1;
  }
  if (!Ntk_NetworkCheck(pNet)) {
      printf("fullsimp aborted\n");
      FREE( pInfo );
      return 1;
  }
  
  /* complete the info initialization */
  pInfo->network = pNet;
  pInfo->pMvcMem = Ntk_NetworkReadManMvc( pNet );
  
  /* read initial values of the network */
  nNodes = Ntk_NetworkReadNodeIntNum( pNet );
  nCost  = SimpNetworkReadCost( pInfo, pNet );
  
  if (pInfo->timeout_net > 0) {
      pInfo->timeout_net *= CLOCKS_PER_SEC;
  }
  if (pInfo->timeout_node > 0) {
      pInfo->timeout_node *= CLOCKS_PER_SEC;
  }
  
  do {
      fChanges = 0;
      pInfo->nIter++;
      
      Ntk_NetworkSweep(pNet, 0,0,0,pInfo->verbose);
      fTimeout = Simp_NetworkFullSimplify(pNet, pInfo);
      
      if (!Ntk_NetworkCheck(pNet)) {
          FREE(pInfo);
          printf("fullsimp aborted\n");
          return 1;
      }
      
      /* test progress */
      nTemp = Ntk_NetworkReadNodeIntNum( pNet );
      if (nNodes != nTemp) {
          fChanges = 1;
          nNodes   = nTemp;
      }
      else {
          nTemp = SimpNetworkReadCost( pInfo, pNet );
          if (nCost != nTemp) {
              fChanges = 1;
              nCost    = nTemp;
          }
      }
      
      nIter--;
  } while (nIter!=0 && fChanges && !fTimeout);
  
  FREE(pInfo);
  
  return 0;

 usage:
  fprintf(pErr, "Usage: fullsimp [-cfnpsrv][-d dctype][-m method][-i n][-t n][-T n]\n");
  fprintf(pErr, "\t-c        : suppress conservative approach in Espresso minimization\n");
  fprintf(pErr, "\t-f        : filter CODC with TFI to speed up computation   \n");
  fprintf(pErr, "\t-n        : heuristic nd-relation minimization using sharp \n");
  fprintf(pErr, "\t-p        : phase assignment (reset the default i-set)     \n");
  fprintf(pErr, "\t-r        : suppress boolean resubstitution of subset supports\n");
  fprintf(pErr, "\t-s        : make sparse after Espresso minimization        \n");
  fprintf(pErr, "\t-v        : verbose for debugging                          \n");
  fprintf(pErr, "\t-d dctype : don't care type                                \n");
  fprintf(pErr, "\t            sdc | codc(default)                            \n");
  fprintf(pErr, "\t-i n      : iterate fullsimp for n times until no change   \n");
  fprintf(pErr, "\t-t n      : timeout at one node after n seconds [default=10]\n");
  fprintf(pErr, "\t-T n      : timeout the whole process after n seconds [default=5000]\n");
  fprintf(pErr, "\t-m method : SOP minimization methods [default=snocomp] \n");
  fprintf(pErr, "\t            espresso|nocomp|snocomp|dcsimp|exact|exactlit\n");

  if (pInfo) FREE(pInfo);
  return 1;
}


/**Function********************************************************************

  Synopsis    [Simplify each node using ESPRESSO.]

  CommandName [simplify]

  CommandSynopsis [Simplify each node using ESPRESSO]

  CommandArguments []

  CommandDescription [ Simplify each node in the network using local don't
  cares. Satisfiability don't cares (SDC) are computed for local fanins, and
  optionally for the nodes with a subset of the immediate fanin support. The
  latter leads to possible Boolean resubstitution. The node is minimized by
  ESPRESSO-MV using the SDC set. The simplified result replaces the old one if
  it has a lower cost (cube count, literal count, or literal count in the
  factored forms).

  Command options:<p>  
  ]
  
  SideEffects []

******************************************************************************/
static int
SimpCommandSimplify(
    Mv_Frame_t *pMvsis,
    int     argc,
    char ** argv)
{
  FILE *pErr;
  Ntk_Network_t *pNet;
  Simp_Info_t  *pInfo=NULL;
  char c;
  
  pNet = Mv_FrameReadNet(pMvsis);
  pErr = Mv_FrameReadErr(pMvsis);
  
  pInfo = Simp_InfoInit( pMvsis, pNet );
  pInfo->timeout_net  = 5000;
  pInfo->use_bres     = TRUE;
  pInfo->dc_type      = 1;
  
  
  util_getopt_reset();
  while ((c = util_getopt(argc, argv, "fcnpsxrvd:m:t:T:")) != EOF) {
    switch(c) {
        case 'f':
            if ( util_optind >= argc )
            {
                fprintf( pErr, "Command line switch \"-f\" should be followed by an integer.\n" );
                goto usage;
            }
            pInfo->fanin_max = atoi(argv[util_optind]);
            util_optind++;
            if ( pInfo->fanin_max < 0 ) 
                goto usage;
            break;
        case 'v':
            pInfo->verbose = TRUE;
            break;
        case 'c':
            pInfo->fConser = TRUE;
            break;
        case 'n':
            pInfo->fRelatn = TRUE;
            break;
        case 'p':
            pInfo->fPhase = TRUE;
            break;
        case 's':
            pInfo->fSparse = TRUE;
            break;
        case 'r':
            pInfo->use_bres = 0;
            break;
            
        case 'd':
            if (strcmp(util_optarg, "none") == 0) {
                pInfo->dc_type = 0;
            }
            else if (strcmp(util_optarg, "sdc") == 0) {
                pInfo->dc_type = 1;
            }
            else {
                goto usage;
            }
            break; 
            
        case 't':
            pInfo->timeout_node = atoi(util_optarg);
            if (pInfo->timeout_node < 0) goto usage;
            break;
        case 'T':
            pInfo->timeout_net = atoi(util_optarg);
            if (pInfo->timeout_net < 0) goto usage;
            break;
            
        case 'm':
            if (strcmp(util_optarg, "simple") == 0) {
                pInfo->method   = SIMP_SIMPLE;
                pInfo->dc_type  = 0;
                pInfo->use_bres = 0;
            }
            else if (strcmp(util_optarg, "nocomp") == 0) {
                pInfo->method = SIMP_NOCOMP;
            }
            else if (strcmp(util_optarg, "snocomp") == 0) {
                pInfo->method = SIMP_SNOCOMP;
            }
            else if (strcmp(util_optarg, "dcsimp") == 0) {
                pInfo->method = SIMP_DCSIMP;
            }
            else if (strcmp(util_optarg, "espresso") == 0) {
                pInfo->method = SIMP_ESPRESSO;
            }
            else if (strcmp(util_optarg, "exact") == 0) {
                pInfo->method = SIMP_EXACT;
            }
            else if (strcmp(util_optarg, "exactlit") == 0) {
                pInfo->method = SIMP_EXACT_LITS;
            }
            else {
                goto usage;
            }
            break;
        default:
            goto usage;
    }
  }
  
  if ( pNet == NULL ) {
      fprintf( Mv_FrameReadErr(pMvsis), "Empty network\n" );
      return 1;
  }
  
  Ntk_NetworkSweep( pNet, 0,0,0,0 );
  Simp_NetworkSimplify( pNet, pInfo );
  Ntk_NetworkSweep( pNet, 1,1,1,0 );
  
  if (!Ntk_NetworkCheck(pNet)) {
      fail("simplify: network error after simplify!");
  }
  
  if (pInfo) FREE(pInfo);
  return 0;
  
 usage:
  fprintf(pErr, "Usage: simplify [-cnpsrv][-d dctype][-t n][-T n][-m method]\n");
  fprintf(pErr, "\t-c        : suppress conservative approach in Espresso minimization \n");
  fprintf(pErr, "\t-n        : heuristic nd-relation minimization using sharp \n");
  fprintf(pErr, "\t-p        : phase assignment (reset the default i-set)     \n");
  fprintf(pErr, "\t-r        : suppress boolean resubstitution of subset supports\n");
  fprintf(pErr, "\t-s        : make sparse after Espresso minimization        \n");
  fprintf(pErr, "\t-v        : verbose for debugging                          \n");
  fprintf(pErr, "\t-d dctype : don't care type                                \n");
  fprintf(pErr, "\t            none | sdc(default)                            \n");
  fprintf(pErr, "\t-t n      : timeout at one node after n seconds [default=10]\n");
  fprintf(pErr, "\t-T n      : timeout the whole process after n seconds [default=5000]\n");
  fprintf(pErr, "\t-f num    : the fanin limit (if more, skip the node) [default = %d]\n", pInfo->fanin_max );
  fprintf(pErr, "\t-m method : SOP minimization methods [default=snocomp]     \n");
  fprintf(pErr, "\t            simple|espresso|nocomp|snocomp|dcsimp|exact|exactlit\n");

  if (pInfo) FREE(pInfo);
  return 1;
}


/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
SimpNetworkReadCost( Simp_Info_t *pInfo, Ntk_Network_t *pNet ) 
{
    Cvr_Cover_t *pCvr;
    Ntk_Node_t  *pNode;
    int         nCost;
    
    nCost = 0;
    if ( pInfo->accept == SIMP_CUBE ) {
        Ntk_NetworkForEachNode( pNet, pNode ) {
            pCvr = Ntk_NodeGetFuncCvr( pNode );
            nCost += Cvr_CoverReadCubeNum( pCvr );
        }
    }
    else if ( pInfo->accept == SIMP_SOP_LIT ) {
        Ntk_NetworkForEachNode( pNet, pNode ) {
            pCvr = Ntk_NodeGetFuncCvr( pNode );
            nCost += Cvr_CoverReadLitSopNum( pCvr );
        }
    }
    else {
        Ntk_NetworkForEachNode( pNet, pNode ) {
            pCvr = Ntk_NodeGetFuncCvr( pNode );
            nCost += Cvr_CoverReadLitFacNum( pCvr );
        }
    }
    
    return nCost;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

