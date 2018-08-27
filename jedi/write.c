
/*
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */
#include "inc/jedi.h"

int write_output();    /* forward declaration */
int add_dont_cares();  /* forward declaration */
int check_dont_care(); /* forward declaration */

extern int expand_code();

write_output(fptr) FILE *fptr;
{
  int i, j;
  int c, k, s;

  /* state code expansion */
  if (expandFlag) {
    expand_code();
  }

  /*
   * print out header
   */
  (void)fprintf(fptr, "#\n");
  (void)fprintf(fptr, "# JEDI, %s\n", RELEASE);
  (void)fprintf(fptr, "#\n");

  /*
   * print out encodings
   */
  for (i = 0; i < ne; i++) {
    for (j = 0; j < enumtypes[i].ns; j++) {
      c = enumtypes[i].symbols[j].code_ptr;
      (void)fprintf(fptr, "# %s.%s %s\n", enumtypes[i].name,
                    enumtypes[i].symbols[j].token,
                    enumtypes[i].codes[c].bit_vector);
    }
    (void)fprintf(fptr, "#\n");
  }

  /*
   * print i/o
   */
  (void)fprintf(fptr, ".type fr\n");
  (void)fprintf(fptr, ".i %d\n", tni);
  (void)fprintf(fptr, ".o %d\n", tno);

  (void)fprintf(fptr, ".ilb ");
  for (i = 0; i < ni; i++) {
    if (inputs[i].boolean_flag) {
      (void)fprintf(fptr, "%s ", inputs[i].name);
    } else {
      c = inputs[i].enumtype_ptr;
      k = enumtypes[c].nb;
      for (j = 0; j < k; j++) {
        (void)fprintf(fptr, "%s.%d ", inputs[i].name, j);
      }
    }
  }
  (void)fprintf(fptr, "\n");

  (void)fprintf(fptr, ".ob ");
  for (i = 0; i < no; i++) {
    if (outputs[i].boolean_flag) {
      (void)fprintf(fptr, "%s ", outputs[i].name);
    } else {
      c = outputs[i].enumtype_ptr;
      k = enumtypes[c].nb;
      for (j = 0; j < k; j++) {
        (void)fprintf(fptr, "%s.%d ", outputs[i].name, j);
      }
    }
  }
  (void)fprintf(fptr, "\n");

  /*
   * print out truth table
   */
  for (i = 0; i < np; i++) {
    /*
     * print inputs
     */
    for (j = 0; j < ni; j++) {
      if (inputs[j].boolean_flag) {
        (void)fprintf(fptr, "%s", inputs[j].entries[i].token);
      } else {
        if (check_dont_care(inputs[j].entries[i].token)) {
          c = inputs[j].enumtype_ptr;
          (void)fprintf(fptr, "%s", enumtypes[c].dont_care);
        } else {
          c = inputs[j].enumtype_ptr;
          s = inputs[j].entries[i].symbol_ptr;
          k = enumtypes[c].symbols[s].code_ptr;
          (void)fprintf(fptr, "%s", enumtypes[c].codes[k].bit_vector);
        }
      }
    }
    (void)fprintf(fptr, " ");

    /*
     * print outputs
     */
    for (j = 0; j < no; j++) {
      if (outputs[j].boolean_flag) {
        (void)fprintf(fptr, "%s", outputs[j].entries[i].token);
      } else {
        if (check_dont_care(outputs[j].entries[i].token)) {
          c = outputs[j].enumtype_ptr;
          (void)fprintf(fptr, "%s", enumtypes[c].dont_care);
        } else {
          c = outputs[j].enumtype_ptr;
          s = outputs[j].entries[i].symbol_ptr;
          k = enumtypes[c].symbols[s].code_ptr;
          (void)fprintf(fptr, "%s", enumtypes[c].codes[k].bit_vector);
        }
      }
    }
    (void)fprintf(fptr, "\n");
  }

  /*
   * add don't cares for unused codes
   */
  if (addDontCareFlag) {
    add_dont_cares(fptr);
  }

  /*
   * print ending comment
   */
  (void)fprintf(fptr, ".e\n");
} /* end of write_output */

add_dont_cares(fptr) FILE *fptr;
{
  int i, j, k, e, e2;

  for (i = 0; i < ni; i++) {
    if (!inputs[i].boolean_flag) {
      e = inputs[i].enumtype_ptr;
      for (j = 0; j < enumtypes[e].nc; j++) {
        if (!enumtypes[e].codes[j].assigned) {
          /*
           * print input
           */
          for (k = 0; k < ni; k++) {
            if (inputs[k].boolean_flag) {
              (void)fprintf(fptr, "-");
            } else if (k == i) {
              (void)fprintf(fptr, "%s", enumtypes[e].codes[j].bit_vector);
            } else {
              e2 = inputs[k].enumtype_ptr;
              (void)fprintf(fptr, "%s", enumtypes[e2].dont_care);
            }
          }
          (void)fprintf(fptr, " ");

          /*
           * print output
           */
          for (k = 0; k < tno; k++) {
            (void)fprintf(fptr, "-");
          }
          (void)fprintf(fptr, "\n");
        }
      }
    }
  }
} /* end of add_dont_cares */

check_dont_care(token) char *token;
{
  if (!strcmp(token, "ANY"))
    return TRUE;
  if (!strcmp(token, "any"))
    return TRUE;
  if (!strcmp(token, "DC"))
    return TRUE;
  if (!strcmp(token, "dc"))
    return TRUE;
  if (!strcmp(token, "-"))
    return TRUE;
  if (!strcmp(token, "*"))
    return TRUE;

  return FALSE;
}

write_blif(fptr) FILE *fptr;
{
  int i, j, c;

  /* print header */
  (void)fprintf(fptr, ".model jedi_output\n");

  /* print state table */
  (void)fprintf(fptr, ".start_kiss\n");
  (void)fprintf(fptr, ".i %d\n", ni - 1);
  (void)fprintf(fptr, ".o %d\n", no - 1);
  (void)fprintf(fptr, ".s %d\n", enumtypes[0].ns);
  (void)fprintf(fptr, ".p %d\n", np);
  (void)fprintf(fptr, ".r %s\n", reset_state);
  for (i = 0; i < np; i++) {
    for (j = 0; j < ni - 1; j++) {
      (void)fprintf(fptr, "%s", inputs[j].entries[i].token);
    }
    (void)fprintf(fptr, " ");
    (void)fprintf(fptr, "%s", inputs[ni - 1].entries[i].token);
    (void)fprintf(fptr, " ");

    (void)fprintf(fptr, "%s", outputs[0].entries[i].token);
    (void)fprintf(fptr, " ");
    for (j = 1; j < no; j++) {
      (void)fprintf(fptr, "%s", outputs[j].entries[i].token);
    }
    (void)fprintf(fptr, "\n");
  }
  (void)fprintf(fptr, ".end_kiss\n");

  /* print encodings */
  for (i = 0; i < enumtypes[0].ns; i++) {
    c = enumtypes[0].symbols[i].code_ptr;
    (void)fprintf(fptr, ".code %s %s\n", enumtypes[0].symbols[i].token,
                  enumtypes[0].codes[c].bit_vector);
  }

  (void)fprintf(fptr, ".end\n");
} /* end of write_blif */
