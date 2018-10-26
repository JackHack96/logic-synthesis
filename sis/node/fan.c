
#include "sis.h"
#include "node_int.h"


static void
fanin_remove_single_fanout(node, i)
node_t *node;
int i;
{
    char *data;

    LS_ASSERT(lsRemoveItem(node->fanin_fanout[i], &data));
    FREE(data);
    /*node->fanin[i]->fanout_changed = 1;*/
}


void
fanin_remove_fanout(node)
node_t *node;
{
    register int i;
    char *data;

    for(i = node->nin-1; i >= 0; i--) {
	LS_ASSERT(lsRemoveItem(node->fanin_fanout[i], &data));
	FREE(data);
/*	node->fanin[i]->fanout_changed = 1;*/
    }
}

static void
fanin_replace_single_fanout(node, i)
node_t *node;
int i;
{
    lsHandle handle; 
    register fanout_t *fanout_rec;
    node_t *fanin;

    fanin = node->fanin[i];
    fanout_rec = ALLOC(fanout_t, 1);
    fanout_rec->fanout = node;
    fanout_rec->pin = i;
    LS_ASSERT(lsNewEnd(fanin->fanout, (char *) fanout_rec, &handle));
    node->fanin_fanout[i] = handle;
/*    fanin->fanout_changed = 1;*/
}


void
fanin_add_fanout(node)
node_t *node;
{
    register int i;

    FREE(node->fanin_fanout);
    node->fanin_fanout = ALLOC(lsHandle, node->nin);
    for(i = node->nin-1; i >= 0; i--) {
	fanin_replace_single_fanout(node, i);
    }
}

lsGen
node_fanout_init_gen(node)
node_t *node;
{
    lsGen gen;

    if (node->network == 0) {
	fail("foreach_fanout: node is not in a network, fanout undefined");
	/* NOTREACHED */
    } 
    gen = lsStart(node->fanout);
    assert(gen != 0);
    return gen;
}


node_t *
node_fanout_gen(gen, pin)
lsGen gen;
int *pin;
{
    char *data;
    fanout_t *fanout_rec;

    if (lsNext(gen, &data, LS_NH) != LS_OK) {
	(void) lsFinish(gen);
	return 0;
    }
    fanout_rec = (fanout_t *) data;
    if (pin != 0) *pin = fanout_rec->pin;
    return fanout_rec->fanout;
}

node_t *
node_get_fanin(node, i)
node_t *node;
int i;
{
    if (i < 0 || i >= node->nin) {
	fail("node_get_fanin: bad fanin index");
    }
    return node->fanin[i];
}


node_t *
node_get_fanout(node, i)
node_t *node;
int i;
{
    int cnt;
    fanout_t *fanout_rec;
    lsGen gen;
    char *data;

    if (i < 0 || i >= node_num_fanout(node)) {
	fail("node_get_fanout: bad fanout index");
    }

    /* painful */
    fanout_rec = 0;
    cnt = 0;
    lsForeachItem(node->fanout, gen, data) {
	if (cnt++ == i) {
	    fanout_rec = (fanout_t *) data;
	    LS_ASSERT(lsFinish(gen));
	    break;
	}
    }
    return fanout_rec->fanout;
}


int
node_get_fanin_index(node, fanin)
register node_t *node, *fanin;
{
    register int i;

    for(i = 0; i < node->nin; i++) {
	if (fanin == node->fanin[i]) {
	    return i;
	}
    }
    return -1;
}


int
node_num_fanin(node)
node_t *node;
{
    return node->nin;
}


int
node_num_fanout(node)
node_t *node;
{
    if (node->network == 0) {
	fail("node_num_fanout: node is not in a network, fanout undefined");
	/* NOTREACHED */
    }
    return lsLength(node->fanout);
}

int 
node_patch_fanin(node, fanin, new_fanin)
node_t *node;
node_t *fanin;
node_t *new_fanin;
{
    int i;

    /* Patch fanin list of node, entry 'fanin' to 'new_fanin' */
    for(i = 0; i < node->nin; i++) {
	if (node->fanin[i] == fanin) {
	    fanin_remove_single_fanout(node, i);
	    node->fanin[i] = new_fanin;
	    fanin_replace_single_fanout(node, i);
	    node->is_dup_free = 0;	/* don't know ... */

	    return 1;		/* success */
	}
    }

    return 0;			/* failure */
}


/*
 * Mimics the functionality of node_patch_fanin() ... However
 * uses the index to find the fanin that is going to be replaced by
 * "new_fanin"
 */
int 
node_patch_fanin_index(node, fanin_index, new_fanin)
node_t *node;
int fanin_index;
node_t *new_fanin;
{
    /* Check if the index is a valid one */
    if (fanin_index < 0 || fanin_index >= node->nin){
	return 0;
    }
      
    /* Patch fanin list of node, entry 'fanin_index' to 'new_fanin' */
    fanin_remove_single_fanout(node, fanin_index);
    node->fanin[fanin_index] = new_fanin;
    fanin_replace_single_fanout(node, fanin_index);
    node->is_dup_free = 0;	/* don't know ... */


    return 1;		/* success */
}
