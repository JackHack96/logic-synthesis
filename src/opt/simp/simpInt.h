/**CFile****************************************************************

  FileName    [simpInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Two-level node minimization internal header file.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpInt.h,v 1.18 2004/02/11 03:29:59 alanmi Exp $]

***********************************************************************/

#ifndef _SIMPINT
#define _SIMPINT

#include "mv.h"
#include "simp.h"


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/
#define SIMP_EXEC(fct, T)        \
    {long t=ptime(); fct; T+=ptime()-t;}

#define Simp_DaemonGetNodeData(n)     (Simp_Node_t*)Ntk_NodeReadData((n))
#define Simp_DaemonSetNodeData(n,d)   Ntk_NodeSetData((n),(void*)(d))


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/**Struct**********************************************************************
  Synopsis    [Information for the toplevel routine.]
  Description []
******************************************************************************/
struct SimpInfoStruct {
    bool verbose;
    bool fPhase;              /* perform phase assignment (reset the default ) */
    bool fFilter;             /* filter out CI's outside TFI to speed up "fs"  */
    bool fConser;             /* be convervative in Espresso: skip large nodes */
    bool fSparse;             /* apply make sparse after Espresso minimization */
    bool fRelatn;             /* heuristic relation minimizer with sharp oper  */
    bool fProgrs;             /* reporting progress bar if necessary           */
    bool use_exdc;            /* utilize the EXDC network whenever possible    */
    bool use_bres;            /* utilize the boolean resub of subset support nodes */
    int  dc_type;             /* type of DC: 0-CODC, 1-SDC, 2-complete         */
    
    bool timeout_occur;       /* TRUE if a timeout has occured                 */
         
    int  nIter;               /* index of the iteration time                    */
    long timeout_net;         /* time allowed for processing each node          */
    long timeout_node;        /* time allowed for processing the whole network  */
    long time_left_node;      /* time left for processing this node             */
    long time_start_node;     /* the last recorded time                         */
    long fanin_max;           /* the nodes with more fanins are not considered  */
         
    long time_mini;           /* time spent in Espresso minimization            */
    long time_imag;           /* time spent in image computation                */
    long time_cspf;           /* time spent in compatability computation        */
    long time_glob;           /* time spent in global BDD construction          */
    
    Ntk_Node_t **    ppNodes; /* BDD ID -> CI node   s      */
    Mva_Func_t   ** ppbCoGlo; /* global BDD for CI's        */
    
    DdManager      *    ddmg; /* global MDD manager         */
    Mvc_Manager_t  * pMvcMem; /* Mvc memory manager         */
    Vmx_VarMap_t   *    pVmx; /* x-var-map for global MDD's */
    Ntk_Network_t  * network; /* mv network being operated  */
    Simp_Method_t     method; /* simplification method      */
    Simp_AcceptType_t accept; /* acceptance criterion       */
};


/**Struct**********************************************************************
  Synopsis    [Information stored for each node in the network.]
  Description []
******************************************************************************/
struct SimpNodeStruct {

    bool fBadnode;        /* orphans having no fanouts               */
    bool fVisited;        /* used in codc network traversal          */
    
    st_table *   stCi;    /* supporting CI's used in CODC compatible */
    
    DdNode   ** lMspf;    /* list of MSPF's on fanin edges           */
    DdNode   *  pCodc;    /* calculated CSPF of this node            */
};



/*---------------------------------------------------------------------------*/
/* Internal function prototypes                                              */
/*---------------------------------------------------------------------------*/


/*==== simpFull.c ====*/
EXTERN sarray_t   *SimpComputeBfsOrder(Ntk_Network_t *net,Simp_Info_t *pInfo);


/*======== simpOdc.c ========*/
EXTERN void        SimpComputeMspf(Ntk_Node_t *node, Simp_Info_t *pInfo);
EXTERN DdNode  *   SimpFilterGlobal(Simp_Info_t *pInfo, DdNode *F, st_table *tfi);


/*======== simpSdc.c ========*/
EXTERN Ntk_Node_t *SimpNodeCreateFromFanins(Ntk_Network_t *pNet,sarray_t *lFanin);
EXTERN void        SimpNodeAssignMvc(Ntk_Node_t *pNode, Mvc_Cover_t *pMvc);


/*======== simpMvf.c ========*/
EXTERN void        SimpMvaArrayListFree( sarray_t **list,int size,int start );
EXTERN void        SimpMvaArrayFree( sarray_t *lFuncs, int iStart);
EXTERN st_table *  SimpMvaComputeSupport( Mva_Func_t *pMva, Simp_Info_t *simp_info);


/*======== simpUtil.c ========*/
EXTERN Ntk_Node_t  *SimpNodeClone( Ntk_Node_t *pNode );
EXTERN Ntk_Node_t  *SimpNodeOr( Ntk_Node_t *pNode1, Ntk_Node_t *pNode2 );
EXTERN Ntk_Node_t  *SimpNodeComplement( Ntk_Node_t *pNode );
EXTERN bool         SimpNodeCompareFanin( Ntk_Node_t *p1, Ntk_Node_t *p2);
EXTERN void         SimpNodeReplace( Ntk_Node_t *pNode, Ntk_Node_t *pNodeNew);
EXTERN bool         SimpTimeOutCheckNode(Simp_Info_t *pInfo);
EXTERN void         SimpTimeOutStartNode(Simp_Info_t *pInfo, long time_left);


#endif /* _SIMP */
