
#include "extract_int.h"
#include "sis.h"

static void gen_all_rect(M, rect, col, record, state) sm_matrix *M;
rect_t *rect;
int col;
int (*record)();
char *state;
{
  register sm_col *c, *pcol, *pcolnext;
  register sm_element *p1;
  sm_matrix *M1;
  rect_t rect1;
  int already_generated;

  /* Loop for all columns in the matrix (from 'col' onwards) */
  sm_foreach_col(M, c) {
    if (c->length < 2 || c->col_num < col)
      continue;

    /* Copy all rows with a 1 in column 'c' */
    M1 = sm_alloc_size(M->last_row->row_num, M->last_col->col_num);
    sm_foreach_col_element(c, p1) {
      sm_copy_row(M1, p1->row_num, sm_get_row(M, p1->row_num));
    }

    /*
     *  new partial rectangle
     */
    rect1.rows = sm_col_dup(c);
    rect1.cols = sm_row_dup(rect->cols);

    /*
     *  factor out all columns of 'M1' which are all 1
     */
    already_generated = 0;
    for (pcol = M1->first_col; pcol != 0; pcol = pcolnext) {
      pcolnext = pcol->next_col;
      if (pcol->length == c->length) {
        if (pcol->col_num < c->col_num) {
          already_generated = 1;
          break;
        }
        (void)sm_row_insert(rect1.cols, pcol->col_num);
        sm_delcol(M1, pcol->col_num);
      }
    }

    if (!already_generated) {
      if ((*record)(M1, &rect1, state)) {
        gen_all_rect(M1, &rect1, c->col_num, record, state);
      }
    }

    sm_row_free(rect1.cols);
    sm_col_free(rect1.rows);
    sm_free(M1);
  }
}

static int has_full_column(M) sm_matrix *M;
{
  register sm_col *pcol;

  sm_foreach_col(M, pcol) {
    if (pcol->length == M->nrows) {
      return 1;
    }
  }
  return 0;
}

void gen_all_rectangles(M, record, state) sm_matrix *M;
int (*record)();
char *state;
{
  rect_t *rect;

  rect = rect_alloc();
  gen_all_rect(M, rect, 0, record, state);
  if (!has_full_column(M)) {
    (*record)(M, rect, state);
  }
  rect_free(rect);
}

rect_t *rect_alloc() {
  rect_t *rect;

  rect = ALLOC(rect_t, 1);
  rect->rows = sm_col_alloc();
  rect->cols = sm_row_alloc();
  rect->value = 0;
  return rect;
}

void rect_free(rect) rect_t *rect;
{
  sm_row_free(rect->cols);
  sm_col_free(rect->rows);
  FREE(rect);
}

rect_t *rect_dup(old_rect) rect_t *old_rect;
{
  rect_t *rect;

  rect = ALLOC(rect_t, 1);
  rect->rows = sm_col_dup(old_rect->rows);
  rect->cols = sm_row_dup(old_rect->cols);
  rect->value = old_rect->value;
  return rect;
}
