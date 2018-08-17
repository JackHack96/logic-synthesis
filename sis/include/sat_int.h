
typedef struct {
    int tos_var;
    int tos_cla;
    int tos_inc;
} stk_status_t;

extern bool sat_add_implication();

/*
 *  sorted, double-linked list insertion
 *
 *  type: object type
 *
 *  first, last: fields (in header) to head and tail of the list
 *  count: field (in header) of length of the list
 *
 *  next, prev: fields (in object) to link next and previous objects
 *  value: field (in object) which controls the order
 *
 *  newval: value field for new object
 *  e: an object to use if insertion needed (set to actual value used)
 */

#define sorted_insert(type, first, last, count, next, prev, value, newval, e) \
    if (last == 0) { \
    e->value = newval; \
    first = e; \
    last = e; \
    e->next = 0; \
    e->prev = 0; \
    count++; \
    } else if (last->value < newval) { \
    e->value = newval; \
    last->next = e; \
    e->prev = last; \
    last = e; \
    e->next = 0; \
    count++; \
    } else if (first->value > newval) { \
    e->value = newval; \
    first->prev = e; \
    e->next = first; \
    first = e; \
    e->prev = 0; \
    count++; \
    } else { \
    type *p; \
    for(p = first; p->value < newval; p = p->next) \
        ; \
    if (p->value > newval) { \
        e->value = newval; \
        p = p->prev; \
        p->next->prev = e; \
        e->next = p->next; \
        p->next = e; \
        e->prev = p; \
        count++; \
    } else { \
        e = p; \
    } \
    }

