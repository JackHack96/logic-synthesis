
/* file @(#)io.c	1.1                      */
/* last modified on 5/29/91 at 12:35:27   */
#include "../include/genlib_int.h"


void
gl_table_by_level(s_max, p_max)
        int s_max, p_max;
{
    int n, s, p, num;

    (void) printf("                  ");
    for (n = 0; n <= 10; n++) {
        (void) printf(" %7d", n);
    }
    (void) printf("\n");

    for (s = 1; s <= s_max; s++) {
        for (p = 1; p <= p_max; p++) {
            num = gl_number_of_gates(s + p, s, p);
            (void) printf("%d %d (%9d):  ", s, p, num);
            for (n = 0; n <= 10; n++) {
                num = gl_number_of_gates(n, s, p);
                if (n > 0) {
                    num -= gl_number_of_gates(n - 1, s, p);
                }
                if (num > 0) {
                    (void) printf(" %7d", num);
                } else {
                    (void) printf(" %7s", " ");
                }
            }
            (void) printf("\n");
        }
    }
}


void
gl_table_of_gate_count(s_max, p_max) {
    int s, p, level, num;

    (void) printf("     ");
    for (s = 1; s <= s_max; s++) {
        (void) printf(" %9d", s);
    }
    (void) printf("\n");
    for (s = 1; s <= s_max; s++) {
        (void) printf("%d :  ", s);
        for (p = 1; p <= p_max; p++) {
            level = s + p - 2;
            num   = gl_number_of_gates(level, s, p) + gl_number_of_gates(level, p, s) - 1;
            (void) printf(" %9d", num);
        }
        (void) printf("\n");
    }
}


void
gl_table_of_nand_forms(s_max, p_max)
        int s_max, p_max;
{
    int s, p, level, num;

    (void) printf("     ");
    for (s = 1; s <= s_max; s++) {
        (void) printf(" %9d", s);
    }
    (void) printf("\n");
    for (s = 1; s <= s_max; s++) {
        (void) printf("%d :  ", s);
        for (p = 1; p <= p_max; p++) {
            level = s + p - 2;
            num   = gl_number_of_nand_forms(level, s, p);
            (void) printf(" %9d", num);
        }
        (void) printf("\n");
    }
}

int
gl_num_leafs(tree)
        tree_node_t *tree;
{
    int i, sum;

    sum    = 0;
    for (i = 0; i < tree->nsons; i++) {
        sum += gl_num_leafs(tree->sons[i]);
    }
    return sum + (tree->nsons == 0);
}


void
gl_print_all_gates_genlib(level, s, p, invert_output)
        int level, s, p, invert_output;
{
    tree_node_t **list, *tree;
    int         i, nlist;

    if (level < 0) level = s + p;
    nlist = gl_generate_complex_gates(level, s, p, &list);

    for (i = 0; i < nlist; i++) {
        list[i]->phase = !invert_output;
    }

    for (i = 0; i < nlist; i++) {
        tree = list[i];
        (void) printf("GATE \"");
        gl_print_tree(stdout, tree);
        (void) printf("\" %d O=", gl_num_leafs(tree) + 1);
        gl_print_tree_algebraic(stdout, tree);
        (void) printf(";\n");
        (void) printf("PIN * INV 1 999 1.0 0.2 1.0 0.2\n");
        gl_free_tree(tree);
    }
    FREE(list);
}

void
gl_print_all_gates(level, s, p)
        int level, s, p;
{
    tree_node_t **list, *tree;
    int         i, nlist;

    if (level < 0) level = s + p;
    nlist = gl_generate_complex_gates(level, s, p, &list);

    for (i = 0; i < nlist; i++) {
        tree = list[i];
        (void) printf("level=%d s=%d p=%d  ",
                      tree->level, tree->s, tree->p);
        gl_print_tree(stdout, tree);
        (void) printf("\n");
    }
    (void) printf("Total forms = %d\n", nlist);

    for (i = 0; i < nlist; i++) {
        gl_free_tree(list[i]);
    }
    FREE(list);
}


void
gl_print_all_nand_forms(level, s, p, use_nor_gate, invert_output)
        int level, s, p;
        int use_nor_gate, invert_output;
{
    tree_node_t **list, **list1, *tree, *nand_tree;
    int         i, j, nlist, nlist1;

    if (level < 0) level = s + p;
    nlist = gl_generate_complex_gates(level, s, p, &list);

    for (i = 0; i < nlist; i++) {
        tree = list[i];
        tree->phase = !invert_output;
        gl_assign_leaf_names(tree);
        nlist1 = gl_nand_gate_forms(tree, &list1, use_nor_gate);

        for (j = 0; j < nlist1; j++) {
            nand_tree = list1[j];
            gl_assign_node_names(nand_tree);

            (void) printf(".model %s", invert_output ? "!(" : "");
            gl_print_tree(stdout, tree);
            (void) printf("%s\n", invert_output ? ")" : "");

            /*
            (void) printf("# (%d of %d) %s-FORM\n",
                    j+1, nlist1, use_nor_gate ? "NOR" : "NAND");
            */
            gl_write_blif(stdout, nand_tree, use_nor_gate, NIL(latch_info_t));
            (void) printf(".end\n");
        }

        for (j = 0; j < nlist1; j++) {
            gl_free_tree(list1[j]);
        }
        FREE(list1);
    }

    for (i = 0; i < nlist; i++) {
        gl_free_tree(list[i]);
    }
    FREE(list);
}

int
gl_number_of_nand_forms(level, s, p) {
    tree_node_t **list, **list1;
    int         i, j, count, nlist, nlist1;

    if (level < 0) level = s + p;
    nlist = gl_generate_complex_gates(level, s, p, &list);

    count  = 0;
    for (i = 0; i < nlist; i++) {
        nlist1 = gl_nand_gate_forms(list[i], &list1, 0);
        count += nlist1;
        for (j = 0; j < nlist1; j++) {
            gl_free_tree(list1[j]);
        }
        FREE(list1);
    }

    for (i = 0; i < nlist; i++) {
        gl_free_tree(list[i]);
    }
    FREE(list);
    return count;
}

void
gl_print_nand_forms(string, use_nor_gate)
        char *string;
        int use_nor_gate;
{
    int         i, nlist;
    tree_node_t **list, *tree;
    char        *output_name;

    if (!gl_convert_expression_to_tree(string, &tree, &output_name)) {
        (void) fprintf(stderr, "%s", error_string());
        exit(1);
    }
    gl_make_well_formed(tree);

    nlist = gl_nand_gate_forms(tree, &list, use_nor_gate);

    for (i = 0; i < nlist; i++) {
        gl_assign_node_names(list[i]);
        FREE(list[i]->name);
        list[i]->name = util_strsav(output_name);

        (void) printf(".model ");
        gl_print_tree(stdout, tree);
        (void) printf("\n");
        gl_write_blif(stdout, list[i], use_nor_gate, NIL(latch_info_t));
        (void) printf(".end\n");
        gl_free_tree(list[i]);
    }
    gl_free_tree(tree);
}
