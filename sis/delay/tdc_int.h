#ifndef TDC_INT_H
#define TDC_INT_H
/*
 *   tdc model stuff.
 */

#include "bdd.h"
#include "node.h"

/* tdc_util.c declares */
int bdd_node_id();

void delay_set_tdc_params();

/* Structures for tdc delay model */
struct pin_member {
    struct pin_member *prev;
    node_t            *source_node;
    int               pin_number;
    struct pin_member *next;
};
typedef struct pin_member pin_member_t;

struct pin_group {
    struct pin_group  *prev;
    double            delay_to_output, group_delay, /* Delay for this multiplexer */
                      total_slack,                     /* Slack contribution for this group */
                      arr_est;                         /* Temporary estimate of arrival time */
    /* Use riseing edge */
    int               fct_count,  /* num. of function lines to next group */
                      group_size, /* number of pins in group */
                      first,      /* ID of first group member  */
                      last,       /* ID of last group member   */
    /* IDs are the same as in leaves */
                      girdle;     /* Girdle width */
    struct pin_member *latest_pin;
    struct pin_group  *next;
};
typedef struct pin_group  pin_group_t;

struct tdc_info {
    int          options;                  /* Used to pass options around	*/
    pin_group_t  *group_list_head; /* List of pin groups */
    pin_member_t *sorted_list;    /* Sorted list of pins */
    bdd_manager  *bdd_mgr;         /* BDD manager */
    bdd_t        *my_bdd;                /* BDD for node indicated node */
    st_table     *leaves;             /* relates node to id number */
};
typedef struct tdc_info   tdc_info_t;
#endif