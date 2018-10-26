
#ifdef SIS
/**********************************************************************/
/*           This is the sequential circuit enumeration source file.  */
/*           Parameters needed: Starting State;                       */
/*           Author: Hi-keung Tony Ma                                 */
/*           last update : 02/19/1989                                 */
/**********************************************************************/

#include "sis.h"
#include "stg_int.h"

/*
 * Procedures to ease the task of storing vectors of integers as
 * packed unsigned integers... Needed when number of latches exceeds 32 !!!!
 */
#define STG_CHUNKSIZE 1000
static unsigned **stg_hash_space;
static int stg_num_chunks;
static int stg_position_in_chunk;
static int stg_sizeof_chunk;
static st_table *stg_storelist;
static char *instring, *outstring;	/* For the save_edge() routine */

/* Compare function for the hashing of sttesa represented as packed bits */
int
stg_statecmp(key1, key2)
char *key1, *key2;
{
    int equal_parts, i;
    unsigned *st1 = (unsigned *)key1;
    unsigned *st2 = (unsigned *)key2;

    equal_parts = 0;
    for ( i = stg_longs_per_state; (equal_parts == 0) && (i-- > 0); ){
	if (st1[i] > st2[i]) equal_parts = 1;
	else if (st1[i] < st2[i]) equal_parts = -1;
    }
    return equal_parts;
}

/*
 * Hashing the packed state bits into a number : We will only worry about
 * a good hashing function that discriminates the 32 LSBits
 */
int
stg_statehash(key, modulus)
char *key;
int modulus;
{
    unsigned *state = (unsigned *)key;

    return (ABS(state[0]% modulus));
}

/*
void
stg_print_hashed_state(fp, s)
FILE *fp;
unsigned *s;
{
    int i;
    for (i = stg_longs_per_state; i-- > 0;){
	fprintf(fp, "%x", s[i]);
    }
}
*/

void
stg_init_state_hash()
{
    stg_num_chunks = 1;
    stg_sizeof_chunk = stg_longs_per_state * STG_CHUNKSIZE;
    stg_position_in_chunk = 0;
    stg_hash_space = SENUM_ALLOC(unsigned *, stg_num_chunks);
    stg_hash_space[0] = SENUM_ALLOC(unsigned, stg_sizeof_chunk );
    stg_storelist = st_init_table(stg_statecmp, stg_statehash);

    instring = ALLOC(char, nlatch+1);
    outstring = ALLOC(char, nlatch+1);
}

static void
stg_realloc_state_hash()
{
    stg_hash_space = REALLOC(unsigned *, stg_hash_space, stg_num_chunks+1);
    stg_hash_space[stg_num_chunks++] = SENUM_ALLOC(unsigned, stg_sizeof_chunk);
    stg_position_in_chunk = 0;
}

unsigned *
stg_get_state_hash()
{
    unsigned *block;

    if (stg_position_in_chunk == STG_CHUNKSIZE){
	stg_realloc_state_hash();
    }
    block = stg_hash_space[stg_num_chunks-1];
    block += stg_longs_per_state * stg_position_in_chunk;
    stg_position_in_chunk++;
    return block;
}

void
stg_end_state_hash()
{
    int i;
    for ( i = 0; i < stg_num_chunks; i++){
	FREE(stg_hash_space[i]);
    }
    FREE(stg_hash_space);
    st_free_table(stg_storelist);
    FREE(instring);
    FREE(outstring);
}

void
stg_translate_hashed_code(h_state, stg_state)
unsigned *h_state;
int *stg_state;
{
    int i, j, k;
    unsigned compact_state;

    for (i = 0, k = 0; i < stg_longs_per_state; i++){
	compact_state = h_state[i];
	for (j = 0; (j < stg_bits_per_long) && (k < nlatch); j++){
	   stg_state[k++] = compact_state & 1;
	   compact_state >>= 1;
	}
    }
}


static int
is_state(cid,obj,level)
int cid;
ndata **obj;
int *level;
{
    node_t *node;
    ndata *n;
    lsGen gen;

    foreach_primary_output (copy,gen,node) {
        n = nptr(node_get_fanin(node,0));
        if (n->value[cid] == 2) {
	    *obj = n;
	    *level = random() & 01;
	    (void) lsFinish(gen);
	    return(FALSE);
	}
    }
    return(TRUE);
}

static ndata *
find_hardest_control(cid,node,noinv)
int cid;
node_t *node;
int *noinv;
{
    register int i;
    ndata *n;

    for (i = node_num_fanin(node) - 1; i >= 0; i--) {
        n = nptr(node_get_fanin(node,i));
        if (n->value[cid] == 2) {
	    *noinv = (nptr(node)->cube >> i) & 01;
	    return(n);
	}
    }
    return(NIL(ndata));
}

static ndata *
find_easiest_control(cid,node,noinv)
int cid;
node_t *node;
int *noinv;
{
    register int i;
    int nin = node_num_fanin(node);
    ndata *n;

    for (i = 0; i < nin; i++) {
        n = nptr(node_get_fanin(node,i));
        if (n->value[cid] == 2) {
	    *noinv = (nptr(node)->cube >> i) & 01;
            return(n);
        }
    }
    return(NIL(ndata));
}

/*
 * This routine and the find_???_control routines depend on the network
 * consisting of solely AND gates (with possibly NOTs on the inputs).
 */
static ndata *
find_pi_assignment(cid,obj,level)
int cid;
ndata *obj;
int level;
{
    ndata *n;
    int noinv;
    node_t *node = obj->node;

    if (node_type(node) == PRIMARY_INPUT) {
        obj->value[cid] = level;
	return(obj);
    }
    n = (level) ? find_hardest_control(cid,node,&noinv)
    		: find_easiest_control(cid,node,&noinv);
    if (n == NIL(ndata)) {
        return(NIL(ndata));
    }
    if (!noinv) {	/* inverter on input */
        level ^= 1;
    }
    return(find_pi_assignment(cid,n,level));
}


static void
save_edge(cid,stg)
int cid;
graph_t *stg;
{
    char *b1,*b2;
    register char *t1,*t2;
    int i, is_first_state;
    unsigned from = 0, to = 0;
    static char statechar[] = {'0','1','-'};
    vertex_t *instate, *outstate;
    edge_t *e;

    is_first_state = (total_no_of_states == 0);

    for (i = nlatch - 1; i >= 0 ; i--) {
        from = from << 1;
	from += (unsigned) stg_pstate[i]->value[cid];
        to = to << 1;
	to += (unsigned) stg_nstate[i]->value[cid];
    }
    /*
    (void)fprintf(sisout, "From %x to %x on ", from, to);
    */

    t1 = instring;
    t2 = outstring;
    for (i = 0; i < nlatch; i++) {
        *t1++ = statechar[stg_pstate[i]->value[cid]];
	*t2++ = statechar[stg_nstate[i]->value[cid]];
    }
    *t1 = '\0';
    *t2 = '\0';
    if (!st_lookup(state_table,instring,(char **) &instate)) {
        instate = g_add_vertex_static(stg);
	total_no_of_states++;
	b1 = util_strsav(instring);
	g_set_v_slot_static(instate,STATE_STRING,(gGeneric) b1);
	g_set_v_slot_static(instate,ENCODING_STRING, (gGeneric) util_strsav(instring));
	(void) st_insert(state_table,b1,(char *) instate);
    }
    if (!st_lookup(state_table,outstring,(char **) &outstate)) {
	total_no_of_states++;
        outstate = g_add_vertex_static(stg);
	b2 = util_strsav(outstring);
	g_set_v_slot_static(outstate,STATE_STRING,(gGeneric) b2);
	g_set_v_slot_static(outstate,ENCODING_STRING,(gGeneric) util_strsav(outstring));
	(void) st_insert(state_table,b2,(char *) outstate);
    }
    if (is_first_state) {
        g_set_g_slot_static(stg,START,(gGeneric) instate);
	g_set_g_slot_static(stg,CURRENT,(gGeneric) instate);
    }
    e = g_add_edge_static(instate,outstate);
    total_no_of_edges++;
    b1 = SENUM_ALLOC(char,npi + 1);
    t1 = b1;

    for (i = 0; i < npi; i++) {
        *t1++ = statechar[varying_node[i]->value[cid]];
    }
    *t1 = '\0';
    b2 = SENUM_ALLOC(char,npo + 1);
    t2 = b2;
    for (i = 0; i < npo; i++) {
        *t2++ = statechar[real_po[i]->value[cid]];
    }
    *t2 = '\0';
    /*
    (void)fprintf(sisout, "%s\n", b1);
    */
    g_set_e_slot_static(e,INPUT_STRING,(gGeneric) b1);
    g_set_e_slot_static(e,OUTPUT_STRING,(gGeneric) b2);
}

int barray[16] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
	8192, 16384, 32768}; 

void
ctable_enum(stg)
graph_t *stg;
{
    ndata **pi_stack,*n,*wpi,*state,*obj;
    int total_state,cur_state,tmp,i,no_state,pi_num,find_state,level;
    char *jflag,*value;
    
    pi_stack = SENUM_ALLOC(ndata *,npi);
    total_state = barray[nlatch];

    for (cur_state = 0; cur_state < total_state; cur_state++) {
	for (i = 0; i < n_varying_nodes; i++) {
	    n = varying_node[i];
	    n->value[0] = 2;
	    n->jflag[0] = 0;
	}
	tmp = cur_state;
	for (i = nlatch - 1; i >= 0; i--) {
	    n = stg_pstate[i];
	    if (tmp >= barray[i]) {
	        tmp -= barray[i];
		n->value[0] = 1;
	    }
	    else {
	        n->value[0] = 0;
	    }
	    n->jflag[0] |= CHANGED;
	}
	stg_sc_sim(0);
	no_state = FALSE;
	pi_num = 0;
	do {
	    wpi = NIL(ndata);
	    find_state = is_state(0,&obj,&level);
	    switch (find_state) {
	        case TRUE:
		    save_edge(0,stg);
		    while (wpi == NIL(ndata) && pi_num != 0) {
		        pi_num--;
			state = pi_stack[pi_num];
			jflag = state->jflag;
			value = state->value;
			if (*jflag & ALL_ASSIGNED) {
			    *jflag &= ~ALL_ASSIGNED;
			    *jflag |= CHANGED;
			    *value = 2;
			}
			else {
			    *jflag |= (ALL_ASSIGNED | CHANGED);
			    *value ^= 1;
			    wpi = state;
			    pi_num++;
			}
		    }
		    if (wpi == NIL(ndata)) {
		        no_state = TRUE;
		    }
		    break;
		case FALSE:
		    wpi = find_pi_assignment(0,obj,level);
		    if (wpi != NIL(ndata)) {
		        pi_stack[pi_num] = wpi;
			*wpi->jflag |= CHANGED;
			pi_num++;
		    }
		    while (wpi == NIL(ndata) && pi_num > 0) {
		        pi_num--;
			state = pi_stack[pi_num];
			jflag = state->jflag;
			value = state->value;
			if (*jflag & ALL_ASSIGNED) {
			    *jflag &= ~ALL_ASSIGNED;
			    *jflag |= CHANGED;
			    *value = 2;
			}
			else {
			    *jflag |= (ALL_ASSIGNED | CHANGED);
			    *value ^= 1;
			    wpi = state;
			    pi_num++;
			}
		    }
		    if (wpi == NIL(ndata)) {
		        no_state = TRUE;
		    }
		    break;
	    }
	    stg_sc_sim(0);
	} while (no_state == FALSE);
	while (pi_num > 0) {
	    pi_num--;
	    pi_stack[pi_num]->jflag[0] &= ~ALL_ASSIGNED;
	}
    }
    FREE(pi_stack);
}

unsigned *
shashcode(estate)
int *estate;
{
    unsigned *new_hashed, state;
    int i, j, k;

    /* Store the elements of the integer array as bits in "hashed_state" */
    j = nlatch % stg_bits_per_long;
    j = (j == 0 ? stg_bits_per_long : j);
    for (k = nlatch, i = stg_longs_per_state; i-- > 0; ){
	state = 0L;
	for (; j-- > 0 ; ){
	    state = state << 1;
	    state += (unsigned)estate[--k];
	}
	hashed_state[i] = state;
	j = stg_bits_per_long;
    }

    if (!st_lookup(stg_storelist, (char *)hashed_state, (char **)&new_hashed)){
	new_hashed = stg_get_state_hash();
	for ( i = 0; i < stg_longs_per_state; i++){
	    *(new_hashed+i) = hashed_state[i];
	}
	(void)st_insert(stg_storelist, (char *)new_hashed, (char *)new_hashed);
    }
    return(new_hashed);
}

void
enumerate(elength, estate, stg)
int elength;
int *estate;
graph_t *stg;
{
    int *next_estate,i,no_state,pi_num,find_state,level;
    ndata **pi_stack,*n,*obj,*wpi,*state;
    unsigned *s;
    char *jflag,*value;

    next_estate = ALLOC(int,nlatch);
    pi_stack = SENUM_ALLOC(ndata *,npi);

    for (i = 0; i < n_varying_nodes; i++) {
        n = varying_node[i];
	n->value[elength] = 2;
	n->jflag[elength] = 0;
    }
    for (i = 0; i < nlatch; i++) {
        state = stg_pstate[i];
        state->value[elength] = estate[i];
        state->jflag[elength] |= CHANGED;
    }
    stg_sc_sim(elength);
    no_state = FALSE;
    pi_num = 0;
    do {
        wpi = NIL(ndata);
        find_state = is_state(elength,&obj,&level);
        switch (find_state) {
	    case TRUE:
	        save_edge(elength,stg);
		for (i = 0; i < nlatch; i++) {
		    next_estate[i] = stg_nstate[i]->value[elength];
                }
		s = shashcode(next_estate);
		if (!st_is_member(slist,(char *) s)) {
                    if ((elength + 1) == MAX_ELENGTH) {
			/*
			(void)fprintf(sisout,"Reached ELENGTH LIMIT for ");
			stg_print_hashed_state(sisout, s);
			(void)fprintf(sisout,"\n");
			*/
		        (void) st_insert(slist,(char *) s,(char *) unfinish_head);
                        unfinish_head = s;
		    }
		    else {
		        (void) st_insert(slist, (char *) s,(char *)(NIL(unsigned)));
		        enumerate(elength + 1, next_estate, stg);
		    }
                }
                while (wpi == NIL(ndata) && pi_num != 0) {
                    pi_num--;
		    state = pi_stack[pi_num];
		    jflag = state->jflag;
		    value = state->value;
                    if (jflag[elength] & ALL_ASSIGNED) {
		        jflag[elength] &= ~ALL_ASSIGNED;
			jflag[elength] |= CHANGED;
                        value[elength] = 2;
                    }
                    else {
		        jflag[elength] |= (ALL_ASSIGNED | CHANGED);
			value[elength] ^= 1;
                        wpi = state;
                        pi_num++;
                    }
                }
                if (wpi == NIL(ndata)) {
		    no_state = TRUE;
		}
                break;
            case FALSE:
		wpi = find_pi_assignment(elength,obj,level);
		if (wpi != NIL(ndata)) {
                    pi_stack[pi_num] = wpi;
                    wpi->jflag[elength] |= CHANGED;
                    pi_num++;
                }
                while (wpi == NIL(ndata) && pi_num > 0) {
                    pi_num--;
		    state = pi_stack[pi_num];
		    jflag = state->jflag;
		    value = state->value;
                    if (jflag[elength] & ALL_ASSIGNED) {
                        jflag[elength] &= ~ALL_ASSIGNED;
                        jflag[elength] |= CHANGED;
                        value[elength] = 2;
                    }
                    else {
                        jflag[elength] |= (ALL_ASSIGNED | CHANGED);
                        value[elength] ^= 1;
                        wpi = state;
                        pi_num++;
                    }
                }
                if (wpi == NIL(ndata)) {
		    no_state = TRUE;
		}
                break;
        }
        stg_sc_sim(elength);
    } while (no_state == FALSE);
    while (pi_num > 0) {
        pi_num--;
        pi_stack[pi_num]->jflag[elength] &= ~ALL_ASSIGNED;
    }
    FREE(pi_stack);
    FREE(next_estate);
}/* end of enumerate */

#endif /* SIS */
