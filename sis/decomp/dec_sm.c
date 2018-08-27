
#include "decomp.h"
#include "decomp_int.h"
#include "sis.h"

sm_matrix *dec_node_to_sm(f) node_t *f;
{
  sm_matrix *M;
  sm_element *element;
  sm_col *column;
  node_cube_t cube;
  node_literal_t literal;
  node_t *fanin;
  int i, j;

  M = sm_alloc();
  for (i = 0; i < node_num_cube(f); i++) {
    cube = node_get_cube(f, i);
    foreach_fanin(f, j, fanin) {
      literal = node_get_literal(cube, j);
      switch (literal) {
      case ONE:
        element = sm_insert(M, i, j);
        sm_put(element, 1);
        column = sm_get_col(M, j);
        sm_put(column, fanin);
        break;
      case ZERO:
        element = sm_insert(M, i, j);
        sm_put(element, 0);
        column = sm_get_col(M, j);
        sm_put(column, fanin);
        break;
      case TWO:
        break;
      default:
        fail("bad literal");
        /* NOTREACHED */
      }
    }
  }
  return M;
}

node_t *dec_sm_to_node(M) sm_matrix *M;
{
  node_t *and, * or, *lit, *temp, *fanin;
  sm_row *row;
  sm_col *col;
  sm_element *element;

  or = node_constant(0);
  sm_foreach_row(M, row) {
    and = node_constant(1);
    sm_foreach_row_element(row, element) {
      col = sm_get_col(M, element->col_num);
      fanin = sm_get(node_t *, col);
      switch (sm_get(int, element)) {
      case 1:
        lit = node_literal(fanin, 1);
        break;
      case 0:
        lit = node_literal(fanin, 0);
        break;
      default:
        fail("bad sm_element_data");
        /* NOTREACHED */
      }
      temp = node_and(and, lit);
      node_free(lit);
      node_free(and);
      and = temp;
    }
    temp = node_or(or, and);
    node_free(or);
    node_free(and);
    or = temp;
  }

  return or ;
}

void dec_sm_print(M) sm_matrix *M;
{
  sm_col *col;
  sm_element *element;
  int i, j;

  for (i = 0; i < M->nrows; i++) {
    for (j = 0; j < M->ncols; j++) {
      element = sm_find(M, i, j);
      if (element == NIL(sm_element)) {
        (void)fprintf(misout, "  .");
      } else {
        switch (sm_get(int, element)) {
        case 0:
          (void)fprintf(misout, "  0");
          break;
        case 1:
          (void)fprintf(misout, "  1");
          break;
        default:
          fail("bad sm_element_data");
          /* NOTREACHED */
        }
      }
    }
    (void)fprintf(misout, "\n");
  }

  sm_foreach_col(M, col) {
    (void)fprintf(misout, "%3s", node_name(sm_get(node_t *, col)));
  }
  (void)fprintf(misout, "\n");
}
