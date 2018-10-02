
/**********************************************************************/
/*           maximal flow algorithm                                   */
/*                                                                    */
/*           Author: Hi-Keung Tony Ma                                 */
/*           last update : 02/26/1988                                 */
/* 	     Packaging and garbage collection: Kanwar Jit Singh       */
/**********************************************************************/

#include "maxflow_int.h"
#include "sis.h"

void maxflow(mfgptr graph, int verify) {
    mfnptr n, ntemp, nscan;
    mfeptr e;
    int    i;
    void   maxflow_init(), augmentation(), construct_cutset();
    void   check(), min_cutset_check();

    maxflow_init(graph);
    graph->source_node->flag |= LABELLED;
    graph->source_node->increment_flow = MAX_FLOW;
    graph->first_label_element         = graph->source_node;
    graph->last_label_element          = graph->source_node;
    nscan = graph->source_node;

    while (nscan) {
        for (i = 0; i < nscan->nout; i++) {
            e = nscan->out_edge[i];
            if (!(e->onode->flag & LABELLED) && (e->flow < e->capacity)) {
                e->onode->flag |= LABELLED;
                e->onode->direction = 1;
                e->onode->path_node = nscan;
                e->onode->path_edge = e;
                e->onode->increment_flow =
                        MIN((e->capacity - e->flow), nscan->increment_flow);
                if (e->onode != graph->sink_node) {
                    graph->last_label_element->pnext = e->onode;
                    graph->last_label_element        = e->onode;
                }
            }
        }
        for (i = 0; i < nscan->nin; i++) {
            e = nscan->in_edge[i];
            if (!(e->inode->flag & LABELLED) && (e->flow > 0)) {
                e->inode->flag |= LABELLED;
                e->inode->direction      = -1;
                e->inode->path_node      = nscan;
                e->inode->path_edge      = e;
                e->inode->increment_flow = MIN(e->flow, nscan->increment_flow);
                if (e->inode != graph->sink_node) {
                    graph->last_label_element->pnext = e->inode;
                    graph->last_label_element        = e->inode;
                }
            }
        }
        nscan = nscan->pnext;

        if (graph->sink_node->flag & LABELLED) {
            augmentation(graph);
            for (n = graph->first_label_element->pnext; n; n = ntemp) {
                ntemp = n->pnext;
                n->pnext = NIL(mf_node_t);
                n->flag &= ~LABELLED;
            }
            graph->first_label_element                       = graph->source_node;
            graph->last_label_element                        = graph->source_node;
            graph->source_node->pnext                        = NIL(mf_node_t);
            nscan = graph->source_node;
        }
    }

    construct_cutset(graph);

    /*mf_display_flow(sisout, graph);*/

    check(graph);
    if (verify)
        min_cutset_check(graph);
} /* end of maxflow */

void augmentation(mfgptr graph) {
    mfnptr n;
    int    increment;

    graph->sink_node->flag &= ~LABELLED;
    n         = graph->sink_node;
    increment = n->increment_flow;
    while (n != graph->source_node) {
        if (n->direction == 1)
            n->path_edge->flow += increment;
        else
            n->path_edge->flow -= increment;
        n = n->path_node;
    }
} /* end of augmentation */

void construct_cutset(mfgptr graph) {
    int i, j;

    for (i = 0; i < graph->num_of_node; i++) {
        if (graph->nlist[i]->flag & LABELLED) {
            for (j = 0; j < graph->nlist[i]->nout; j++) {
                if (!(graph->nlist[i]->out_edge[j]->onode->flag & LABELLED)) {
                    graph->nlist[i]->out_edge[j]->flag |= CUTSET;
                }
            }
        }
    }
} /* end of construct_cutset */

void mf_display_graph(FILE *outfile, mfgptr graph) {
    int    i, j;
    mfnptr n;
    mfeptr e;

    (void) fprintf(outfile, "Source:%-14s Sink:%-14s\n",
                   mf_node_name(mf_get_source_node(graph)),
                   mf_node_name(mf_get_sink_node(graph)));
    (void) fprintf(outfile, "%-14s %-14s %-14s\n", "From", "To", "Capacity");
    mf_foreach_node(graph, i, n) {
        mf_foreach_fanout(n, j, e) {
            (void) fprintf(outfile, "%-14s %-14s %-14d\n", n->name, e->onode->name,
                           e->capacity);
        }
    }
} /* end of mf_display_graph */

void mf_display_cutset(FILE *outfile, mfgptr graph) {
    int       i, j;
    mf_node_t *node;
    mf_edge_t *edge;

    /*(void) fprintf(outfile,"\n");*/
    (void) fprintf(outfile, "Cutset Edges:\n");
    (void) fprintf(outfile, "-------------\n");
    (void) fprintf(outfile, "Input node     Output node    Flow\n");
    mf_foreach_node(graph, i, node) {
        mf_foreach_fanout(node, j, edge) {
            if (edge->flag & CUTSET) {
                (void) fprintf(outfile, "\n");
                (void) fprintf(outfile, "%-14s %-14s %-14d\n", mf_node_name(node),
                               mf_node_name(mf_head_of_edge(edge)),
                               mf_get_edge_capacity(edge));
            }
        }
    }
} /* end of mf_display_cutset */

void mf_display_flow(FILE *outfile, mfgptr graph) {
    int i, j;

    (void) fprintf(outfile, "\n");
    (void) fprintf(outfile, "\n");
    (void) fprintf(outfile, "debugging output flow...\n");
    for (i = 0; i < graph->num_of_node; i++) {
        for (j = 0; j < graph->nlist[i]->nout; j++) {
            (void) fprintf(outfile, "\n");
            (void) fprintf(outfile, "%-9s %-9s %-9d\n", graph->nlist[i]->name,
                           graph->nlist[i]->out_edge[j]->onode->name,
                           graph->nlist[i]->out_edge[j]->flow);
        }
    }
} /* end of mf_display_flow */

/* this routitne checks
 * 1) the conservation of flow;
 * 2) flow is smaller than the capacity;
 * 3) flow in cutset edge equals to capacity;
 * 4) cutset cuts all paths from source to sink
 */
void check(mfgptr graph) {
    int i, j, inflow, outflow;
    void unmark_every_node();
    int trace_path();

    for (i = 0; i < graph->num_of_node; i++) {
        inflow  = 0;
        outflow = 0;
        for (j  = 0; j < graph->nlist[i]->nin; j++) {
            inflow += graph->nlist[i]->in_edge[j]->flow;
        }
        for (j  = 0; j < graph->nlist[i]->nout; j++) {
            outflow += graph->nlist[i]->out_edge[j]->flow;
            if (graph->nlist[i]->out_edge[j]->flow >
                graph->nlist[i]->out_edge[j]->capacity) {
                (void) fprintf(siserr, "\n");
                (void) fprintf(siserr,
                               "Internal Error: flow is larger than the capacity!\n");
                (void) fprintf(siserr, "edge: input node = %s; output node = %s\n",
                               graph->nlist[i]->name,
                               graph->nlist[i]->out_edge[j]->onode->name);
                abort();
            }
            if ((graph->nlist[i]->out_edge[j]->flag & CUTSET) &&
                (graph->nlist[i]->out_edge[j]->flow !=
                 graph->nlist[i]->out_edge[j]->capacity)) {
                (void) fprintf(siserr, "\n");
                (void) fprintf(
                        siserr,
                        "Internal Error: flow in cutset edge is not equal to capacity!\n");
                (void) fprintf(siserr, "edge: input node = %s; output node = %s\n",
                               graph->nlist[i]->name,
                               graph->nlist[i]->out_edge[j]->onode->name);
                abort();
            }
        }
        if ((outflow != inflow) && (graph->nlist[i] != graph->source_node) &&
            (graph->nlist[i] != graph->sink_node)) {
            (void) fprintf(siserr, "\n");
            (void) fprintf(siserr,
                           "Internal Error: Violation of conservation of flow!\n");
            (void) fprintf(siserr, "node: %s\n", graph->nlist[i]->name);
            abort();
        }
    }

    /* check whether the cutset cuts all paths from source to sink */
    unmark_every_node(graph);
    if (trace_path(graph, graph->source_node)) {
        (void) fprintf(siserr, "\n");
        (void) fprintf(siserr, "Internal Error: Cutset unable to cut all the paths "
                               "from source to sink!\n");
        abort();
    }
} /* end of check */

int trace_path(mfgptr graph, mfnptr n) {
    int i;
    int trace;

    if (n == graph->sink_node)
        return (TRUE);
    else {
        trace = FALSE;
        if (!(n->flag & MARKED)) {
            n->flag |= MARKED;
            for (i = 0; i < n->nout; i++) {
                if (!(n->out_edge[i]->flag & CUTSET)) {
                    if (trace_path(graph, n->out_edge[i]->onode))
                        trace = TRUE;
                }
            }
        }
    }
    return (trace);
} /* end of trace_path */

void min_cutset_check(mfgptr graph) {
    int i, j;
    void unmark_every_node();
    int trace_path();

    for (i = 0; i < graph->num_of_node; i++) {
        for (j = 0; j < graph->nlist[i]->nout; j++) {
            if (graph->nlist[i]->out_edge[j]->flag & CUTSET) {
                unmark_every_node(graph);
                graph->nlist[i]->out_edge[j]->flag &= ~CUTSET;
                if (!(trace_path(graph, graph->source_node))) {
                    (void) fprintf(siserr, "\n");
                    (void) fprintf(siserr, "Internal Error: Cutset is not minimum!\n");
                    abort();
                }
                graph->nlist[i]->out_edge[j]->flag |= CUTSET;
            }
        }
    }
} /* end of min_cutset_check */

void unmark_every_node(mfgptr graph) {
    int i;

    for (i = 0; i < graph->num_of_node; i++) {
        graph->nlist[i]->flag &= ~MARKED;
    }
} /* end of unmark_every_node */

void get_cutset(mfcptr cutset) {
    int i, j, k;

    cutset->narcs = 0;
    /* count the number of cutset edges */
    for (i = 0; i < cutset->graph->num_of_node; i++)
        for (j = 0; j < cutset->graph->nlist[i]->nout; j++)
            if (cutset->graph->nlist[i]->out_edge[j]->flag & CUTSET)
                (cutset->narcs)++;

    /* allocate memory */
    if (!(cutset->from_node = MF_ALLOC(cutset->narcs, char *)) && cutset->narcs)
        mf_error("Memory allocation failure", "get_cutset");
    if (!(cutset->to_node   = MF_ALLOC(cutset->narcs, char *)) && cutset->narcs)
        mf_error("Memory allocation failure", "get_cutset");
    if (!(cutset->capacity  = MF_ALLOC(cutset->narcs, int)) && cutset->narcs)
        mf_error("Memory allocation failure", "get_cutset");

    /* fill in the cutset edges info */
    for (i = 0, k = 0; i < cutset->graph->num_of_node; i++) {
        for (j = 0; j < cutset->graph->nlist[i]->nout; j++) {
            if (cutset->graph->nlist[i]->out_edge[j]->flag & CUTSET) {
                if (cutset->graph->nlist[i]->flag & FICTITIOUS)
                    cutset->from_node[k] = cutset->graph->nlist[i]->fname;
                else
                    cutset->from_node[k] = cutset->graph->nlist[i]->name;
                if (cutset->graph->nlist[i]->out_edge[j]->onode->flag & FICTITIOUS)
                    cutset->to_node[k] =
                            cutset->graph->nlist[i]->out_edge[j]->onode->fname;
                else
                    cutset->to_node[k] =
                            cutset->graph->nlist[i]->out_edge[j]->onode->name;
                cutset->capacity[k] = cutset->graph->nlist[i]->out_edge[j]->flow;
                k++;
            }
        }
    }
} /* end of get_cutset */

mf_cutset_t *mf_get_cutset(mf_graph_t *graph, array_t **from_array,
                           array_t **to_array, array_t **flow_array) {
    mf_cutset_t *cut;
    int         i;

    /* Allocate the cutset structure */
    if (!(cut = MF_ALLOC(1, mf_cutset_t)))
        mf_error("Memory allocation failure", "mf_get_cutset");
    cut->graph = graph;

    *from_array = array_alloc(char *, 0);
    *to_array   = array_alloc(char *, 0);
    *flow_array = array_alloc(int, 0);

    get_cutset(cut);

    for (i = 0; i < cut->narcs; i++) {
        array_insert(char *, *from_array, i, (cut->from_node)[i]);
        array_insert(char *, *to_array, i, (cut->to_node)[i]);
        array_insert(int, *flow_array, i, (cut->capacity)[i]);
    }

    return cut;
}

void maxflow_init(mfgptr graph) {
    int    i, j;
    mfnptr n;
    mfeptr e;
    void   fix_loop(), unmark_every_node();

    if (!graph->source_node)
        mf_error("Source node unspecified", "maxflow_init");
    if (!graph->sink_node)
        mf_error("Sink node unspecified", "maxflow_init");
    fix_loop(graph);
    /*
     * Make sure fanin pointers are consistent.
     * Unmark all the nodes and the edges
     */
    mf_foreach_node(graph, i, n) {
        n->flag = 0;
        if (n->in_edge)
            FREE(n->in_edge);
        if (!(n->in_edge = MF_ALLOC(n->nin, mfeptr)) && n->nin)
            mf_error("Memory allocation failure", "maxflow_init");
        n->nin           = 0;
        n->pnext         = NIL(mf_node_t);
    }
    mf_foreach_node(graph, i, n) {
        for (j = 0; j < n->nout; j++) {
            e = n->out_edge[j];
            e->flag = 0;
            e->flow = 0;
            e->onode->in_edge[e->onode->nin] = e;
            e->onode->nin++;
        }
    }
} /* end of maxflow_init */

mfnptr lnode1, lnode2;

void fix_loop(mfgptr graph) {
    int i, no_loop = FALSE;
    void break_loop();

    while (!no_loop) {
        for (i = 0; i < graph->num_of_node; i++) {
            graph->nlist[i]->flag &= ~MARKED;
            graph->nlist[i]->flag &= ~CUR_TRACE;
        }
        graph->source_node->flag |= MARKED;
        graph->source_node->flag |= CUR_TRACE;
        if (check_loop(graph->source_node) == TRUE) {
            /* testing check_loop */
            /*(void) fprintf(siserr,"\nnodes %s and %s are involved in a loop!\n",
            lnode1->name,lnode2->name);

            abort();*/
            break_loop(graph, lnode1, lnode2);
            no_loop = FALSE;
        } else {
            no_loop = TRUE;
        }
        if (no_loop) {
            for (i = 0; i < graph->num_of_node; i++) {
                if (!(graph->nlist[i]->flag & MARKED)) {
                    graph->nlist[i]->flag |= MARKED;
                    graph->nlist[i]->flag |= CUR_TRACE;
                    if (check_loop(graph->nlist[i]) == TRUE) {
                        break_loop(graph, lnode1, lnode2);
                        no_loop = FALSE;
                        break;
                    }
                }
            }
        }
    }
} /* end of fix_loop */

int check_loop(mfnptr node) {
    int    i;
    mfnptr nnode;

    for (i = 0; i < node->nout; i++) {
        nnode = node->out_edge[i]->onode;
        if (!(nnode->flag & MARKED)) {
            nnode->flag |= MARKED;
            nnode->flag |= CUR_TRACE;
            if (check_loop(nnode) == TRUE)
                return (TRUE);
        } else if (nnode->flag & CUR_TRACE) {
            lnode1 = node;
            lnode2 = nnode;
            return (TRUE);
        }
    }
    node->flag &= ~CUR_TRACE;
    return (FALSE);
} /* end of traverse_node */

void break_loop(mfgptr graph, mfnptr node1, mfnptr node2) {
    int    index;
    char   nname1[MF_MAXSTR], nname2[MF_MAXSTR], intstring[5];
    char   *dummy;
    mfnptr n;

    index = 0;
    while (node1->out_edge[index]->onode != node2)
        index++;
    node1->nfict++;
    node2->nfict++;
    mf_itoa(node1->nfict, intstring);
    (void) sprintf(nname1, "%s_fict%s", node1->name, intstring);
    mf_read_node(graph, nname1, -1);
    if (st_lookup(graph->node_table, nname1, &dummy)) {
        n = (mf_node_t *) dummy;
    } else
        mf_error("node not found", "break_loop");
    n->fname = node1->name;
    mf_itoa(node2->nfict, intstring);
    (void) sprintf(nname2, "%s_fict%s", node2->name, intstring);
    mf_read_node(graph, nname2, -1);
    if (st_lookup(graph->node_table, nname2, &dummy)) {
        n = (mf_node_t *) dummy;
    } else
        mf_error("node not found", "break_loop");
    n->fname = node2->name;
    mf_read_edge(graph, graph->source_node->name, nname1, (int) MAX_FLOW);
    mf_read_edge(graph, nname2, graph->sink_node->name, (int) MAX_FLOW);
    mf_read_edge(graph, nname1, node2->name, node1->out_edge[index]->capacity);
    if (st_lookup(graph->node_table, node2->name, &dummy)) {
        n = (mf_node_t *) dummy;
    } else
        mf_error("node not found", "break_loop");
    n->nin--;
    if (st_lookup(graph->node_table, nname2, &dummy)) {
        n = (mf_node_t *) dummy;
    } else
        mf_error("node not found", "break_loop");
    node1->out_edge[index]->onode = n;
    n->nin++;
} /* end of break_loop */

/*
 *   The itoa function
 */

void mf_reverse(char s[]) {
    int c, i, j;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void mf_itoa(int n, char s[]) {
    int i, sign;

    if ((sign = n) < 0) /* record sign */
        n = -n;
    i     = 0;
    do {                     /*  generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0); /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i]       = '\0';
    mf_reverse(s);
} /*   end of itoa  */

int mf_sizeof_cutset(mfgptr graph) {
    int i, j, num_arcs;

    num_arcs = 0;
    /* count the number of cutset edges */
    for (i   = 0; i < graph->num_of_node; i++)
        for (j = 0; j < graph->nlist[i]->nout; j++)
            if (graph->nlist[i]->out_edge[j]->flag & CUTSET)
                num_arcs++;

    return num_arcs;
}

mf_node_t *mf_get_node(mf_graph_t *graph, char *name) {
    char *dummy;

    if (st_lookup(graph->node_table, name, &dummy)) {
        return ((mf_node_t *) dummy);
    } else {
        return (NIL(mf_node_t));
    }
}
