
#ifdef OCT

/*
 * routine for writing SISII logic networks (mapped and unmapped) into
 * Oct facets
 */
#include "../port/copyright.h"
#include "../port/port.h"
#undef assert
#include "sis.h"
#include "farray.h"
#include "st.h"
#undef ALLOC
#undef REALLOC
#undef FREE
#include "errtrap.h"
#include "../utility/utility.h"
#include "oct.h"
#include "oh.h"
#include "octio.h"

#define node_oct_name(node)   (node)->name
static char *octCellPath, *octCellView;

/* name of the program - for errtrap */
char *optProgName = "sis";



/*
 * special routines to make octGetByName faster (use hash tables)
 *
 *   why: octGetByName does a linear search to find an object,
 *   as the number of objects increases, the search times becomes
 *   dominant.  The special functions surround octGetByName and
 *   hash information as it is found/created.
 */

static st_table *NetTable = NIL(st_table);

static void
initialize_net_table()
{
    NetTable = st_init_table(strcmp, st_strhash);
    return;
}

static octStatus
specialGetByNetName(facet, net, name)
octObject *facet, *net;
char *name;
{
    char *ptr;
    
    if (st_lookup(NetTable, name, &ptr)) {
    net->objectId = *(octId *) ptr;
    OH_ASSERT(octGetById(net));
    return(OCT_OK);
    } else {
    if (ohGetByNetName(facet, net, name) == OCT_OK) {
            octId *id = ALLOC(octId, 1);
            *id = net->objectId;
        (void) st_insert(NetTable, name, (char *) id);
        return(OCT_OK);
    }
    return(OCT_ERROR);
    }
}

static void
specialCreateNet(facet, net, name)
octObject *facet, *net;
char *name;
{
    octId *id;

    if (st_lookup(NetTable, name, NIL(char *))) {
    errRaise(SIS_PKG_NAME, OCT_ERROR,
         "trying to create a net that already exists: %s",
         name);
    }
    if (ohCreateNet(facet, net, name) != OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR,
         "can not create net %s (%s)",
         name,
         octErrorString());
    }
    id = ALLOC(octId, 1);
    *id = net->objectId;
    (void) st_insert(NetTable, name, (char *) id);
    return;
}

static void
specialGetOrCreateNet(facet, net, name)
octObject *facet, *net;
char *name;
{
    if (specialGetByNetName(facet, net, name) == OCT_OK) {
    return;
    }
    specialCreateNet(facet, net, name);
    return;
}



/*
 * routines to attach required SYMBOLIC policy properties
 */

static void
attachPolicyTermProperties(term, direction, termtype)
octObject *term;
char *direction, *termtype;
{
    octObject prop;

    OH_ASSERT(ohCreatePropStr(term, &prop, "DIRECTION", direction));
    OH_ASSERT(ohCreatePropStr(term, &prop, "TERMTYPE", termtype));
    return;
}

static void
attachPolicyProperties(facet)
    octObject *facet;
{
    char *technology, *editstyle, *viewtype;
    octObject prop;

    /* attach POLICY properties */
    if ((technology = com_get_flag("OCT-TECHNOLOGY")) == NIL(char)) {
    ohCreateOrModifyPropStr(facet, &prop, "TECHNOLOGY", "scmos");
    } else {
    ohCreateOrModifyPropStr(facet, &prop, "TECHNOLOGY", technology);
    }
    if ((editstyle = com_get_flag("OCT-EDITSTYLE")) == NIL(char)) {
    ohCreateOrModifyPropStr(facet, &prop, "EDITSTYLE", "SYMBOLIC");
    } else {
    ohCreateOrModifyPropStr(facet, &prop, "EDITSTYLE", editstyle);
    }
    if ((viewtype = com_get_flag("OCT-VIEWTYPE")) == NIL(char)) {
    ohCreateOrModifyPropStr(facet, &prop, "VIEWTYPE", "SYMBOLIC");
    } else {
    ohCreateOrModifyPropStr(facet, &prop, "VIEWTYPE", viewtype);
    }
    return;
}

static void
createInterface(facet)
octObject *facet;
{
    octObject interface, prop, term, iterm, dummy;
    octGenerator tgen;

    interface.type = OCT_FACET;
    interface.contents.facet.cell = facet->contents.facet.cell;
    interface.contents.facet.view = facet->contents.facet.view;
    interface.contents.facet.facet = "interface";
    interface.contents.facet.version = OCT_CURRENT_VERSION;
    interface.contents.facet.mode = "w";

    if (octOpenFacet(&interface) < OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR,
         "can not open the facet %s (%s)",
         ohFormatName(&interface),
         octErrorString());
    }

    attachPolicyProperties(&interface);

    OH_ASSERT(octInitGenContents(facet, OCT_TERM_MASK, &tgen));
    while (octGenerate(&tgen, &term) == OCT_OK) {
    if (ohGetByTermName(&interface, &iterm, term.contents.term.name) < OCT_OK) {
        errRaise(SIS_PKG_NAME, OCT_ERROR,
             "can not get the formal terminal %s on %s (%s)",
             term.contents.term.name,
             ohFormatName(&interface),
             octErrorString());
    }
    if (ohGetByPropName(&term, &prop, "DIRECTION") < OCT_OK) {
        errRaise(SIS_PKG_NAME, OCT_ERROR,
             "can not get the DIRECTION property on terminal %s of %s (%s)",
             term.contents.term.name,
             ohFormatName(&interface),
             octErrorString());
    }
    OH_ASSERT(ohCreatePropStr(&iterm, &dummy, "DIRECTION", prop.contents.prop.value.string));
    if (ohGetByPropName(&term, &prop, "TERMTYPE") < OCT_OK) {
        errRaise(SIS_PKG_NAME, OCT_ERROR,
             "can not get the TERMTYPE property on terminal %s of %s (%s)",
             term.contents.term.name,
             ohFormatName(&interface),
             octErrorString());
    }
    OH_ASSERT(ohCreatePropStr(&iterm, &dummy, "TERMTYPE", prop.contents.prop.value.string));
    }
    OH_ASSERT(ohCreatePropStr(&interface, &prop, "CELLCLASS", "MODULE"));

    OH_ASSERT(octCloseFacet(&interface));

    return;
}



/*
 * replacements for the 'library' package functions to handle
 * generic instances
 */

static int IsGeneric = 0;

static void
createGenericMaster(node, name, view)
node_t *node;
char *name, *view;
/*
 * create a generic master
 */
{
    octObject facet, interface, prop, term;
    octStatus status;
    int i;
    char buffer[32];

    status = ohOpenFacet(&facet, name, view, "contents", "a");
    if ((status == OCT_OLD_FACET) || (status == OCT_ALREADY_OPEN)) {
    return;
    } else if (status == OCT_NEW_FACET) {
    /* new cell */
    attachPolicyProperties(&facet);
    OH_ASSERT(ohCreatePropStr(&facet, &prop, "CELLCLASS", "LEAF"));
    OH_ASSERT(ohCreatePropStr(&facet, &prop, "CELLTYPE", "COMBINATIONAL"));
    /* put some terminals down */
    for (i = 0; i < node_num_fanin(node); i++) {
        (void) sprintf(buffer, "in%d", i);
        OH_ASSERT(ohCreateTerm(&facet, &term, buffer));
        attachPolicyTermProperties(&term, "INPUT", "SIGNAL");
    }
    OH_ASSERT(ohCreateTerm(&facet, &term, "out0"));
    attachPolicyTermProperties(&term, "OUTPUT", "SIGNAL");
    OH_ASSERT(octFlushFacet(&facet));

    /* create the interface facet */

    OH_ASSERT(ohOpenInterface(&interface, &facet, "w"));

    attachPolicyProperties(&interface);
    OH_ASSERT(ohCreatePropStr(&interface, &prop, "CELLCLASS", "LEAF"));
    OH_ASSERT(ohCreatePropStr(&interface, &prop, "CELLTYPE", "COMBINATIONAL"));
    /* put some terminals down */
    for (i = 0; i < node_num_fanin(node); i++) {
        (void) sprintf(buffer, "in%d", i);
        if (ohGetByTermName(&interface, &term, buffer) < OCT_OK) {
        errRaise(SIS_PKG_NAME, OCT_ERROR,
             "can not get formal terminal %s on %s (%s)",
             buffer,
             ohFormatName(&interface),
             octErrorString());
        }
        attachPolicyTermProperties(&term, "INPUT", "SIGNAL");
    }
    if (ohGetByTermName(&interface, &term, "out0") < OCT_OK) {
        errRaise(SIS_PKG_NAME, OCT_ERROR,
             "can not get formal terminal %s on %s (%s)",
             "out0",
             ohFormatName(&interface),
             octErrorString());
    }
    attachPolicyTermProperties(&term, "OUTPUT", "SIGNAL");
    OH_ASSERT(octFlushFacet(&interface));

    return;
    }
    errRaise(SIS_PKG_NAME, status,
         "can not open the generic master %s (%s)",
         ohFormatName(&facet),
         octErrorString());
    return;
}

/* XXX TOO MUCH STRING SAVING GOING ON HERE */

static void
unpackname(name, cell, view, defaultView)
char *name;
char **cell;
char **view;
char *defaultView;
{
    char *ptr;
    char buffer[4096];

    (void) strcpy(buffer, name);
    ptr = strchr(buffer, ':');
    if (ptr == NIL(char)) {
    *cell = util_strsav(buffer);
    *view = util_strsav(defaultView);
    } else {
    *ptr = '\0';
    *cell = util_strsav(buffer);
    *view = util_strsav(ptr + 1);
    }
    return;
}

static void
oct_lib_get_gate_cell_and_view(node, facet, cell, view)
    node_t *node;
    octObject *facet;
    char **cell, **view;
{
    static char master[1024];
    char *gateName;
    char path[1024];
    int pinCount;

    if ((gateName = lib_get_gate_name(node)) == NIL(char)) {
    /* generic node */
    pinCount = node_num_fanin(node);
    (void) sprintf(path, "%s/generic-%d", facet->contents.facet.cell,
               pinCount);
    createGenericMaster(node, path, octCellView);
    IsGeneric = 1;
    (void) sprintf(master, "generic-%d", pinCount);
    *cell = master;
    *view = octCellView;
    return;

    } else {
    IsGeneric = 0;
    unpackname(gateName, cell, view, octCellView);
    if ((**cell != '/') && (**cell != '~') && (octCellPath != NIL(char))) {
        /* prepend octCellPath to cell */
        (void) sprintf(master, "%s/%s", octCellPath, *cell);
        *cell = master;
    }
    return;
    }
}



static void
factored_form(network, orig_node, node, fform)
network_t *network;
node_t *orig_node, *node;
array_t *fform;
{
    node_cube_t cube;
    node_literal_t literal;
    int i, j, cube_count, fanin_count;
    char buffer[32];

    if (node_network(node) == network) {
    int size;

    /* the fanin_index stuff should be cached ... */
    (void) sprintf(buffer, "in%d", node_get_fanin_index(orig_node, node));
    size = strlen(buffer);
    for (i = 0; i < size; i++) {
        f_array_insert_last(char, fform, buffer[i]);
    }
    return;
    }

    cube_count = node_num_cube(node);
    if (cube_count == 0) {
    return;
    }

    if (cube_count > 1) {
    f_array_insert_last(char, fform, '(');
    f_array_insert_last(char, fform, '+');
    }

    for (i = 0; i < cube_count; i++) {
    cube = node_get_cube(node, i);
    fanin_count = node_num_fanin(node);
    if (fanin_count == 0) {
        continue;
    }
    if (fanin_count > 1) {
        if (i > 0) {
        f_array_insert_last(char, fform, ' ');
        }
        f_array_insert_last(char, fform, '(');
        f_array_insert_last(char, fform, '*');
    }

    for (j = 0; j < fanin_count; j++) {
        literal = node_get_literal(cube, j);
        switch (literal) {
        case ZERO:
        f_array_insert_last(char, fform, ' ');
        f_array_insert_last(char, fform, '(');
        f_array_insert_last(char, fform, '!');
        f_array_insert_last(char, fform, ' ');
        factored_form(network, orig_node, node_get_fanin(node, j), fform);
        f_array_insert_last(char, fform, ')');
        break;
        case ONE:
        f_array_insert_last(char, fform, ' ');
        factored_form(network, orig_node, node_get_fanin(node, j), fform);
        break;
        }
    }
    if (fanin_count > 1) {
        f_array_insert_last(char, fform, ')');
    }
    }

    if (cube_count > 1) {
    f_array_insert_last(char, fform, ')');
    }

    return;
}


static array_t *
oct_lib_get_factored_function(node)
node_t *node;
{
    network_t *network;
    array_t *nodes;
    int i;
    array_t *fform;

    network = node_network(node);
    nodes = factor_to_nodes(node);
    fform = array_alloc(char, 1024);
    factored_form(network, node, array_fetch(node_t *, nodes, 0), fform);
    f_array_insert_last(char, fform, '\0');
    for (i = 0; i < array_n(nodes); i++) {
    node_free(array_fetch(node_t *, nodes, i));
    }
    return fform;
}


static char *
oct_lib_get_pin_name(node, index)
node_t *node;
int index;
{
    static char buffer[32];

    if ( IsGeneric ) {
    (void) sprintf(buffer, "in%d", index);
    return buffer;
    } else {
    lib_gate_t *gate = lib_gate_of( node );
    return lib_gate_pin_name(gate, index, 1);
    }
}


static char *
oct_lib_get_out_pin_name(node, index)
node_t *node;
int index;
{
    static char buffer[32];

    if (IsGeneric) {
    (void) sprintf(buffer, "out%d", index);
    return(buffer);
    } else {
        lib_gate_t *gate = lib_gate_of( node );
        return lib_gate_pin_name(gate, index, 0);
    }
}


static void
attachLogicFunction(term, node)
octObject *term;
node_t *node;
{
    node_function_t funct;

    octObject prop;
    if ( IsGeneric ) {
    funct = node_function(node);
    switch (funct) {
    case NODE_UNDEFINED:
        errRaise(SIS_PKG_NAME, OCT_ERROR,
             "Function for node %s is undefined.", node_oct_name(node));
        return;

    case NODE_0:
        OH_ASSERT(ohCreatePropStr(term, &prop, "LOGICFUNCTION", "(0)"));
        break;

    case NODE_1:
        OH_ASSERT(ohCreatePropStr(term, &prop, "LOGICFUNCTION", "(1)"));
        break;

    default:
        {
        array_t *fform = oct_lib_get_factored_function(node);
        char *data = array_data(char, fform);

        OH_ASSERT(ohCreatePropStr(term, &prop, "LOGICFUNCTION", data));
        array_free(fform);
        }
        break;

    }
    }
    return;
}

create_primary_output( facet, node )
    octObject *facet;
    node_t *node;
{
    octObject term, net;
    char* nodeName = node_oct_name(node);

    OH_ASSERT(ohCreateTerm(facet, &term, nodeName));
    attachPolicyTermProperties(&term, "OUTPUT", "SIGNAL");
    specialGetOrCreateNet(facet, &net, node_oct_name(node_get_fanin(node, 0)));
    OH_ASSERT(octAttach(&net, &term));
}

create_primary_input( facet, node )
    octObject *facet;
    node_t *node;
{
    octObject term, net, prop;
    char* nodeName = node_oct_name(node);

    OH_ASSERT(ohCreateTerm(facet, &term, nodeName));
    specialGetOrCreateNet(facet, &net, nodeName );
#ifdef SIS
    if (network_get_control(node_network(node), node) == NIL(node_t)){
    attachPolicyTermProperties(&term, "INPUT", "SIGNAL");
    } else {
    OH_ASSERT(ohCreatePropStr(&net, &prop, "NETTYPE", "CLOCK"));
    attachPolicyTermProperties(&term, "INPUT", "CLOCK");
    }
#else
    attachPolicyTermProperties(&term, "INPUT", "SIGNAL");
#endif /* SIS */
    OH_ASSERT(octAttach(&net, &term));
}

void oct_connect_instance_term( facet, instance, termName, netName )
    octObject *facet;
    octObject *instance;
    char *termName;
    char *netName;
{
    octObject net,term;

    if (ohGetByTermName(instance, &term, termName) < OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR,
         "can not get the actual terminal %s of %s (%s)",
         termName,
         ohFormatName(instance),
         octErrorString());
    }

    specialGetOrCreateNet(facet, &net, netName);

    OH_ASSERT( octAttach( &net, &term ) );
}

#ifdef SIS   /* Support for writing latches */
void create_latch( facet, node, latch )
    octObject *facet;
    node_t *node;
    latch_t *latch;
{
    lib_gate_t *gate = NIL(lib_gate_t);
    int i;
    char* name = 0;
    char *cell, *view;
    node_t *fanin, *temp, *po;
    char *gateName, *netName, *termName;
    octObject prop, instance, bag;

    OH_ASSERT(ohGetOrCreateBag(facet, &bag, "INSTANCES"));

    gate = latch_get_gate( latch );
    if ( gate != NIL(lib_gate_t) ) {
    po = latch_get_input( latch ); /* Get the input */
    temp = node_get_fanin(po, 0); /*  */

    gateName = lib_gate_name( gate );
    oct_lib_get_gate_cell_and_view(temp, facet, &cell, &view);
      /* unpackname( gateName, &cell, &view, octCellView ); */
    /*
    (void)fprintf(sisout,  "Creating %s of %s:%s in %s\n", gateName,
        cell, view, ohFormatName( facet ) );
    */
    OH_ASSERT(ohCreateInst( facet, &instance, gateName, cell, view ));
    octAttach( &bag, &instance );


    assert(lib_gate_of(temp) == gate);
    foreach_fanin(temp, i, fanin){
        if (lib_gate_latch_pin(gate) != i){
        /* Exclude the feedback pin -- if any */
        netName = node_oct_name( fanin );
        termName = lib_gate_pin_name( gate, i, 1 );
        oct_connect_instance_term( facet, &instance, termName, netName );
        }
    }

    assert(node == latch_get_output( latch ));
    netName = node_oct_name( node );
    termName = lib_gate_pin_name( gate, 0, 0 );
    oct_connect_instance_term( facet, &instance, termName, netName );

    temp = latch_get_control( latch );
    if (temp != NIL(node_t)){
        assert(temp->type == PRIMARY_OUTPUT);
        fanin = node_get_fanin(temp, 0);
        netName = node_oct_name( fanin );
        termName = lib_gate_pin_name( gate, 0, 2 );
        oct_connect_instance_term( facet, &instance, termName, netName );
    }

    /* write out the initial value with the INSTANCE */
    OH_ASSERT(ohCreatePropInt(&instance, &prop, "INITIAL_VALUE",
        latch_get_initial_value(latch)));

    } else {
    errRaise( SIS_PKG_NAME, 1, "Cannot write generic latches into OCT yet." );
    }
}
#endif /* SIS */


#ifdef SIS   /* Support for writing clocks */

#define DONE_RISE_TRANSITION 1
#define DONE_FALL_TRANSITION 2
#define DONE_BOTH_TRANSITION 3

/*
 * Utility routine that will get the name of the node representing the clock
 * This is required since short names may be used during writing. So we
 * cannot use the names stored with the sis_clock_t structures
 */
static char *
octio_clock_name(clock, short_flag)
sis_clock_t *clock;
int short_flag;
{
    node_t *node;
    extern char *io_name();	/* Should be in io.h */

    node = network_find_node(clock->network, clock_name(clock));
    assert(node != NIL(node_t));
    return io_name(node, short_flag);
}

static void
doEdge( edge, clockEventsBag, done_table )
    clock_edge_t *edge;
    octObject *clockEventsBag;
    st_table *done_table;
{
    char prefix = edge->transition == RISE_TRANSITION ? 'r' : 'f';
    char buffer[256];
    double min, max;
    octObject prop, eventBag;
    int done;
    char *dummy;

    if ( st_lookup_int( done_table, (char *)(edge->clock), &done ) ) {
    if ( done == DONE_BOTH_TRANSITION ) return;
    if ( done == DONE_RISE_TRANSITION && edge->transition == RISE_TRANSITION ) return;
    if ( done == DONE_FALL_TRANSITION && edge->transition == FALL_TRANSITION ) return;

    done = DONE_BOTH_TRANSITION;
    } else {
    done = (edge->transition == RISE_TRANSITION) ? DONE_RISE_TRANSITION : DONE_FALL_TRANSITION;
    }

    (void)sprintf(buffer, "%c'%s", prefix, octio_clock_name(edge->clock, 0 ) );
    max = clock_get_parameter(*edge, CLOCK_UPPER_RANGE);
    min = clock_get_parameter(*edge, CLOCK_LOWER_RANGE);

    /*
    (void)fprintf (sisout, "%s %lf %lf\n", buffer, min, max );
    */

    OH_ASSERT(ohCreateBag(clockEventsBag, &eventBag, "EVENT" ));
    ohCreatePropStr( &eventBag, &prop, "DESCRIPTION", buffer );
    ohCreatePropReal( &eventBag, &prop, "MAX", max );
    ohCreatePropReal( &eventBag, &prop, "MIN", min );

    (void) st_insert(done_table, (char *)(edge->clock), (char *)done);
}

static void
write_gen_event(sisBag, clock, transition, done_table, short_flag)
    octObject *sisBag;
    sis_clock_t *clock;
    int transition;
    st_table *done_table;
    int short_flag;
{
    octObject clockEventsBag, prop;
    clock_edge_t edge, *new_edge;
    lsGen gen;
    char *dummy;
    int done;
    char prefix;

    edge.transition = transition;
    edge.clock = clock;

    if (clock_get_parameter(edge, CLOCK_NOMINAL_POSITION) == CLOCK_NOT_SET) {
        return;
    }

    OH_ASSERT(ohCreateBag( sisBag, &clockEventsBag, "CLOCK_EVENTS"));
    OH_ASSERT(ohCreatePropReal( &clockEventsBag, &prop, "TIME",
        clock_get_parameter(edge, CLOCK_NOMINAL_POSITION)));

    doEdge(&edge, &clockEventsBag, done_table );

    gen = clock_gen_dependency(edge);
    while (lsNext(gen, (char **)&new_edge, LS_NH) == LS_OK) {
    doEdge( new_edge, &clockEventsBag, done_table );
    }
    lsFinish(gen);
}

/*
 * To write out clock data-str into SIS . Look at oct_read.c for a
 * description of the SIS_CLOCKS policy.
 */

write_oct_clocks(network, facet, short_flag )
    network_t *network;
    octObject *facet;
    int short_flag;	/* whether to use long or short names */
{
    int first = 1;
    lsGen gen;
    sis_clock_t *clock;
    node_t *node;
    double cycle;
    st_table *done_table;
    octObject sisBag, prop;
    int trans;

    ohGetOrCreateBag( facet, &sisBag, "SIS_CLOCKS" );
    ohRecursiveDelete( &sisBag, OCT_BAG_MASK | OCT_PROP_MASK );
    OH_ASSERT(ohGetOrCreateBag( facet, &sisBag, "SIS_CLOCKS"));

    cycle = clock_get_cycletime(network);
    if (cycle > 0) {
    ohGetOrCreatePropReal( &sisBag, &prop, "CYCLETIME", cycle );
    }

    done_table = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_clock (network, gen, clock) {
        if (st_lookup_int(done_table, (char *) clock, &trans) != 0) {

        switch (trans) {
        case DONE_BOTH_TRANSITION:
            break;
        case DONE_FALL_TRANSITION:

            write_gen_event(&sisBag, clock, RISE_TRANSITION, done_table,
                    short_flag);
        break;
        default:
            write_gen_event(&sisBag, clock, FALL_TRANSITION, done_table,
                    short_flag);
        }
    } else {
        write_gen_event(&sisBag, clock, RISE_TRANSITION, done_table, short_flag);
        (void) st_lookup_int(done_table, (char *) clock, &trans);
        if (trans != DONE_BOTH_TRANSITION) {
            write_gen_event(&sisBag, clock, FALL_TRANSITION, done_table,
                    short_flag);
        }
    }
    }
    st_free_table(done_table);
}
#endif /* SIS */


void
write_oct(network, facet)
    network_t **network;
    octObject *facet;
/*
 * write_oct: write a network out into an Oct facet (procedural interface)
 *
 */
{
    octObject instance, net, term, prop, bag;
    char *termName, *nodeName, *inTermName, *inNetName;
    char *masterCell, *masterView;
    int input, i;
    array_t *nodes;
    node_t *node, *fanin;
#ifdef SIS
    int is_mapped;
    latch_t *latch;
#endif
    node_function_t funct;

    if (octOpenFacet(facet) < OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR,
         "can not open %s (%s)",
         ohFormatName(facet),
         octErrorString());
    }

    attachPolicyProperties(facet);

    /* flush the facet (side effect to create directory for GENERIC masters) */
    OH_ASSERT(octFlushFacet(facet));

    OH_ASSERT(ohCreatePropStr(facet, &prop, "CELLCLASS", "MODULE"));

    /* get a few needed items */
    octCellPath = com_get_flag("OCT-CELL-PATH");
    if ((octCellView = com_get_flag("OCT-CELL-VIEW")) == NIL(char)) {
    octCellView = "physical";
    }

    OH_ASSERT(ohGetOrCreateBag(facet, &bag, "INSTANCES"));

    initialize_net_table();

#ifdef SIS
    /* Write out the clock information */
    write_oct_clocks(*network, facet, 0 /* Use long names */);
    is_mapped = lib_network_is_mapped(*network);
#endif /* SIS */

    /* loop over all nodes in the network from the inputs */

    nodes = network_dfs(*network);

    for (i = 0; i < array_n(nodes); i++) {
    node = array_fetch(node_t *, nodes, i);
#ifdef SIS
    /* Skip the internal node corresponding to mapped latches */
    if (is_mapped && node->type == INTERNAL &&
        lib_gate_type(lib_gate_of(node)) != COMBINATIONAL){
        continue;
    }

    if ( (latch = latch_from_node( node )) != NIL(latch_t) ) {
        /* Skip the PO corrsponding to the latch input */
        if (node->type == PRIMARY_OUTPUT) continue;

        create_latch(facet, node, latch );
        continue;
    }
#endif /* SIS */
    funct = node_function(node);

    switch (funct) {

    case NODE_PO:
#ifdef SIS
        if (network_is_real_po(*network, node ) ) {
        create_primary_output( facet, node );
        }
#else
        create_primary_output( facet, node );
#endif /* SIS */
        break;
    case NODE_PI:
#ifdef SIS
        if (network_is_real_pi(*network, node ) ) {
        create_primary_input( facet, node );
        }
#else
        create_primary_input( facet, node );
#endif /* SIS */
        break;
    default:


        nodeName = node_oct_name(node);

        oct_lib_get_gate_cell_and_view(node, facet, &masterCell, &masterView);

        /* instantiate the master */
        OH_ASSERT(ohCreateInst(facet, &instance, nodeName, masterCell, masterView));
        OH_ASSERT(octAttach(&bag, &instance));

        /* connect up the inputs */

        foreach_fanin(node, input, fanin) {
        inTermName = oct_lib_get_pin_name(node, input);
        inNetName = node_oct_name(fanin);
        oct_connect_instance_term( facet, &instance, inTermName, inNetName );
        }

        termName = oct_lib_get_out_pin_name(node, 0);
        oct_connect_instance_term( facet, &instance, termName, nodeName );

        attachLogicFunction(&term, node);
    }
    }

    createInterface(facet);

    OH_ASSERT(octCloseFacet(facet));

    /* free up some space */
    st_free_table(NetTable);

    return;
}



static jmp_buf sis_oct_error_buf;

static void
sis_oct_error_handler(pkgName, code, message)
char *pkgName;
int code;
char *message;
{
    (void) fprintf(stderr, "%s: error (%d)\n\t%s\n", pkgName, code, message);
    (void) longjmp(sis_oct_error_buf, -1);
    return;
}


int
external_write_oct(network, argc, argv)
    network_t **network;
    int argc;
    char **argv;
/*
 * write_oct: write a network out into an Oct facet
 *
 */
{
    octObject facet;


    if (argc != 2) {
    (void) fprintf(siserr, "usage: write_oct cell[:view]\n");
    return(1);
    }

    octBegin();

    if (setjmp(sis_oct_error_buf) == 0) {
    errPushHandler(sis_oct_error_handler);

    (void) ohUnpackDefaults(&facet, "w", ":logic:contents");
    if (ohUnpackFacetName(&facet, argv[1]) != OCT_OK) {
        errRaise(SIS_PKG_NAME, OCT_ERROR, "usage: write_oct cell[:view]");
    }

    error_init();
    write_oct(network, &facet);
    errPopHandler();
    } else {
    return 1;		/* Error detected. */
    }
    
    octEnd();
    
    return 0;			/* OK. */
}

#endif /* OCT */
