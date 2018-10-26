
#include "sis.h"
#include "factor.h"
#include "factor_int.h"

static void ft_tree_print();
static void ft_and_print();
static void ft_or_print();
static void ft_leaf_print();
static void ft_inv_print();
static int  ft_len();
static int line_width;

void
factor_print(fp, f)
FILE *fp;
node_t *f;
{
    int length = 0;

    if (f->factored == NIL(char)) {
	factor_quick(f);
    }

    ft_tree_print(fp, f, (ft_t *) f->factored, &length);
}

static void
ft_tree_print(fp, node, f, len)
FILE *fp;
node_t *node;
ft_t *f;
int *len;
{
    switch ((int) f->type) {
    case FACTOR_0:
	(void) fprintf(fp, "-0-");
	break;
    case FACTOR_1:
	(void) fprintf(fp, "-1-");
	break;
    case FACTOR_AND:
	ft_and_print(fp, node, f, len);
	break;
    case FACTOR_OR:
	ft_or_print(fp, node, f, len);
	break;
    case FACTOR_INV:
	ft_inv_print(fp, node, f, len);
	break;
    case FACTOR_LEAF:
	ft_leaf_print(fp, node, f, len);
	break;
    default:
	(void) fprintf(miserr, "ft internal error, wrong type\n");
	exit(-1);
    }
}

static void
ft_leaf_print(fp, node, f, len)
FILE *fp;
node_t *node;
ft_t *f;
int *len;
{
    node_t *np, *node_get_fanin();

    np = node_get_fanin(node, f->index);
    if ( (strlen(node_name(np)) + (*len)) > line_width ) {
	(void) fprintf(fp, "\n\t");
	*len = 0;
    }

    (void) fprintf(fp, "%s", node_name(np));
    *len += strlen(node_name(np));
}

static void
ft_inv_print(fp, node, f, len)
FILE *fp;
node_t *node;
ft_t *f;
int *len;
{
    ft_tree_print(fp, node, f->next_level, len);
    (void) fprintf(fp, "'");
    *len += 1;
}

static void
ft_or_print(fp, node, f, len)
FILE *fp;
node_t *node;
ft_t *f;
int *len;
{
    ft_t *p;
    bool first = TRUE;

    for(p = f->next_level; p != NIL(ft_t); p = p->same_level) {
	if (first) {
	    first = FALSE;
	} else {
	    (void) fprintf(fp, " + ");
	    *len += 3;
	}
	ft_tree_print(fp, node, p, len);
    }
}

static void
ft_and_print(fp, node, f, len)
FILE *fp;
node_t *node;
ft_t *f;
int *len;
{
    ft_t *p;
    int first = TRUE;

    for(p = f->next_level; p != NIL(ft_t); p = p->same_level) {
	if (first) {
	    first = FALSE;
	} else {
	    (void) fprintf(fp, " ");
	    *len += 1;
	}
	if (p->type == FACTOR_OR) {
	    (void) fprintf(fp, "(");
	    ft_tree_print(fp, node, p, len);
	    (void) fprintf(fp, ")");
	} else {
	    ft_tree_print(fp, node, p, len);
	}
    }
}

static char *and_name = "AND";
static char *or_name = "OR";
static char *inv_name = "INV";
static char *zero_name = "-0-";
static char *one_name = "-1-";
static char *unknown_name = "???";
static char *ft_name();

void
ft_print(r, f, level)
node_t *r;
ft_t *f;
int level;
{
    ft_t *p;

    if (f != NIL(ft_t)) {

	(void) fprintf(misout, "%d: %s\t", level, ft_name(r, f));
	for (p=f->next_level; p != NIL(ft_t); p = p->same_level) {
	    (void) fprintf(misout, "\t%s", ft_name(r, p));
	}
	(void) fprintf(misout, "\n");

	for (p=f->next_level; p != NIL(ft_t); p = p->same_level) {
	    ft_print(r, p, level+1);
	}
    }
}

static char *
ft_name(r, f)
node_t *r;
ft_t *f;
{
    char *name;

    switch (f->type) {
    case FACTOR_0:
	name = zero_name;
	break;
    case FACTOR_1:
	name = one_name;
	break;
    case FACTOR_AND:
	name = and_name;
	break;
    case FACTOR_OR:
	name = or_name;
	break;
    case FACTOR_INV:
	name = inv_name;
	break;
    case FACTOR_LEAF:
	name = node_name(node_get_fanin(r, f->index));
	break;
    default:
	name = unknown_name;
    }

    return name;
}

/*
 * pretty printer of factored form
 */
/* ARGSUSED */
void
ft_len_init(r, f)
node_t *r;
ft_t *f;
{
    factor_traverse(r, ft_len, NIL(char), FACTOR_TRAV_POST_ORDER);
}

/* ARGSUSED */
static int
ft_len(r, f, stat)
node_t *r;
ft_t *f;
char *stat;
{
    ft_t *p;

    switch (f->type) {
    case FACTOR_0:
	f->len = 3;
	break;
    case FACTOR_1:
	f->len = 3;
	break;
    case FACTOR_LEAF:
	f->len = strlen(node_name(node_get_fanin(r, f->index)));
	break;
    case FACTOR_INV:
	f->len = f->next_level->len + 1;
	break;
    case FACTOR_AND:
	f->len = -1;
	for(p = f->next_level; p != NIL(ft_t); p = p->same_level) {
	    f->len = f->len + p->len + 1;
	}
	break;
    case FACTOR_OR:
	f->len = -1;
	for(p = f->next_level; p != NIL(ft_t); p = p->same_level) {
	    f->len = f->len + p->len + 3;
	}
	break;
    default:
	fail("Error: wrong factor type in ft_len\n");
    }

    return 0;
}

void
set_line_width()
{
    line_width = 60;
}
