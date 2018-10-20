/**CFile****************************************************************

  FileNameIn  [eijk.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [VanEijk-based sequential optimization]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: eijk.c,v 1.3 2005/04/08 22:03:36 casem Exp $]

***********************************************************************/

#include "eijk.h"
#include "fraig.h"
#include "fraigInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
static Fraig_Man_t* Eijk_MakeFraig( Ntk_Network_t* pNet,
                                    int* pnLatches,
                                    int nVerbose,
                                    FILE* pOut);

static Ntk_Network_t* Eijk_MakeNtk( Fraig_Man_t* pMan,
                                    Mv_Frame_t* pMvsis,
                                    int nLatches,
                                    int nVerbose,
                                    FILE* pOut);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Eijk_Init]

  Description [This will be called when mvsis first starts up.  It adds the
               command "seq_resub" to the list of available commands.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Eijk_Init( Mv_Frame_t * pMvsis )
{
  Cmd_CommandAdd( pMvsis, // Pointer to the mvsis structure
                  "Sequential synthesis", // command category
                  "vaneijk", // command name
                  Eijk_CommandEijk, // callback function binding
                  1 ); // does this function change the network?
}

/**Function*************************************************************

  Synopsis    [Eijk_End]

  Description [This is called when mvsis shuts down.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Eijk_End( Mv_Frame_t * pMvsis )
{
}

/**Function*************************************************************

  Synopsis    [Eijk_CommandEijk]

  Description [This is bound to the "vaneijk" command in the mvsis
               environment.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Eijk_CommandEijk( Mv_Frame_t * pMvsis, int argc, char **argv )
{
  FILE * pOut = 0, * pErr = 0;
  Ntk_Network_t * pNet, * pNetNew;
  Fraig_Man_t* pFraigMan, *pFraigManNew;
  int nVerbose = 1;
  int nStats = 1;
  int nMultiRun = 1;
  char c;
  int nLatches;

  // get a few basic things from mvsis
  pNet = Mv_FrameReadNet(pMvsis); // Get the current network from mvsis
  pOut = Mv_FrameReadOut(pMvsis); // This gets the current output file (stdout)
  pErr = Mv_FrameReadErr(pMvsis); // Fetch the current error output file
                                  // (stderr)

  // parse the command line options
  util_getopt_reset();
  while ( ( c = util_getopt( argc, // the number of arguments
                             argv, // the argument string
                             "vsmh" ) // a list of valid options
            ) != EOF ) {
    switch ( c ) {
    case 'v':
      nVerbose ^= 1;
      break;
    case 's':
      nStats ^= 1;
      break;
    case 'm':
      nMultiRun ^= 1;
      break;
    case 'h':
      goto usage;
    default:
      goto usage;
    }
  }

  // is there actually a network loaded?
  if (pNet == NULL) {
    fprintf(pErr, "ERROR: no network loaded.  Use read_blif.\n");
    goto usage;
  }

  // convert the network to a FRAIG
  pFraigMan = Eijk_MakeFraig(pNet, // the net to convert
                             &nLatches, // the number of latches
                             nVerbose, // verbose?
                             pOut); // logging file

  // run vaneijk's algorithm
  pFraigManNew = vaneijk_algo(pFraigMan,
                              &nLatches,
                              nStats,
                              nMultiRun,
                              nVerbose,
                              pOut);

  // convert the FRAIG back into an NTK network
  pNetNew = Eijk_MakeNtk( pFraigManNew, // the fraig
                          pNet->pMvsis, // the frame
                          nLatches, // the number of latches in the aig
                          nVerbose, // verbose?
                          pOut ); // the logging file
  Mv_FrameReplaceCurrentNetwork( pNet->pMvsis, pNetNew );

  // clean up and exit
  Fraig_ManSetVerbose( pFraigMan, 0 );
  Fraig_ManFree( pFraigMan );
  if (pFraigManNew != pFraigMan) {
    Fraig_ManSetVerbose( pFraigManNew, 0 );
    Fraig_ManFree( pFraigManNew );
  }
  return 0; // normal exit
  
usage:
    fprintf( pErr, "\n" );
    fprintf( pErr, "vaneijk\n" );
    fprintf( pErr, "This is an adaptation of van Eijk's technique to logic synthesis.\n" );
    fprintf( pErr, "\n");
    fprintf( pErr, "Options:\n");
    fprintf( pErr, "-v    Toggle verbose [default=%s]\n",
             nVerbose ? "verbose" : "quiet");
    fprintf( pErr, "-s    Toggle stats [default=%s]\n",
             nStats ? "stats" : "quiet");
    fprintf( pErr, "-m    Toggle multi-round van Eijk [default=%s]\n",
             nMultiRun ? "multi-run" : "single-run");
    fprintf( pErr, "-h    Help\n");

    return 1;                   // error exit 
}

/**Function*************************************************************

  Synopsis    [Eijk_MakeFraig]

  Description [This will convert an ntk network into an AIG, with some verbose
               commenting.  pNet is the net to convert, nVerbose is whether or
               not to be verbose, and pOut is the file handle to write the
               verbose loging info to.

               This function does a number of things to make converting back to
               an NTK network easier.  It first stores the names of the CI / CO
               nodes.  Next, it stores the number of 
               latches in pnLatches.  This number is needed to figure out how
               many CI/CO nodes correspond to latches. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Man_t* Eijk_MakeFraig( Ntk_Network_t* pNet,
                             int* pnLatches,
                             int nVerbose,
                             FILE* pOut) {

  Fraig_Man_t* pRet;
  int fFuncRed = 1;
  int fFeedBack = 1;
  int fDoSparse = 1;
  int fChoicing = 0;
  int fBalance = 1;
  int i;
  Ntk_Node_t * pNode;
  Fraig_Node_t ** pgInputs, ** pgOutputs;
  int nInputs, nOutputs;

  ////////////////////
  // Make it a fraig

  if (nVerbose) {
    fprintf(pOut, "Starting NTK->FRAIG conversion...\n");
    fprintf(pOut, "  Functional reduction: %s\n",
            fFuncRed ? "enabled" : "disabled");
    fprintf(pOut, "  SAT feedback: %s\n",
            fFeedBack ? "enabled" : "disabled");
    fprintf(pOut, "  Sparse equivalence checking: %s\n",
            fDoSparse ? "enabled" : "disabled");
    fprintf(pOut, "  Recording of structural choices: %s\n",
            fChoicing ? "enabled" : "disabled");
    fprintf(pOut, "  Balanced AIG: %s\n",
            fBalance ? "enabled" : "disabled");
  }

  pRet = Ntk_NetworkFraig(pNet, // network to FRAIG
                          fFuncRed, // functional reduction
                          fFeedBack, // SAT feedback
                          fDoSparse, // sparse equivalence checking
                          fChoicing, // structural choice encoding
                          0, // verbose
                          fBalance); // balanced AIG

  // get parameters of AIG
  pgInputs  = Fraig_ManReadInputs( pRet );
  pgOutputs = Fraig_ManReadOutputs( pRet );
  nInputs = Fraig_ManReadInputNum( pRet );
  nOutputs = Fraig_ManReadOutputNum( pRet );

  ////////////////////////////////////////
  // fetch the number of latches
  *pnLatches = Ntk_NetworkReadLatchNum( pNet );
  if (nVerbose)
    fprintf(pOut, "  The network has %d latches.\n", *pnLatches);

  ////////////////////////////////////////
  // give each node a new Eijk_AuxInfo structure
  for (i = 0; i < Fraig_ManReadNodeNum(pRet); i++) {
    SetEijkAux( Fraig_ManReadIthNode(pRet, i),
                (Eijk_AuxInfo*) calloc(1, sizeof(Eijk_AuxInfo) ) );
  }

  ////////////////////
  // store the original ci / co node names.  This will be needed to
  // convert the aig back into an ntk network
  if (nVerbose)
    fprintf(pOut, "  Storing CI node names\n");
  i = 0;
  Ntk_NetworkForEachCi( pNet, pNode ) {
    ReadEijkAux(pgInputs[i])->pCIName = strdup( Ntk_NodeGetNameLong(pNode) );
    i++;
  }

  if (nVerbose)
    fprintf(pOut, "  Storing CO node names\n");
  i = 0;
  Ntk_NetworkForEachCo( pNet, pNode ) {
    ReadEijkAux(Fraig_Regular(pgOutputs[i]))->pCOName
      = strdup( Ntk_NodeGetNameLong(pNode) );
    ReadEijkAux(Fraig_Regular(pgOutputs[i]))->nIsCO = 1;
    ReadEijkAux(Fraig_Regular(pgOutputs[i]))->nOutputNumber = i;
    i++;
  }

  if (nVerbose) {
    fprintf(pOut,
            "  Conversion complete: FRAIG has %d nodes (not counting inputs)\n",
            Fraig_CountNodes(pRet)); // this gets the number of nodes
  }
  
  return pRet;
}


/**Function*************************************************************

  Synopsis    [Eijk_MakeNtk]

  Description [This will convert an AIG to an NTK network.  pMan is the AIG to
               convert, nVerbose is whether or not to be verbose, and pOut is
               the file handle to write the verbose loging info to. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ntk_Network_t* Eijk_MakeNtk( Fraig_Man_t* pMan,
                             Mv_Frame_t* pMvsis,
                             int nLatches,
                             int nVerbose,
                             FILE* pOut) {
  Ntk_Network_t * pNetNew;
  Fraig_Node_t ** pgInputs, ** pgOutputs;
  int nInputs, nOutputs;
  int i;
  Ntk_Node_t *pNodeNew;
  Fraig_Node_t *pNodeIn, *pNodeOut;
  Ntk_Latch_t * pLatchNew;
  Eijk_AuxInfo* pAux;
  Ntk_Node_t* pCurrCo;
  char *szName, *szName2;
  int nUnique;

  if (nVerbose)
    fprintf(pOut, "Starting FRAIG -> NTK conversion...\n");
  
  // get parameters of AIG
  pgInputs  = Fraig_ManReadInputs( pMan );
  pgOutputs = Fraig_ManReadOutputs( pMan );
  nInputs = Fraig_ManReadInputNum( pMan );
  nOutputs = Fraig_ManReadOutputNum( pMan );
  
  // allocate the empty network
  pNetNew = Ntk_NetworkAlloc( pMvsis );
  // register the name 
  pNetNew->pName = Ntk_NetworkRegisterNewName( pNetNew, "vanEijk_modified" );

  // add the CI nodes to pNetNew
  for (i = 0; i < nInputs; i++) {
    if (nVerbose)
      fprintf(pOut, "  Creating CI with name %s\n",
              ReadEijkAux(pgInputs[i])->pCIName);
    pNodeNew = (Ntk_Node_t *)Ntk_NodeCreate( pNetNew,
                                             ReadEijkAux(pgInputs[i])->pCIName,
                                             MV_NODE_CI,
                                             2 );
    Ntk_NetworkAddNode( pNetNew, pNodeNew, 0 );
  }

  // add the CO nodes to pNetNew
  for (i = 0; i < nOutputs; i++) {
    szName = strdup( ReadEijkAux(Fraig_Regular(pgOutputs[i]))->pCOName );

    if (nVerbose)
      fprintf(pOut, "  Creating CO with name %s\n", szName);
              
    //    nUnique = 0;
    //    while (nUnique == 0) {

    //      // make sure the CO name is unique
    //      nUnique = 1;
    //      Ntk_NetworkForEachCo(pNetNew, pCurrCo) {
    //        if (strcmp(Ntk_NodeGetNamePrintable(pCurrCo),
    //                   ReadEijkAux(Fraig_Regular(pgOutputs[i]))->pCOName) == 0) {
    //          nUnique = 0;
    //          szName2 = (char*) malloc( (strlen(szName) + 2) * sizeof(char) );
    //          strcpy(szName2, szName);
    //          szName2[strlen(szName)+1] = '0' + rand() % 10;
    //          szName2[strlen(szName)+2] = 0;
    //          free(szName);
    //          szName = szName2;
    //          break;
    //        }
    //      }
    //    } // end of while not unique

    //    if (nVerbose)
    //      fprintf(pOut, "    Name resolved to %s\n", szName);

    pNodeNew
      = (Ntk_Node_t *)Ntk_NodeCreate( pNetNew,
                                      szName,
                                      MV_NODE_CO,
                                      2 );
    Ntk_NetworkAddNode( pNetNew, pNodeNew, 0 );
    free(szName);
  }

  // make all the latches
  for (i = 0; i < nLatches; i++) {
    pNodeIn = Fraig_Regular(pgOutputs[nOutputs - nLatches + i]);
    pNodeOut = pgInputs[nInputs - nLatches + i];
    if (nVerbose)
      fprintf(pOut, "  Making a new latch with in=%s, out=%s\n",
              ReadEijkAux(pNodeIn)->pCOName,
              ReadEijkAux(pNodeOut)->pCIName);
    pLatchNew = Ntk_LatchCreate( pNetNew, // the network the latch lives in
                                 0, // the node that resets this latch (STUB)
                                 0, // the reset value (STUB)
                                 ReadEijkAux(pNodeIn)->pCOName, // input name
                                 ReadEijkAux(pNodeOut)->pCIName ); // out name
    Ntk_NodeSetSubtype( pLatchNew->pInput,  MV_BELONGS_TO_LATCH );
    Ntk_NodeSetSubtype( pLatchNew->pOutput,  MV_BELONGS_TO_LATCH );
    Ntk_NetworkAddLatch( pNetNew, pLatchNew );
  }

  // delete all saved Eijk_AuxInfo's
  for (i = 0; i < Fraig_ManReadNodeNum(pMan); i++) {
    pAux = ReadEijkAux( Fraig_ManReadIthNode(pMan, i) );
    free(pAux->pCIName);
    free(pAux->pCOName);
    free(pAux);
    SetEijkAux( Fraig_ManReadIthNode(pMan, i), 0 );
  }

  if (nVerbose)
    fprintf(pOut, "  Conversion complete\n");
  
  // now call Ntk_NetworkFraigToNet
  return Ntk_NetworkFraigToNet( pNetNew, // what this func thnks is the orig net
                                pMan, // the fraig
                                0, // don't use ands with more than 2 inputs
                                0, // don't duplicate area
                                0 ); // says I prefer one type of and config
}

