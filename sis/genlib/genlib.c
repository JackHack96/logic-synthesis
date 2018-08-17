
/* file @(#)genlib.c	1.1                      */
/* last modified on 5/29/91 at 12:35:22   */
#include "genlib_int.h"

static int check_internal_phase();

static int set_functions();


static int
leaf_compare(t1, t2)
        char **t1, **t2;
{
    tree_node_t *tree1 = *(tree_node_t **) t1;
    tree_node_t *tree2 = *(tree_node_t **) t2;

    return strcmp(tree1->name, tree2->name);
}



int
gl_convert_gate_to_blif(name, area, function, pins, use_nor)
        char *name;
        double area;
        function_t *function;
        pin_info_t *pins;
        int use_nor;
{
    pin_info_t  *pin, *pinnext;
    tree_node_t **list, **leafs;
    int         i, j, nlist, nleafs;
    FILE        *fpout = gl_out_file;

    if (!check_internal_phase(function->tree, 0)) {
        return 0;
    }

    if (function->tree->type == ZERO_NODE || function->tree->type == ONE_NODE) {
        /* special case for constants */
        nlist = 1;
        list  = ALLOC(tree_node_t * , 1);
        list[0] = gl_dup_tree(function->tree);

    } else {
        /* make the tree strictly alternate AND/OR */
        gl_make_well_formed(function->tree);

        /* generate all nand or nor gate forms */
        nlist = gl_nand_gate_forms(function->tree, &list, use_nor);

        /* set all tree nodes to a specific type */
        for (i = 0; i < nlist; i++) {
            set_functions(list[i], use_nor);
        }
    }

    /* get the leaf names */
    nleafs = gl_get_unique_leaf_pointers(function->tree, &leafs);
    qsort((char *) leafs, nleafs, sizeof(tree_node_t *), leaf_compare);

    for (i = 0; i < nlist; i++) {
        gl_assign_node_names(list[i]);
        FREE(list[i]->name);
        list[i]->name = util_strsav(function->name);

        (void) fprintf(fpout, "# (%d of %d) %s-FORM of ",
                       i + 1, nlist, use_nor ? "NOR" : "NAND");
        gl_print_tree(fpout, function->tree);
        (void) fprintf(fpout, "\n");
        (void) fprintf(fpout, ".model %s\n", name);

        gl_write_blif(fpout, list[i], use_nor, NIL(latch_info_t));

        (void) fprintf(fpout, ".area %3.2f\n", area);

        for (pin = pins; pin != 0; pin = pin->next) {
            if (strcmp(pin->name, "*") == 0) {
                if (pin != pins || pin->next != 0) { /* only 1 allowed */
                    read_error("improper use of pin wildcard '*'");
                    return 0;
                }
                for (j = 0; j < nleafs; j++) {
                    (void) fprintf(fpout,
                                   ".delay %s %s %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f\n",
                                   leafs[j]->name, pin->phase, pin->value[0],
                                   pin->value[1], pin->value[2], pin->value[3],
                                   pin->value[4], pin->value[5]);
                }
            } else {
                for (j = 0; j < nleafs; j++) {
                    if (strcmp(pin->name, leafs[j]->name) == 0) {
                        break;
                    }
                }
                if (j == nleafs) {
                    read_error("pin (in delay info) not found");
                    return 0;
                }
                (void) fprintf(fpout,
                               ".delay %s %s %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f\n",
                               pin->name, pin->phase, pin->value[0],
                               pin->value[1], pin->value[2], pin->value[3],
                               pin->value[4], pin->value[5]);
            }
        }

        (void) fprintf(fpout, ".end\n\n");

        gl_free_tree(list[i]);
    }
    FREE(list);
    FREE(leafs);

    /* free memory used */
    for (pin = pins; pin != 0; pin = pinnext) {
        pinnext = pin->next;
        FREE(pin->name);
        FREE(pin->phase);
        FREE(pin);
    }
    gl_free_tree(function->tree);
    FREE(function->name);
    FREE(function);
    FREE(name);
    return 1;
}

/* sequential support */
int
gl_convert_latch_to_blif(name, area, function, pins, use_nor, latch, clock, constraints)
        char *name;
        double area;
        function_t *function;
        pin_info_t *pins;
        int use_nor;
        latch_info_t *latch;            /* sequential support */
        pin_info_t *clock;              /* sequential support */
        constraint_info_t *constraints; /* sequential support */
{
    pin_info_t        *pin, *pinnext;
    tree_node_t       **list, **leafs;
    int               i, j, nlist, nleafs;
    FILE              *fpout = gl_out_file;
    constraint_info_t *constr, *constrnext;

    if (!check_internal_phase(function->tree, 0)) {
        return 0;
    }

    if (function->tree->type == ZERO_NODE || function->tree->type == ONE_NODE) {
        /* special case for constants */
        nlist = 1;
        list  = ALLOC(tree_node_t * , 1);
        list[0] = gl_dup_tree(function->tree);

    } else {
        /* make the tree strictly alternate AND/OR */
        gl_make_well_formed(function->tree);

        /* generate all nand or nor gate forms */
        nlist = gl_nand_gate_forms(function->tree, &list, use_nor);

        /* set all tree nodes to a specific type */
        for (i = 0; i < nlist; i++) {
            set_functions(list[i], use_nor);
        }
    }

    /* get the leaf names */
    nleafs = gl_get_unique_leaf_pointers(function->tree, &leafs);
    qsort((char *) leafs, nleafs, sizeof(tree_node_t *), leaf_compare);

    for (i = 0; i < nlist; i++) {
        gl_assign_node_names(list[i]);
        FREE(list[i]->name);
        list[i]->name = util_strsav(function->name);

        (void) fprintf(fpout, "# (%d of %d) %s-FORM of ",
                       i + 1, nlist, use_nor ? "NOR" : "NAND");
        gl_print_tree(fpout, function->tree);
        (void) fprintf(fpout, "\n");
        (void) fprintf(fpout, ".model %s\n", name);

        gl_write_blif(fpout, list[i], use_nor, latch);

        (void) fprintf(fpout, ".area %3.2f\n", area);

        for (pin = pins; pin != 0; pin = pin->next) {
            if (strcmp(pin->name, "*") == 0) {
                if (pin != pins || pin->next != 0) { /* only 1 allowed */
                    read_error("improper use of pin wildcard '*'");
                    return 0;
                }
                for (j = 0; j < nleafs; j++) {
                    /* sequential support */
                    /* don't print delay info for latch output */
                    if (strcmp(leafs[j]->name, latch->out) == 0) continue;
                    (void) fprintf(fpout,
                                   ".delay %s %s %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f\n",
                                   leafs[j]->name, pin->phase, pin->value[0],
                                   pin->value[1], pin->value[2], pin->value[3],
                                   pin->value[4], pin->value[5]);
                }
            } else {
                for (j = 0; j < nleafs; j++) {
                    if (strcmp(pin->name, leafs[j]->name) == 0) {
                        break;
                    }
                }
                if (j == nleafs) {
                    read_error("pin (in delay info) not found");
                    return 0;
                }
                /* sequential support */
                /* don't print delay info for latch output */
                if (strcmp(pin->name, latch->out) != 0) {
                    (void) fprintf(fpout,
                                   ".delay %s %s %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f\n",
                                   pin->name, pin->phase, pin->value[0],
                                   pin->value[1], pin->value[2], pin->value[3],
                                   pin->value[4], pin->value[5]);
                }
            }
        }

        /* sequential support */
        /* take care of the clock signal */
        if (clock != NIL(pin_info_t)) {
            (void) fprintf(fpout, ".clock %s\n", clock->name);
            (void) fprintf(fpout,
                           ".delay %s %s %4.3f %4.3f %4.3f %4.3f %4.3f %4.3f\n",
                           clock->name, clock->phase, clock->value[0],
                           clock->value[1], clock->value[2], clock->value[3],
                           clock->value[4], clock->value[5]);
        } else {
            if (latch != NIL(latch_info_t) && strcmp(latch->type, "as") != 0) {
                read_error("no clock delay info found for synchronous latch");
                return 0;
            }
        }
        /* take care of setup/hold times */
        if (constraints != NIL(constraint_info_t)) {
            for (constr = constraints; constr != 0; constr = constr->next) {
                if (strcmp(constr->name, "*") == 0) {
                    if (constr != constraints || constr->next != 0) { /* only 1 allowed */
                        read_error("improper use of pin wildcard '*'");
                        return 0;
                    }
                    for (j = 0; j < nleafs; j++) {
                        /* sequential support */
                        /* don't print delay info for latch output */
                        if (strcmp(leafs[j]->name, latch->out) == 0) continue;
                        (void) fprintf(fpout,
                                       ".input_arrival %s %4.3f %4.3f\n",
                                       leafs[j]->name, constr->setup, constr->hold);
                    }
                } else {
                    for (j = 0; j < nleafs; j++) {
                        if (strcmp(constr->name, leafs[j]->name) == 0) {
                            break;
                        }
                    }
                    if (j == nleafs) {
                        read_error("constr (in delay info) not found");
                        return 0;
                    }
                    if (strcmp(constr->name, latch->out) != 0) {
                        (void) fprintf(fpout,
                                       ".input_arrival %s %4.3f %4.3f\n",
                                       constr->name, constr->setup, constr->hold);
                    }
                }
            }
        } else {
            /* no setup/hold times are specified; use 0.0 */
            for (j = 0; j < nleafs; j++) {
                /* don't print delay info for latch output */
                if (strcmp(leafs[j]->name, latch->out) == 0) continue;
                (void) fprintf(fpout, ".input_arrival %s 0.0 0.0\n", leafs[j]->name);
            }
        }

        /* take care of latch connections */
        /* .latch requires input output type clock initial-value */
        /* use 0 for initial-value; it's not used anyway */
        (void) fprintf(fpout, ".latch %s %s %s %s 0\n", latch->in, latch->out, latch->type,
                       clock == NIL(pin_info_t) ? "NIL" : clock->name);

        (void) fprintf(fpout, ".end\n\n");

        gl_free_tree(list[i]);
    }
    FREE(list);
    FREE(leafs);

    /* free memory used */
    for (pin = pins; pin != 0; pin = pinnext) {
        pinnext = pin->next;
        FREE(pin->name);
        FREE(pin->phase);
        FREE(pin);
    }
    gl_free_tree(function->tree);
    FREE(function->name);
    FREE(function);
    FREE(name);
    /* sequential support */
    if (clock != NIL(pin_info_t)) {
        FREE(clock->name);
        FREE(clock->phase);
        FREE(clock);
    }
    for (constr = constraints; constr != 0; constr = constrnext) {
        constrnext = constr->next;
        FREE(constr->name);
        FREE(constr);
    }
    FREE(latch->in);
    FREE(latch->out);
    FREE(latch->type);
    FREE(latch);
    return 1;
}

static int
check_internal_phase(tree, level)
        tree_node_t *tree;
        int level;
{
    int i;

    if (tree->nsons != 0) {
        for (i = 0; i < tree->nsons; i++) {
            if (!check_internal_phase(tree->sons[i], level + 1)) {
                return 0;    /* error in some child */
            }
        }
        /* if (level > 0 && tree->phase != 1) {
         *   read_error("inversion allowed only at root and leaves of function");
         *   return 0;
         * }
         */
        if (level > 0 && (tree->type == ZERO_NODE || tree->type == ONE_NODE)) {
            read_error("0 and 1 are allowed only at the root of a function");
            return 0;
        }

    }
    return 1;
}


static int
set_functions(tree, use_nor)
        tree_node_t *tree;
        int use_nor;
{
    int i;

    for (i = 0; i < tree->nsons; i++) {
        set_functions(tree->sons[i], use_nor);
    }
    tree->type = use_nor ? NOR_NODE : NAND_NODE;
}
