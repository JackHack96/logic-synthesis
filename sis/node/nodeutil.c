
#include "sis.h"
#include "node_int.h"

static daemon_t *daemon_func[4];
static          global_sis_id = 0;


node_t *
node_alloc() {
    register node_t   *node;
    register daemon_t *d;

    node = ALLOC(node_t, 1);

    node->name = NIL(
    char);
    node->short_name = NIL(
    char);
    node->type   = UNASSIGNED;
    node->sis_id = global_sis_id++;

    node->fanin_changed  = 0;
    node->fanout_changed = 0;
    node->is_dup_free    = 0;
    node->is_min_base    = 0;
    node->is_scc_minimal = 0;

    node->nin   = 0;
    node->fanin = NIL(node_t * );

    node->fanout       = lsCreate();
    node->fanin_fanout = 0;

    node->F = NIL(set_family_t);
    node->D = NIL(set_family_t);
    node->R = NIL(set_family_t);

    node->copy = NIL(node_t);

    node->network    = NIL(network_t);
    node->net_handle = 0;

    node->simulation = 0;
    node->factored   = 0;
    node->delay      = 0;
    node->map        = 0;
    node->buf        = 0;
    node->pld        = 0;
    node->ite        = 0;
    node->bin        = 0;
    node->cspf       = 0;
    node->atpg       = 0;
    node->undef1     = 0;

    for (d = daemon_func[(int) DAEMON_ALLOC]; d != 0; d = d->next) {
        (*d->func)(node);
    }

    return node;
}

void
node_free(node)
        node_t *node;
{
    daemon_t *d;

    if (node == 0) return;

    FREE(node->name);
    FREE(node->short_name);

    FREE(node->fanin);

    LS_ASSERT(lsDestroy(node->fanout, free));
    FREE(node->fanin_fanout);

    if (node->F != 0) sf_free(node->F);
    if (node->D != 0) sf_free(node->D);
    if (node->R != 0) sf_free(node->R);

    for (d = daemon_func[(int) DAEMON_FREE]; d != 0; d = d->next) {
        (*d->func)(node);
    }

    FREE(node);
}

node_t *
node_dup(old)
        register node_t *old;
{
    register node_t   *new;
    register daemon_t *d;

    if (old == 0) return 0;

    new = node_alloc();

    if (old->name != 0) {
        new->name = util_strsav(old->name);
    }
    if (old->short_name != 0) {
        new->short_name = util_strsav(old->short_name);
    }

    new->type = old->type;

    new->fanin_changed  = old->fanin_changed;
    new->fanout_changed = old->fanout_changed;
    new->is_dup_free    = old->is_dup_free;
    new->is_min_base    = old->is_min_base;
    new->is_scc_minimal = old->is_scc_minimal;

    new->nin = old->nin;
    if (old->nin != 0) {
        new->fanin = nodevec_dup(old->fanin, old->nin);
    }

    /* do NOT copy old->fanout ... */
    /* do NOT copy old->fanin_fanout ... */

    if (old->F != 0) new->F = sf_save(old->F);
    if (old->D != 0) new->D = sf_save(old->D);
    if (old->R != 0) new->R = sf_save(old->R);

    new->copy = 0;                /* ### for saber */

    /* do NOT copy old->network ... */
    /* do NOT copy old->net_handle ... */

    for (d = daemon_func[(int) DAEMON_DUP]; d != 0; d = d->next) {
        (*d->func)(old, new);
    }
    return new;
}

node_t **
nodevec_dup(fanin, nin)
        register node_t **fanin;
        int nin;
{
    register int    i;
    register node_t **new_fanin;

    new_fanin = ALLOC(node_t * , nin);
    for (i    = nin - 1; i >= 0; i--) {
        new_fanin[i] = fanin[i];
    }
    return new_fanin;
}


void
node_invalid(node)
        register node_t *node;
{
    register daemon_t *d;

    if (node->D != 0) {
        sf_free(node->D);
        node->D = 0;
    }
    if (node->R != 0) {
        sf_free(node->R);
        node->R = 0;
    }

    for (d = daemon_func[(int) DAEMON_INVALID]; d != 0; d = d->next) {
        (*d->func)(node);
    }
    node->is_dup_free    = 0;
    node->is_min_base    = 0;
    node->is_scc_minimal = 0;
    /* fanin_changed, fanout_changed are updated by fan.c */
}


void
node_register_daemon(type, func)
        node_daemon_type_t type;
        void (*func)();
{
    daemon_t *daemon;

    switch (type) {
        case DAEMON_ALLOC:
        case DAEMON_FREE:
        case DAEMON_INVALID:
        case DAEMON_DUP: daemon = ALLOC(daemon_t, 1);
            daemon->func = func;
            daemon->next = daemon_func[(int) type];
            daemon_func[(int) type] = daemon;
            break;

        default: fail("node_register_daemon: bad daemon type");
    }
}


void
node_discard_all_daemons() {
    node_daemon_type_t type;
    daemon_t           *d, *dnext;

    for (type = DAEMON_ALLOC; (int) type <= (int) DAEMON_DUP;
         type = (node_daemon_type_t)((int) type + 1)) {
        /* ANSI would allow the much simpler type++ */
        for (d = daemon_func[(int) type]; d != 0; d = dnext) {
            dnext = d->next;
            FREE(d);
        }
        daemon_func[(int) type] = 0;
    }
}

network_t *
node_network(node)
        node_t *node;
{
    return node->network;
}
