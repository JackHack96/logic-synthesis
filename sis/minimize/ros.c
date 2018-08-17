
/******************************************************************************
 * This module has been written by Abdul A. Malik                             *
 * Please forward all feedback, comments, bugs etc to:                        *
 *                                                                            *
 * ABDUL A. MALIK                                                             *
 * malik@ic.berkeley.edu                                                      *
 *                                                                            *
 * Department of Electrical Engineering and Computer Sciences                 *
 * University of California at Berkeley                                       *
 * Berkeley, CA 94704.                                                        *
 *                                                                            *
 * Telephone: (415) 642-5048                                                  *
 ******************************************************************************/

/*
 * This file contains routines to find the overexpanded cube and blocking
 * matrix (reduced offset) in the dynamic mode. The dynamic mode is
 * characterized by the fact that the unate tree subtree is not stored 
 * for the cofactor passed to one of these routines. To get to unate cofactors
 * of the subtree, all the necessary cofactors will have to be evaluated.
 */

#include "espresso.h"

#define extern

#include "ros.h"

#undef extern

static bool is_taut;
static bool d1_active;

/*
 * Get the blocking matrix (Reduced Off Set) for p.
 */

void
get_ROS1(p, overexpanded_cube, pNode)
        pcube p, overexpanded_cube;
        pnc_node pNode;

{
    pcover BB;
    long   t;
    pcube  temp;

    if (trace)
        t = ptime();

    temp = new_cube();
    (void) set_or(temp, p, cube.mv_mask);
    Num_act_vars = cube.size - set_ord(temp);
    Num_act_vars += cube.num_vars - cube.num_binary_vars;

    BB  = setupBB(p, overexpanded_cube, pNode, 1);
    ROS = sf_contain(BB);
    free_cube(temp);

    if (trace) {
        Max_ROS_size = MAX(Max_ROS_size, ROS->count);
        ROS_count++;
        ROS_time += ptime() - t;
    }
}

/*
 * Get reduced offset for the ith cube in F, combine it with as many other
 * cubes as possible.
 */

pcover
get_ROS(i, F, Fold)
        int i;
        pcover F, Fold;

{

    int   max_ones, last, j;
    pcube old_cube, overexpanded_cube;

    if (find_correct_ROS(GETSET(F, i))) {
        return (ROS);
    }

    ROS_cube          = new_cube();
    old_cube          = new_cube();
    overexpanded_cube = new_cube();

    max_ones = cube.size - ROS_zeros;
    last     = F->count - 1;

    if (Fold == (pcover) NULL)
        Fold = F;

    /* Form the ROS_cube by multiplying all the cubes in F beginning with
       ith cube until ROS_cube has more ones then max_ones. Form the super-
       cube of the corresponding old cubes in old_cube */
    (void) set_copy(ROS_cube, GETSET(F, i));
    (void) set_copy(old_cube, GETSET(Fold, i));
    for (j = i + 1; j <= last; j++) {
        if (set_ord(ROS_cube) > max_ones) {
            (void) set_and(ROS_cube, ROS_cube, GETSET(F, j));
            (void) set_or(old_cube, old_cube, GETSET(Fold, j));
        } else {
            break;
        }
    }

    /* The overexpanded cube of ROS_cube must contain the old_cube;
       therefore, if old_cube is fullset so is overexpanded_cube. If
       overexpanded_cube equals GETSET(F,i) or GETSET(Fold, i) then
       the overexpanded_cube is itself a prime and ROS is simply
       complement of overexpanded_cube */

    if (!setp_equal(old_cube, cube.fullset)) {
        get_OC(ROS_cube, overexpanded_cube, old_cube);
        if (setp_equal(overexpanded_cube, GETSET(F, i))
            || (setp_equal(overexpanded_cube, GETSET(Fold, i)))) {
            ROS = nc_compl_cube(overexpanded_cube);
            store_ROS();
            return (ROS);
        }
    } else {
        (void) set_copy(overexpanded_cube, cube.fullset);
    }

    get_ROS1(ROS_cube, overexpanded_cube, root);

    if (!setp_equal(overexpanded_cube, cube.fullset))
        ROS = sf_append(ROS, nc_compl_cube(overexpanded_cube));

    store_ROS();

    free_cube(old_cube);
    free_cube(overexpanded_cube);

    return (ROS);

}

/*
 * Initialize parameters used by ROS related subroutines.
 * Most of these variables are declared in minimize.h
 */

void
init_ROS(F, D, maxLevel, nLevel, rosZeros)
        pcover F;
        pcover D;
        int maxLevel;
        int nLevel;
        int rosZeros;

{

    pcube *T;
    long  t;

    ROS       = (pcover) NULL;/* Reduced Offset itself */
    ROS_count = 0;    /* Number of computations of ROS */
    ROS_time  = 0;    /* Aggregate time for computation of ROS */

    ROS_cube = cube.fullset;
    /* Cube for which current ROS was computed */

    Max_ROS_size = 0;    /* Maximum size of ROS computed at any time */

    ROS_zeros = rosZeros;
    /* Maximum number of zeros allowed in ROS_cube.
       This controls the maximum size of ROS returned. */

    ROS_max_store = 2 * (F->count);
    /* Don't store more than ROS_max_store ROSs */

    if (F->count != 0)
        ROS_max_size_store = (int) (ROS_MEM_SIZE / ROS_max_store);
    /* Don't store any ROS that has size more than this */

    N_ROS = 0;        /* Number of ROSs stored so far */

    /* Allocate memory to ROS_array */
    ROS_array = ALLOC(ros_holder_t, ROS_max_store);

    /* Initialize variables to collect statistics about OC */
    OC_count = 0;
    OC_time  = 0;

    /* Assigne memory to temporary cubes. */
    nc_setup_tmp_cubes(33);

    /* Generate unate cofactors of T = (F U D) to use latter for computing
       reduced off sets */
    Tree_cover = sf_save(F);
    Tree_cover = sf_append(Tree_cover, sf_save(D));
    T          = cube1list(Tree_cover);

    /* Max_level is the maximum depth of the static cofactoring tree.
       N_level is the number of levels after which some special filtering is
       done. */
    Max_level = maxLevel;
    N_level   = nLevel;

    /* Generate the cofactoring tree */
    root = ALLOC(nc_node_t, 1);
    if (trace) {
        t = ptime();
    }

    nc_generate_cof_tree(root, T, 1);

    if (trace) {
        t = ptime() - t;
        (void) printf("# COFACTOR TREE\tTime was %s, level=%d\n",
                      print_time(t), maxLevel);
    }

}

/*
 * find_correct_ROS -- Find a ROS that is usable to expand p.
 * If such a ROS is found, set ROS to it and ROS_cube to its cube.
 * return TRUE, otherwise return FALSE.
 */

bool
find_correct_ROS(p)
        pcube p;

{

    int         i;
    pros_holder pros_h;

    /* Check to see if the current ROS is usable. If so don't
       have to do anything */
    if (setp_implies(ROS_cube, p))
        return (TRUE);

    for (i = 0; i < N_ROS; i++) {
        pros_h = ROS_array + i;
        if (setp_implies(pros_h->ros_cube, p)) {
            ROS      = pros_h->ros;
            ROS_cube = pros_h->ros_cube;
            (pros_h->count)++;
            return (TRUE);
        }
    }

    return (FALSE);

}

/*
 * store_ROS -- store current ROS in ROS_array.
 */

void
store_ROS() {

    pros_holder pros_h;
    int         count, min, i;

    /* If the size of ROS is larger than the maximum size allowed for
       storage then don't store */
    if (ROS->count > ROS_max_size_store)
        return;

    if (N_ROS < ROS_max_store) {
        pros_h = ROS_array + N_ROS;
        N_ROS++;
    } else { /* Find a ros with the least count */
        min   = 0;
        count = ROS_array[0].count;
        for (i = 1; i < N_ROS; i++) {
            if (ROS_array[i].count < count) {
                min   = i;
                count = ROS_array[i].count;
            }
        }
        pros_h = ROS_array + min;
        sf_free(pros_h->ros);
        free_cube(pros_h->ros_cube);
    }

    pros_h->ros      = ROS;
    pros_h->ros_cube = ROS_cube;
    pros_h->count    = 1;

}

/*
 * Free all the stuff in ROS_array.
 */

void
close_ROS() {

    int         i;
    pros_holder pros_h;

    /* Print stat about ROS and OC in this run */
    if (trace) {
        (void) printf("# OVEREXPANDED CUBES\tTime was %s, count=%d\n",
                      print_time(OC_time), OC_count);
        (void) printf("# REDUCED OFFSETS\tTime was %s, count=%d, maximum size=%d\n",
                      print_time(ROS_time), ROS_count, Max_ROS_size);
    }

    /* Free the memory used in the unate tree */
    nc_free_unate_tree(root);
    sf_free(Tree_cover);

    /* Free temporary cubes */
    nc_free_tmp_cubes(33);

    /* First free all ros and ros_cubes in the array */
    for (i = 0; i < N_ROS; i++) {
        pros_h = ROS_array + i;
        sf_free(pros_h->ros);
        free_cube(pros_h->ros_cube);
    }

    /* Finally free the array */
    FREE(ROS_array);

}

/*
 * Find the overexpanded cube of cube p.
 *
 * This subroutine is used when the entire unate tree is stored
 * rather than only the leafs.
 */

void
find_overexpanded_cube_dyn(p, overexpanded_cube, T, dist_cube, dist, var, level)
        pcube p;            /* Node to be expanded */
        pcube overexpanded_cube;    /* overexpanded cube so far */
        pcube *T;        /* The current cubelist */
        pcube dist_cube;    /* Node where distance changed from 0 to 1 */
        int dist;        /* Distance so far between cofactoring cube and p */
        int var;        /* Conflicting variable */
        int level;        /* Level in the unate tree */

{
    pcube cl, cr;
    pcube *Tl, *Tr;
    int   best;

    if (find_oc_dyn_special_cases(p, overexpanded_cube, T,
                                  dist_cube, dist, var, level)) {
        return;
    } else {

        cl = new_cube();
        cr = new_cube();

        best = binate_split_select(T, cl, cr, COMPL);

        if (best < cube.num_binary_vars) {
            nc_bin_cof(T, &Tl, &Tr, cl, cr, best);
        } else {
            Tl = scofactor(T, cl, best);
            nc_cof_contain(Tl);
            Tr = scofactor(T, cr, best);
            nc_cof_contain(Tr);
        }

        if (dist == 0) {

            /* Process the left subtree */
            if (cdist0v(cl, p, best))
                find_overexpanded_cube_dyn(p, overexpanded_cube,
                                           Tl, (pcube) NULL, 0, 0, level + 1);
            else if (cdist0v(cl, overexpanded_cube, best)) {
                d1_active = TRUE;
                find_overexpanded_cube_dyn(p, overexpanded_cube,
                                           Tl, cr, 1, best, level + 1);
            }

            /* Process the right subtree */
            if (cdist0v(cr, p, best))
                find_overexpanded_cube_dyn(p, overexpanded_cube,
                                           Tr, (pcube) NULL, 0, 0, level + 1);
            else if (cdist0v(cr, overexpanded_cube, best)) {
                d1_active = TRUE;
                find_overexpanded_cube_dyn(p, overexpanded_cube,
                                           Tr, cl, 1, best, level + 1);
            }

        } else {  /* dist == 1 */

            if (var == best) { /* Split is on the conflicting variable */
                find_overexpanded_cube_dyn(p, overexpanded_cube,
                                           Tl, dist_cube, 1, var, level + 1);
                find_overexpanded_cube_dyn(p, overexpanded_cube,
                                           Tr, dist_cube, 1, var, level + 1);
            } else {
                if (cdist0v(cl, p, best))
                    find_overexpanded_cube_dyn(p, overexpanded_cube,
                                               Tl, dist_cube, 1, var, level + 1);
                if (cdist0v(cr, p, best) && (d1_active))
                    find_overexpanded_cube_dyn(p, overexpanded_cube,
                                               Tr, dist_cube, 1, var, level + 1);
            }
        }

        /* Free left and right cubelists and partition cubes */
        free_cubelist(Tl);
        free_cubelist(Tr);
        free_cube(cl);
        free_cube(cr);

    }

}

/*
 * Special cases for finding overexpanded cube.
 */
bool
find_oc_dyn_special_cases(p, overexpanded_cube, T, dist_cube, dist, var, level)
        pcube p;        /* Node to be expanded */
        pcube overexpanded_cube;/* overexpanded cube so far */
        pcube T[];        /* The current cubelist */
        pcube dist_cube;    /* Node where distance changed from 0 to 1 */
        int dist;        /* Distance so far between cofactoring cube and p */
        int var;        /* Conflicting variable */
        int level;        /* Level in the unate tree */

{
    pcube temp;
    int   k, last_word;
    pcube mask;

    if (dist == 0) {

        /* If all the variables in T are either already lowered in
           the overexpanded cube or not present in p then there is no
           point in processing any cofactors of T */

        temp = set_diff(new_cube(), overexpanded_cube, T[0]);
        if (setp_implies(temp, p)) {
            free_cube(temp);
            return (TRUE);
        }
        free_cube(temp);

    }

    /* Remove unnecessary cubes from the cofactor. These are the cubes
    that don't contain p in their unate variables. */

    if (!(level % N_level)) {
        nc_rem_unnec_cubes(p, T);
    }

    /* Avoid massive count if there is no cube or only one cube in the cover */
    if ((T[2] == NULL) || (T[3] == NULL))
        cdata.vars_unate = cdata.vars_active = 1;

    else massive_count(T);

    /* Check for unate cover */
    if (cdata.vars_active == cdata.vars_unate) { /* T is unate */

        temp = new_cube();

        if (dist == 0) {
            nc_or_unnec(p, T, temp);
            (void) set_and(overexpanded_cube, overexpanded_cube, temp);
        } else { /* dist == 1 */

            if (var < cube.num_binary_vars) {
                /* Conflicting variable is binary variable */
                if (!nc_is_nec(p, T)) {
                    d1_active = FALSE;
                    (void) set_and(overexpanded_cube, overexpanded_cube, dist_cube);
                }
            } else {
                /* Conflicting variable is an mv.
                Lower essential parts in overexpanded cube */
                nc_or_unnec(p, T, temp);
                last_word = cube.last_word[var];
                mask      = cube.var_mask[var];
                for (k = cube.first_word[var]; k <= last_word; k++)
                    overexpanded_cube[k] &= temp[k] | ~mask[k];
            }

        }

        free_cube(temp);

        return (TRUE);

    }

    return (FALSE);

}

/*
 * Generate the blocking matrix for cube p.
 *
 * This routine is similar to setupBB except that it is
 * used when blocking matrix is comupted dynamically for a subtree.
 *
 */

pcover
setupBB_dyn(p, overexpanded_cube, T, level)
        pcube p;
        pcube overexpanded_cube;
        pcube *T;
        int level;

{

    pcover BBl, BBr;
    pcube  temp;
    pcube  cl, cr;
    pcube  *Tl, *Tr, *Temp;
    int    best;
    bool   was_taut;


    if (BB_dyn_special_cases(&BBl, p, overexpanded_cube, T, level)) {
        return (BBl);
    } else {

        cl = new_cube();
        cr = new_cube();

        best = binate_split_select(T, cl, cr, COMPL);

        if (best < cube.num_binary_vars) {

            nc_bin_cof(T, &Tl, &Tr, cl, cr, best);
            free_cubelist(T);

            if (GETINPUT(p, best) == 0) {
                BBl = setupBB_dyn(p, overexpanded_cube, Tl, level + 1);

                /* Use was_taut to temporarily hold is_taut */
                was_taut = is_taut;
                BBr      = setupBB_dyn(p, overexpanded_cube, Tr, level + 1);

                if (was_taut && is_taut) {
                    sf_free(BBr);
                } else {
                    nc_sf_multiply(BBl, cl, best);
                    nc_sf_multiply(BBr, cr, best);
                    BBl     = sf_append(BBl, BBr);
                    is_taut = FALSE;
                }

                free_cube(cl);
                free_cube(cr);
                is_taut = FALSE;
                return (BBl);

            }

            /* Swap left and right sides if cl is not dist0 from p.
               Since both left and right cannot be dist1 from p,
               cl must be dist0 from p after swapping */

            if (!cdist0bv(p, cl, best)) {
                temp = cl;
                cl   = cr;
                cr   = temp;

                Temp = Tl;
                Tl   = Tr;
                Tr   = Temp;
            }

            BBl = setupBB_dyn(p, overexpanded_cube, Tl, level + 1);
            free_cube(cl);

            /* If BBl is a tautology, no need to evaluate BBr */
            if (is_taut) {
                free_cubelist(Tr);
                free_cube(cr);
                return (BBl);
            }

            BBr = setupBB_dyn(p, overexpanded_cube, Tr, level + 1);

            if (cdist0bv(p, cr, best)) {
                if (is_taut) {
                    sf_free(BBl);
                    free_cube(cr);
                    return (BBr);
                } else {
                    BBl = sf_append(BBl, BBr);
                    free_cube(cr);
                    return (BBl);
                }
            } else if (BBr->count == 0) {
                sf_free(BBr);
                free_cube(cr);
                return (BBl);
            } else {
                is_taut = FALSE;
                nc_sf_multiply(BBr, cr, best);
                BBl = sf_append(BBl, BBr);
                free_cube(cr);
                return (BBl);
            }
        } else { /* Splitting variable is multivalued */

            Tl = scofactor(T, cl, best);
            nc_cof_contain(Tl);
            Tr = scofactor(T, cr, best);
            nc_cof_contain(Tr);

            free_cubelist(T);

            BBl = setupBB_dyn(p, overexpanded_cube, Tl, level + 1);
            if (cdist0v(p, cl, best)) {
                free_cube(cl);
                if (is_taut) {
                    free_cube(cr);
                    return (BBl);
                }
            } else if (BBl->count != 0) {
                nc_sf_multiply(BBl, cl, best);
                free_cube(cl);
            }

            BBr = setupBB_dyn(p, overexpanded_cube, Tr, level + 1);
            if (cdist0v(p, cr, best)) {
                free_cube(cr);
                if (is_taut) {
                    sf_free(BBl);
                    return (BBr);
                } else {
                    BBl = sf_append(BBl, BBr);
                    return (BBl);
                }
            } else if (BBr->count == 0) {
                sf_free(BBr);
                free_cube(cr);
                return (BBl);
            } else {
                is_taut = FALSE;
                nc_sf_multiply(BBr, cr, best);
                free_cube(cr);
                BBl = sf_append(BBl, BBr);
                return (BBl);
            }
        }

    }

}

/*
 * Special cases for setting up blocking matrix (reduced offset)
 * This routine uses global variable Num_act_vars declared in 
 * minimize.h
 */
bool
BB_dyn_special_cases(BB, p, overexpanded_cube, T, level)
        pcover *BB;
        pcube p;
        pcube overexpanded_cube;
        pcube *T;
        int level;

{

    pcube temp;

    if (!(level % N_level)) {

        /* If the cofactoring cube is contained in p, the cofactor
           is a tautology. In this case return an empty cover. */
        if (level > Num_act_vars) {
            temp = set_diff(new_cube(), cube.fullset, T[0]);
            if (setp_implies(temp, p)) {
                free_cube(temp);
                is_taut = FALSE;
                *BB = new_cover(0);
                free_cubelist(T);
                return (TRUE);
            }

            free_cube(temp);
        }

        /* Remove unnecessary cubes from the cofactor */
        nc_rem_unnec_cubes(p, T);
    }

    /* Avoid massive count if there is no cube or only one cube in the cover */
    if ((T[2] == NULL) || (T[3] == NULL))
        cdata.vars_unate = cdata.vars_active = 1;
    else
        massive_count(T);

    /* Check for unate cover */
    if (cdata.vars_unate == cdata.vars_active) { /* T is unate */

        /* Remove those cubes from the cover that don't contain p */
        nc_rem_noncov_cubes(p, T);

        if (T[2] == (pcube) NULL) { /* Empty cubelist. Compl is tautology */
            is_taut = TRUE;
            *BB = sf_addset(new_cover(1), cube.fullset);
            free_cubelist(T);
            return (TRUE);
        } else {
            is_taut = FALSE;
        }

        temp = new_cube();
        (void) set_or(T[0], T[0], set_diff(temp, cube.fullset, overexpanded_cube));

        nc_compl_special_cases(T, BB);

        free_cube(temp);
        return (TRUE);

    }

    /* Try to partition T */
    if (level % N_level)
        if (partition_ROS(p, overexpanded_cube, level, T, BB))
            return (TRUE);

    return (FALSE);

}

/*
 * Find the overexpanded cube of cube p.
 *
 * This subroutine is used when the entire unate tree is stored
 * rather than only the leafs.
 */

void
find_overexpanded_cube(p, overexpanded_cube, pNode, dist_cube, dist, var, level)
        pcube p;        /* Node to be expanded */
        pcube overexpanded_cube;/* overexpanded cube so far */
        pnc_node pNode;        /* The current node */
        pcube dist_cube;    /* Node where distance changed from 0 to 1 */
        int dist;        /* Distance so far between cofactoring cube and p */
        int var;        /* Conflicting variable */
        int level;        /* Level from the root of the tree */

{
    int k;

    if (find_oc_special_cases(p, overexpanded_cube, pNode, dist_cube,
                              dist, var, level)) {
        return;
    } else {

        if (dist == 0) {
            for (k = 0; k <= 1; k++) {

                if (cdist0v(pNode->part_cube[k], p, pNode->var))
                    find_overexpanded_cube(p, overexpanded_cube,
                                           pNode->child[k], (pcube) NULL, 0, 0, level + 1);

                else if (cdist0v(pNode->part_cube[k], overexpanded_cube,
                                 pNode->var)) {
                    d1_active = TRUE;
                    find_overexpanded_cube(p, overexpanded_cube,
                                           pNode->child[k], pNode->part_cube[1 - k],
                                           1, pNode->var, level + 1);
                }
            }
        } else {  /* dist == 1 */

            if (var == pNode->var) { /* Split is on the conflicting variable */
                find_overexpanded_cube(p, overexpanded_cube,
                                       pNode->child[LEFT], dist_cube, 1, var, level + 1);
                find_overexpanded_cube(p, overexpanded_cube,
                                       pNode->child[RIGHT], dist_cube, 1, var, level + 1);
            } else { /* Splitting variable is not the same as conflicting var */
                if (cdist0v(pNode->part_cube[LEFT], p, pNode->var))
                    find_overexpanded_cube(p, overexpanded_cube,
                                           pNode->child[LEFT], dist_cube, 1, var, level + 1);

                if ((cdist0v(pNode->part_cube[RIGHT], p, pNode->var))
                    && (d1_active))
                    find_overexpanded_cube(p, overexpanded_cube,
                                           pNode->child[RIGHT], dist_cube, 1, var, level + 1);
            }
        }
    }
}

/*
 * Special cases for finding overexpanded cube.
 */
bool
find_oc_special_cases(p, overexpanded_cube, pNode, dist_cube, dist, var, level)
        pcube p;        /* Node to be expanded */
        pcube overexpanded_cube;/* overexpanded cube so far */
        pnc_node pNode;        /* The current node */
        pcube dist_cube;    /* Node where distance changed from 0 to 1 */
        int dist;        /* Distance so far between cofactoring cube and p */
        int var;        /* Conflicting variable */
        int level;        /* Level from the root of the tree */

{

    pcube temp;
    pcube *T_d;
    pcube mask;
    int   last_word, k;

    /* If all the literals present in p are either in overexpanded cube
       already or have been cofactored out, there is no need to continue */
    if (dist == 0) {
        temp = set_diff(new_cube(), overexpanded_cube, pNode->cof);
        if (setp_implies(temp, p)) {
            free_cube(temp);
            return (TRUE);
        }
        free_cube(temp);
    }

    if (pNode->cubelist != NULL) {
        if (level < Max_level) {  /* A unate leaf has been reached */

            temp = new_cube();

            if (dist == 0) {
                nc_or_unnec(p, pNode->cubelist, temp);
                (void) set_and(overexpanded_cube, overexpanded_cube, temp);
            } else { /* dist == 1 */
                if (var < cube.num_binary_vars) {
                    /* Conflicting variable is binary */

                    if (!nc_is_nec(p, pNode->cubelist)) {
                        (void) set_and(overexpanded_cube,
                                       overexpanded_cube, dist_cube);
                        d1_active = FALSE;
                    }
                } else {
                    /* Conflicting variable is an mv.
                       Lower essential parts in overexpanded cube */

                    nc_or_unnec(p, pNode->cubelist, temp);
                    last_word = cube.last_word[var];
                    mask      = cube.var_mask[var];
                    for (k = cube.first_word[var]; k <= last_word; k++)
                        overexpanded_cube[k] &= temp[k] | ~mask[k];

                }

            }

            free_cube(temp);

            return (TRUE);

        } else {

            /* The leaf is not necessarily unate. Switch to dynamic mode. */

            /* Copy the cubelist because it will be disposed of by
               find_overexpanded_cube_dyn. */

            temp    = set_diff(new_cube(), cube.fullset, pNode->cubelist[0]);
            if (setp_implies(temp, overexpanded_cube))
                T_d = nc_copy_cubelist(pNode->cubelist);
            else {
                T_d = cofactor(pNode->cubelist, overexpanded_cube);
                nc_cof_contain(T_d);
            }
            free_cube(temp);

            if (dist == 0)
                find_overexpanded_cube_dyn(p, overexpanded_cube, T_d,
                                           (pcube) NULL, 0, 0, level + 1);
            else
                find_overexpanded_cube_dyn(p, overexpanded_cube, T_d,
                                           dist_cube, 1, var, level + 1);

            free_cubelist(T_d);
            return (TRUE);

        }
    }

    return (FALSE);
}

/*
 * Generate the blocking matrix for cube p.
 *
 * Compute the Blocking Matrix (reduced offset) recursively.
 */

pcover
setupBB(p, overexpanded_cube, pNode, level)
        pcube p;
        pcube overexpanded_cube;
        pnc_node pNode;
        int level;

{

    pset_family BBl, BBr;
    pcube       cl, cr;
    int         var;

    if (BB_special_cases(&BBl, p, overexpanded_cube, pNode, level)) {
        return (BBl);
    } else { /* Not quite at a leaf yet */

        cl  = pNode->part_cube[LEFT];
        cr  = pNode->part_cube[RIGHT];
        var = pNode->var;

        /* Only those subtrees need to be considered that are not orthogonal
           to the overexpanded_cube */

        if (cdist0v(overexpanded_cube, cl, var)) {

            BBl = setupBB(p, overexpanded_cube, pNode->child[LEFT], level + 1);

            if (cdist0v(p, cl, var)) {
                if (is_taut) return (BBl);
            } else {
                nc_sf_multiply(BBl, cl, var);
            }

            if (cdist0v(overexpanded_cube, cr, var)) {
                BBr = setupBB(p, overexpanded_cube, pNode->child[RIGHT], level + 1);
                if (cdist0v(p, cr, var)) {
                    if (is_taut) {
                        sf_free(BBl);
                        return (BBr);
                    } else {
                        if (BBr->count == 0) {
                            sf_free(BBr);
                            return (BBl);
                        } else {
                            BBl = sf_append(BBl, BBr);
                            return (BBl);
                        }
                    }
                } else {
                    if (BBr->count == 0) {
                        sf_free(BBr);
                        return (BBl);
                    } else {
                        nc_sf_multiply(BBr, cr, var);
                        BBl     = sf_append(BBl, BBr);
                        is_taut = FALSE;
                        return (BBl);
                    }
                }

            } else {
                return (BBl);
            }

        }

            /* Left subtree is distance 1 therefore right subtree must be
               distance 0 from the overexpanded_cube */

        else {
            BBr = setupBB(p, overexpanded_cube, pNode->child[RIGHT], level + 1);
            if ((cdist0v(p, cr, var)) || (BBr->count == 0)) return (BBr);

            is_taut = FALSE;
            nc_sf_multiply(BBr, cr, var);
            return (BBr);
        }

    }

}

/*
 * Special cases for setting up blocking matrix (reduced offset)
 * This routine uses global variable Num_act_vars declared in 
 * minimize.h
 */

bool
BB_special_cases(BB, p, overexpanded_cube, pNode, level)
        pcover *BB;
        pcube p;
        pcube overexpanded_cube;
        pnc_node pNode;
        int level;

{

    pcube temp;
    pcube *T;

    if (!(level % N_level)) {

        /* If the cofactoring cube is contained in p, the cofactor
           is a tautology. In this case return an empty cover. */
        if (level > Num_act_vars) {
            temp = new_cube();
            (void) set_diff(temp, cube.fullset, pNode->cof);
            if (setp_implies(temp, p)) {
                free_cube(temp);
                is_taut = FALSE;
                *BB = new_cover(0);
                return (TRUE);
            }

            free_cube(temp);
        }

    }

    if (pNode->cubelist != NULL) { /* A leaf */

        /* Remove those cubes from the cover that don't contain p */
        if (level < Max_level) { /* A unate leaf */

            T = nc_copy_cubelist(pNode->cubelist);
            nc_rem_noncov_cubes(p, T);

            if (T[2] == (pcube) NULL) { /* Empty cubelist. Compl is tautology */
                is_taut = TRUE;
                *BB = sf_addset(new_cover(1), cube.fullset);
                free_cubelist(T);
                return (TRUE);
            } else {
                is_taut = FALSE;
            }

            temp = new_cube();
            (void) set_or(T[0], T[0], set_diff(temp, cube.fullset, overexpanded_cube));

            nc_compl_special_cases(T, BB);

            free_cube(temp);
            return (TRUE);

        } else { /* The leaf is not necessarily unate. Switch to dynamic mode */

            temp = set_diff(new_cube(), cube.fullset, pNode->cubelist[0]);

            /* If all variables in overexpanded_cube have been cofactored out
               then cofactoring will not gain anything. The test below is
               sufficient because the cofactoring cube is dist 0 from
               overexpanded cube. */

            if (setp_implies(temp, overexpanded_cube)) {
                free_cube(temp);
                T = nc_copy_cubelist(pNode->cubelist);
                *BB = setupBB_dyn(p, overexpanded_cube, T, level + 1);
            } else {
                free_cube(temp);
                T = cofactor(pNode->cubelist, overexpanded_cube);
                *BB = setupBB_dyn(p, overexpanded_cube, T, level + 1);
            }

            return (TRUE);

        }

    }

    return (FALSE);

}

/*
 * Get the overexpanded cube.
 * 
 * This subroutine uses global variable root.
 */

void
get_OC(p, overexpanded_cube, old_cube)
        pcube p, overexpanded_cube, old_cube;

{


    long t;

    if (trace) {
        t = ptime();
    }

    if (old_cube == (pcube) NULL) {
        (void) set_copy(overexpanded_cube, cube.fullset);
        find_overexpanded_cube(p, overexpanded_cube, root,
                               (pcube) NULL, 0, 0, 1);
    } else {

        /* Temporarily remove all the parts from overexpanded cube
           present in the cube before reduce. These parts cannot
           be in the overexpanded cube. They are put in the overexpanded
           cube to speed up its computation */

        (void) set_diff(overexpanded_cube, cube.fullset, old_cube);
        (void) set_or(overexpanded_cube, overexpanded_cube, p);

        find_overexpanded_cube(p, overexpanded_cube, root,
                               (pcube) NULL, 0, 0, 1);

        /* Put back the parts into overexpanded cube that were
           temporarily removed from it */

        (void) set_or(overexpanded_cube, overexpanded_cube, old_cube);

    }

    if (trace) {
        OC_time = ptime() - t;
        OC_count++;
    }

}

/*
 * Partition the cubelist in two parts such that no-nonunate variables are
 * shared. If such a partition exist, find reduced offset for each part
 * and multiply the two reduced offsets.
 */
bool
partition_ROS(p, overexpanded_cube, level, T, R)
        pcube p, overexpanded_cube;
        int level;
        pcube *T;
        pcover *R;

{

    pcube  temp, temp1, cof;
    pcube  q, *T1;
    pcube  *A, *B;
    pcover RA, RB;
    int    i;
    int    temp_int, last;

    /* If the cubelist is empty or has only one cube, don't do anything */
    if ((T[2] == NULL) || (T[3] == NULL))
        return (FALSE);

    temp  = new_cube();
    temp1 = new_cube();
    cof   = new_cube();

    (void) set_copy(temp, cube.fullset);

    /* "And" all the cubes together. */
    for (T1 = T + 2; (q = *T1++) != NULL;)
        (void) set_and(temp, temp, q);

    (void) set_or(temp, temp, T[0]);

    /* Find the non-unate and inactive binary variables. Set their values
       to all 0s in temp1. Set all mvs to all 0s */

    last = cube.last_word[cube.num_binary_vars - 1];
    for (i = 1; i <= last; i++) {
        temp_int = (temp[i] & (temp[i] >> 1)) | (~temp[i] & ((~temp[i]) >> 1));
        temp_int &= DISJOINT;
        temp1[i] = temp[i] | temp_int | (temp_int << 1);
    }
    (void) set_and(temp1, temp1, cube.fullset); /* Some problem with set_ord if I
					    don't do this */
    (void) set_or(temp1, temp1, cube.mv_mask);

    /* Mask out the unate variables by fixing T[0]. */
    (void) set_copy(cof, T[0]); /* Save T[0] in cof */
    (void) set_or(T[0], T[0], set_diff(temp, cube.fullset, temp1));

    /* Partition T. */
    if (!nc_cubelist_partition(T, &A, &B, FALSE)) {
        (void) set_copy(T[0], cof);
        free_cube(temp);
        free_cube(temp1);
        free_cube(cof);
        return (FALSE);
    }

    /* Free T */
    free_cubelist(T);

    /* Restore the cofactoring cube */
    (void) set_copy(A[0], cof);
    (void) set_copy(B[0], cof);

    /* Find reduced offsets for A and B */
    RA = setupBB_dyn(p, overexpanded_cube, A, level);

    if (RA->count == 0) { /* The product of RA and RB will be Null */
        *R = RA;
        free_cubelist(B);
        is_taut = FALSE;
    } else if (is_taut) { /* No need to multiply RA and RB */
        *R = setupBB_dyn(p, overexpanded_cube, B, level);
        sf_free(RA);
    } else {
        RB = setupBB_dyn(p, overexpanded_cube, B, level);
        /* Multiply A and B together */
        *R = multiply2_sf(RA, RB);
        is_taut = FALSE;
    }

    free_cube(temp);
    free_cube(temp1);
    free_cube(cof);
    return (TRUE);

}

/*
 * multiply2_sf -- multiply two set families. Return the result.
 * Assume that no cube in one set family is orthagonal to any one in
 * the other set family.
 */

pcover
multiply2_sf(A, B)
        pcover A, B; /* Disposes of A and B */

{

    pcover C, R;
    pcube  pA, pB, lastA, lastB;

    /* If either A or B is Null, the product is Null */
    if (A->count == 0) {
        sf_free(B);
        return (A);
    }

    if (B->count == 0) {
        sf_free(A);
        return (B);
    }

    A = sf_contain(A);
    B = sf_contain(B);

    /* If B->count is less than A->count, swap A and B */
    if (B->count < A->count) {
        C = A;
        A = B;
        B = C;
    }

    if (A->sf_size != B->sf_size)
        fatal("multiply2_sf: sf_size mismatch");

    R = sf_new(A->count * B->count, A->sf_size);

    /* Multiply them together */

    foreach_set(A, lastA, pA)
    foreach_set(B, lastB, pB)
    set_and(GETSET(R, R->count++), pB, pA);
    R->active_count = A->active_count * B->active_count;

    sf_free(A);
    sf_free(B);

    return (R);

}
/*
 *  cubelist_partition -- take a cubelist T and see if it has any components;
 *  if so, return cubelist's of the two partitions A and B; the return value
 *  is the size of the partition; if not, A and B
 *  are undefined and the return value is 0
 */

int
nc_cubelist_partition(T, A, B, comp_debug)
        pcube *T;            /* a list of cubes */
        pcube **A, **B;            /* cubelist of partition and remainder */
        unsigned int comp_debug;
{
    register pcube *T1, p, seed, cof;
    pcube          *A1, *B1;
    bool           change;
    int            count, numcube;

    numcube = CUBELISTSIZE(T);

    /* Mark all cubes -- covered cubes belong to the partition */
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        RESET(p, COVERED);
    }

    /* Pick a seed that is not a row of all 1s */
    seed    = new_cube();
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        (void) set_or(seed, p, T[0]);
        if (!setp_equal(seed, cube.fullset)) {
            (void) set_copy(seed, p);
            SET(p, COVERED);
            break;
        }
    }

    /*
     *  Extract a partition from the cubelist T; start with the first cube as a
     *  seed, and then pull in all cubes which share a variable with the seed;
     *  iterate until no new cubes are brought into the partition.
     */

    cof   = T[0];
    count = 1;

    do {
        change  = FALSE;
        for (T1 = T + 2; (p = *T1++) != NULL;) {
            if (!TESTP(p, COVERED) && ccommon(p, seed, cof)) {
                INLINEset_and(seed, seed, p);
                SET(p, COVERED);
                change = TRUE;
                count++;
            }

        }
    } while (change);

    set_free(seed);

    if (comp_debug) {
        (void) printf("COMPONENT_REDUCTION: split into %d %d\n",
                      count, numcube - count);
    }

    if (count != numcube) {
        /* Allocate and setup the cubelist's for the two partitions */
        *A = A1 = ALLOC(pcube, numcube + 3);
        *B = B1 = ALLOC(pcube, numcube + 3);
        (*A)[0] = set_save(T[0]);
        (*B)[0] = set_save(T[0]);
        A1 = *A + 2;
        B1 = *B + 2;

        /* Loop over the cubes in T and distribute to A and B */
        for (T1 = T + 2; (p = *T1++) != NULL;) {
            if (TESTP(p, COVERED)) {
                *A1++ = p;
            } else {
                *B1++ = p;
            }
        }

        /* Stuff needed at the end of the cubelist's */
        *A1++ = NULL;
        (*A)[1] = (pcube) A1;
        *B1++ = NULL;
        (*B)[1] = (pcube) B1;
    }

    /* Mark all the cubes non-covered */
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        RESET(p, COVERED);
    }

    return numcube - count;
}

/*
 * This routine recursively generates unate cofactors of T.
 * The cofactors are stored at the leafs.
 */

void
nc_generate_cof_tree(pNode, T, level)
        pnc_node pNode;
        pcube *T;
        int level;

{

    register pcube cl, cr;
    register int   best;
    pcube          *Tl, *Tr;

    /* If the level is not less than Max_level, store the cubelist and leave. */
    if (level == Max_level) {
        pNode->cubelist = T;
        pNode->cof      = T[0];
        return;
    }

    /* Check for empty cubelist */
    if (T[2] == NULL) {
        pNode->cubelist = T;
        pNode->cof      = T[0];
        return;
    }

    /* Check for only a single cube in the cover */
    if (T[3] == NULL) {
        pNode->cubelist = T;
        pNode->cof      = T[0];
        return;
    }

    /* Collect column counts, determine unate variables, etc. */
    massive_count(T);

    /* Check for unate cover */
    if (cdata.vars_unate == cdata.vars_active) { /* T is unate */
        pNode->cubelist = T;
        pNode->cof      = T[0];
        return;
    }

    /* Store the cofactoring cube */
    pNode->cof = set_copy(new_cube(), T[0]);


    /* Allocate space for the partition cubes */
    cl = new_cube();
    cr = new_cube();

    best = binate_split_select(T, cl, cr, COMPL);

    if (best < cube.num_binary_vars) {
        nc_bin_cof(T, &Tl, &Tr, cl, cr, best);
    } else {
        Tl = scofactor(T, cl, best);
        nc_cof_contain(Tl);
        Tr = scofactor(T, cr, best);
        nc_cof_contain(Tr);
    }

    free_cubelist(T);

    /* Save the splitting variable */
    pNode->var = best;

    /* Allocate space for the children of *pNode */
    pNode->child[RIGHT] = ALLOC(nc_node_t, 1);
    pNode->child[LEFT]  = ALLOC(nc_node_t, 1);

    /* Store the partition cubes */
    pNode->part_cube[LEFT]  = cl;
    pNode->part_cube[RIGHT] = cr;

    /* Just to be suer that the cubelist pointer is NULL */
    pNode->cubelist = NULL;

    /* Generate cofactoring tree recursively at the children */
    nc_generate_cof_tree(pNode->child[LEFT], Tl, level + 1);
    nc_generate_cof_tree(pNode->child[RIGHT], Tr, level + 1);

    return;

}

/* 
 * nc_compl_cube -- return the complement of a single cube (De Morgan's law)
 */
pcover
nc_compl_cube(p)
        register pcube p;
{
    register pcube diff = cube.temp[7], pdest, mask, full = cube.fullset;
    int            var;
    pcover         R;

    /* Allocate worst-case size cover (to avoid checking overflow) */
    R = new_cover(cube.num_vars);

    /* Compute bit-wise complement of the cube */
    INLINEset_diff(diff, full, p);

    for (var = 0; var < cube.num_vars; var++) {
        mask = cube.var_mask[var];
        /* If the bit-wise complement is not empty in var ... */
        if (!setp_disjoint(diff, mask)) {
            pdest = GETSET(R, R->count++);
            INLINEset_merge(pdest, diff, full, mask);
        }
    }
    return R;
}

/*
 * Free the unate tree.
 */

void
nc_free_unate_tree(pNode)
        pnc_node pNode;

{

    if (pNode->cubelist == NULL) { /* Not a leaf */

        /* Free the left and right subtrees */
        nc_free_unate_tree(pNode->child[LEFT]);
        nc_free_unate_tree(pNode->child[RIGHT]);

        /* Free all the partition cubes at this node */
        free_cube(pNode->part_cube[LEFT]);
        free_cube(pNode->part_cube[RIGHT]);
        free_cube(pNode->cof);

    } else {  /* A leaf of the tree */

        /* Free the cubelist */
        free_cubelist(pNode->cubelist);

    }

    /* Free the node itself */
    FREE(pNode);

}

/*
 * Cofactor the given cubelist T with respect to the binary
 * variable var.
 *
 * The resulting cubelists should be minimum with respect to single
 * cube containment assuming that T is minimum w.r.t single cube containment.
 */

void
nc_bin_cof(T, Tl, Tr, cleft, cright, var)
        pcube *T, **Tl, **Tr;
        pcube cleft, cright;
        int var;

{

    pcube *T1, *Tc, *Tlc, *Trc, *Tld, *Trd, *Ttmp;
    pcube *Tleft, *Tright;
    pcube p, mask, temp;
    int   size, value;
    int   left, right;

    mask = new_cube();
    temp = new_cube();

    /* Set up the mask */
    (void) set_diff(mask, cube.fullset, T[0]);
    set_remove(mask, 2 * var);
    set_remove(mask, 2 * var + 1);

    /* Setup Tl and Tr */
    size = CUBELISTSIZE(T) + 5;

    Tleft  = *Tl = ALLOC(pcube, size);
    Tright = *Tr = ALLOC(pcube, size);

    Tleft[0] = set_diff(new_cube(), cube.fullset, cleft);
    (void) set_or(Tleft[0], Tleft[0], T[0]);

    Tright[0] = set_diff(new_cube(), cube.fullset, cright);
    (void) set_or(Tright[0], Tright[0], T[0]);

    left  = GETINPUT(cleft, var);
    right = GETINPUT(cright, var);

    /* Set Tc to the location of the first cube that can not be in both lists */
    Tc         = T + 2;
    while (((p = *Tc) != NULL) && (GETINPUT(p, var) == 3)) Tc++;

    /* Make one pass through T to remove cubes which can be in Tleft
       or Tright but not in both */

    Tlc = Tleft + 2;
    Trc = Tright + 2;

    for (T1 = Tc; (p = *T1++) != NULL;) {
        value = GETINPUT(p, var);
        if (value == left) *Tlc++ = p;
        else if (value == right) *Trc++ = p;
        else *Tc++ = p;
    }

    *Tc = NULL;
    T[1] = (pcube)(Tc + 1);
    Tld = Tlc;
    Trd = Trc;

    /* Make one more pass through the remaining cubes in T to copy
       them into Tleft and Tright provided they are not covered by
       any cube that was moved to Tleft or Tright in the first pass */

    for (T1 = T + 2; (p = *T1++) != NULL;) {

        (void) set_and(temp, mask, p);

        for (Ttmp = Tleft + 2; Ttmp < Tld; Ttmp++)
            if (setp_implies(temp, *Ttmp)) goto skipl;

        *Tlc++ = p;

        skipl:

        for (Ttmp = Tright + 2; Ttmp < Trd; Ttmp++)
            if (setp_implies(temp, *Ttmp)) goto skipr;

        *Trc++ = p;

        skipr:;

    }

    /* Wrap up and leave */
    *Tlc++ = NULL;
    *Trc++ = NULL;
    Tleft[1]  = (pcube) Tlc;
    Tright[1] = (pcube) Trc;
    free_cube(mask);
    free_cube(temp);

}

/*
 * Make cofactor T minimum w.r.t single cube containment
 */

void
nc_cof_contain(T)

INOUT       pcube
*
T;

{

register pcube *Tc, *Topen, *Tp, c, p, temp = nc_tmp_cube[23];

for (
Tp = T + 2;
(
p = *Tp++
) !=
NULL;
) {

(void)
set_or(temp, p, T[0]
);

/* Find the first cube contained in temp other than p */
for (
Tc = T + 2;
(
c = *Tc++
) !=
NULL;
)
if (
setp_implies(c, temp
) && (p != c)) break;

/* No cube is contained in p so go to the next cube
   in the cubelist */
if (c == NULL) continue;

Topen = Tc - 1;
while ((
c = *Tc++
) != NULL)
if ( p == c ) {
Tp = Topen;
*Topen++ =
c;
}
else if (!
setp_implies(c, temp
)) *Topen++ =
c;

*Topen++ = (pcube)
NULL;
T[1] = (pcube)
Topen;

}

}

/*
 * Find if the cube p and q are distance zero away from each other
 * in variable var. If so return some non-zero integer, otherwise,
 * return zero.
 */

bool
cdist0v(p, q, var)
        pcube p, q;
        int var;

{

    int k, last_word;
    pcube
    mask;

    if (var < cube.num_binary_vars)
        return ((GETINPUT(p, var)) & (GETINPUT(q, var)));

    last_word = cube.last_word[var];
    mask      = cube.var_mask[var];
    for (k    = cube.first_word[var]; k <= last_word; k++) {
        if (p[k] & q[k] & mask[k])
            return (TRUE);
    }

    return (FALSE);

}

/*
 * Remove the cubes from T that don't contain the cube p. p is the node
 * to be expanded. All cubes in the unate leafs of the cofactors of T
 * that are derived from such cubes will not contain p and will be thrown
 * away if kept. Therefore, removing such cubes may decrease the paths to 
 * unate leafs.
 */

void
nc_rem_unnec_cubes(p, T)
        pcube p;
        pcube *T;

{

    pcube
    temp, temp1;
    pcube
    q, *T1;
    pcube * Tc, *Topen;
    int i, j, is_zero;
    int temp_int, last;

    /* If the cubelist is empty or has only one cube, don't do anything */
    if ((T[2] == NULL) || (T[3] == NULL))
        return;

    temp  = new_cube();
    temp1 = new_cube();

    (void) set_copy(temp, cube.fullset);

    /* "And" all the cubes together. */
    for (T1 = T + 2; (q = *T1++) != NULL;)
        (void) set_and(temp, temp, q);

    (void) set_or(temp, temp, T[0]);

    /* Find the non-unate and inactive variables.
       Set their values to all 0s in temp1 */

    /* Copy the binary part of p to temp1 */
    (void) set_and(temp1, p, cube.binary_mask);

    last = cube.last_word[cube.num_binary_vars - 1];
    for (i = 1; i <= last; i++) {
        temp_int = (temp[i] & (temp[i] >> 1)) | (~temp[i] & ((~temp[i]) >> 1));
        temp_int &= DISJOINT;
        temp1[i] &= ~(temp_int | (temp_int << 1));
    }

    temp1[last] &= cube.binary_mask[last];

    /* insert unate parts in the unate variables */
    for (i = cube.num_binary_vars; i < cube.num_vars; i++) {
        is_zero = 0;
        for (j = cube.first_part[i]; j <= cube.last_part[i]; j++) {
            if (!is_in_set(temp, j)) {
                if (is_zero) goto next_var;
                else is_zero = j + 1;
            }
        }
        if (is_zero) {
            is_zero--;
            if (is_in_set(p, is_zero))
                set_insert(temp1, is_zero);
        }
        next_var:;
    }

    /* If temp1 is contained in temp then for each cube q in the cofactor,
       q contains temp1. In this case no cubes will be removed */
    if (setp_implies(temp1, temp)) {
        free_cube(temp);
        free_cube(temp1);
        return;
    }

    /* Remove each cube q from the cubelist which does not contain temp1 */

    /* Find the first cube q such that q does not contain temp1 */
    for (Tc = T + 2; (q = *Tc++) != NULL;) {
        if (!setp_implies(temp1, q))
            break;
    }

    /* If there is no such cube in the cube list, return */
    if (q == NULL) {
        free_cube(temp);
        free_cube(temp1);
        return;
    }

    Topen     = Tc - 1;
    while ((q = *Tc++) != NULL) {
        if (setp_implies(temp1, q)) *Topen++ = q;
    }

    *Topen++ = (pcube)
    NULL;
    T[1] = (pcube)
    Topen;

    free_cube(temp);
    free_cube(temp1);

}

/*
 * Form or of all the cubes that will not be removed by nc_rem_unnec
 * while expanding c.
 * If All the cubes in T are such that they will be droped by nc_rem_unnec
 * then set orred_cube to fullset.
 */

void
nc_or_unnec(c, T, orred_cube)
        pcube c, *T, orred_cube;

{

    register pcube temp = nc_tmp_cube[26];
    register pcube *T1, p;
    register bool  empty;

    (void) set_copy(orred_cube, T[0]);

    (void) set_diff(temp, cube.fullset, T[0]);
    (void) set_and(temp, temp, c);

    /* Loop for each cube in the list. Determine suitability and or */

    empty   = TRUE;
    for (T1 = T + 2; (p = *T1++) != NULL;)
        if (setp_implies(temp, p)) {
            (void) set_or(orred_cube, orred_cube, p);
            empty = FALSE;
        }

    if (empty)
        (void) set_copy(orred_cube, cube.fullset);
}

/*
 * Find if there is a cube in T that will not be removed by nc_copy_unnec
 * while expanding c. If so, return TRUE, else return FALSE.
 */

bool
nc_is_nec(c, T)
        pcube c, *T;

{

    register pcube *T1, p, temp = nc_tmp_cube[27];

    (void) set_diff(temp, cube.fullset, T[0]);
    (void) set_and(temp, temp, c);

    /* Loop for each cube in the list. Return TRUE if the cube qualifies */

    for (T1 = T + 2; (p = *T1++) != NULL;)
        if (setp_implies(temp, p)) {
            return (TRUE);
        }

    return (FALSE);

}

/*
 * Multiply a set_family by a single variable cube.
 */

void
nc_sf_multiply(A, c, var)
        pset_family A;
        pcube c;
        int var;

{

    pcube
    p, last, temp;
    int fword, lword, i;

    temp = new_cube();
    (void) set_diff(temp, cube.fullset, cube.var_mask[var]);
    (void) set_or(temp, temp, c);

    fword = cube.first_word[var];
    lword = cube.last_word[var];

    foreach_set(A, last, p)
    for (i = fword; i <= lword; i++) p[i] &= temp[i];

    free_cube(temp);

}

/*
 * Remove all the cubes from T that don't contain p.
 */

void
nc_rem_noncov_cubes(p, T)
        pcube p, *T;

{

    pcube
    temp, q;
    pcube * Tc, *Topen;

    temp = new_cube();

    /* Remove each cube q from the cubelist which does not contain p */

    /* Knock out variables that have been cofactored out */
    (void) set_diff(temp, p, T[0]);

    /* Find the first cube q such that q does not contain temp */
    for (Tc = T + 2; (q = *Tc++) != NULL;) {
        if (!setp_implies(temp, q))
            break;
    }

    /* If there is no such cube in the cube list, return */
    if (q == NULL) {
        free_cube(temp);
        return;
    }

    /* Go through the rest of the cubelist, saving those cubes
       that contain temp */
    Topen     = Tc - 1;
    while ((q = *Tc++) != NULL) {
        if (setp_implies(temp, q)) *Topen++ = q;
    }

    *Topen++ = (pcube)
    NULL;
    T[1] = (pcube)
    Topen;

    free_cube(temp);

}

/*
 * nc_compl_special_cases is to be called only when the cubelist
 * T is known to have only one cube or is known to be unate.
 * max_lit is the number of maximum literals allowed in any cube in
 * the complement.
 */
void
nc_compl_special_cases(T, Tbar)
        pcube *T;            /* will be disposed if answer is determined */
        pcover *Tbar;            /* returned only if answer determined */

{
    register pcube *T1, p, ceil, cof = T[0];
    pcover         A, ceil_compl;

    /* Check for no cubes in the cover */
    if (T[2] == NULL) {
        *Tbar = sf_addset(new_cover(1), cube.fullset);
        free_cubelist(T);
        return;
    }

    /* Check for only a single cube in the cover */
    if (T[3] == NULL) {
        *Tbar = nc_compl_cube(set_or(cof, cof, T[2]));
        free_cubelist(T);
        return;
    }

    /* Check for a row of all 1's (implies complement is null) */
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        if (full_row(p, cof)) {
            *Tbar = new_cover(0);
            free_cubelist(T);
            return;
        }
    }

    /* Check for a column of all 0's which can be factored out */
    ceil    = set_save(cof);
    for (T1 = T + 2; (p = *T1++) != NULL;) {
        INLINEset_or(ceil, ceil, p);
    }
    if (!setp_equal(ceil, cube.fullset)) {
        ceil_compl = nc_compl_cube(ceil);
        (void) set_or(cof, cof, set_diff(ceil, cube.fullset, ceil));
        set_free(ceil);
        nc_compl_special_cases(T, Tbar);
        *Tbar = sf_append(*Tbar, ceil_compl);
        return;
    }
    set_free(ceil);

    /* Collect column counts, determine unate variables, etc. */
    massive_count(T);

    /* If single active variable not factored out above, then tautology ! */
    if (cdata.vars_active == 1) {
        *Tbar = new_cover(0);
        free_cubelist(T);
        return;

        /* The cover is unate since all else failed */
    } else {
        A = map_cover_to_unate(T);
        free_cubelist(T);
        A = unate_compl(A);
        *Tbar = map_unate_to_cover(A);
        sf_free(A);
        return;
    }

}

/*
 * Copy cubelist T. Return the copy.
 */
pcube *
nc_copy_cubelist(T)
        pcube *T;

{

    pcube * Tc, *Tc_save;
    register pcube p, *T1;
    int            listlen;

    listlen = CUBELISTSIZE(T) + 5;

    /* Allocate a new list of cube pointers (max size is previous size) */
    Tc_save = Tc = ALLOC(pcube, listlen);

    /* pass on which variables have been cofactored against */
    *Tc++ = set_copy(new_cube(), T[0]);
    Tc++;

    /* Loop for each cube in the list and save */
    for (T1 = T + 2; (p = *T1++) != NULL;) *Tc++ = p;

    *Tc++ = (pcube)
    NULL;                       /* sentinel */
    Tc_save[1] = (pcube)
    Tc;                    /* save pointer to last */
    return Tc_save;
}

/*
 * Assign memory to temporary cubes to be used in subroutines in
 * this file.
 */

void
nc_setup_tmp_cubes(num)
        int num;

{

    int i;

    nc_tmp_cube = ALLOC(pcube, num);

    for (i = 0; i < num; i++)
        nc_tmp_cube[i] = new_cube();
}

/*
 * Free memory assigned to the temporary cubes to be used in subroutines
 * in this file.
 */

void
nc_free_tmp_cubes(num)
        int num;

{

    int i;

    for (i = 0; i < num; i++)
        free_cube(nc_tmp_cube[i]);

    FREE(nc_tmp_cube);
}
