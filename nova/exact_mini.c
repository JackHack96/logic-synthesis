
/*******************************************************************************
 *   Invokes the minimizer to get the final boolean representation * This
 *version uses the espresso-II version#2.0 binary-valued minimizer      *
 *******************************************************************************/

#include "inc/nova.h"

exact_mini() {

  FILE *fopen(), *fp3, *fp1;
  char line[MAXLINE], *fgets();
  char string1[MAXSTRING];
  char string2[MAXSTRING];
  char string3[MAXSTRING];
  char string4[MAXSTRING];
  char string5[MAXSTRING];
  char command[MAXSTRING];
  int i, j, cursor, linelength;
  int code_index, notany_index;

  if ((fp3 = fopen(temp3, "w")) == NULL) {
    fprintf(stderr, "fopen in exact_mini: can't create temp3\n");
    exit(-1);
  }

  if ((fp1 = fopen(temp1, "r")) == NULL) {
    fprintf(stderr, "fopen in exact_mini: can't read temp1\n");
    exit(-1);
  }

  st_codelength = strlen(states[0].exact_code);

  if (TYPEFR && !ONEHOT)
    fputs(".type fr\n", fp3);
  if (TYPEFR && ONEHOT)
    fputs(".type f\n", fp3); /* no exdc with 1-h enc. */
  if (ISYMB) {
    inp_codelength = strlen(inputs[0].exact_code);
    sprintf(string1, ".i %d\n", inp_codelength + st_codelength);
  } else {
    sprintf(string1, ".i %d\n", inputfield + st_codelength);
  }
  fputs(string1, fp3);
  /*printf("string1 = %s\n", string1);*/

  sprintf(string1, ".o %d\n", st_codelength + outputfield - 1);
  fputs(string1, fp3);
  /*printf("string1 = %s\n", string1);*/

  while (fgets(line, MAXLINE, fp1) != (char *)0) {

    linelength = strlen(line);

    /* skip blank lines , comment lines and command lines */
    if ((linelength == 1) || (myindex(line, "#") >= 0) ||
        (myindex(line, ".") >= 0)) {
      ;
    } else {

      /* takes care of proper inputs */
      if (ISYMB) {

        for (i = 0; i < inputnum; i++) {
          string2[i] = line[i];
        }

        string2[inputnum] = '\0';
        /*printf("string2 = %s\n", string2);*/
        code_index = myindex(string2, "1");
        /*printf("index = %d\n", code_index);*/

        if (code_index != -1) {
          strcpy(string2, inputs[code_index].exact_code);
        } else {
          for (j = 0; j < inp_codelength; j++) {
            string2[j] = '-';
          }
          string2[inp_codelength] = '\0';
        }

        /*printf("string2 = %s\n", string2);*/
        cursor = inputnum + 3;

      } else {

        for (i = 0; i < inputfield; i++) {
          string2[i] = line[i];
        }
        string2[inputfield] = '\0';
        /*printf("string2 = %s\n", string2);*/
        cursor = inputfield + 3;
      }

      /* takes care of present states */
      for (i = cursor; i < cursor + statenum; i++) {
        string3[i - cursor] = line[i];
      }
      string3[statenum] = '\0';
      /*printf("string3 = %s\n", string3);*/
      notany_index = myindex(string3, "0");
      /*printf("notany_index = %d\n", notany_index);*/
      if (notany_index != -1) {
        code_index = myindex(string3, "1");
        /*printf("code_index = %d\n", code_index);*/
        strcpy(string3, states[code_index].exact_code);
      } else {
        for (j = 0; j < st_codelength; j++) {
          string3[j] = '-';
        }
        string3[st_codelength] = '\0';
      }
      /*printf("string3 = %s\n", string3);*/
      cursor = i + 3;

      /* takes care of next states */
      for (i = cursor; i < cursor + statenum; i++) {
        string4[i - cursor] = line[i];
      }
      string4[statenum] = '\0';
      /*printf("string4 = %s\n", string4);*/
      notany_index = myindex(string4, "0");
      /*printf("notany_index = %d\n", notany_index);*/
      if (notany_index != -1) {
        code_index = myindex(string4, "1");
        /*printf("code_index = %d\n", code_index);*/
        strcpy(string4, states[code_index].exact_code);
      } else {
        for (j = 0; j < st_codelength; j++) {
          string4[j] = '-';
        }
        string4[st_codelength] = '\0';
      }
      /*printf("string4 = %s\n", string4);*/
      cursor = i + 1;

      /* takes care of proper outputs */
      for (i = cursor; i < cursor + outputfield; i++) {
        string5[i - cursor] = line[i];
      }
      string5[outputfield] = '\0';
      /*printf("string5 = %s\n", string5);*/

      sprintf(string1, "%s  %s   %s %s\n", string2, string3, string4, string5);
      fputs(string1, fp3);
    }
  }

  fclose(fp1);

  fclose(fp3);

  if (!ONEHOT) { /* hack to skip minimization with 1-hot encoding */
    if (VERBOSE)
      printf("\nRunning Espresso from exact_mini\n");
    /*sprintf(command,"espresso -fr %s > %s", temp3, temp4);changed*/
    sprintf(command, "espresso %s > %s", temp3, temp4);
    system(command);
    if (VERBOSE)
      printf("Espresso terminated\n");
  } else {
    sprintf(command, "cp %s %s", temp3, temp4);
    system(command);
    sprintf(command, "cp %s %s", temp3, temp5);
    system(command);
  }
}
