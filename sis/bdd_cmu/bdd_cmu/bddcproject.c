/* Written by Gitanjali M. Swamy */
/* 	bddcproject.c,v 1.2 1994/06/02 02:36:30 shiple Exp	 */
/*      bddcproject.c,v
 * Revision 1.2  1994/06/02  02:36:30  shiple
 * Fixed bug in RETURN_BDD use.
 *
 * Revision 1.1  1994/06/02  00:48:50  shiple
 * Initial revision
 *
 * Revision 1.5  1994/05/31  15:19:32  gms
 * May31 Tues
 *                                                            */

#ifndef lint
static char vcid[] = "bddcproject.c,v 1.2 1994/06/02 02:36:30 shiple Exp";
#endif /* lint */

#include "bddint.h"   /* CMU internal routines; for use in bdd_get_node() */

#define OP_CPROJ 5000001

extern bdd cmu_bdd_project();

/* INTERNAL ONLY */

/*
 *    smooth - recursively perform the smoothing
 *
 *    return the result of the reorganization
 */

static
bdd
#if defined(__STDC__)
cmu_bdd_smooth_g_step(cmu_bdd_manager bddm, bdd f, long op, var_assoc vars ,long id)
#else
cmu_bdd_smooth_g_step(bddm, f, op, vars, id)
     cmu_bdd_manager bddm;
     bdd f;
     long op;
     var_assoc vars;
     long id;
#endif
{
    bdd temp1, temp2;
    bdd result;
    int quantifying;

    BDD_SETUP(f);
    if ((long)BDD_INDEX(bddm, f) > vars->last)
        {
            BDD_TEMP_INCREFS(f);
            return (f);
        }
    if (bdd_lookup_in_cache1(bddm, op, f, &result))
        return (result);
    quantifying=(vars->assoc[BDD_INDEXINDEX(f)] != 0);

    temp1=cmu_bdd_smooth_g_step(bddm, BDD_THEN(f), op, vars,id);

    if ((quantifying && temp1 == BDD_ONE(bddm))&&((long)BDD_INDEX(bddm, f) > id ))

        result=temp1;
    else
        {
            temp2=cmu_bdd_smooth_g_step(bddm, BDD_ELSE(f), op, vars,id);
            if (quantifying)
                {
                    BDD_SETUP(temp1);
                    BDD_SETUP(temp2);
                    bddm->op_cache.cache_level++;
                    result=cmu_bdd_ite_step(bddm, temp1, BDD_ONE(bddm), temp2);
                    BDD_TEMP_DECREFS(temp1);
                    BDD_TEMP_DECREFS(temp2);
                    bddm->op_cache.cache_level--;
                }
            else
                result=bdd_find(bddm, BDD_INDEXINDEX(f), temp1, temp2);
        }
    bdd_insert_in_cache1(bddm, op, f, result);

   
    return (result);
}


bdd
#if defined(__STDC__)
cmu_bdd_smooth_g(cmu_bdd_manager bddm, bdd f, long id)
#else
cmu_bdd_smooth_g(bddm, f, id)
     cmu_bdd_manager bddm;
     bdd f;
     long id;
#endif
{
    long op;
    
            if (bddm->curr_assoc_id == -1)
                op=bddm->temp_op--;
            else
                op=OP_QNT+bddm->curr_assoc_id;
            RETURN_BDD(cmu_bdd_smooth_g_step(bddm, f, op, bddm->curr_assoc,id));
  }


/*
 *    project - recursively perform compatible projection
 *
 *    return the result of the reorganization
 */



static
bdd
#if defined(__STDC__)
cmu_bdd_project_step(cmu_bdd_manager bddm, bdd f, long op, var_assoc vars)
#else
cmu_bdd_project_step(bddm, f, op, vars)
     cmu_bdd_manager bddm;
     bdd f;
     long op;
     var_assoc vars;
#endif

{
    bdd temp1, temp2;
    bdd sm, pr;
    bdd result;
    int quantifying;

    BDD_SETUP(f);
    if ((long)BDD_INDEX(bddm, f) > vars->last)
        {
            BDD_TEMP_INCREFS(f);
            return (f);
        }
    if (bdd_lookup_in_cache1(bddm, op, f, &result))
        return (result);
    quantifying=(vars->assoc[BDD_INDEXINDEX(f)] != 0);

    if (quantifying)
        {

            sm  = cmu_bdd_smooth_g(bddm,BDD_THEN(f),(long)BDD_INDEXINDEX(f)); 
            if (sm == BDD_ONE(bddm))
                {
                    pr  = cmu_bdd_project_step(bddm, BDD_THEN(f), op, vars);
                    {
                    BDD_SETUP(pr);
                    result = bdd_find(bddm, BDD_INDEXINDEX(f), pr,BDD_ZERO(bddm));
                   BDD_TEMP_DECREFS(pr);
                    }
                    
                }
      else if (sm == BDD_ZERO(bddm))
                {
                    pr = cmu_bdd_project_step(bddm, BDD_ELSE(f), op, vars);
                    {
                    BDD_SETUP(pr);
                    result = bdd_find(bddm, BDD_INDEXINDEX(f), BDD_ZERO(bddm), pr);
                   BDD_TEMP_DECREFS(pr);
                    }
                    
                }
            else 
                {
                    temp1 = cmu_bdd_project_step(bddm, BDD_THEN(f), op, vars);
                    temp2 = cmu_bdd_project_step(bddm, BDD_ELSE(f),op, vars);
                    {
                    BDD_SETUP(temp1);
                    BDD_SETUP(temp2);
                    pr = cmu_bdd_ite_step(bddm, sm, BDD_ZERO(bddm), temp2);
                    bddm->op_cache.cache_level++;
                    result = bdd_find(bddm, BDD_INDEXINDEX(f), temp1, pr);
                    BDD_TEMP_DECREFS(temp1);
                    BDD_TEMP_DECREFS(temp2);
                    bddm->op_cache.cache_level--;
                    }
            
                }
        }
  
    else
        {
            temp1=cmu_bdd_project_step(bddm, BDD_THEN(f), op, vars);
            temp2=cmu_bdd_project_step(bddm, BDD_ELSE(f), op, vars);
            result=bdd_find(bddm, BDD_INDEXINDEX(f), temp1, temp2);

        }
  
    bdd_insert_in_cache1(bddm, op, f, result);
    return (result);
}

bdd
#if defined(__STDC__)
cmu_bdd_project(cmu_bdd_manager bddm, bdd f)
#else
cmu_bdd_project(bddm, f)
     cmu_bdd_manager bddm;
     bdd f;
#endif
{
    long op;

    if (bdd_check_arguments(1, f))
        {
            FIREWALL(bddm);
            if (bddm->curr_assoc_id == -1)
                op=bddm->temp_op--;
            else
                op=OP_CPROJ+bddm->curr_assoc_id;
            RETURN_BDD(cmu_bdd_project_step(bddm, f, op, bddm->curr_assoc));
        }
    return ((bdd)0);
}











