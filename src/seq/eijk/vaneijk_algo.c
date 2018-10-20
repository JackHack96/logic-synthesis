/**CFile****************************************************************

  FileNameIn  [vaneijk_algo.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [VanEijk-based sequential optimization]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: vaneijk_algo.c,v 1.3 2005/04/08 22:03:37 casem Exp $]

***********************************************************************/

#include "eijk.h"
#include "fraig.h"
#include "fraigInt.h"
#include "util.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
static void BuildTimeFrames( Fraig_Man_t* pMan,
                             EquivClsSet* pEquivClsSet,
                             int nLatches,
                             EquivClsSet* pAbsoluteEquivClsSet,
                             int nVerbose,
                             FILE* pOut );


static int ProcessEquivalences( EquivClsSet* pEquivClsSet,
                                int nVerbose,
                                FILE* pOut );

static EquivClsSet* InitialEquivCls( Fraig_Man_t* pMan,
                                     int nLatches,
                                     int nVerbose,
                                     FILE* pOut );

static Fraig_Man_t* CollapseEquivalents(Fraig_Man_t* pMan,
                                        EquivClsSet* pEquivClsSet,
                                        int* pnLatches,
                                        int nVerbose,
                                        FILE* pOut);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [vaneijk_algo]

  Description [This is the implementation of van Eijk's main algorithm.
               Parameters:
                                 
]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Man_t* vaneijk_algo( Fraig_Man_t* pMan,
                           int* pnLatches,
                           int nStats,
                           int nMultiRun,
                           int nVerbose,
                           FILE* pOut ) {

  EquivClsSet* pEquivClsSet;
  int nContinueLoop, nContinueOverApproxLoop;
  Fraig_Man_t* pManNew;
  long lStartTime = clock();
  int nIterations = 0;
  int nRedundancies = 0;
  EquivClsSet* pEquivClsSetIter;
  EquivClsSet* pAbsoluteEquivClsSet = 0;
  EquivCls* pEquivClsIter;

  if (nVerbose)
    fprintf(pOut, "Starting VanEijk's Algorithm...\n");

  ////////////////////////////////////////
  // the state space overapproximation loop
  nContinueOverApproxLoop = 1;
  while (nContinueOverApproxLoop) {

    ////////////////////////////////////////
    // Construct the initial equivalence class
    if (nVerbose)
      fprintf(pOut, "  Building initial equivalence class with every node in it\n");
    //  pEquivClsSet = InitEquivClassSet();
    //  pCurrEquivCls = NewEquivClass(pEquivClsSet);
    //  for (i = 0; i < Fraig_ManReadNodeNum(pMan); i++) {
    //    AddToClass( Fraig_ManReadIthNode(pMan, i),
    //                pCurrEquivCls );
    //  }
    pEquivClsSet = InitialEquivCls( pMan, *pnLatches, nVerbose, pOut );

    ////////////////////////////////////////
    // van Eijk's iteration
    nContinueLoop = 1;
    while (nContinueLoop) {
      nIterations++;
      BuildTimeFrames( pMan,
                       pEquivClsSet,
                       *pnLatches,
                       pAbsoluteEquivClsSet,
                       nVerbose,
                       pOut );
      nContinueLoop = ProcessEquivalences( pEquivClsSet,
                                           nVerbose,
                                           pOut );
    } // end of van Eijk


    nContinueOverApproxLoop = DiffEquivClassSets( pEquivClsSet,
                                                  pAbsoluteEquivClsSet );
    if (nVerbose)
      fprintf(pOut, "  pAbsoluteEquivClsSet %s pEquivClsSet\n",
              nContinueOverApproxLoop ? "!=" : "==");
    if (pAbsoluteEquivClsSet != 0)
      FreeEquivClassSet(pAbsoluteEquivClsSet);
    pAbsoluteEquivClsSet = pEquivClsSet;

    ////////////////////
    // statistics time
    if ((nStats == 1) || (nVerbose == 1)) {
      fprintf(pOut, "Van Eijk statistics:\n");
      fprintf(pOut, "  Time taken: %10.3lf seconds\n",
              (clock() - lStartTime) / (double)CLOCKS_PER_SEC);
      fprintf(pOut, "  Iterations: %d\n", nIterations);
    
      // count the number of redundancies in pEquivClsSet.  This is equal to the
      // number of nodes in an equivalent class that are not the first element in
      // that class
      nRedundancies = 0;
      for (pEquivClsSetIter = pEquivClsSet;
         pEquivClsSetIter != 0;
           pEquivClsSetIter = pEquivClsSetIter->pNext) {
        
        for (pEquivClsIter = pEquivClsSetIter->pEquivCls;
             pEquivClsIter != 0;
             pEquivClsIter = pEquivClsIter->pNext) {
          
          if (pEquivClsIter != pEquivClsSetIter->pEquivCls)
            nRedundancies++;
        }
      } //  end of counting redundancies
      
      fprintf(pOut, "  Redundancies found: %d\n", nRedundancies);        
    
    } // end of statistics

    if (nMultiRun == 0)
      break;
  } // end of state space overapproximation loop

  if (nVerbose)
    fprintf(pOut, "Finished with VanEijk's Algorithm\n");

  ////////////////////////////////////////
  // cleanup
  pManNew = CollapseEquivalents( pMan,
                                 pEquivClsSet,
                                 pnLatches,
                                 nVerbose,
                                 pOut );
  FreeEquivClassSet(pEquivClsSet);

  return pManNew;
}

////////////////////////////////////////////////////////////////////////////////
// Function: BuildTimeFrames
// Purpose: This will construct a combinational circuit representing two time
//          frames.  At the end of the function nodes that should stay in their
//          respective equivalence classes will be marked.
// Note: pAbsoluteEquivClsSet is a set of equivalences that we KNOW hold.  This
//       is used as an over-approximation of the reached state space if
//       pAbsoluteEquivClsSet != 0
////////////////////////////////////////////////////////////////////////////////
void BuildTimeFrames( Fraig_Man_t* pMan,
                      EquivClsSet* pEquivClsSet,
                      int nLatches,
                      EquivClsSet* pAbsoluteEquivClsSet,
                      int nVerbose,
                      FILE* pOut ) {

  Fraig_Man_t* pUnrolledAIG;
  int i;
  int nPIs, nPOs;
  EquivClsSet* pEquivClsSetIter;
  EquivCls* pEquivClsIter;
  Fraig_Node_t *pNewNode, *pInput1, *pInput2, *pCurrNode, *pGoodStateIndicator;
  Fraig_Node_t *pReachableStateIndicator;

  if (nVerbose)
    fprintf(pOut, "  BuildTimeFrames() called\n");

  ////////////////////////////////////////
  // Build an Fraig_Man_t with some default parameters
  if (nVerbose)
    fprintf(pOut, "    Creating a new AIG with some default parameters\n");
  nPIs = Fraig_ManReadInputNum(pMan) - nLatches;
  nPOs = Fraig_ManReadOutputNum(pMan) - nLatches;
  if (nVerbose) {
    fprintf(pOut, "      We have %d PIs, %d POs, and %d latches\n",
            nPIs, nPOs, nLatches);
  }
  // Note: the first and second frames have different PIs/POs
  //  pUnrolledAIG = Fraig_ManCreate( 2*nPIs + nLatches,
  pUnrolledAIG = Fraig_ManCreate( 3*nPIs + nLatches,
                                  2*nPOs + nLatches );
  Fraig_ManSetFuncRed ( pUnrolledAIG, 1 );       
  Fraig_ManSetFeedBack( pUnrolledAIG, 1 );      
  Fraig_ManSetDoSparse( pUnrolledAIG, 1 );      
  Fraig_ManSetRefCount( pUnrolledAIG, 0 );      
  Fraig_ManSetChoicing( pUnrolledAIG, 0 );      
  Fraig_ManSetVerbose ( pUnrolledAIG, 0 );       

  ////////////////////////////////////////
  // store pointers back to each node's equivalence class and do general prep on
  // all the Eijk_AuxInfo's
  if (nVerbose)
    fprintf(pOut, "    Storing pointers back to equivalence classes\n");
  for (pEquivClsSetIter = pEquivClsSet;
       pEquivClsSetIter != 0;
       pEquivClsSetIter = pEquivClsSetIter->pNext) {

    i=0;
    for (pEquivClsIter = pEquivClsSetIter->pEquivCls;
       pEquivClsIter != 0;
       pEquivClsIter = pEquivClsIter->pNext) {

      i++;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pEquivCls
        = pEquivClsSetIter->pEquivCls;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pAbsoluteEquivCls
        = 0;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pFrame1Rep
        = 0;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pFrame2Latch
        = 0;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pGoodStateRep
        = 0;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pFrame2ReachableRep
        = 0;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pFrame2ReachableEquiv
        = 0;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pFrame2Equiv
        = 0;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pFrame2Rep
        = 0;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->nStayInEquivCls
        = 1;
    } // end of iteration over nodes in 1 equivalence class

  if (nVerbose)
    fprintf(pOut, "      Processed equivalence class with %d nodes\n", i);
  } // end of iteration over all equivalence classes

  ////////////////////////////////////////
  // Build the first time frame in topological order
  if (nVerbose)
    fprintf(pOut, "    Building first time frame\n");
  for (i = 0; i < Fraig_ManReadNodeNum(pMan); i++) {
    pCurrNode = Fraig_ManReadIthNode(pMan, i);

    if (nVerbose)
      fprintf(pOut, "      Processing node %d\n", i);

    // is this node the first of its class to be instantiated?  This is
    // indicated by the absense of a designated representative
    if ( ReadEijkAux(pCurrNode)->pFrame1Rep == 0 ) {

      if (nVerbose)
        fprintf(pOut, "        Creating new node for equivalence class\n");

      // is this an input node?
      if (i < nPIs + nLatches) {
        if (nVerbose)
          fprintf(pOut, "        Linking node to an input node\n");
        pNewNode = Fraig_ManReadIthNode( pUnrolledAIG, i );
      }

      // otherwise it's not an input
      else {
        // Get the reprentatives of the two input nodes
        pInput1 = Fraig_Regular( Fraig_NodeReadOne(pCurrNode) );
        pInput1 = ReadEijkAux(pInput1)->pFrame1Rep;
        pInput2 = Fraig_Regular( Fraig_NodeReadTwo(pCurrNode) );
        pInput2 = ReadEijkAux(pInput2)->pFrame1Rep;
        // the pointers should be valid because we are iterating in topological order
        assert( pInput1 && pInput2 );
        
        // instantiate the node.  this node will now be the designated
        // representative of the equivalence class
        if (Fraig_IsComplement( Fraig_NodeReadOne(pCurrNode) ))
          pInput1 = Fraig_Not(pInput1);
        if (Fraig_IsComplement( Fraig_NodeReadTwo(pCurrNode) ))
          pInput2 = Fraig_Not(pInput2);
        if (nVerbose)
          fprintf(pOut,
                  "        Building new and gate with inputs %s%d and %s%d\n",
                  Fraig_IsComplement(pInput1) ? "¬" : "",
                  Fraig_NodeReadNum( Fraig_Regular(pInput1) ),
                  Fraig_IsComplement(pInput2) ? "¬" : "",
                  Fraig_NodeReadNum( Fraig_Regular(pInput2) ));
        pNewNode = Fraig_NodeAnd( pUnrolledAIG, pInput1, pInput2 );
      }
        
      // iterate through the equivalence class and store pointers to the new
      // designated representative
      if (nVerbose)
        fprintf(pOut, "          Other nodes in my class: ");
      for (pEquivClsIter = ReadEijkAux(pCurrNode)->pEquivCls;
           pEquivClsIter != 0;
           pEquivClsIter = pEquivClsIter->pNext) {
        if (nVerbose)
          fprintf(pOut, "%s%d ",
                  Fraig_IsComplement(pEquivClsIter->pNode) ? "¬" : "",
                  Fraig_NodeReadNum(Fraig_Regular(pEquivClsIter->pNode)) );
        ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pFrame1Rep
          = (Fraig_IsComplement(pEquivClsIter->pNode)
             ? Fraig_Not(pNewNode)
             : pNewNode);
      }
      if (nVerbose)
        fprintf(pOut, "\n");
      if (nVerbose)
        fprintf(pOut, "        Made node %d %s\n",
                Fraig_NodeReadNum(Fraig_Regular(pNewNode)),
                Fraig_IsComplement(pNewNode) ? "(complemented)" : "");
      
    } // end of if this is the first node of its class to be instantiated
    else if (nVerbose)
      fprintf(pOut, "        Using equivalence class representative (%s%d)\n",
              Fraig_IsComplement(ReadEijkAux(pCurrNode)->pFrame1Rep) ? "¬" : "",
              Fraig_NodeReadNum(Fraig_Regular(ReadEijkAux(pCurrNode)->pFrame1Rep)));

    // is this node a latch output of the first time frame?  if so then we need
    // to store a pointer to this node so that the second time frame can use it
    // as input.
    if (ReadEijkAux(pCurrNode)->nIsCO
        && (ReadEijkAux(pCurrNode)->nOutputNumber >= nPOs)) {
      pInput1
        = (Fraig_ManReadOutputs(pMan))[ReadEijkAux(pCurrNode)->nOutputNumber];
      if (nVerbose) {
        fprintf(pOut, "        This is an input to the second time frame!\n");
        fprintf(pOut,
                "        Corresponds to output %d, next frame input %d %s\n",
                ReadEijkAux(pCurrNode)->nOutputNumber,
                nPIs + (ReadEijkAux(pCurrNode)->nOutputNumber - nPOs),
                Fraig_IsComplement(pInput1) ? "(complemented)" : "");
      }
      // get the frame 1 node and complement if necessary
      pInput2 = ReadEijkAux(pCurrNode)->pFrame1Rep;
      if (Fraig_IsComplement(pInput1))
        pInput2 = Fraig_Not(pInput2);
      // store a pointer to this output to be used as the input for the next
      // time frame.
      pInput1
        = Fraig_ManReadIthNode(pMan,
                               nPIs+(ReadEijkAux(pCurrNode)->nOutputNumber-nPOs));
      ReadEijkAux(pInput1)->pFrame2Latch = pInput2;
    } // end of is this a latch output?

  } // end of loop over all nodes in the first time frame

  ////////////////////////////////////////
  // Build the characteristic function of a good state.  This function shall be
  // 1 if the current state is sufficient for the equivalence relations to hold
  // in the first frame
  if (nVerbose)
    fprintf(pOut, "    Building the characteristic of a good state\n");
  pGoodStateIndicator = Fraig_ManReadConst1( pUnrolledAIG );
  for (i = 0; i < Fraig_ManReadNodeNum(pMan); i++) {
    pCurrNode = Fraig_ManReadIthNode(pMan, i);

    if (nVerbose)
      fprintf(pOut, "      Processing node %d\n", i);

    // is this a PI?
    if (i < nPIs) {
      if (nVerbose)
        fprintf(pOut, "        This is a PI\n");
      pNewNode = Fraig_ManReadIthNode( pUnrolledAIG, nPIs + nLatches + i );
      //pNewNode = Fraig_ManReadIthNode( pUnrolledAIG, i );
      if (nVerbose)
        fprintf(pOut, "        Got node %d\n", Fraig_NodeReadNum(pNewNode));
    }

    // is this a latch?
    else if ((i >= nPIs) && (i < nPIs + nLatches)) {
      if (nVerbose)
        fprintf(pOut, "        This is a latch\n");
      pNewNode = Fraig_ManReadIthNode( pUnrolledAIG, i );
      if (nVerbose)
        fprintf(pOut, "        Got node %d\n", Fraig_NodeReadNum(pNewNode));
    }

    // otherwise we need to make a new node
    else {
      
      // Get the the two inputs
      pInput1 = Fraig_Regular( Fraig_NodeReadOne(pCurrNode) );
      pInput1 = ReadEijkAux(pInput1)->pGoodStateEquiv;
      pInput2 = Fraig_Regular( Fraig_NodeReadTwo(pCurrNode) );
      pInput2 = ReadEijkAux(pInput2)->pGoodStateEquiv;
      // the pointers we just got should be valid because we are iterating in
      // topological order
      assert( pInput1 && pInput2 );
      
      // instantiate the node.  this node will now be the designated
      // representative of the equivalence class
      if (Fraig_IsComplement( Fraig_NodeReadOne(pCurrNode) ))
        pInput1 = Fraig_Not(pInput1);
      if (Fraig_IsComplement( Fraig_NodeReadTwo(pCurrNode) ))
        pInput2 = Fraig_Not(pInput2);
      if (nVerbose)
        fprintf(pOut,
                "        Building new and gate with inputs %s%d and %s%d\n",
                Fraig_IsComplement(pInput1) ? "¬" : "",
                Fraig_NodeReadNum( Fraig_Regular(pInput1) ),
                Fraig_IsComplement(pInput2) ? "¬" : "",
                Fraig_NodeReadNum( Fraig_Regular(pInput2) ));
      pNewNode = Fraig_NodeAnd( pUnrolledAIG, pInput1, pInput2 );

      if (nVerbose)
        fprintf(pOut, "        Made node %s%d\n",
                Fraig_IsComplement(pNewNode) ? "¬" : "",
                Fraig_NodeReadNum(Fraig_Regular(pNewNode)));
    } // end of if we have to make a new node

    // save away the good state equivalent of pCurrNode
    ReadEijkAux(pCurrNode)->pGoodStateEquiv = pNewNode;

    // no designated representative?
    if ( ReadEijkAux(pCurrNode)->pGoodStateRep == 0 ) {

      if (nVerbose)
        fprintf(pOut,
                "        There was no designated representative "
                "for this class\n");

      // this shall be the representative
      for (pEquivClsIter = ReadEijkAux(pCurrNode)->pEquivCls;
           pEquivClsIter != 0;
           pEquivClsIter = pEquivClsIter->pNext) {
        ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pGoodStateRep
          = (Fraig_IsComplement(pEquivClsIter->pNode)
             ? Fraig_Not(pNewNode)
             : pNewNode);
      }
    }

    // otherwise there was a designated representative for this class and we
    // therefore need to add (this == representative) to the good state
    // characteristic function
    else {

      if (nVerbose) {
        fprintf(pOut,
                "        There WAS designated representative for this class\n");
        fprintf(pOut,
                "          Checking that %s%d == %s%d\n",
                Fraig_IsComplement(pNewNode) ? "¬" : "",
                Fraig_NodeReadNum(Fraig_Regular(pNewNode)),
                (Fraig_IsComplement( ReadEijkAux(pCurrNode)->pGoodStateRep )
                 ? "¬"
                 : ""),
                Fraig_NodeReadNum(Fraig_Regular( ReadEijkAux(pCurrNode)->pGoodStateRep )));
      }

      // BUG - this does not check for anequivalence

      // build pInput1 := designator xnor this
      pInput1 = Fraig_Not( Fraig_NodeExor( pUnrolledAIG, 
                                           pNewNode,
                                      ReadEijkAux(pCurrNode)->pGoodStateRep ) );
      if (nVerbose && (pInput1 == Fraig_Not(Fraig_ManReadConst1(pUnrolledAIG))))
        fprintf(pOut, "            Error: resulted in const 0\n");

      // now and the xnor into pGoodStateIndicator
      pGoodStateIndicator = Fraig_NodeAnd( pUnrolledAIG,
                                           pInput1,
                                           pGoodStateIndicator );

      if (nVerbose)
        fprintf(pOut, "          pGoodStateIndiator = %s%d\n",
                Fraig_IsComplement(pGoodStateIndicator) ? "¬" : "",
                Fraig_NodeReadNum(Fraig_Regular(pGoodStateIndicator)));      
    }
  }
  // sanity check
  if ( Fraig_Not(Fraig_ManReadConst1(pUnrolledAIG)) ==
       pGoodStateIndicator ) {
    fprintf(pOut,
            "Warning: There is no state such that the equivalence\n"
            "relations hold in the first time frame.  Van\n"
            "Eijk is breaking down!\n");
  }
  if ( Fraig_ManReadConst1(pUnrolledAIG) == pGoodStateIndicator ) {
    fprintf(pOut,
            "Warning: The equivalence relations hold in all states.  "
            "Was the network not combinationally reduced?");
  }

  ////////////////////////////////////////
  // Build the characteristic function of a reachable state in the second time
  // frame
  if (pAbsoluteEquivClsSet != 0) {

    // save the absolute equivalence class in the nodes
    if (nVerbose)
      fprintf(pOut,
              "    Storing pointers back to absolute"
              "equivalence classes\n");
    for (pEquivClsSetIter = pAbsoluteEquivClsSet;
         pEquivClsSetIter != 0;
         pEquivClsSetIter = pEquivClsSetIter->pNext) {
      
      i=0;
      for (pEquivClsIter = pEquivClsSetIter->pEquivCls;
           pEquivClsIter != 0;
           pEquivClsIter = pEquivClsIter->pNext) {
        
        i++;
        ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pAbsoluteEquivCls
          = pEquivClsSetIter->pEquivCls;
      } // end of iteration over nodes in 1 equivalence class
      
      if (nVerbose)
        fprintf(pOut, "      Processed equivalence class with %d nodes\n", i);
    } // end of iteration over all equivalence classes

    if (nVerbose)
      fprintf(pOut,
              "    Building characteristic of reachable state in frame 2\n");
    pReachableStateIndicator = Fraig_ManReadConst1( pUnrolledAIG );

    for (i = 0; i < Fraig_ManReadNodeNum(pMan); i++) {
      pCurrNode = Fraig_ManReadIthNode(pMan, i);
      
      if (nVerbose)
        fprintf(pOut, "      Processing node %d\n", i);

      // is this a PI?
      if (i < nPIs) {
        if (nVerbose)
          fprintf(pOut, "        This is a PI for the characteristic frame\n");
        pNewNode = Fraig_ManReadIthNode( pUnrolledAIG, 2*nPIs + nLatches + i );
      }
      
      // is this a latch?
      else if ((i >= nPIs) && (i < nPIs + nLatches)) {
        
        if (nVerbose)
          fprintf(pOut, "        This is a latch for the characteristic frame\n");
        // this reaches back across the latch boundary to get the right node in
        // frame 1
        pNewNode = ReadEijkAux(pCurrNode)->pFrame2Latch;
        if (nVerbose)
          fprintf(pOut, "          Got node %d %s\n",
                  Fraig_NodeReadNum(Fraig_Regular(pNewNode)),
                  Fraig_IsComplement(pNewNode) ? "(complemented)" : "");
      }

      // otherwise make a new and gate
      else {
               
        // Get the the two inputs
        pInput1 = Fraig_Regular( Fraig_NodeReadOne(pCurrNode) );
        pInput1 = ReadEijkAux(pInput1)->pFrame2ReachableEquiv;
        pInput2 = Fraig_Regular( Fraig_NodeReadTwo(pCurrNode) );
        pInput2 = ReadEijkAux(pInput2)->pFrame2ReachableEquiv;
        assert(pInput1 && pInput2);
        
        // instantiate the node.  this node will now be the designated
        // representative of the equivalence class
        if (Fraig_IsComplement( Fraig_NodeReadOne(pCurrNode) ))
          pInput1 = Fraig_Not(pInput1);
        if (Fraig_IsComplement( Fraig_NodeReadTwo(pCurrNode) ))
          pInput2 = Fraig_Not(pInput2);
        if (nVerbose)
          fprintf(pOut,
                  "        Building new and gate with inputs %s%d and %s%d\n",
                  Fraig_IsComplement(pInput1) ? "¬" : "",
                  Fraig_NodeReadNum( Fraig_Regular(pInput1) ),
                  Fraig_IsComplement(pInput2) ? "¬" : "",
                  Fraig_NodeReadNum( Fraig_Regular(pInput2) ));
        pNewNode = Fraig_NodeAnd( pUnrolledAIG, pInput1, pInput2 );

        if (nVerbose)
          fprintf(pOut,
                  "          Made node %d\n",
                  Fraig_NodeReadNum(Fraig_Regular(pNewNode)));
      }

      // save this newly created node
      ReadEijkAux(pCurrNode)->pFrame2ReachableEquiv = pNewNode;
    
      // no designated representative?
      if ( ReadEijkAux(pCurrNode)->pFrame2ReachableRep == 0 ) {
        
        if (nVerbose)
          fprintf(pOut,
                  "        There was no designated "
                  "representative in this time frame\n");
        
        // this shall be the representative
        for (pEquivClsIter = ReadEijkAux(pCurrNode)->pAbsoluteEquivCls;
             pEquivClsIter != 0;
             pEquivClsIter = pEquivClsIter->pNext) {
          ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pFrame2ReachableRep
            = (Fraig_IsComplement(pEquivClsIter->pNode)
               ? Fraig_Not(pNewNode)
               : pNewNode);
        }

      } // end of no designated representative
      
      // otherwise there is a designated representative
      else {
        
        if (nVerbose)
          fprintf(pOut, "        This class has a designated representative\n");
        
        // check to see if we got something equal to the representative
        pInput1 = Fraig_Not( Fraig_NodeExor(pUnrolledAIG,
                                            pNewNode,
                                  ReadEijkAux(pCurrNode)->pFrame2ReachableRep ));

        pReachableStateIndicator = Fraig_NodeAnd(pUnrolledAIG,
                                                 pReachableStateIndicator,
                                                 pInput1);

      } // end of designated representative present
      
    } // end of loop over all nodes

    // modify the good state indicator to include pReachableStateIndicator
    if (nVerbose)
      fprintf(pOut,
              "    Modifying pGoodStateIndicator to include absolute equivalences\n");
    pGoodStateIndicator = Fraig_NodeAnd( pUnrolledAIG,
                                         pGoodStateIndicator,
                                         pReachableStateIndicator );
    // another sanity check
    if ( Fraig_Not(Fraig_ManReadConst1(pUnrolledAIG)) ==
         pGoodStateIndicator ) {
      fprintf(pOut,
              "Warning: There is no state such that the equivalence\n"
              "relations hold in the first time frame AND the second\n"
              "frame is reachable.  Van Eijk is breaking down!\n");
    }

  } // end of building characteristic of reachable state in frame 2

  ////////////////////////////////////////
  // Build the second time frame in topological order
  if (nVerbose)
    fprintf(pOut, "    Building second time frame\n");
  for (i = 0; i < Fraig_ManReadNodeNum(pMan); i++) {
    pCurrNode = Fraig_ManReadIthNode(pMan, i);

    if (nVerbose)
      fprintf(pOut, "      Processing node %d\n", i);

    // is this a PI?
    if (i < nPIs) {
      if (nVerbose)
        fprintf(pOut, "        This is a PI for the second time frame (node %d)\n",
                2*nPIs + nLatches + i);
      pNewNode = Fraig_ManReadIthNode( pUnrolledAIG, 2*nPIs + nLatches + i );
    }

    // is this a latch?
    else if ((i >= nPIs) && (i < nPIs + nLatches)) {

      if (nVerbose)
        fprintf(pOut, "        This is a latch for the second time frame\n");
      // this reaches back across the latch boundary to get the right node in
      // frame 1
      pNewNode = ReadEijkAux(pCurrNode)->pFrame2Latch;
      if (nVerbose)
        fprintf(pOut, "          Got node %d %s\n",
                Fraig_NodeReadNum(Fraig_Regular(pNewNode)),
                Fraig_IsComplement(pNewNode) ? "(complemented)" : "");
    }

    // otherwise make a new and gate
    else {

      // Get the the two inputs
      pInput1 = Fraig_Regular( Fraig_NodeReadOne(pCurrNode) );
      pInput1 = ReadEijkAux(pInput1)->pFrame2Equiv;
      pInput2 = Fraig_Regular( Fraig_NodeReadTwo(pCurrNode) );
      pInput2 = ReadEijkAux(pInput2)->pFrame2Equiv;
      assert(pInput1 && pInput2);
      
      // instantiate the node.  this node will now be the designated
      // representative of the equivalence class
      if (Fraig_IsComplement( Fraig_NodeReadOne(pCurrNode) ))
        pInput1 = Fraig_Not(pInput1);
      if (Fraig_IsComplement( Fraig_NodeReadTwo(pCurrNode) ))
        pInput2 = Fraig_Not(pInput2);
      if (nVerbose)
        fprintf(pOut,
                "        Building new and gate with inputs %s%d and %s%d\n",
                Fraig_IsComplement(pInput1) ? "¬" : "",
                Fraig_NodeReadNum( Fraig_Regular(pInput1) ),
                Fraig_IsComplement(pInput2) ? "¬" : "",
                Fraig_NodeReadNum( Fraig_Regular(pInput2) ));
      pNewNode = Fraig_NodeAnd( pUnrolledAIG, pInput1, pInput2 );

      if (nVerbose)
        fprintf(pOut,
                "          Made node %d\n",
                Fraig_NodeReadNum(Fraig_Regular(pNewNode)));
    }

    // save this node away
    ReadEijkAux(pCurrNode)->pFrame2Equiv = pNewNode;
    
    // no designated representative?
    if ( ReadEijkAux(pCurrNode)->pFrame2Rep == 0 ) {

      if (nVerbose)
        fprintf(pOut, "        There was no designated representative in this time frame\n");

      // this shall be the representative
      for (pEquivClsIter = ReadEijkAux(pCurrNode)->pEquivCls;
           pEquivClsIter != 0;
           pEquivClsIter = pEquivClsIter->pNext) {
        ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pFrame2Rep
          = (Fraig_IsComplement(pEquivClsIter->pNode)
             ? Fraig_Not(pNewNode)
             : pNewNode);
      }

      // this node should stay in the equivalence class in the next van Eijk
      // iteration.
      ReadEijkAux(pCurrNode)->nStayInEquivCls = 1;
    } // end of no designated representative

    // otherwise there is a designated representative
    else {

      if (nVerbose) {
        fprintf(pOut, "        This class has a designated representative\n");
        fprintf(pOut,
                "        Checking that %s%d == %s%d\n",
                Fraig_IsComplement(pNewNode) ? "¬" : "",
                Fraig_NodeReadNum(Fraig_Regular(pNewNode)),
                (Fraig_IsComplement( ReadEijkAux(pCurrNode)->pFrame2Rep )
                 ? "¬"
                 : ""),
                Fraig_NodeReadNum(Fraig_Regular( ReadEijkAux(pCurrNode)->pFrame2Rep )));
      }


      // check to see if we got something equal to the representative in all
      // good states (pGoodStateIndicator => (this == representative))
      pInput1 = Fraig_NodeOr( pUnrolledAIG,
                              Fraig_Not( Fraig_NodeExor(pUnrolledAIG,
                                                        pNewNode,
                                           ReadEijkAux(pCurrNode)->pFrame2Rep )),
                              Fraig_Not(pGoodStateIndicator) );

      // did we get something equivalent to the representative?
      if ( pInput1 == Fraig_ManReadConst1(pUnrolledAIG) ) {
        if (nVerbose) {
          fprintf(pOut,
                  "          I am equal to the representative\n");
        }
        ReadEijkAux(pCurrNode)->nStayInEquivCls = 1;
      } else {
        if (nVerbose)
          fprintf(pOut, "          I am NOT equal to the representative\n");
        ReadEijkAux(pCurrNode)->nStayInEquivCls = 0;
      }

    } // end of designated representative present

    if (nVerbose) {
      fprintf(pOut, "        Flagged as belonging to the equivalence class: %s\n",
              ReadEijkAux(pCurrNode)->nStayInEquivCls ? "true" : "false");
    }

  } // end of loop over second time frame nodes

  ////////////////////////////////////////
  // clean up
  Fraig_ManFree(pUnrolledAIG);
  if (nVerbose)
    fprintf(pOut, "  BuildTimeFrames() finished\n");
}


////////////////////////////////////////////////////////////////////////////////
// Function: ProcessEquivalences
// Purpose: This assumes that all nodes that should stay in their respective
//          equivalence classes are marked.  It then processes pEquivClsSet to
//          reflect the changes that need to be made.  It will return 1 if it
//          changed anything, 0 else.
int ProcessEquivalences( EquivClsSet* pEquivClsSet,
                         int nVerbose,
                         FILE* pOut ) {

  EquivClsSet *pEquivClsSetIter;
  EquivCls *pEquivClsIter, *pNewEquivCls;
  int i=0, nRet=0;
  int nComp;
  int nLowest;
  Fraig_Node_t* pTemp;

  if (nVerbose)
    fprintf(pOut, "  ProcessEquivalences() started\n");

  // iterate over all equivalence classes
  for (pEquivClsSetIter = pEquivClsSet;
       pEquivClsSetIter != 0;
       pEquivClsSetIter = pEquivClsSetIter->pNext) {

    pNewEquivCls = 0;
    i=0;

    // iterate over all nodes in this equivalence class
    pEquivClsIter = pEquivClsSetIter->pEquivCls;
    while (pEquivClsIter != 0) {  

      // should this node go in a different equivalence class?
      if (ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->nStayInEquivCls
          == 0) {
        ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->nStayInEquivCls
          = 1;
        if (pNewEquivCls == 0)
          pNewEquivCls = NewEquivClass(pEquivClsSetIter);
        if (pEquivClsIter->pPrev) {
          pEquivClsIter = pEquivClsIter->pPrev;
          MoveNodeToNewClass(pEquivClsIter->pNext, // from
                             pNewEquivCls); // to
          pEquivClsIter = pEquivClsIter->pNext;
        } else {
          pEquivClsIter = pEquivClsIter->pNext;
          MoveNodeToNewClass(pEquivClsIter->pPrev, // from
                             pNewEquivCls); // to
        }
        // if this was the first equivalence class member then the start of the
        // list just moved
        if (i == 0) {
          pEquivClsSetIter->pEquivCls = pEquivClsIter;
        }
        nRet = 1;
      } else {
        i++;
        pEquivClsIter = pEquivClsIter->pNext;
      }

    } // end of iteration over nodes in this class
  } // end of iteration over all equivalence classes

  ////////////////////////////////////////
  // post-process to make sure that the first node has the lowest number
  for (pEquivClsSetIter = pEquivClsSet;
       pEquivClsSetIter != 0;
       pEquivClsSetIter = pEquivClsSetIter->pNext) {
   
    for (pEquivClsIter = pEquivClsSetIter->pEquivCls;
       pEquivClsIter != 0;
       pEquivClsIter = pEquivClsIter->pNext) {

      if (pEquivClsIter == pEquivClsSetIter->pEquivCls)
        nLowest = Fraig_NodeReadNum(Fraig_Regular(pEquivClsIter->pNode));
      else
        if (Fraig_NodeReadNum(Fraig_Regular(pEquivClsIter->pNode)) < nLowest) {
          nLowest = Fraig_NodeReadNum(Fraig_Regular(pEquivClsIter->pNode));
          pTemp = pEquivClsIter->pNode;
          pEquivClsIter->pNode = pEquivClsSetIter->pEquivCls->pNode;
          pEquivClsSetIter->pEquivCls->pNode = pTemp;
        }
          
    }
  }  

  ////////////////////////////////////////
  // post-process to insure that the first node in every class is not complemented
  for (pEquivClsSetIter = pEquivClsSet;
       pEquivClsSetIter != 0;
       pEquivClsSetIter = pEquivClsSetIter->pNext) {

    i=0;
    nComp = (Fraig_IsComplement(pEquivClsSetIter->pEquivCls->pNode)
             ? 1
             : 0);
    for (pEquivClsIter = pEquivClsSetIter->pEquivCls;
       pEquivClsIter != 0;
       pEquivClsIter = pEquivClsIter->pNext) {

      i++;
      if (nComp == 1)
        pEquivClsIter->pNode = Fraig_Not(pEquivClsIter->pNode);
    }

    if (nVerbose) {
      fprintf(pOut,
              "    Processed equivalence class (0x%x) with %d elements\n",
              (unsigned int)pEquivClsSetIter,
              i);
      pEquivClsIter = pEquivClsSetIter->pEquivCls;
      while (pEquivClsIter != 0) {  
        fprintf(pOut, "      %d (%s,%s) %s\n",
                Fraig_NodeReadNum(Fraig_Regular(pEquivClsIter->pNode)),
                ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pCIName
                  ? ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pCIName
                  : "",
                ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pCOName
                  ? ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pCOName
                  : "",
                Fraig_IsComplement(pEquivClsIter->pNode)
                  ? "complemented"
                  : "");
        pEquivClsIter = pEquivClsIter->pNext;
      }
    }
  }

  if (nVerbose)
    fprintf(pOut, "  ProcessEquivalences() finished\n");

  return nRet;
}

////////////////////////////////////////////////////////////////////////////////
// Function: InitialEquivCls
// Purpose: This will use FRAIG to partition all AIG nodes into equivalence
//          classes.
////////////////////////////////////////////////////////////////////////////////
EquivClsSet* InitialEquivCls( Fraig_Man_t* pMan,
                              int nLatches,
                              int nVerbose,
                              FILE* pOut ) {

  EquivClsSet *pRet;
  EquivCls *pCurrEquivCls;
  int nPIs, nPOs, i;
  Fraig_Man_t* pSimAIG;
  Fraig_Node_t *pCurrNode, *pNewNode, *pInput1, *pInput2;

  if (nVerbose)
    fprintf(pOut, "  InitialEquivCls() called\n");  

  pRet = InitEquivClassSet();

  ////////////////////////////////////////
  // Build an Fraig_Man_t with some default parameters
  if (nVerbose)
    fprintf(pOut, "    Creating a new AIG with some default parameters\n");
  nPIs = Fraig_ManReadInputNum(pMan) - nLatches;
  nPOs = Fraig_ManReadOutputNum(pMan) - nLatches;
  if (nVerbose) {
    fprintf(pOut, "      We have %d PIs, %d POs, and %d latches\n",
            nPIs, nPOs, nLatches);
  }
  pSimAIG = Fraig_ManCreate( nPIs, 0 );
  Fraig_ManSetFuncRed ( pSimAIG, 1 );       
  Fraig_ManSetFeedBack( pSimAIG, 1 );      
  Fraig_ManSetDoSparse( pSimAIG, 1 );      
  Fraig_ManSetRefCount( pSimAIG, 0 );      
  Fraig_ManSetChoicing( pSimAIG, 0 );      
  Fraig_ManSetVerbose ( pSimAIG, 0 );       

  ////////////////////////////////////////
  // prep the Eijk_AuxInfo's
  for (i = 0; i < Fraig_ManReadNodeNum(pMan); i++) {
    pCurrNode = Fraig_ManReadIthNode(pMan, i);
    ReadEijkAux(pCurrNode)->pInitialEquiv = 0;
  }

  ////////////////////////////////////////
  // build the simulation AIG in topological order
  if (nVerbose)
    fprintf(pOut, "    Building simulation AIG\n");
  for (i = 0; i < Fraig_ManReadNodeNum(pMan); i++) {
    pCurrNode = Fraig_ManReadIthNode(pMan, i);

    if (nVerbose)
      fprintf(pOut, "      Processing node %d\n", i);

    // is this a PI?
    if (i < nPIs) {
      if (nVerbose)
        fprintf(pOut, "        This is a PI\n");
      pNewNode = Fraig_ManReadIthNode( pSimAIG, i );
    }

    // is this a latch?
    else if ((i >= nPIs) && (i < nPIs + nLatches)) {

      if (nVerbose)
        fprintf(pOut, "        This is a latch\n");
      fprintf(pOut, "Warning: assuming initial state is 00...0\n");

      pNewNode = Fraig_Not( Fraig_ManReadConst1(pSimAIG) );
    }

    // otherwise make a new and gate
    else {

      if (nVerbose)
        fprintf(pOut, "        Constructing new and gate\n");

      // Get the the two inputs
      pInput1 = Fraig_Regular( Fraig_NodeReadOne(pCurrNode) );
      pInput1 = ReadEijkAux(pInput1)->pInitialEquiv;
      pInput2 = Fraig_Regular( Fraig_NodeReadTwo(pCurrNode) );
      pInput2 = ReadEijkAux(pInput2)->pInitialEquiv;
      assert(pInput1 && pInput2);

      if (nVerbose) {
        fprintf(pOut, "          Input 1 is node %d %s\n",
                Fraig_NodeReadNum(Fraig_Regular(pInput1)),
                Fraig_IsComplement(pInput1) ? "(complemented)" : "");
        fprintf(pOut, "          Input 2 is node %d %s\n",
                Fraig_NodeReadNum(Fraig_Regular(pInput2)),
                Fraig_IsComplement(pInput2) ? "(complemented)" : "");
      }
      
      // instantiate the node.  this node will now be the designated
      // representative of the equivalence class
      if (Fraig_IsComplement( Fraig_NodeReadOne(pCurrNode) ))
        pInput1 = Fraig_Not(pInput1);
      if (Fraig_IsComplement( Fraig_NodeReadTwo(pCurrNode) ))
        pInput2 = Fraig_Not(pInput2);
      pNewNode = Fraig_NodeAnd( pSimAIG, pInput1, pInput2 );
    }

    // save this node away
    ReadEijkAux(pCurrNode)->pInitialEquiv = pNewNode;

    // do we have an equivalence here?
    // Note: the equivalence class of a node in the sim aig will be stored in
    // that node's data0
    if (Fraig_NodeReadData0(Fraig_Regular(pNewNode)) != 0) {
      if (nVerbose) {
        fprintf(pOut,
                "          This node belongs in someone else's "
                "equivalence class (node %d)\n",
                Fraig_NodeReadNum( Fraig_Regular(pNewNode) ) );
      }

      pCurrEquivCls = (EquivCls*) Fraig_NodeReadData0(Fraig_Regular(pNewNode));
      if (ReadEijkAux(pCurrEquivCls->pNode)->pInitialEquiv
          == Fraig_Not(pNewNode)) {
        if (nVerbose)
          fprintf(pOut,
                  "            Node is the comp of the class designator\n");
        AddToClass( Fraig_Not(pCurrNode), pCurrEquivCls );
      } else {
        AddToClass( pCurrNode, pCurrEquivCls );
      }
    }

    // otherwise make a new equivalence class for this node
    else {
      if (nVerbose)
        fprintf(pOut,
                "          Creating new equivalence class for this node\n");

      pCurrEquivCls = NewEquivClass(pRet);
      AddToClass( pCurrNode, pCurrEquivCls );
      Fraig_NodeSetData0( Fraig_Regular(pNewNode),
                          (Fraig_Node_t*) pCurrEquivCls );
    }

  } // end of loop over all the nodes in pMan

  ////////////////////////////////////////
  // clean up
  Fraig_ManFree(pSimAIG);
  if (nVerbose)
    fprintf(pOut, "  InitialEquivCls() finished\n");  
  return pRet;
}


////////////////////////////////////////////////////////////////////////////////
// Function: CollapseEquivalents
// Purpose: This will rewrite the fraig and collapse equivalent nodes
////////////////////////////////////////////////////////////////////////////////
Fraig_Man_t* CollapseEquivalents(Fraig_Man_t* pMan,
                                 EquivClsSet* pEquivClsSet,
                                 int* pnLatches,
                                 int nVerbose,
                                 FILE* pOut) {
  Fraig_Man_t* pRet;
  EquivClsSet *pEquivClsSetIter;
  EquivCls *pEquivClsIter;
  int i;
  Fraig_Node_t *pCurrNode, *pNewNode, *pInput1, *pInput2;
  char* pNewName;

  if (nVerbose)
    fprintf(pOut, "  CollapseEquivalents() started\n");  

  ////////////////////////////////////////
  // Build an Fraig_Man_t with some default parameters
  if (nVerbose)
    fprintf(pOut, "    Creating a new AIG with some default parameters\n");
  pRet = Fraig_ManCreate( Fraig_ManReadInputNum(pMan),
                          Fraig_ManReadOutputNum(pMan) );
  Fraig_ManSetFuncRed ( pRet, 1 );       
  Fraig_ManSetFeedBack( pRet, 1 );      
  Fraig_ManSetDoSparse( pRet, 1 );      
  Fraig_ManSetRefCount( pRet, 0 );      
  Fraig_ManSetChoicing( pRet, 0 );      
  Fraig_ManSetVerbose ( pRet, 0 );       

  ////////////////////////////////////////
  // Prime pMan to be ripped.
  for (pEquivClsSetIter = pEquivClsSet;
       pEquivClsSetIter != 0;
       pEquivClsSetIter = pEquivClsSetIter->pNext) {

    for (pEquivClsIter = pEquivClsSetIter->pEquivCls;
         pEquivClsIter != 0;
         pEquivClsIter = pEquivClsIter->pNext) {

      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pEquivCls
        = pEquivClsSetIter->pEquivCls;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pCollapseEquiv
        = 0;
      ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pCollapseRep
        = 0;

    } // end of loop over all nodes in equivalence class
  } // end of loop over all equivalence classes

  ////////////////////////////////////////
  // copy the network
  for (i = 0; i < Fraig_ManReadNodeNum(pMan); i++) {
    pCurrNode = Fraig_ManReadIthNode(pMan, i);
    if (nVerbose)
      fprintf(pOut, "    Processing node %d\n", i);  

    // Does this node not have an equivalent class representative?
    // if this is a CI or CO then we should also explicitly make a new node
    if ((ReadEijkAux(pCurrNode)->pCollapseRep == 0) ||
        (i < Fraig_ManReadInputNum(pMan)) ||
        (ReadEijkAux(pCurrNode)->nIsCO == 1)) {
      
      // is this an input?
      if (i < Fraig_ManReadInputNum(pMan)) {
        if (nVerbose)
          fprintf(pOut, "      This is an input (%s)\n",
                  ReadEijkAux(pCurrNode)->pCIName);
        pNewNode = Fraig_ManReadIthNode(pRet, i);
      }

      // otherwise we need to build a new and gate
      else {
        if (nVerbose)
          fprintf(pOut, "      Building a new and gate\n");

        // Get the the two inputs
        pInput1 = Fraig_Regular( Fraig_NodeReadOne(pCurrNode) );
        pInput1 = ReadEijkAux(pInput1)->pCollapseEquiv;
        pInput2 = Fraig_Regular( Fraig_NodeReadTwo(pCurrNode) );
        pInput2 = ReadEijkAux(pInput2)->pCollapseEquiv;
        assert(pInput1 && pInput2);

        if (nVerbose) {
          fprintf(pOut, "      Input 1 is node %d %s\n",
                  Fraig_NodeReadNum(Fraig_Regular(pInput1)),
                  Fraig_IsComplement(pInput1) ? "(complemented)" : "");
          fprintf(pOut, "      Input 2 is node %d %s\n",
                  Fraig_NodeReadNum(Fraig_Regular(pInput2)),
                  Fraig_IsComplement(pInput2) ? "(complemented)" : "");
        }
        
        // instantiate the node.  this node will now be the designated
        // representative of the equivalence class
        if (Fraig_IsComplement( Fraig_NodeReadOne(pCurrNode) )) {
          pInput1 = Fraig_Not(pInput1);
        }
        if (Fraig_IsComplement( Fraig_NodeReadTwo(pCurrNode) )) {
          pInput2 = Fraig_Not(pInput2);
        }
        pNewNode = Fraig_NodeAnd( pRet, pInput1, pInput2 );
      } // end of building an and gate

      
      // save this node in every node from the equivalence class.  note that
      // this will automatically save pNewNode in pCurrNode
      if (ReadEijkAux(pCurrNode)->pCollapseRep == 0) {
        if (nVerbose)
          fprintf(pOut, "      Saving this node across equivalence class\n");
        for (pEquivClsIter = ReadEijkAux(pCurrNode)->pEquivCls;
             pEquivClsIter != 0;
             pEquivClsIter = pEquivClsIter->pNext) {
          ReadEijkAux(Fraig_Regular(pEquivClsIter->pNode))->pCollapseRep
            = (Fraig_IsComplement(pEquivClsIter->pNode)
               ? Fraig_Not(pNewNode)
               : pNewNode);
        }
      }

      // make a new Eijk_AuxInfo for this node
      SetEijkAux( Fraig_Regular(pNewNode),
                  (Eijk_AuxInfo*) calloc(1, sizeof(Eijk_AuxInfo) ) );

      // copy the CI/CO names from pCurrNode to pNewNode
      if (ReadEijkAux(pCurrNode)->pCIName)
        ReadEijkAux(Fraig_Regular(pNewNode))->pCIName
          = strdup( ReadEijkAux(pCurrNode)->pCIName );
      if (ReadEijkAux(pCurrNode)->pCOName)
        ReadEijkAux(Fraig_Regular(pNewNode))->pCOName
          = strdup( ReadEijkAux(pCurrNode)->pCOName );

    } // end of if this node was something we had to create

    // otherwise this equivalence class has a representative
    else {
      if (nVerbose)
        fprintf(pOut, "      Using class representative\n");
      pNewNode = ReadEijkAux(pCurrNode)->pCollapseRep;

      // append the CI names from pCurrNode to pNewNode
      if (ReadEijkAux(Fraig_Regular(pNewNode))->pCIName) {
        if (ReadEijkAux(pCurrNode)->pCIName) {
          pNewName
            = (char*) malloc( sizeof(char)
                              * (
                                 strlen(ReadEijkAux(Fraig_Regular(pNewNode))->pCIName)
                                 +
                                 strlen(ReadEijkAux(pCurrNode)->pCIName)
                                 +
                                 3
                                )
                            );
          strcpy(pNewName, ReadEijkAux(Fraig_Regular(pNewNode))->pCIName);
          strcat(pNewName, "__");
          strcat(pNewName, ReadEijkAux(pCurrNode)->pCIName);
        } else {
          pNewName = strdup(ReadEijkAux(Fraig_Regular(pNewNode))->pCIName);
        }
      } else {
        if (ReadEijkAux(pCurrNode)->pCIName) {
          pNewName = strdup(ReadEijkAux(pCurrNode)->pCIName);
        } else {
          pNewName = 0;
        }
      }
      free(ReadEijkAux(Fraig_Regular(pNewNode))->pCIName);
      ReadEijkAux(Fraig_Regular(pNewNode))->pCIName = pNewName;      

      // append the CO names from pCurrNode to pNewNode
      if (ReadEijkAux(Fraig_Regular(pNewNode))->pCOName) {
        if (ReadEijkAux(pCurrNode)->pCOName) {
          pNewName
            = (char*) malloc( sizeof(char)
                              * (
                                 strlen(ReadEijkAux(Fraig_Regular(pNewNode))->pCOName)
                                 +
                                 strlen(ReadEijkAux(pCurrNode)->pCOName)
                                 +
                                 3
                                )
                            );
          strcpy(pNewName, ReadEijkAux(Fraig_Regular(pNewNode))->pCOName);
          strcat(pNewName, "__");
          strcat(pNewName, ReadEijkAux(pCurrNode)->pCOName);
        } else {
          pNewName = strdup(ReadEijkAux(Fraig_Regular(pNewNode))->pCOName);
        }
      } else {
        if (ReadEijkAux(pCurrNode)->pCOName) {
          pNewName = strdup(ReadEijkAux(pCurrNode)->pCOName);
        } else {
          pNewName = 0;
        }
      }
      free(ReadEijkAux(Fraig_Regular(pNewNode))->pCOName);
      ReadEijkAux(Fraig_Regular(pNewNode))->pCOName = pNewName;      
    } // end of if this equiv cls had a rep

    // save away the new node
    ReadEijkAux(pCurrNode)->pCollapseEquiv = pNewNode;

    // if this was an output then it needs to be stored in the new network's
    // output array
    if (ReadEijkAux(pCurrNode)->nIsCO) {
      if (nVerbose)
        fprintf(pOut, "      This is an output (%s, %d) -> (%s)\n",
                ReadEijkAux(pCurrNode)->pCOName,
                ReadEijkAux(pCurrNode)->nOutputNumber,
                ReadEijkAux(Fraig_Regular(pNewNode))->pCOName);
      // is the output complemented?
      if (Fraig_IsComplement(
             (Fraig_ManReadOutputs(pMan))
               [ReadEijkAux(pCurrNode)->nOutputNumber] )) {
        if (nVerbose)
          fprintf(pOut, "        It's complemented\n");
        (Fraig_ManReadOutputs(pRet))[ReadEijkAux(pCurrNode)->nOutputNumber]
          = Fraig_Not(pNewNode);
      } else {
        if (nVerbose)
          fprintf(pOut, "        It's not complemented\n");
        (Fraig_ManReadOutputs(pRet))[ReadEijkAux(pCurrNode)->nOutputNumber]
          = pNewNode;
      }
    }

  } // end of loop over all nodes

  if (nVerbose)
    fprintf(pOut, "  CollapseEquivalents() finished\n");  
  return pRet;
}

