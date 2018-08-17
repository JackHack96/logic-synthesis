
#ifdef SIS
#include "sis.h"

extern void read_error();
extern int read_lineno;
extern char *read_filename;

#define GETC(c,fp)	(((((c) = getc(fp)) == '\n') ? read_lineno++ \
                             : 0) , c)
static int
rk_read_line(str,fp)
char *str;
FILE *fp;
{
    int c;

top:
    while (GETC(c,fp) != EOF && isspace(c)) {
    }

    if (c == EOF) {
        return(EOF);
    }
    if (c == '#') {
        while (GETC(c,fp) != EOF && c != '\n') {
    }
    goto top;
    }

    do {
        *str++ = c;
    } while (GETC(c,fp) != EOF && c != '\n');

    --str;
    while (isspace(*str)) {
        --str;
    }

    str[1] = '\0';
    return(!EOF);
}

int
read_kiss(f, g)
FILE *f;
graph_t **g;
{
    int c, index, num;
    int status[4];
    char buf[512],err[512];
    static char *errmsg[] = {"inputs","products","states","outputs"};

    int products,states, num_i, num_o;
    st_table *state_table;
    graph_t *stg;
    char input[512],state[512],next[512],output[512];
    char *temp;
    char start[512];
    vertex_t *instate, *outstate;
    vertex_t *v;
    edge_t *e;
    lsGen gen;
    extern int stg_testing;

    state_table = st_init_table(strcmp,st_strhash);
    stg = stg_alloc();

    status[0] = 0;
    status[1] = 0;
    status[2] = 0;
    status[3] = 0;

    start[0] = '\0';
    while (rk_read_line(buf,f) != EOF) {
        if (*buf != '.') {
        break;
    }
    if (buf[1] == 'r') {
        if (sscanf(buf, "%s %s", err, start) != 2) {
        (void) sprintf(err, "Missing argument to header option .r");
        goto bad_exit;
        }
    } else {
        if (isspace(buf[2])) {
        c = buf[1];
        switch(c) {
          case 'i': index = 0; break;
          case 'p': index = 1; break;
          case 's': index = 2; break;
          case 'o': index = 3; break;
          default: (void) sprintf(err,"Invalid header option .%c",c);
            goto bad_exit;
        }
        if (status[index] != 0) {
            (void) sprintf(err,"Header option .%c specified twice",c);
            goto bad_exit;

        }
        if (sscanf(buf,"%s %d",err,&num) != 2 || num < 1) {
            (void) sprintf(err,"Missing/bad argument to header option .%c",c);
            goto bad_exit;
        }
        status[index] = num;
        }
        else {
        (void) fprintf(miserr, "%s\n", buf);
        continue;
        }
    }
    }

    /* index 1 and 2 are optional */
    if (status[0] == 0) {
    (void) sprintf(err,"%s not specified in header",errmsg[0]);
    goto bad_exit;
    } else if (status[3] == 0) {
    (void) sprintf(err,"%s not specified in header",errmsg[3]);
    goto bad_exit;
    }

    products = states = 0;

    do {
        if(buf[0] == '.'){ 		/* end of state table */
            if(buf[1] != 'e'){
                (void)sprintf(err, "kiss input must end with .end_kiss\n  reading aborted");
                goto bad_exit;
            } else break;
        }

        if (sscanf(buf,"%s %s %s %s %s",input,state,next,output,err) != 4) {
        (void) sprintf(err,"Invalid line: %s",buf);
        goto bad_exit;
    }
    if (!st_lookup(state_table,state,(char **) &instate)) {
        instate = g_add_vertex_static(stg);
        states++;
        temp = util_strsav(state);
        g_set_v_slot_static(instate,STATE_STRING,(gGeneric) temp);
        g_set_v_slot_static(instate,ENCODING_STRING, (gGeneric) 0);
        (void) st_insert(state_table,temp,(char *) instate);
    }
    if (!st_lookup(state_table,next,(char **) &outstate)) {
        outstate = g_add_vertex_static(stg);
        states++;
        temp = util_strsav(next);
        g_set_v_slot_static(outstate,STATE_STRING,(gGeneric) temp);
        g_set_v_slot_static(outstate,ENCODING_STRING,(gGeneric) 0);
        (void) st_insert(state_table,temp,(char *) outstate);
    }
    if (products == 0 && start[0] == '\0') {
        g_set_g_slot_static(stg,START,(gGeneric) instate);
        g_set_g_slot_static(stg,CURRENT,(gGeneric) instate);
        (void) strcpy(start, stg_get_state_name(instate));
    }
    if (products == 0) {
        num_i = strlen(input);
        num_o = strlen(output);
    }
    if (strlen(input) != num_i) {
        (void) sprintf(err,"\nInvalid number of inputs, line: %s",buf);
        goto bad_exit;
    }
    if (strlen(output) != num_o) {
        (void) sprintf(err,"\nInvalid number of outputs, line: %s",buf);
        goto bad_exit;
    }
    e = g_add_edge_static(instate,outstate);
    g_set_e_slot_static(e,INPUT_STRING,(gGeneric) util_strsav(input));
    g_set_e_slot_static(e,OUTPUT_STRING,(gGeneric) util_strsav(output));
    products++;
    } while (rk_read_line(buf,f) != EOF);

    if (status[0] != num_i) {
    (void) fprintf(sisout, "Number of inputs given is not correct.\n");
    (void) fprintf(sisout, ".i line ignored\n");
    }
    if (status[3] != num_o) {
    (void) fprintf(sisout, "Number of outputs given is not correct.\n");
    (void) fprintf(sisout, ".o line ignored\n");
    }
    g_set_g_slot_static(stg,NUM_INPUTS,(gGeneric) num_i);
    g_set_g_slot_static(stg,NUM_OUTPUTS,(gGeneric) num_o);
    g_set_g_slot_static(stg,NUM_STATES,(gGeneric) states);
    g_set_g_slot_static(stg,NUM_PRODUCTS,(gGeneric) products);

    if (start[0] != '\0') {
    if (!strcmp(start, "ANY") || !strcmp(start, "*")) {
        stg_foreach_state(stg, gen, v) {
        temp = stg_get_state_name(v);
        if ((strcmp(temp, "ANY")) && (strcmp(temp, "*"))) {
            g_set_g_slot_static(stg, START, (gGeneric) v);
            g_set_g_slot_static(stg, CURRENT, (gGeneric) v);
            lsFinish(gen);
            break;
        }
        }
    } else {
        if ((v = stg_get_state_by_name(stg, start)) == NIL(vertex_t)) {
            (void) sprintf(err, "Start state given is not a valid state");
            goto bad_exit;
        }
        g_set_g_slot_static(stg, START, (gGeneric) v);
        g_set_g_slot_static(stg, CURRENT, (gGeneric) v);
    }
    }

    if (!stg_check(stg)) {
    (void) sprintf(err, "STG failed stg_check\n");
    goto bad_exit;
    }

    st_free_table(state_table);

    *g = stg;
    return 1;

bad_exit:
    read_error(err);
    st_free_table(state_table);
    stg_free(stg);
    stg = NIL(graph_t);
    *g = NIL(graph_t);
    return 0;
}
#endif /* SIS */
