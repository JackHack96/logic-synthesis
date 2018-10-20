/**CFile****************************************************************

  FileName    [cvrEspresso.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Call Espresso for two-level minimization]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - March 15, 2003.]

  Revision    [$Id: cvrEspresso.c,v 1.27 2003/05/27 23:14:56 alanmi Exp $]

***********************************************************************/

#include "mv.h"
#include "cvrInt.h"
#include "espresso.h"
#include "minimize.h"

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static pset_family * Cvr_CoverDerivePsfFromMvcVector(Mvc_Cover_t **ppMvc, int nCovers);
static pset_family   Cvr_CoverDerivePsfFromMvc(Mvc_Cover_t *pMvc);
static Mvc_Cover_t **Cvr_CoverDeriveMvcFromPsfVector(Mvc_Manager_t *pMem,
                                                     pset_family *ppCovers,
                                                     int nCovers);
static Mvc_Cover_t * Cvr_CoverDeriveMvcFromPsf(Mvc_Manager_t *pMem,
                                               pset_family pCover);

static void CvrSetupCube( int num_vars, int *part_size );
static void CvrSetupCdata( struct cube_struct *pCube );
static void CvrSetdownCube( void );
static void CvrSetdownCdata( void );

static void _PsetFamilyResetDefault( pset_family *pOnset, int nVal );
static void _PsetFamilyArrayPrint(FILE *pF, pset_family *pcovers,int size,bool verb);
static void _PsetFamilyPrint(FILE *pFile, pset_family psf, bool verb);
static void _PsetPrint(FILE *fp, pcube c, char *out_map);


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static int  nCvrCubeAllocated=64;
static bool fCvrCubeAllocated=0;

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis    [Simplify a multi-valued cover using don't cares provided.]
  Description [Simplify a multi-valued cover using don't cares
  provided. According to the `method' provided, call appropriate ESPRESSO
  routines for two-level SOP minimization. Assume the function f and don't
  care dc are made common base, i.e. having the same number of fanins and same
  value for each fanin.]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
Cvr_CoverEspresso(
    Cvr_Cover_t * pCf,
    Mvc_Cover_t * pCd,     /* binary cover */
    int           method,
    bool          fSparse,
    bool          fConserve,
    bool          fPhase) 
{
    Mvc_Cover_t **ppMvc, *pMvc;
    pset_family   dcset;
    pset_family   offset;
    pset_family  *ponset;
    int           nVal, i;
    bool          fDefault;
    
    if (pCf==NULL) return;
    
    /* deal with tautology i-set */
    if ( Cvr_CoverEspressoSpecial( pCf ) )
        return;
    
    nVal = Vm_VarMapReadValuesOutNum( pCf->pVm );
    
    Cvr_CoverEspressoSetup(pCf->pVm);  /* prepare the cube structure */
    
    
    /* for pure MV pla's, we have to resort to Espresso */
    if (cube.num_binary_vars==0
        && method != 3      /* DCSIMP */
        && method != 6      /* EBD_ISOP */
        && method != 7)     /* SIMPLE */ {
        method = 0;         /* ESPRESSO */
    }
    
    /* if conservative; skip large SOP's */
    if ( method == 0 && fConserve && Cvr_CoverIsTooLarge( pCf ) ) {
        method = 7;    /* conservative */
    }
    
    /* if asked to reset default */
    if ( fPhase ) {
        Cvr_CoverComputeDefault(pCf);
    }
    
    /* convert mvc covers to pset_family */
    ppMvc = Cvr_CoverReadIsets(pCf);
    ponset = Cvr_CoverDerivePsfFromMvcVector(ppMvc, nVal);
    
    
    if (pCd) {
        dcset = Cvr_CoverDerivePsfFromMvc(pCd);
    } else {
        dcset = sf_new(1,0);  /* constant zero */
    }
    
#ifdef DEBUG_ESPRESSO
    printf("\nONSET:\n");
    _PsetFamilyArrayPrint(stdout, ponset,nVal,1);
    printf("\nDCSET:\n");
    _PsetFamilyPrint(stdout, dcset,1);
#endif

    /* Set these to make Espresso() and Minimize() work */
    for (i=0; i<nVal; ++i) {
        if (ponset[i]==NULL || ponset[i]->sf_size==0)
            continue;
        pMvc = ppMvc[i];
        dcset->wsize   = ponset[i]->wsize;
        dcset->sf_size = ponset[i]->sf_size;
        break;
    }
    
    /* assume there is no default i-set */
    fDefault = FALSE;
    
    /* start minimization */
    switch(method) {
        case 0:  /* SIMP_ESPRESSO */
            if ( !fSparse ) {
                skip_make_sparse = 1;
            }
            else {
                skip_make_sparse = 0;
            }
            
            /* minimize each value at a time */
            for (i=0; i<nVal; i++) {
                if ( ponset[i] == NULL ) {
                    fDefault = TRUE;  //there is a default
                    continue;
                }
                if ( ponset[i]->count == 0 )
                    continue;
                
                if (dcset->sf_size==0||dcset->count==0) {
                    
                    if ( nVal == 2 && ponset[nVal-1-i] ) {
                        offset = sf_save( ponset[nVal-1-i] );
                    }
                    else {
                        offset = complement(cube1list(ponset[i]));
                    }
                }
                else  {
                    offset = complement(cube2list(ponset[i],dcset)); 
                }
                ponset[i] = espresso(ponset[i], dcset, offset);
                sf_free(offset); offset = NULL;
            }
            break;
            
        case 1:  /* SIMP_NOCOMP: */
            for (i=0; i<nVal; i++) {
                if ( ponset[i] == NULL ) {
                    fDefault = TRUE;
                    continue;
                }
                if ( ponset[i]->count==0 )
                    continue;
                ponset[i] = minimize(ponset[i], dcset, 0);  /* NOCOMP */
            }
            break;
            
        case 2: /* SIMP_SNOCOMP: */
            for (i=0; i<nVal; i++) {
                if ( ponset[i] == NULL ) {
                    fDefault = TRUE;
                    continue;
                }
                if ( ponset[i]->count==0 )
                    continue;
                ponset[i] = minimize(ponset[i], dcset, 1); /* SNOCOMP */
            }
            break;
            
        case 3: /* SIMP_DCSIMP: */
            for (i=0; i<nVal; i++) {
                if ( ponset[i] == NULL ) {
                    fDefault = TRUE;
                    continue;
                }
                if ( ponset[i]->count==0 )
                    continue;
                ponset[i] = minimize(ponset[i], dcset, 2);  /* DCSIMP */
            }
            break;
            
        case 4: /* SIMP_EXACT: */
            for (i=0; i<nVal; i++) {
                if ( ponset[i] == NULL ) {
                    fDefault = TRUE;
                    continue;
                }
                if ( ponset[i]->count==0 )
                    continue;
                ponset[i] = minimize_exact(ponset[i], dcset, NIL(set_family_t), 1);
            }
            break;
            
        case 5: /* SIMP_EXACT_LITS: */
            for (i=0; i<nVal; i++) {
                if ( ponset[i] == NULL ) {
                    fDefault = TRUE;
                    continue;
                }
                if ( ponset[i]->count==0 )
                    continue;
                ponset[i] = minimize_exact_literals(ponset[i], dcset, NIL(set_family_t), 1);
            }
            break;
            /***
        case SIMP_EBD_ISOP:
            for (i=0; i<nVal; i++) {
                if (ponset[i] == NULL) continue;
                ponset[i] = Simp_EbdIsopCompute(cubestruct, ponset[i],dcset);
            }
            break;
            ***/
        case 7: /* SIMP_SIMPLE: */
            for (i=0; i<nVal; i++) {
                int j;
                if ( ponset[i] == NULL ) {
                    fDefault = TRUE;
                    continue;
                }
                if ( ponset[i]->count==0 )
                    continue;
                
                for (j = 0; j < cube.num_vars; j++) {
                     ponset[i] = d1merge(ponset[i], j);
                }
                ponset[i] = sf_contain( ponset[i] );
            }
            break;
        default:
        {}
    }
    
#ifdef DEBUG_ESPRESSO
    printf("\nRESULT:\n");
    _PsetFamilyArrayPrint(stdout, ponset,nVal,1);
#endif
    
    /* reset the default if asked to do so, or all i-sets are present */
    if ( fPhase || !fDefault ) {
        _PsetFamilyResetDefault( ponset, nVal );
    }
    
    /* convert pset_family to mvc covers in the same mem manager */
    ppMvc = Cvr_CoverDeriveMvcFromPsfVector(pMvc->pMem, ponset, nVal);
    
    /* free temporary pset_families */
    sf_free(dcset);
    for (i=0; i<nVal; ++i) {
        if (ponset[i]) sf_free(ponset[i]);
    }
    FREE( ponset );
    
    /* save the result */
    Cvr_CoverSetIsets(pCf, ppMvc);
    
    /* clean the recycled pset_family's - added by alanmi to prevent memory leaks */
    sf_cleanup();
    Cvr_CoverEspressoSetdown(pCf->pVm);
    return;
}


/**Function********************************************************************
  Synopsis    [Minimize only an i-set]
  Description [Minimize only an i-set. assume that the onset and dcset have
  the same domain. dcset can be set to NULL. need to give the cube structure
  which both f and dc share. Input dcset will remain; a new pointer onset
  will be returned; THE OLD ONSET IS DISPOSED OF!]
  SideEffects []
  SeeAlso     []
******************************************************************************/
Mvc_Cover_t *
Cvr_IsetEspresso(
    Vm_VarMap_t *  pVm,
    Mvc_Cover_t *  pMvcOnset,
    Mvc_Cover_t *  pMvcDcset,
    int            method,
    bool           fSparse,
    bool           fConserve )
{
    bool fCubeAllocated;
    pset_family pOffset, pDcset, pOnset;
    Mvc_Cover_t *pMvcNew;
    
    if ( pMvcOnset==NULL )
        return NULL;
    if ( Mvc_CoverIsEmpty( pMvcOnset ) || Mvc_CoverIsTautology( pMvcOnset ) )
        return Mvc_CoverDup( pMvcOnset );
    
    fCubeAllocated = (cube.fullset == NULL);
    if ( fCubeAllocated ) {
        Cvr_CoverEspressoSetup(pVm);
    }
    
    if (cube.num_binary_vars==0
        && method != 3      /* DCSIMP */
        && method != 6      /* EBD_ISOP */
        && method != 7)     /* SIMPLE */ {
        method = 0;         /* ESPRESSO */
    }
    
    /* conservative; use d1merge */
    if ( method == 0 && fConserve && Cvr_IsetIsTooLarge( pVm, pMvcOnset ) ) {
        method = 7;
    }
    
    /* convert mvc covers to pset_family */
    pOnset = Cvr_CoverDerivePsfFromMvc(pMvcOnset);
    
    if ( pMvcDcset && method != 7 ) {
        pDcset = Cvr_CoverDerivePsfFromMvc(pMvcDcset);
    }
    else {
        pDcset = sf_new(1, cube.size);
    }
    pDcset->wsize   = pOnset->wsize;
    pDcset->sf_size = pOnset->sf_size;
    
#ifdef DEBUG_ESPRESSO
    printf("\nONSET:\n");
    _PsetFamilyPrint(stdout, pOnset,1);
    printf("\nDCSET:\n");
    _PsetFamilyPrint(stdout, pDcset,1);
#endif
    
    switch(method) {
        case 0: /* SIMP_ESPRESSO: */
          if (pDcset->sf_size==0||pDcset->count==0) {
              pOffset = complement(cube1list(pOnset));
          }
          else  {
              pOffset = complement(cube2list(pOnset, pDcset)); 
          }
          if ( !fSparse ) {
              skip_make_sparse = 1;
          }
          else {
              skip_make_sparse = 0;
          }
          pOnset = espresso(pOnset, pDcset, pOffset);
          sf_free(pOffset); pOffset = NULL;
          break;
          
        case 1:  /* SIMP_NOCOMP: */
          pOnset = minimize(pOnset, pDcset, 0);  /* NOCOMP */
          break;
          
        case 2:  /* SIMP_SNOCOMP: */
          pOnset = minimize(pOnset, pDcset, 1); /* SNOCOMP */
          break;
          
        case 3:  /* SIMP_DCSIMP: */
          pOnset = minimize(pOnset, pDcset, 2);  /* DCSIMP */
          break;
          
        case 4:  /* SIMP_EXACT: */
          pOnset = minimize_exact(pOnset, pDcset, NIL(set_family_t), 1);
          break;
          
        case 5:  /* SIMP_EXACT_LITS: */
          pOnset = minimize_exact_literals(pOnset, pDcset, NIL(set_family_t), 1);
          break;
          
        case 7: /* SIMP_SIMPLE: */
        {
            int i;
            for (i = 0; i < cube.num_vars; i++) {
                pOnset = d1merge( pOnset, i );
            }
            pOnset = sf_contain( pOnset );
            break;
        }
        
      default:
	  {}
    }
    
#ifdef DEBUG_ESPRESSO
    printf("\nRESULT:\n");
    _PsetFamilyPrint(stdout, pOnset,1);
#endif
    
    /* convert results back to mvc covers */
    pMvcNew = Cvr_CoverDeriveMvcFromPsf(pMvcOnset->pMem, pOnset);
    
    sf_free(pOnset);
    sf_free(pDcset);
    
    if ( fCubeAllocated ) {
        Cvr_CoverEspressoSetdown(pVm);
    }
    
    return pMvcNew;
}


/**Function********************************************************************
  Synopsis      []
  Description   [derive the complement of a binary output Mvc cover]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
Mvc_Cover_t *
Cvr_CoverComplement(
    Vm_VarMap_t *pVm,
    Mvc_Cover_t *pMvc ) 
{
    bool        fCubeAllocated;
    pset_family psfOnset, psfOffset;
    Mvc_Cover_t *pCmpl;
    
    if (pMvc == NULL) return NULL;
    
    fCubeAllocated = (cube.fullset == NULL);
    if ( fCubeAllocated ) {
        Cvr_CoverEspressoSetup(pVm);
    }
    
    psfOnset = Cvr_CoverDerivePsfFromMvc(pMvc);
    
    psfOffset = complement(cube1list(psfOnset));
    
    pCmpl = Cvr_CoverDeriveMvcFromPsf(pMvc->pMem, psfOffset);
    
    sf_free(psfOnset);
    sf_free(psfOffset);
    
    if ( fCubeAllocated ) {
        Cvr_CoverEspressoSetdown(pVm);
    }
    
    return pCmpl;
}


/**Function********************************************************************
  Synopsis      [clean up temporarily allocated cube memory]
  Description   [normally called while quitting mvsis]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
void
Cvr_CubeCleanUp( void ) 
{
    if ( fCvrCubeAllocated ) {
        
        FREE( cube.part_size  );
        FREE( cube.first_part );
        FREE( cube.last_part  );
        FREE( cube.first_word );
        FREE( cube.last_word  );
        FREE( cube.var_mask   );
        FREE( cube.sparse     );
        FREE( cube.temp       );
    }
    return;
}

/**Function********************************************************************
  Synopsis      [Free cubes]
  Description   [Free cubes but not the arrays.]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
bool
Cvr_CoverEspressoSpecial( Cvr_Cover_t *pCvr ) 
{
    bool fTaut;
    int  i, nVal, iTaut;
    Mvc_Cover_t  * pMvcTaut;
    
    fTaut = FALSE;
    nVal  = Vm_VarMapReadValuesOutNum( pCvr->pVm );
    for ( i = 0; i < nVal; ++i ) {
        if ( pCvr->ppCovers[i] ) {
            if ( Mvc_CoverIsTautology( pCvr->ppCovers[i] ) ) {
                pMvcTaut = pCvr->ppCovers[i];
                fTaut = TRUE;
                iTaut = i;
            }
        }
    }
    if ( fTaut ) {
        for ( i = 0; i < nVal; ++i ) {
            if ( pCvr->ppCovers[i] && i != iTaut ) {
                Mvc_CoverFree( pCvr->ppCovers[i] );
                pCvr->ppCovers[i] = Mvc_CoverAlloc(pMvcTaut->pMem, pMvcTaut->nBits);
                /* allocate an empty cover */
            }
        }
    }
    
    return fTaut;
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************
  Synopsis      []
  Description   []
  SideEffects   []
  SeeAlso       []
******************************************************************************/
pset_family *
Cvr_CoverDerivePsfFromMvcVector(Mvc_Cover_t **ppMvc, int nCovers)
{
    int i;
    pset_family *pFam;
    
    if (nCovers == 0) return NULL;
    
    pFam = ALLOC(pset_family, nCovers);
    for (i=0; i<nCovers; ++i) {
        if (ppMvc[i])
            pFam[i] = Cvr_CoverDerivePsfFromMvc(ppMvc[i]);
        else
            pFam[i] = NULL;
    }
    return pFam;
}

/**Function********************************************************************
  Synopsis      []
  Description   [assume the global cube structure has been set up.]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
pset_family 
Cvr_CoverDerivePsfFromMvc(Mvc_Cover_t *pMvc)
{
    Mvc_Cube_t  *pMvcCube;
    pset_family pFam;
    pset        pSet;
    int         nCubes, nWords, nBits, i;
    
    if (pMvc==NULL) return NULL;
    
    if ( Mvc_CoverIsEmpty(pMvc) ) {
        pFam = sf_new(0, cube.size);
        return pFam;
    }
    if ( Mvc_CoverIsTautology(pMvc) ) {
        pFam = sf_new(1, cube.size);
        pSet = GETSET(pFam, pFam->count++);
        set_copy( pSet, cube.fullset );
        return pFam;
    }
    
    nCubes = Mvc_CoverReadCubeNum(pMvc);
    pFam   = sf_new((nCubes+1), cube.size);
    nWords = Mvc_CoverReadWordNum(pMvc);
    nBits  = Mvc_CoverReadBitNum(pMvc);
    
    Mvc_CoverForEachCube( pMvc, pMvcCube ) {
        pSet = GETSET(pFam, pFam->count++);
        PUTLOOP(pSet, nWords);
        PUTSIZE(pSet, nBits);
        i = nWords;
        do pSet[i] = pMvcCube->pData[i-1]; while (--i > 0);
        set_and(pSet, pSet, cube.fullset);
    }
    
    return pFam;
}

/**Function********************************************************************
  Synopsis      [Free cubes]
  Description   [Free cubes but not the arrays.]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
Mvc_Cover_t **
Cvr_CoverDeriveMvcFromPsfVector(Mvc_Manager_t *pMem, pset_family *ppCovers, int nCovers)
{
    int i;
    Mvc_Cover_t **ppMvc;
    
    if (nCovers == 0) return NULL;
    
    ppMvc = ALLOC(Mvc_Cover_t *, nCovers);
    for (i=0; i<nCovers; ++i) {
        ppMvc[i] = Cvr_CoverDeriveMvcFromPsf(pMem, ppCovers[i]);
    }
    return ppMvc;
}

/**Function********************************************************************
  Synopsis      [Free cubes]
  Description   [Free cubes but not the arrays.]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
Mvc_Cover_t *
Cvr_CoverDeriveMvcFromPsf(Mvc_Manager_t *pMem, pset_family pCover)
{
    Mvc_Cube_t  *pMvcCube;
    Mvc_Cover_t *pMvcCover;
    pset        pLast, pThis;
    int         nCubes, nWords, nBits, i;
    
    if (pCover==NULL) return NULL;
    
    if ( pCover->count==0 || pCover->sf_size==0 ) {
        
        pMvcCover = Mvc_CoverAlloc(pMem, cube.size);
        return pMvcCover;
    }
    
    nCubes = pCover->count;
    nWords = pCover->wsize;
    nBits  = pCover->sf_size;
    pMvcCover = Mvc_CoverAlloc(pMem, nBits);
    
    foreach_set( pCover, pLast, pThis) {
        pMvcCube = Mvc_CubeAlloc(pMvcCover);
        i = nWords;
        do pMvcCube->pData[i-2] = pThis[i-1]; while (--i > 1);
        //pMvcCube->pData[nWords-2] |= (~cube.fullset[nWords-1]);
        Mvc_CoverAddCubeTail (pMvcCover, pMvcCube);
    }
    
    return pMvcCover;
}



/**Function********************************************************************
  Synopsis      [setup the global cdata structure from ESPRESSO]
  Description   [setup the global cdata structure from ESPRESSO]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
void
CvrSetupCdata(
    struct cube_struct *pCube)
{
    //CvrSetdownCdata();
    cdata.part_zeros   = ALLOC(int, pCube->size);
    cdata.var_zeros    = ALLOC(int, pCube->num_vars);
    cdata.parts_active = ALLOC(int, pCube->num_vars);
    cdata.is_unate     = ALLOC(int, pCube->num_vars);
}  

/**Function********************************************************************
  Synopsis      [setdown the global cdata structure from ESPRESSO]
  Description   [setdown the global cdata structure from ESPRESSO]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
void
CvrSetdownCdata(
    void)
{
    FREE(cdata.part_zeros);
    FREE(cdata.var_zeros);
    FREE(cdata.parts_active);
    FREE(cdata.is_unate);
}

/**Function********************************************************************
  Synopsis      [setup the Espresso cube structure.]
  Description   [directly write to the global variable "cube"]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
void
CvrSetupCube(
    int num_vars,
    int *part_size)
{
    register int i, var;
    register pcube p;
    
    
    cube.num_vars  = num_vars;
    if (!fCvrCubeAllocated) {
        cube.part_size  = ALLOC(int, nCvrCubeAllocated);
        cube.first_part = ALLOC(int, nCvrCubeAllocated);
        cube.last_part  = ALLOC(int, nCvrCubeAllocated);
        cube.first_word = ALLOC(int, nCvrCubeAllocated);
        cube.last_word  = ALLOC(int, nCvrCubeAllocated);
        cube.var_mask   = ALLOC(pset,nCvrCubeAllocated);
        cube.sparse     = ALLOC(int, nCvrCubeAllocated);
        cube.temp       = ALLOC(pset,CUBE_TEMP);
        fCvrCubeAllocated = 1;
    }
    if (num_vars > nCvrCubeAllocated) {
        cube.part_size  = REALLOC(int, cube.part_size,  num_vars);
        cube.first_part = REALLOC(int, cube.first_part, num_vars);
        cube.last_part  = REALLOC(int, cube.last_part,  num_vars);
        cube.first_word = REALLOC(int, cube.first_word, num_vars);
        cube.last_word  = REALLOC(int, cube.last_word,  num_vars);
        cube.var_mask   = REALLOC(pset,cube.var_mask,   num_vars);
        cube.sparse     = REALLOC(int, cube.sparse,     num_vars);
        nCvrCubeAllocated = num_vars;
    }
    
    /* reset the num_binary_vars */
    cube.num_binary_vars = 0;
    for (i=0; i<cube.num_vars; ++i) {
        cube.part_size[i] = part_size[i];
    }
    i=0; while (cube.part_size[i++]==2 && i<=num_vars)
        (cube.num_binary_vars)++;
    
    cube.num_mv_vars = cube.num_vars - cube.num_binary_vars;
    cube.output = -1;
    /*cube.output = cube.num_mv_vars > 0 ? cube.num_vars - 1 : -1;*/
    
    cube.size = 0;
    for(var = 0; var < cube.num_vars; var++) {
        if (var < cube.num_binary_vars) {
            cube.part_size[var] = 2;
        }
        cube.first_part[var] = cube.size;
        cube.first_word[var] = WHICH_WORD(cube.size);
        cube.size += ABS(cube.part_size[var]);
        cube.last_part[var] = cube.size - 1;
        cube.last_word[var] = WHICH_WORD(cube.size - 1);
    }
    
    /* the cubes have been freed in CvrSetdownCube() */
    cube.binary_mask = set_new(cube.size);
    cube.mv_mask = set_new(cube.size);
    
    for(var = 0; var < cube.num_vars; var++) {
        p = cube.var_mask[var] = set_new(cube.size);
        for(i = cube.first_part[var]; i <= cube.last_part[var]; i++)
            set_insert(p, i);
        if (var < cube.num_binary_vars) {
            INLINEset_or(cube.binary_mask, cube.binary_mask, p);
            cube.sparse[var] = 0;
        } else {
            INLINEset_or(cube.mv_mask, cube.mv_mask, p);
            cube.sparse[var] = 1;
        }
    }
    if (cube.num_binary_vars == 0)
        cube.inword = -1;
    else {
        cube.inword = cube.last_word[cube.num_binary_vars - 1];
        cube.inmask = cube.binary_mask[cube.inword] & DISJOINT;
    }
    
    for(i = 0; i < CUBE_TEMP; i++) {
        cube.temp[i] = set_new(cube.size);
    }
    cube.fullset  = set_fill(set_new(cube.size), cube.size);
    cube.emptyset = set_new(cube.size);
    
    return;
}

/**Function********************************************************************
  Synopsis      [Free cubes]
  Description   [Free cubes but not the arrays.]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
void
CvrSetdownCube( void )
{
    int i, var;
    
    if (cube.binary_mask) free_cube(cube.binary_mask);
    if (cube.mv_mask)     free_cube(cube.mv_mask);
    if (cube.fullset)     free_cube(cube.fullset);
    if (cube.emptyset)    free_cube(cube.emptyset);
    for(var = 0; var < cube.num_vars; var++) {
        if (cube.var_mask[var]) {
            free_cube(cube.var_mask[var]);
            cube.var_mask[var] = (pcube)NULL;
        }
    }
    
    if (cube.temp) {
        for(i = 0; i < CUBE_TEMP; i++) {
            if (cube.temp[i]) {
                free_cube(cube.temp[i]);
                cube.temp[i] = (pcube)NULL;
            }
        }
    }
    
    cube.binary_mask = cube.mv_mask  = (pcube) NULL;
    cube.fullset     = cube.emptyset = (pcube) NULL;
    
    return;
}



/**Function********************************************************************
  Synopsis      [Free cubes]
  Description   [Free cubes but not the arrays.]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
void
Cvr_CoverEspressoSetup(
    Vm_VarMap_t *pVm)
{
    int *part_size, nVars, i;
    
    /* derive part_size */
    nVars = Vm_VarMapReadVarsInNum(pVm);
    part_size = ALLOC(int, nVars);
    for (i=0; i<nVars; ++i) {
        part_size[i] = Vm_VarMapReadValues(pVm, i);
    }
    CvrSetupCube(nVars, part_size);
    CvrSetupCdata(&cube);
    FREE( part_size );
    
    return;
}

/**Function********************************************************************
  Synopsis      [Free cubes]
  Description   [Free cubes but not the arrays.]
  SideEffects   []
  SeeAlso       []
******************************************************************************/
void
Cvr_CoverEspressoSetdown(
    Vm_VarMap_t *pVm)
{
    CvrSetdownCube( );
    CvrSetdownCdata( );
    return;
}

/**Function********************************************************************
  Synopsis    [Print out an array of Isets]
  Description [Print out an array of Isets]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
_PsetFamilyArrayPrint(
    FILE        *pFile,
    pset_family *pcovers,
    int          size,
    bool         verbose) 
{
    int k;

    for (k=0; k<size; ++k) {
        fprintf(pFile, "PART[%d] ", k);
        _PsetFamilyPrint(pFile, pcovers[k], verbose);
    }
}


/**Function********************************************************************
  Synopsis    [Print out an iset]
  Description [Print out an iset]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
_PsetFamilyPrint(
    FILE        *pFile,
    pset_family psf,
    bool        verbose)
{
    int i;
    pset p;
    if (psf==NULL) {
        printf("NULL\n"); return;
    }
    fprintf(pFile, "wsize=%d\tsf_size=%d\tcount=%d\n",
            psf->wsize, /*psf->sf_size,*/ cube.size, psf->count);
    
    if (psf->sf_size==0 || psf->count==0) {
        if (psf->count>0) { printf(" ---TAUTOLOGY---\n"); }
        else { printf(" ---EMPTY---\n"); }
        return;
    }
    
    if (verbose) {
        foreachi_set(psf, i, p) {
            _PsetPrint(pFile, p, "01");
        }
    }
    
    return;
}


/**Function********************************************************************
  Synopsis    [Print out a cube]
  Description [Print out a cube]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
_PsetPrint(
    FILE *fp,
    pcube c,
    char *out_map)
{
    register int i, var, ch;


    for(var = 0; var < cube.num_binary_vars; var++) {
	ch = "?01-" [GETINPUT(c, var)];
	putc(ch, fp);
    }
    for(var = cube.num_binary_vars; var < cube.num_vars; var++) {
	putc(' ', fp);
	for(i = cube.first_part[var]; i <= cube.last_part[var]; i++) {
	    ch = "01" [is_in_set(c, i) != 0];
	    putc(ch, fp);
	}
    }
    putc('\n', fp);
}

/**Function********************************************************************
  Synopsis    [reset the default cover of a pset_family array]
  Description [assume all i-sets are available]
  SideEffects []
  SeeAlso     []
******************************************************************************/
void
_PsetFamilyResetDefault( pset_family *pOnset, int nVal ) 
{
    int i, iDef, nCost;
    nCost = -1;
    for ( i=0; i<nVal; ++i ) {
        assert( pOnset[i] );
        if ( pOnset[i]->count > nCost ) {
            iDef = i;
            nCost = pOnset[i]->count;
        }
    }
    sf_free( pOnset[iDef] );
    pOnset[iDef] = NULL;
    return;
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
Cvr_CoverIsTooLarge( Cvr_Cover_t *pCf ) 
{
    int i, nVals, nBits, nCubes;
    
    nVals = Vm_VarMapReadValuesOutNum( pCf->pVm );
    nBits = Vm_VarMapReadValuesInNum( pCf->pVm );
    nCubes = 0;
    for ( i=0; i<nVals; ++i ) {
        if ( pCf->ppCovers[i] )
            nCubes += Mvc_CoverReadCubeNum( pCf->ppCovers[i] );
    }
    
    return ( (nCubes>=100 && nBits>=50) ||
             (nCubes>=200 && nBits>=30) ||
             (nCubes>=300 && nBits>=20) );
}

/**Function********************************************************************
  Synopsis    []
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
bool
Cvr_IsetIsTooLarge( Vm_VarMap_t *pVm, Mvc_Cover_t *pOnset ) 
{
    int nVals, nBits, nCubes;
    
    nVals  = Vm_VarMapReadValuesOutNum( pVm );
    nBits  = Vm_VarMapReadValuesInNum( pVm );
    nCubes = Mvc_CoverReadCubeNum( pOnset );
    
    return ( (nCubes>=50 && nBits>=50) ||
             (nCubes>=100 && nBits>=30) ||
             (nCubes>=200 && nBits>=20) );
}
