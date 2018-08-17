
#include "sis.h"
#include "../include/pld_int.h"

/*	apply, straight from bryant's paper	
*/

/* table needed in applyStep */
st_table *app_table;

typedef struct two_key_defn {
    int field1;
    int field2;
}        TWO_KEY, *TWO_KEY_PTR;

/*extern ACT_VERTEX_PTR applyStep();
extern int two_cmp();
extern int two_hash();
extern enum st_retval applyCleanUp();*/

ACT_VERTEX_PTR
apply(v1, v2, op)
        ACT_VERTEX_PTR v1, v2;
        int op;

{
    ACT_VERTEX_PTR u, actReduce(), applyStep();
    char           dummy;
    int            two_cmp(), two_hash();
    enum st_retval applyCleanUp();

    app_table = st_init_table(two_cmp, two_hash);
    u         = applyStep(v1, v2, op);
    st_foreach(app_table, applyCleanUp, &dummy);
    st_free_table(app_table);
    u = actReduce(u);
    return (u);
}

ACT_VERTEX_PTR
applyStep(v1, v2, op)
        ACT_VERTEX_PTR v1, v2;
        int op;

{
    char           *dummy;
    ACT_VERTEX_PTR u, vlow1, vhigh1, vlow2, vhigh2;
    TWO_KEY_PTR    key;

    key = ALLOC(TWO_KEY, 1);
    key->field1 = v1->id;
    key->field2 = v2->id;
    if (st_lookup(app_table, (char *) key, &dummy)) {
        u = (ACT_VERTEX_PTR) dummy;
        FREE(key);
        return (u);
    } else {
        u = ALLOC(ACT_VERTEX, 1);
        u->mark = FALSE;
        (void) st_insert(app_table, (char *) key, (char *) u);
        if (v1->value == NO_VALUE || v2->value == NO_VALUE) {
            u->value      = NO_VALUE;
            u->id         = 0;
            u->index_size = v1->index_size;
            u->index      = min(v1->index, v2->index);
            if (v1->index == u->index) {
                vlow1  = v1->low;
                vhigh1 = v1->high;
            } else {
                vlow1  = v1;
                vhigh1 = v1;
            }
            if (v2->index == u->index) {
                vlow2  = v2->low;
                vhigh2 = v2->high;
            } else {
                vlow2  = v2;
                vhigh2 = v2;
            }
            u->low  = applyStep(vlow1, vlow2, op);
            u->high = applyStep(vhigh1, vhigh2, op);
        } else {
            u->index      = v1->index_size;
            u->low        = NIL(act_t);
            u->high       = NIL(act_t);
            u->id         = 0;
            u->index_size = v1->index_size;
            switch (op) {
                case (AND + P11):
                    u->value = v1->value &&
                               v2->value;
                    break;
                case (AND + P10) : u->value = v1->value && !(v2->value);
                    break;
                case (AND + P01) : u->value = !(v1->value) && v2->value;
                    break;
                case (AND + P00) : u->value = !(v1->value) && !(v2->value);
                    break;

                case (OR + P11): u->value = v1->value || v2->value;
                    break;
                case (OR + P10) : u->value = v1->value || !(v2->value);
                    break;
                case (OR + P01) : u->value = !(v1->value) || v2->value;
                    break;
                case (OR + P00) : u->value = !(v1->value) || !(v2->value);
                    break;
                case (XOR)     :
                    u->value = (v1->value && !(v2->value)) ||
                               (v2->value && !(v1->value));
                    break;
                case (XNOR)    :
                    u->value = (v1->value && v2->value) ||
                               (!(v2->value) && !(v1->value));
                    break;
                default: break;
            }
        }
        return (u);
    }
}

int
two_cmp(key1, key2)
        char *key1, *key2;
{
    TWO_KEY_PTR nkey1, nkey2;

    nkey1 = (TWO_KEY_PTR) key1;
    nkey2 = (TWO_KEY_PTR) key2;

    if (nkey1->field1 == nkey2->field1) {
        if (nkey1->field2 == nkey2->field2) {
            return (0);
        } else {
            if (nkey1->field2 < nkey2->field2) return (-1);
            else return (1);
        }
    } else {
        if (nkey1->field1 < nkey2->field1) return (-1);
        else return (1);
    }
}

int
two_hash(key, modulus)
        char *key;
        int modulus;
{
    TWO_KEY_PTR nkey;
    int         bucket;

    nkey   = (TWO_KEY_PTR) key;
    bucket = (1009 * (nkey->field1) + 101 * (nkey->field2)) % modulus;
    return (bucket);
}

/*ARGSUSED*/
enum st_retval
applyCleanUp(key, value, arg)
        char *key;
        char *value;
        char *arg;

{
    TWO_KEY_PTR nkey;
    nkey = (TWO_KEY_PTR) key;
    FREE(nkey);
    return (ST_CONTINUE);
}

