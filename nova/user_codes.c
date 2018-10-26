
#include "inc/nova.h"

user_codes() {

  FILE *fp, *fopen();
  char line[MAXLINE], *fgets(), name[MAXSTRING], code[MAXSTRING];
  char codefile[MAXSTRING];
  int i;

  sprintf(codefile, "%s.codes", sh_filename);
  if ((fp = fopen(codefile, "r")) == NULL) {
    fprintf(stderr, "Error in opening codefile\n");
    exit(-1);
  }

  while (fgets(line, MAXLINE, fp) != NULL) {
    if (myindex(line, "scode") >= 0) {
      sscanf(line, "scode %s %s", code, name);

      for (i = 0; i < statenum; i++) {
        if (strcmp(states[i].name, name) == 0) {
          strcpy(states[i].exact_code, code);
        }
      }
    }
    if (myindex(line, "icode") >= 0) {
      sscanf(line, "icode %s %s", code, name);

      for (i = 0; i < inputnum; i++) {
        if (strcmp(inputs[i].name, name) == 0) {
          strcpy(inputs[i].exact_code, code);
        }
      }
    }
    /*printf("%s", line);*/
  }
  fclose(fp);

  exact_rotation();
  exact_output();
}

onehot_codes() {

  int i, j;

  for (i = 0; i < statenum; i++) {
    for (j = 0; j < statenum; j++) {
      /* OLD WAY: states[i].exact_code[j] = ZERO;*/
      states[i].exact_code[j] = DASH;
    }
    states[i].exact_code[i] = ONE;
  }

  if (ISYMB) {
    for (i = 0; i < inputnum; i++) {
      for (j = 0; j < inputnum; j++) {
        /* OLD WAY: inputs[i].exact_code[j] = ZERO;*/
        inputs[i].exact_code[j] = DASH;
      }
      inputs[i].exact_code[i] = ONE;
    }
  }

  if (VERBOSE) {
    if (ISYMB)
      show_exactcode(inputs, inputnum);
    show_exactcode(states, statenum);
  }

  exact_rotation();
  exact_output();
}
