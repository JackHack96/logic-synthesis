
#include "sis.h"

#include "node_int.h"

/*
 *  manage the node names
 */

name_mode_t name_mode;

static int long_name_index;
static int short_name_index;

static char *hack_name();

char *node_name(node_t *node) {
    if (name_mode == LONG_NAME_MODE) {
        if (node->name == NIL(char))
            node_assign_name(node);
        return hack_name(node, node->name);
    } else {
        if (node->short_name == NIL(char))
            node_assign_short_name(node);
        return hack_name(node, node->short_name);
    }
}

char *node_long_name(node_t *node) { return node->name; }

void node_assign_name(node_t *node) {
    static char buf[80];
    if (node->name != NIL(char))
        FREE(node->name);
    (void) sprintf(buf, "[%d]", long_name_index);
    node->name = ALLOC(char, strlen(buf) + 1);
    strcpy(node->name, buf);
    long_name_index++;
}

void node_assign_short_name(node_t *node) {
    int i, c;

    c = "abcdefghijklmnopqrstuvwxyz"[short_name_index % 26];
    i = short_name_index / 26;

    if (node->short_name != NIL(char))
        FREE(node->short_name);
    node->short_name = ALLOC(char, 10);
    if (i == 0) {
        (void) sprintf(node->short_name, "%c", c);
    } else {
        (void) sprintf(node->short_name, "%c%d", c, i - 1);
    }
    short_name_index++;
}

static char *hack_name(node_t *node, char *name) {
    static char name1[1024];
    char        *name2;
    int         count;
    node_t      *fanout;
    lsGen       gen;
    node_type_t type;
    network_t   *net;

    type = node->type;
    net  = node->network;
    if (type == UNASSIGNED || net == NIL(network_t)) {
        return (name);
    }
    if (type == PRIMARY_INPUT) {
        return (name);
    } else if (type == PRIMARY_OUTPUT) {
#ifdef SIS
        if (network_is_real_po(net, node) == 0) {
          count = 0;
          foreach_fanout(node->fanin[0], gen, fanout) {
            if (fanout->type == PRIMARY_OUTPUT) {
              count++;
            }
          }
          if (count == 1) {
            node = node->fanin[0];
            name = name_mode == LONG_NAME_MODE ? node->name : node->short_name;
          }
        }
#endif /* SIS */
        (void) strcpy(name1, "{");
        (void) strcat(name1, name);
        (void) strcat(name1, "}");
        return name1;
    }
    count = 0;
    (void) strcpy(name1, "{");
    foreach_fanout(node, gen, fanout) {

#ifdef SIS
        if (network_is_real_po(net, fanout) != 0) {
          if (++count > 1) {
            (void)strcat(name1, ",");
          }
          name2 = name_mode == LONG_NAME_MODE ? fanout->name : fanout->short_name;
          (void)strcat(name1, name2);
        }
#else
        if (fanout->type == PRIMARY_OUTPUT) {
            if (++count > 1) {
                (void) strcat(name1, ",");
            }
            name2 = name_mode == LONG_NAME_MODE ? fanout->name : fanout->short_name;
            (void) strcat(name1, name2);
        }
#endif /* SIS */
    }
    (void) strcat(name1, "}");
    return count == 0 ? name : name1;
}

int node_is_madeup_name(char *name, int *value) {
    if (name[0] == '[' && name[strlen(name) - 1] == ']') {
        if (sscanf(name, "[%d]", value) == 1) {
            return 1;
        }
    }
    return 0; /* not a made-up name */
}

void network_reset_long_name(network_t *network) {
    lsGen  gen;
    node_t *node;
    int    index;

    long_name_index = 0;
    foreach_node(network, gen, node) {
        if (node_is_madeup_name(node->name, &index)) {
            node_assign_name(node);
        }
    }
    network_rehash_names(network, /* long */ 1, /* short */ 0);
}

void network_reset_short_name(network_t *network) {
    lsGen  gen;
    node_t *node;

    short_name_index = 0;
    foreach_primary_input(network, gen, node) { node_assign_short_name(node); }
    foreach_primary_output(network, gen, node) { node_assign_short_name(node); }
    foreach_node(network, gen, node) {
        if (node->type != PRIMARY_INPUT && node->type != PRIMARY_OUTPUT) {
            node_assign_short_name(node);
        }
    }
    network_rehash_names(network, /* long */ 0, /* short */ 1);
}

void network_rehash_names(network_t *network, register int long_name, register int short_name) {
    lsGen  gen;
    node_t *p;
    int    found;

    if (long_name) {
        st_free_table(network->name_table);
        network->name_table = st_init_table(strcmp, st_strhash);
    }
    if (short_name) {
        st_free_table(network->short_name_table);
        network->short_name_table = st_init_table(strcmp, st_strhash);
    }

    foreach_node(network, gen, p) {
        if (long_name) {
            found = st_insert(network->name_table, p->name, (char *) p);
            assert(!found);
        }
        if (short_name) {
            found = st_insert(network->short_name_table, p->short_name, (char *) p);
            assert(!found);
        }
    }
}
