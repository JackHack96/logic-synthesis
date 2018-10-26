
#include "sis.h"
#include "sim_int.h"

static void sim_gen_code();


sim_verify_codegen(network1, network2, num, root)
network_t *network1;
network_t *network2;
int num;
char *root;
{
    FILE *fp, *fpread;
    int i, j, c, nin, nout, nodes1, nodes2, *inmap, *outmap;
    node_t *node1, *node2;
    lsGen gen1, gen2;
    char buffer[1024];

    if (network_num_pi(network1) != network_num_pi(network2)) {
	(void) fprintf(miserr, "Number of inputs do not agree\n");
	return 0;
    }

    if (network_num_po(network1) != network_num_po(network2)) {
	(void) fprintf(miserr, "Number of outputs do not agree\n");
	return 0;
    }

    /* generate the input mapping table */
    inmap = ALLOC(int, network_num_pi(network1));
    i = 0;
    foreach_primary_input(network1, gen1, node1) {
	j = 0;
	foreach_primary_input(network2, gen2, node2) {
	    if (strcmp(node1->name, node2->name) == 0) {
		inmap[i] = j;
		break;
	    }
	    j++;
	}
	if (j == network_num_pi(network2)) {
	    (void) fprintf(miserr, 
		"No match for input '%s' in network2\n", node_name(node1));
	    return 0;
	}
	i++;
    }

    /* generate the output mapping table */
    outmap = ALLOC(int, network_num_po(network1));
    i = 0;
    foreach_primary_output(network1, gen1, node1) {
	j = 0;
	foreach_primary_output(network2, gen2, node2) {
	    if (strcmp(node1->name, node2->name) == 0) {
		outmap[i] = j;
		break;
	    }
	    j++;
	}
	if (j == network_num_po(network2)) {
	    (void) fprintf(miserr, 
		"No match for output '%s' in network2\n", node_name(node1));
	    return 0;
	}
	i++;
    }

    /* open the C source file */
    (void) sprintf(buffer, "%s.c", root);
    fp = com_open_file(buffer, "w", NIL(char *), /* silent */ 0);
    if (fp == 0) return 0;

    /* generate constants */
    nin = network_num_pi(network1);
    nout = network_num_po(network2);
    nodes1 = network_num_internal(network1) + nin + nout + 10;
    nodes2 = network_num_internal(network2) + nin + nout + 10;
    (void) fprintf(fp, "#define nin %d\n", nin);
    (void) fprintf(fp, "#define nout %d\n", nout);
    (void) fprintf(fp, "#define nodes1 %d\n", nodes1);
    (void) fprintf(fp, "#define nodes2 %d\n", nodes2);

    /* generate code for the two networks */
    sim_gen_code(fp, network1, "func1");
    sim_gen_code(fp, network2, "func2");

    /* generate the name for outputs */
    (void) fprintf(fp, "char *output_names[%d] = {\n", nout);
    foreach_primary_output(network1, gen1, node1) {
	(void) fprintf(fp, "    \"%s\",\n", node1->name);
    }
    (void) fprintf(fp, "};\n");

    /* print the input mapping table */
    (void) fprintf(fp, "int input_map[%d] = {\n", nin);
    for(i = 0; i < nin; i++) {
	(void) fprintf(fp, "    %d,\n", inmap[i]);
    }
    (void) fprintf(fp, "};\n");

    /* print the output mapping table */
    (void) fprintf(fp, "int output_map[%d] = {\n", nout);
    for(i = 0; i < nout; i++) {
	(void) fprintf(fp, "    %d,\n", outmap[i]);
    }
    (void) fprintf(fp, "};\n");

    /* copy the standard driver source */
    fpread = com_open_file("driver.c", "r", NIL(char *), /* silent */ 0);
    if (fpread == 0) return 0;
    while ((c = getc(fpread)) != EOF) {
	putc(c, fp);
    }
    (void) fclose(fpread);

    (void) fclose(fp);

    (void) sprintf(buffer, "cc -o %s %s.c", root, root);
    (void) system(buffer);

    (void) sprintf(buffer, "%s %d", root, num / 32 + 1);
    (void) system(buffer);

    FREE(inmap);
    FREE(outmap);
    return 1;
}

static void
sim_gen_code(fp, network, func_name)
FILE *fp;
network_t *network;
char *func_name;
{
    int i, j, k, first_literal, first_cube;
    nodeindex_t *table;
    node_cube_t cube;
    node_literal_t literal;
    node_t *node, *fanin;
    array_t *nodevec;
    lsGen gen;

    table = nodeindex_alloc();
    foreach_primary_input(network, gen, node) {
	(void) nodeindex_insert(table, node);
    }
    foreach_primary_output(network, gen, node) {
	(void) nodeindex_insert(table, node);
    }
    foreach_node(network, gen, node) {
	if (node->type == INTERNAL) {
	    (void) nodeindex_insert(table, node);
	}
    }

    (void) fprintf(fp, "void %s(a)\nregister unsigned *a;\n{\n", func_name);
    nodevec = network_dfs(network);

    for(i = 0; i < array_n(nodevec); i++) {
	node = array_fetch(node_t *, nodevec, i);
	if (node->type == INTERNAL) {
	    (void) fprintf(fp, "    a[%d] = ", nodeindex_indexof(table, node));

	    if (node_function(node) == NODE_0) {
		(void) fprintf(fp, "0");

	    } else if (node_function(node) == NODE_1) {
		(void) fprintf(fp, "(unsigned) -1");

	    } else {
		first_cube = 1;

		for(j = node_num_cube(node)-1; j >= 0; j--) {
		    cube = node_get_cube(node, j);
		    if (! first_cube) (void) fprintf(fp, "|");
		    first_literal = 1;

		    foreach_fanin(node, k, fanin) {
			literal = node_get_literal(cube, k);
			switch(literal) {
			case ONE:
			    if (! first_literal) (void) fprintf(fp, "&");
			    (void) fprintf(fp, "a[%d]", 
				nodeindex_indexof(table, fanin));
			    first_literal = 0;
			    break;
			case ZERO:
			    if (! first_literal) (void) fprintf(fp, "&");
			    (void) fprintf(fp, "~a[%d]", 
				nodeindex_indexof(table, fanin));
			    first_literal = 0;
			    break;
			}
		    }
		    first_cube = 0;
		}
	    }
	    (void) fprintf(fp, ";\n");

	} else if (node->type == PRIMARY_OUTPUT) {
	    fanin = node_get_fanin(node, 0);
	    (void) fprintf(fp, "    a[%d] = a[%d];\n", 
		nodeindex_indexof(table, node),
		nodeindex_indexof(table, fanin));
	}
    }
    array_free(nodevec);
    nodeindex_free(table);
    (void) fprintf(fp, "}\n");
}
