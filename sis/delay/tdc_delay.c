
/*
 *   Utility functions for timing driven cofactor.
 *    Author: Paul Gutwin     Date: 12/03/90
 */

#include "delay_int.h"
#include "tdc_int.h"
#include "sis.h"

#define TDC_MAXLINE 255
/* Macro Definition */
#define LOG2(val) (log10(val) / .30103)

static double delay_eqn();

static double tdc_get_slack();

static double tdc_get_flslack();

static double do_delay_to_output();

static double tdc_group_delay();

static network_t *tdc_bdd_to_network();

static void do_girdle();

static void tdc_free_info();

static void tdc_sort_inputs();

static void do_tdc_bdd_node_list();

static tdc_info_t *tdc_alloc_info();

static st_table *tdc_bdd_node_list();

static int girdle();

static int count_bdd_fct_lines();

static int do_count_bdd_fct_lines();

static struct tdc_param_struct {
    double K_0;
    double K_1;
    double K_2;
} tdc_model_params;

static struct tdc_param_struct tdc_default_parameters = {
        /* K_0 */ 0.35,
        /* K_1 */ 0.6,
        /* K_2 */ 0.5};

/*
 *  Compute delay values based on TDC model.
 *  Option is a flag that indicates which level of the tdc model
 *  to use.  options = 0 indicates the simplest tdc model should
 *  be used.  options = 1 indicates that the fanout of the node
 *  should modify the delay values.
 *   options = 2 indicates that complexity and fanout are considers.
 */

void compute_tdc_params(node_t *node, int options) {

    int          i, j;
    double       fanout_delay, temp_delay;
    tdc_info_t   *info;
    pin_member_t *current_member, *new_member;
    pin_group_t  *current_group, *new_group;

#ifdef TDC_DEBUG
    (void)fprintf(sisout, "->Computing tdc params for node %s\n",
                  node_name(node));
#endif

    info = tdc_alloc_info(options);
    tdc_sort_inputs(node, info);

    /* calculate the delay for each group to the output  */
    info->group_list_head->delay_to_output =
            do_delay_to_output(info->group_list_head);

    /*
     * If options = 1,2,  we need to get the fanout count and lump that delay
     * at the output.
     */
    if ((options == 1) || (options == 2)) {
        fanout_delay = ((double) node_num_fanout(node)) * .2;
    } else {
        fanout_delay = 0;
    }

    /*
     * calculate the delay for each of the input pins of this node
     * and set up the pin_params
     */
    current_group = info->group_list_head;
    while (current_group != NULL) {
        current_member = current_group->latest_pin;
        for (i         = 0; i < current_group->group_size; i++) {
            j          = current_member->pin_number;
            temp_delay = current_group->delay_to_output + fanout_delay;
            DELAY(node)->pin_params[j]->block.rise = temp_delay;
            DELAY(node)->pin_params[j]->block.fall = temp_delay;
            DELAY(node)->pin_params[j]->drive.rise = 1;
            DELAY(node)->pin_params[j]->drive.fall = 1;
            DELAY(node)->pin_params[j]->phase      = PHASE_INVERTING;
#ifdef TDC_DEBUG
            (void)fprintf(sisout, "PARAMETERS FROM INPUT %d\n", j);
            (void)fprintf(sisout, "b = %g load = %g\n", temp_delay,
                          DELAY(node)->pin_params[j]->load);
#endif
            current_member = current_member->prev;
        }
        current_group  = current_group->next;
    }
    DELAY(node)->pin_params_valid = DELAY_MODEL_TDC;

#ifdef DEBUG2
    /*
    printf("Slack Difference\n");
    for (current_group = info->group_list_head, j = 1; current_group != NULL;
        current_group = current_group->next, j++){
        (void)fprintf(sisout, "Slack difference for group %d = %f\n", j,
        (current_group->total_slack - tdc_get_flslack(current_group)));
    }
    */
#endif

    /*
     * OK, all done! Free the BDDs, and all other structures!!
     */
    tdc_free_info(info);
}

/*
 * Allocate and initialize the data structure for the tdc routines
 */
static tdc_info_t *tdc_alloc_info(int options) {
    tdc_info_t *info;

    info = ALLOC(tdc_info_t, 1);
    info->options         = options;
    info->bdd_mgr         = NULL;
    info->my_bdd          = NULL;
    info->sorted_list     = NULL;
    info->group_list_head = NULL;

    return info;
}

static void tdc_free_info(tdc_info_t *info) {
    pin_member_t *current_member, *new_member;
    pin_group_t  *current_group, *new_group;

    ntbdd_end_manager(info->bdd_mgr);
    st_free_table(info->leaves);
    for (current_member = info->sorted_list; current_member != NULL;) {
        new_member = current_member->next;
        FREE(current_member);
        current_member = new_member;
    }
    for (current_group  = info->group_list_head; current_group != NULL;) {
        new_group = current_group->next;
        FREE(current_group);
        current_group = new_group;
    }
    FREE(info);
}

/*
 * Interface routine: Allows the user to set different values for
 * the coefficients.
 */
void delay_set_tdc_params(char *fname) {
    FILE   *fp;
    int    i, found_params;
    double d0, d1, d2;
    char   *real_filename, line[TDC_MAXLINE];

    if (fname != NIL(char)) {
        fp = com_open_file(fname, "r", &real_filename, 0);
        if (fp != NIL(FILE)) {
            found_params = FALSE;
            while (fgets(line, TDC_MAXLINE, fp) != NULL) {
                i = sscanf(line, "%lf %lf %lf", &d0, &d1, &d2);
                if (i == 3) {
                    found_params = TRUE;
                    break;
                }
            }
            if (found_params) {
                tdc_model_params.K_0 = d0;
                tdc_model_params.K_1 = d1;
                tdc_model_params.K_2 = d2;
            } else {
                (void) fprintf(siserr, "Could not parse parameters: Using defaults\n");
                tdc_model_params = tdc_default_parameters;
            }
            (void) fclose(fp);
        } else {
            (void) fprintf(siserr, "ERROR: cannot open %s for TDC parameters\n",
                           real_filename);
            (void) fprintf(siserr, "       Using the default TDC parameters\n");
            tdc_model_params = tdc_default_parameters;
        }
        FREE(real_filename);
    } else {
        tdc_model_params = tdc_default_parameters;
    }

    (void) fprintf(sisout, "TDC parameters are %f %f %f\n", tdc_model_params.K_0,
                   tdc_model_params.K_1, tdc_model_params.K_2);
}

/*
 *  Count function lines crossing a cut in the specified bdd from the specific
 *  group.
 */

static int count_bdd_fct_lines(bdd_t *my_bdd, pin_group_t *source_group) {
    bdd_t    *then_node;
    bdd_t    *my_bdd_top_node;
    st_table *visited;
    int      fct_line_count, first_src, last_src, first_dest, last_dest;

#ifdef TDC_DEBUG
    (void)fprintf(sisout, "Computing bdd function lines for bdd %x\n", my_bdd);
#endif
    /* check to see if this is the last group.  If so, return zero. */
    if (source_group->next == NULL) {
#ifdef TDC_DEBUG
        (void)fprintf(sisout, "source_group->next == NULL\n");
#endif
        return (0);
    }

    /* initialize stuff  */
    visited    = st_init_table(st_ptrcmp, st_ptrhash);
    first_src  = source_group->first;
    last_src   = source_group->last;
    first_dest = (source_group->next)->first;
    last_dest  = (source_group->next)->last;

    /* count the children out of this group */
    fct_line_count = do_count_bdd_fct_lines(my_bdd, first_src, last_src,
                                            first_dest, last_dest, visited);

    st_free_table(visited);

    return fct_line_count;
}

/*
 *  Recursive routine to count the number of nodes in the destination group
 *  that are touched by a function line out of the source group.
 *
 *  First, check to see if we have been visited.  If so, return 0.
 *   If we are one or zero, return 0.
 *  Next, see if we are in the destination group and the caller is in the
 *   source group.  If so, mark self as visited, and return 1.
 *  If we are in the srouce group, set source group flag and recurse.
 *  If not in the source group, clear source group flag and recurse.
 *
 */

static int do_count_bdd_fct_lines(bdd_t *my_bdd_node, int first_src, int last_src, int first_dest, int last_dest,
                                  st_table *visited, boolean source_grp) {
    boolean  in_src;
    bdd_node *bdd_reg;
    bdd_t    *then_node, *else_node;
    int      then_count, else_count, return_count, my_bdd_node_id;

    bdd_reg = bdd_get_node(my_bdd_node, &in_src);

    /*
     * If the node has been visited then we dont do anything
     */
    if (st_is_member(visited, (char *) bdd_reg)) { /* yes if true */
#ifdef TDC_DEBUG
        (void)fprintf(sisout, "Found visited node, %x\n", bdd_reg);
#endif
        return (0);
    }
    /*
     * am I a one or zero node? If so, bail out!
     */
    if (bdd_is_tautology(my_bdd_node, 1) || bdd_is_tautology(my_bdd_node, 0)) {
        return (0);
    }

    my_bdd_node_id = bdd_top_var_id(my_bdd_node);

    /*
     * Am I in the dest. group and the source_grp flag set?
     * If so, mark self as visited, and return count of 1
     */
    if (first_dest <= (int) my_bdd_node_id && last_dest >= (int) my_bdd_node_id &&
        source_grp) {
#ifdef TDC_DEBUG
        (void)fprintf(sisout, "Found target node, %x\n", bdd_reg);
#endif
        return_count = 1;
        (void) st_insert(visited, (char *) bdd_reg, (char *) &return_count);
        return (1);
    }

    /* Am I in the source group? */
    in_src =
            (first_src <= (int) my_bdd_node_id) && (last_src >= (int) my_bdd_node_id);

    /* get the children pointers */
    then_node = bdd_then(my_bdd_node);
    else_node = bdd_else(my_bdd_node);

    /*
     * now go for the recursion
     */
    then_count = do_count_bdd_fct_lines(then_node, first_src, last_src,
                                        first_dest, last_dest, visited, in_src);
    else_count = do_count_bdd_fct_lines(else_node, first_src, last_src,
                                        first_dest, last_dest, visited, in_src);

    /* sum up the results and return */
    return_count = then_count + else_count;
#ifdef DEBUG2
    (void)fprintf(sisout, "Return count = %d\n", return_count);
#endif
    return (return_count);
}

static double do_delay_to_output(pin_group_t *current_group) {
    double temp_delay;

    if (current_group == NULL)
        return (0);

    temp_delay =
            current_group->group_delay + do_delay_to_output(current_group->next);

    /* HACK!  (Sorry about that) */
    current_group->delay_to_output = temp_delay;

    return (temp_delay);
}

/*
     This routine will calculate the total slack for the group specified.
     If new_pin is specified, it will be treated as the latest pin in the
     group.
*/
static double tdc_get_slack(pin_group_t *group, pin_member_t *new_pin) {
    int          i;
    double       slack;
    pin_member_t *last_node, *current_node;
    delay_node_t *last_node_delay, *current_node_delay;

    slack = 0;
    if (new_pin != NULL) {
        last_node_delay = DELAY(new_pin->source_node);
        current_node    = group->latest_pin;
    } else {
        last_node       = group->latest_pin;
        current_node    = last_node->prev;
        last_node_delay = DELAY(last_node->source_node);
    }

    for (i = 1; i < group->group_size; i++) {
        current_node_delay = DELAY(current_node->source_node);
        slack += (last_node_delay->arrival.rise - current_node_delay->arrival.rise);
        current_node       = current_node->prev;
    }

    if (group->prev != NULL) {
        slack += (last_node_delay->arrival.rise - (group->prev)->arr_est);
    }

    return (slack);
}

/*
     This routine will calculate the slack for the group
     taking into account the number of function lines going from one group
     to another.
*/
static double tdc_get_flslack(bdd_t *bdd, pin_group_t *group) {
    int          i;
    double       slack, prev_group_output_arrival;
    pin_member_t *last_node, *current_node;
    delay_node_t *last_node_delay, *prev_last_node_delay, *current_node_delay;

    slack           = 0;
    last_node       = group->latest_pin;
    current_node    = last_node->prev;
    last_node_delay = DELAY(last_node->source_node);

    /*
     * Here we just calculate the slack due to the variables in the group
     */
    for (i = 1; i < group->group_size; i++) {
        current_node_delay = DELAY(current_node->source_node);
        slack += (last_node_delay->arrival.rise - current_node_delay->arrival.rise);
        current_node       = current_node->prev;
    }

    /*
     * Here we calculate the slack due to the function lines crossing
     */
    if (group->prev != NULL) {
        prev_last_node_delay = DELAY(group->prev->latest_pin->source_node);
        prev_group_output_arrival =
                prev_last_node_delay->arrival.rise + group->prev->group_delay;
        slack += (group->prev->fct_count) *
                 (last_node_delay->arrival.rise - prev_group_output_arrival);
    }

    return (slack);
}

/*
    This routine calculates the delay for a particular mux based on the
    number of function lines crossing into the function.
    If options = 2, we need to worry about the complexity.
*/
static double tdc_group_delay(bdd_t *bdd, pin_group_t *group, int options) {

    pin_group_t *prev_group;
    int         complexity, fct_line_count, temp_fct_count;
/*
     If this is not the first group, go get the function line count for
     the previous group, and save it in the previous group structure.
*/
#ifdef TDC_DEBUG
    (void)fprintf(sisout, "Computing delay for group %d -> %d\n", group->first,
                  group->last);
#endif
    prev_group     = group->prev;
    fct_line_count = 0;
    if (prev_group != NULL) {
        fct_line_count = count_bdd_fct_lines(bdd, prev_group);
        prev_group->fct_count = fct_line_count;
    }
    /*
         This is a hack.  I need to keep the function line count correct, and
         this is a good place to do it.  The above case will take care of
         everyting except the first group.  So, let's just take care of this
         little piece of housekeeping
    */
    temp_fct_count = count_bdd_fct_lines(bdd, group);
    group->fct_count = temp_fct_count;
#ifdef TDC_DEBUG
    (void)fprintf(sisout, "fct_line_count = %d\n", temp_fct_count);
#endif

    /*
         If options is 2, we should get the girdle size and use it as a measure of
         the complexity ONLY for the first group.
    */
    complexity = 1;
    if ((2 == options) && (prev_group == NULL)) {
        complexity = girdle(bdd, group);
    }
    group->girdle = complexity;

    group->group_delay =
            delay_eqn((group->group_size + fct_line_count), complexity);

    return (group->group_delay);
}

/*
     This routine will calculate the delay of the MUX based on the number
     of input pins.
*/
static double delay_eqn(int count, int complexity) {
    double delay, t1, t2, t3;

    t1    = t2 = 0;
    if (complexity > 1) {
        t2 = tdc_model_params.K_2 * LOG2(complexity);
    }
    if (count > 1) {
        t1 = tdc_model_params.K_1 * LOG2(count);
    }
    delay = tdc_model_params.K_0 + t1 + t2;

    return (delay);
}

/*
    This is the recursive part of the girdle counting routine.
*/
static void do_girdle(bdd_t *bdd, int first_src, int last_src, st_table *visited, st_table *counter) {
    bdd_t    *then_node, *else_node;
    bdd_node *bdd_reg;
    int      bdd_node_id, count;
    boolean  dummy;

    bdd_reg = bdd_get_node(bdd, &dummy);

    /* am I a one or zero node? If so, bail out! */
    if (bdd_is_tautology(bdd, 1) || bdd_is_tautology(bdd, 0)) {
        return;
    }
    /* have I been visited?  If so, bail out! */
    if (st_is_member(visited, (char *) bdd_reg)) { /* yes if true */
        return;
    }

    bdd_node_id = bdd_top_var_id(bdd);

    /* don't care about anything below the bottom of our group */
    if (bdd_node_id > last_src) {
        return;
    }

    /* mark self as visited, and increment count for this node_id */
    (void) st_insert(visited, (char *) bdd_reg, (char *) bdd_node_id);
    count = 0;
    (void) st_lookup_int(counter, (char *) bdd_node_id, &count);
    count++;
    (void) st_insert(counter, (char *) bdd_node_id, (char *) count);

    /* get the children pointers */
    then_node = bdd_then(bdd);
    else_node = bdd_else(bdd);

    /* recurse on the children */
    do_girdle(then_node, first_src, last_src, visited, counter);
    do_girdle(else_node, first_src, last_src, visited, counter);
}

/*
    This routine returns the size of the girdle of the group of interest
*/
static int girdle(bdd_t *bdd, pin_group_t *group) {
    st_table *visited, *counter;
    int      i, count, girdle_count, first_src, last_src;

    /* initialize stuff  */
    visited   = st_init_table(st_ptrcmp, st_ptrhash);
    counter   = st_init_table(st_numcmp, st_numhash);
    first_src = group->first;
    last_src  = group->last;

    /* Go do the recursion step */
    do_girdle(bdd, first_src, last_src, visited, counter);

    /* count up the results */
    girdle_count = 0;
    for (i       = first_src; i <= last_src && (last_src > 0); i++) {
        assert(st_lookup_int(counter, (char *) i, &count));
        if (count > girdle_count)
            girdle_count = count;
    }

    /* return what we found */
    st_free_table(visited);
    st_free_table(counter);
    return girdle_count;
}

/*
 *  Sort inputs for node based on arrival times.
 *  (Timing Driven Cofactor)
 *  Option (in tdc_info_t) is a flag that indicates which level of the tdc model
 *  to use.  options = 0 indicates the simplest tdc model should
 *  be used.  options = 1 indicates that the fanout of the node
 *  should modify the delay values.
 *   options = 2 indicates that complexity and fanout are considers.
 */

static void tdc_sort_inputs(node_t *node, tdc_info_t *info) {
    int          i, j, fanin_count, fct_count, fanout_count, group_count;
    double       new_group_slack, temp_delay;
    pin_member_t *sorted_list, *new_member, *current_member, *last_pin;
    pin_group_t  *group_list_head, *current_group, *new_group;

    node_t       *fanin;
    st_table     *leaves;
    bdd_manager  *bdd_mgr;
    bdd_t        *my_bdd;
    delay_time_t new_arr_time;
    delay_node_t *next_node_delay, *node_delay, *fanin_delay;

#ifdef TDC_DEBUG
    (void)fprintf(sisout, "->Sorting and grouping inputs (tdc) for node %s\n",
                  node_name(node));
#endif

    /* Create a sorted list of input signals */
    sorted_list = NULL;
    node_delay  = NIL(delay_node_t);

    fanin_count = node_num_fanin(node);
    foreach_fanin(node, i, fanin) {
        /* first get some storage for next input signal */
        new_member = ALLOC(pin_member_t, 1);
        new_member->next        = NULL;
        new_member->source_node = fanin;
        new_member->pin_number  = i;
        /* don't forget where the head is and other stuff... */
        if (sorted_list == NULL) {
            sorted_list = new_member;
            new_member->prev = NULL;
        }

        if (new_member == sorted_list) {
            node_delay = DELAY(new_member->source_node);
            continue;
        }
        /*
         * find the right place to put it
         */
        current_member          = sorted_list; /* start at the head */
        while (current_member != NULL) {
            node_delay  = DELAY(current_member->source_node);
            fanin_delay = DELAY(fanin);
            if (fanin_delay->arrival.rise >= node_delay->arrival.rise) {
                if (current_member->next == NULL) {
                    current_member->next = new_member;
                    new_member->prev     = current_member;
                    break;
                } else {
                    next_node_delay = DELAY((current_member->next)->source_node);
                    if (fanin_delay->arrival.rise <= next_node_delay->arrival.rise) {
                        new_member->next             = current_member->next;
                        (current_member->next)->prev = new_member;
                        current_member->next         = new_member;
                        new_member->prev             = current_member;
                        break;
                    } /* if (delay fanin < delay next member) */
                    current_member = current_member->next; /* not this one */
                }
            } else { /* smaller than the smallest */
                sorted_list->prev = new_member;
                new_member->next  = sorted_list;
                sorted_list = new_member;
                sorted_list->prev = NULL;
                break;
            }
        }
    }

    /*
     *  Input signals now sorted in sorted_list.  Create the BDD so we can
     *  make up the groups.
     *  Since the sorted list in earliest to latest, and we want a BDD with
     *  the latest node at the top of the BDD, we need to stick the nodes
     *  in the leaves table in reverse order.
     */

    /* place the sorted list in a hash table for bdd construction */
    leaves             = st_init_table(st_ptrcmp, st_ptrhash);
    current_member     = sorted_list;
    while (current_member != NULL && current_member->next != NULL)
        current_member = current_member->next;
    i                  = 0;
    while (current_member != NULL) {
        (void) st_insert(leaves, (char *) current_member->source_node, (char *) i);
        current_member->pin_number = i;
        i++;
        last_pin       = current_member; /* save this for later */
        current_member = current_member->prev;
    }

    /* now create the BDD!  */
    bdd_mgr = ntbdd_start_manager(st_count(leaves));
    my_bdd  = ntbdd_node_to_bdd(node, bdd_mgr, leaves); /* Finally! */

    /* We are currently doing the grouping based on a very simple hueristic:
         Create a new group if the arrival time at the output of the tdc is
         less than the arrival time of the signal in question.  Otherwise,
         keep it in the same group.
    */
    /*
     * Now we need to make up the groups.
     * Put the first pin in first group, and all others in the second group.
     */
    group_list_head = ALLOC(pin_group_t, 1);
    group_list_head->next = NULL;
    current_group = group_list_head;
    current_group->prev = NULL;
    current_member = sorted_list;
    /*
     * Put the first input pin in the first group
     */
    if (current_member != NULL) {
        current_group->group_size  = 1;
        current_group->first       = current_member->pin_number;
        current_group->last        = current_member->pin_number;
        current_group->latest_pin  = current_member;
        current_group->arr_est     = 0;
        current_group->total_slack = 0;
        current_member = current_member->next;
        group_count    = 1;
    }

    /* put all the other pins in the last group */
    if (current_member != NULL) {
        new_group = ALLOC(pin_group_t, 1);
        new_group->next        = NULL;
        new_group->prev        = current_group;
        new_group->group_size  = fanin_count - 1;
        if (current_member->next != NULL) {
            new_group->first = current_member->next->pin_number;
        } else {
            new_group->first = 0;
        }
        new_group->last        = 0;
        new_group->latest_pin  = last_pin;
        new_group->total_slack = 0;
        new_group->arr_est     = 0;
        current_group->next    = new_group;
        group_count++;
    } else {
        new_group = NULL;
    }

    /* Get the delays for these two groups to start with */
    current_group->group_delay =
            tdc_group_delay(my_bdd, current_group, info->options);
    current_group->arr_est =
            (node_delay == NIL(delay_node_t) ? DELAY_NOT_SET
                                             : node_delay->arrival.rise) +
            current_group->group_delay;
    if (new_group != NULL) {
        new_group->group_delay = tdc_group_delay(my_bdd, new_group, info->options);
        new_group->arr_est     = node_delay->arrival.rise + new_group->group_delay;
    }

    /* now start looping through the sorted input pins */
    while (current_member != NULL) {
        node_delay = DELAY(current_member->source_node);
        if (node_delay->arrival.rise > current_group->arr_est) {
            /*
             * create new group for signal
             */
            new_group = ALLOC(pin_group_t, 1);
            new_group->next           = current_group->next;
            current_group->next->prev = new_group; /* point the catchall group back */
            new_group->prev           = current_group;
            new_group->group_size     = 1;
            new_group->first          = current_member->pin_number;
            new_group->last           = current_member->pin_number;
            new_group->latest_pin     = current_member;
            new_group->total_slack    = 0;
            new_group->arr_est        = 0;
            current_group->next       = new_group;
            current_group = current_group->next;
            group_count++;
            /*
             * move the new pin out of the catchall group
             */
            current_group->next->first--;

            /* get delay for this new group */
            current_group->group_delay =
                    tdc_group_delay(my_bdd, current_group, info->options);
            current_group->arr_est =
                    node_delay->arrival.rise + current_group->group_delay;
        } else {
            /* move the new pin out of the catchall group to this group */
            current_group->last = current_member->pin_number;
            current_group->group_size++;
            current_group->latest_pin = current_member;
            current_group->next->first--;

            /* get delay for this new group */
            current_group->group_delay =
                    tdc_group_delay(my_bdd, current_group, info->options);
            current_group->arr_est =
                    node_delay->arrival.rise + current_group->group_delay;
        }
        current_member = current_member->next;
    } /* while(current_member) */

    /*
     * At this point, current_group->next should point to the empty
     * catchall group.  This needs to be removed
     */
    FREE(current_group->next);
    current_group->next = NULL;
    group_count--;

    /*
     * We want to loop through all the groups and get good arrival times
     * and function line counts on everything
     */
    current_group = group_list_head;
    while (current_group != NULL) {
        current_group->group_delay =
                tdc_group_delay(my_bdd, current_group, info->options);
        current_group->arr_est =
                (node_delay == NIL(delay_node_t) ? DELAY_NOT_SET
                                                 : node_delay->arrival.rise) +
                current_group->group_delay;
        current_group = current_group->next;
    }

    /* get the girdle width of each group */
/*
    if (2 == info->options)  {
        current_group = group_list_head;
        while (current_group != NULL) {
            current_group->girdle = girdle(my_bdd, current_group);
            current_group = current_group->next;
            }
        }
*/
#ifdef TDC_DEBUG
    j = 1;
    (void)fprintf(sisout, "Created %d groups for this node.\n", group_count);
    current_group = group_list_head;
    while (current_group != NULL) {
      (void)fprintf(sisout, "Group %d\n", j);
      j++;
      (void)fprintf(sisout, "Select group size for this group: %d\n",
                    current_group->group_size);
      (void)fprintf(sisout, "   Starting pin for select group: %d\n",
                    current_group->first);
      (void)fprintf(sisout, "     Ending pin for select group: %d\n",
                    current_group->last);
      (void)fprintf(sisout, "Function line count for this group: %d\n",
                    current_group->fct_count);
      (void)fprintf(sisout, "       Girdle width for this group: %d\n",
                    current_group->girdle);
      (void)fprintf(sisout, "Delay for this group: %f\n",
                    current_group->group_delay);
      current_member = current_group->latest_pin;
      for (i = 0; i < current_group->group_size; i++) {
        node_delay = DELAY(current_member->source_node);
        (void)fprintf(sisout, " Pin %s arrival time: %f\n",
                      node_name(current_member->source_node),
                      node_delay->arrival.rise);
        current_member = current_member->prev;
      }
      current_group = current_group->next;
    }
#endif

    /* OK, all done! */
    /* return the lists and bdd stuff to caller. */
    info->bdd_mgr         = bdd_mgr;
    info->my_bdd          = my_bdd;
    info->sorted_list     = sorted_list;
    info->group_list_head = group_list_head;
    info->leaves          = leaves;
}

/*
 * routine to convert bdd specified in info->my_bdd to a network of mux's
 * as defined by the equivelence groups.
 */
static network_t *tdc_bdd_to_network(tdc_info_t *info, network_t *network) {
    int          i, j, node_id;
    char         *pi_name;
    lsGen        node_gen;
    st_generator *gen;
    st_table     *id2node_list, *visited, *leaves_bar;
    node_t       *node, *node_pi, *node_g, *node_f;
    pin_group_t  *current_group;
    bdd_t        *my_bdd;
    bdd_node     *bdd_reg;

    array_t   *node_names, *node_array;
    network_t *new_net;

#ifdef TDC_DEBUG
    printf("Source BDD\n");
    bdd_print(info->my_bdd);
#endif

    /* initialize stuff  */
    node_names = array_alloc(char *, 0);
    st_foreach_item_int(info->leaves, gen, (char **) &node, &node_id) {
#ifdef TDC_DEBUG
        printf("node name %s is node number %d\n", node_name(node), node_id);
#endif
        array_insert(char *, node_names, node_id, node_name(node));
    }

    new_net       = ntbdd_bdd_single_to_network(info->my_bdd, "bdd_out", node_names);
    /* now collapse groups together */
    current_group = info->group_list_head;
    while (current_group != NULL && current_group->group_size > 0) {
        for (i        = current_group->last + 1; i <= current_group->first; i++) {
            pi_name = array_fetch(char *, node_names, i);
            node_pi = network_find_node(new_net, pi_name);
            foreach_fanout(node_pi, node_gen, node_g) {
                if (node_num_fanout(node_g) > 0) {
                    node_f = node_get_fanout(node_g, 0);
                }
#ifdef TDC_DEBUG
                printf(" Collapse %s into %s \n", node_name(node_g), node_name(node_f));
#endif
                if (node_f != NULL)
                    node_collapse(node_f, node_g);
            }
        }
        current_group = current_group->next;
    }

    network_sweep(new_net);
#ifdef TDC_DEBUG
    node_array = network_dfs(new_net);
    printf("Nodes in new BDD network\n");
    node_array = network_dfs(new_net);
    for (i = 0; i < array_n(node_array); i++) {
      node = array_fetch(node_t *, node_array, i);
      node_print(sisout, node);
    }
    array_free(node_array);
#endif

    array_free(node_names);
    return (new_net);
}

/*
    Returns a hash table of bdd varible id's to array_t lists of bdd_node
    for each group listed in info->group_list_head.  If a node in the
    BDD is reaced by an arc that comes from another group, an entry will
    be made in the hash table.
*/
static st_table *tdc_bdd_node_list(tdc_info_t *info, bdd_t *my_bdd) {
    int         i, num_vars;
    bdd_node    *bdd_reg;
    st_table    *new_table, *visited;
    pin_group_t *current_group;

    num_vars  = st_count(info->leaves);
    new_table = st_init_table(st_numcmp, st_numhash);
    visited   = st_init_table(st_ptrcmp, st_ptrhash);

    do_tdc_bdd_node_list(info, my_bdd, visited, new_table, -1);

    st_free_table(visited);

    return (new_table);
}

static void
do_tdc_bdd_node_list(tdc_info_t *info, bdd_t *my_bdd, st_table *visited, st_table *new_table, int parent_id) {
    int         dummy, bdd_node_id, parent_group_num, current_group_num, group_num;
    bdd_t       *then_bdd_node, *else_bdd_node;
    bdd_node    *bdd_reg;
    array_t     *curr_array;
    pin_group_t *current_group;
    boolean     bdummy;

    bdd_reg = bdd_get_node(my_bdd, &bdummy);
    if ((!st_lookup(visited, (char *) bdd_reg, (char **) &dummy)) &&
        !(bdd_is_tautology(my_bdd, 1) || bdd_is_tautology(my_bdd, 0))) {

        bdd_node_id      = bdd_top_var_id(my_bdd);
        current_group    = info->group_list_head;
        parent_group_num = current_group_num = group_num = 0;
        while (current_group != NULL) {
            if ((bdd_node_id <= current_group->first) &&
                (bdd_node_id >= current_group->last)) {
                current_group_num = group_num;
            }
            if ((parent_id <= current_group->first) &&
                (parent_id >= current_group->last)) {
                parent_group_num = group_num;
            }
            group_num++;
            current_group = current_group->next;
        }

        if (current_group_num != parent_group_num) {
            /* Add current node */
            if (st_lookup(new_table, (char *) bdd_node_id, (char **) &curr_array)) {
                array_insert_last(bdd_t *, curr_array, my_bdd);
                (void) st_insert(visited, (char *) bdd_reg, (char *) dummy);
            } else {
                curr_array = array_alloc(bdd_t *, 0);
                array_insert_last(bdd_t *, curr_array, my_bdd);
                (void) st_insert(new_table, (char *) bdd_node_id, (char *) curr_array);
                (void) st_insert(visited, (char *) bdd_reg, (char *) dummy);
            }
        }

        /* get the children pointers and recurse on them */
        then_bdd_node = bdd_then(my_bdd);
        else_bdd_node = bdd_else(my_bdd);
        do_tdc_bdd_node_list(info, then_bdd_node, visited, new_table, bdd_node_id);
        do_tdc_bdd_node_list(info, else_bdd_node, visited, new_table, bdd_node_id);
    }
}

/*
 * NOTE: the routine assumes that "delay_set_tdc_params()" has been called
 * before this routine to properly set the coefficients for the delay equations
 */
network_t *tdc_factor_network(network_t *network) {
    char         *name;
    lsGen        gen;
    tdc_info_t   *info;
    node_t       *curr_node, *new_node, *po, *pi, *new_pi, *new_po;
    double       val;
    delay_time_t temp;
    network_t    *new_net;

    delay_trace(network, DELAY_MODEL_TDC);

    info      = tdc_alloc_info(/* options */ 2);
    po        = network_get_po(network, 0);
    curr_node = node_get_fanin(po, 0);

    tdc_sort_inputs(curr_node, info);
    name     = util_strsav(node_long_name(po));
    new_net  = tdc_bdd_to_network(info, network);
    new_node = network_get_po(new_net, 0);
    network_change_node_name(new_net, new_node, name);

    /*
     * Also update the delay information at the primary inputs and outputs
     */
    val = delay_get_default_parameter(network, DELAY_WIRE_LOAD_SLOPE);
    delay_set_default_parameter(new_net, DELAY_WIRE_LOAD_SLOPE, val);

    foreach_primary_input(network, gen, pi) {
        new_pi = network_find_node(new_net, node_long_name(pi));
        if (new_pi != NIL(node_t)) {
            if (delay_get_pi_arrival_time(pi, &temp)) {
                delay_set_parameter(new_pi, DELAY_ARRIVAL_RISE, temp.rise);
                delay_set_parameter(new_pi, DELAY_ARRIVAL_FALL, temp.fall);
            }
            if (delay_get_pi_drive(pi, &temp)) {
                delay_set_parameter(new_pi, DELAY_DRIVE_RISE, temp.rise);
                delay_set_parameter(new_pi, DELAY_DRIVE_FALL, temp.fall);
            }
        }
    }
    foreach_primary_output(network, gen, po) {
        new_po = network_find_node(new_net, node_long_name(po));
        if (new_po != NIL(node_t)) {
            if (delay_get_po_required_time(po, &temp)) {
                delay_set_parameter(new_po, DELAY_REQUIRED_RISE, temp.rise);
                delay_set_parameter(new_po, DELAY_REQUIRED_FALL, temp.fall);
            }
            if (delay_get_po_load(po, &(temp.rise))) {
                delay_set_parameter(new_po, DELAY_OUTPUT_LOAD, temp.rise);
            }
        }
    }

    tdc_free_info(info);
    return (new_net);
}

#undef TDC_MAXLINE
