
#ifdef SIS
#include "sis.h"
#include "retime_int.h"

static int greater();
static void sum();

#define IS_POS_LARGE 5000

void
re_computeWD(graph, N, table, pWD)
re_graph *graph;	/* Graph under consideration */
int N;			/* Number of rows and cols -- also index of host node */
st_table *table;	/* The correspondence between the variables and nodes */
wd_t ***pWD;		/* Returned array of W and D */
{
    double t;
    wd_t **WD;
    re_edge *edge;
    re_node *sink;
    int i, j, k, index, delay, offset;

    WD = ALLOC(wd_t *, N);
    for (i = N; i-- > 0; ){
        WD[i] = ALLOC(wd_t, N);
    }

    for (i = N; i-- > 0; ){
    for (j = N; j-- > 0; ){
        WD[i][j].w = POS_LARGE;
        WD[i][j].d = 0;
        }
    }

    re_foreach_edge(graph, index, edge){
        i = edge->source->lp_index;
        j = edge->sink->lp_index;
    /*
     * The weight corresponds to the largest weight between the nodes
     */
    if (WD[i][j].w != POS_LARGE){
        WD[i][j].w = MIN(WD[i][j].w, edge->weight);
    } else {
        WD[i][j].w = edge->weight;
    }
    /* Check to see if the input has a user specified arrival time */
    offset = 0;
    if (edge->source->type == RE_PRIMARY_INPUT &&
        edge->source->user_time > RETIME_TEST_NOT_SET){
        WD[i][j].d = MIN(WD[i][j].d,
        offset = edge->source->scaled_user_time);
    }
    delay = 0 - (edge->source->scaled_delay + offset);
    WD[i][j].d = MIN(WD[i][j].d, delay);
    }

    for (k = N; k-- > 0; ){
    for (i = N; i-- > 0; ){
        for (j = N; j-- > 0; ){
                if (greater(WD[i][j], WD[i][k], WD[k][j]))
            sum(&(WD[i][j]), WD[i][k], WD[k][j]);
            }
        }
    }

    for (i = N; i-- > 0; ){
    for (j = N; j-- > 0; ){
        if (j >= N-2){ /* Host vertex */
        t = 0;
        } else {
        assert(st_lookup(table, (char *)j, (char **)&sink));
        t = sink->scaled_delay;
        }
        WD[i][j].d = t - WD[i][j].d;
        }
    }

    if (retime_debug > 40){
    (void)fprintf(sisout,"Correspondence between lp_index & nodes\n");
    for (i = N-2; i-- > 0; ){
        assert(st_lookup(table, (char *)i, (char **)&sink));
        if (sink->type == RE_IGNORE){
        (void)fprintf(sisout,"Index %d is REG_SHAR_NODE\n", i);
        } else {
        (void)fprintf(sisout,"Index %d is node %s\n", i, sink->node->name);
        }
    }
    if (retime_debug > 60){
        (void)fprintf(sisout,"Index %d is HOST_NODE_SOURCE\n", N-2);
        (void)fprintf(sisout,"Index %d is HOST_NODE_SINK\n", N-1);
        (void)fprintf(sisout,"Data on the WD values\n");
        for (i = N; i-- > 0; ){
        (void)fprintf(sisout,"%d::",i);
        for (j = N; j-- > 0; ){
            if (WD[i][j].w > IS_POS_LARGE ){
            (void)fprintf(sisout," INFIN");
            } else {
            (void)fprintf(sisout," %d-%4d", WD[i][j].w, WD[i][j].d);
            }
        }
        (void)fprintf(sisout,"\n");
        }
    }
    }

    *pWD = WD;
}

static int
greater(WD1, WD2, WD3)
wd_t WD1, WD2, WD3;
{
    if(WD1.w > (WD2.w + WD3.w)) return 1;

    if(WD1.w == (WD2.w + WD3.w))
        if(WD1.d > (WD2.d + WD3.d)) return 1;

    return 0;
}

static void
sum(pWD1, WD2, WD3)
wd_t *pWD1, WD2, WD3;
{
    pWD1->w = WD2.w + WD3.w;
    pWD1->d = WD2.d + WD3.d;
}
#endif /* SIS */
