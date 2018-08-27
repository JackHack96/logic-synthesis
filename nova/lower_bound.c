
#include "inc/nova.h"

lower_bound() {

  INPUTTABLE *new;
  char **arg_array;
  BOOLEAN INCOMPATIBLE;
  int **inco1_array, **inco0_array;
  int i, j, inp_length, st_length, ones, zeroes, i_col, j_col, ones_color,
      zero_color, bound;

  if ((arg_array = (char **)calloc((unsigned)4, sizeof(char *))) ==
      (char **)0) {
    fprintf(stderr, "Insufficient memory for 'arg_array'");
    exit(-1);
  }
  inp_length = strlen(firstable->input);
  st_length = strlen(firstable->pstate);
  for (i = 0; i < 2; i++) {
    if ((arg_array[i] = (char *)calloc((unsigned)inp_length + 1,
                                       sizeof(char))) == (char *)0) {
      fprintf(stderr, "Insufficient memory for arg_array[i]");
      exit(-1);
    }
    arg_array[i][inp_length] = '\0';
  }
  for (i = 2; i < 4; i++) {
    if ((arg_array[i] = (char *)calloc((unsigned)st_length + 1,
                                       sizeof(char))) == (char *)0) {
      fprintf(stderr, "Insufficient memory for arg_array[i]");
      exit(-1);
    }
    arg_array[i][st_length] = '\0';
  }

  ones = 0;
  for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {
    for (i = 0; i < strlen(new->output) - 1; i++) {
      if (new->output[i] == ONE) {
        ones++;
      }
    }
  }
  printf("Incompatibility graph (size = %d):\n", ones);

  if (ones > 0) {
    if ((inco1_array = (int **)calloc((unsigned)ones, sizeof(int *))) ==
        (int **)0) {
      fprintf(stderr, "Insufficient memory for 'inco1_array'");
      exit(-1);
    }
    for (i = 0; i < ones; i++) {
      if ((inco1_array[i] = (int *)calloc((unsigned)ones, sizeof(int))) ==
          (int *)0) {
        fprintf(stderr, "Insufficient memory for inco1_array[i]");
        exit(-1);
      }
    }

    for (i = 0; i < ones; i++) {
      for (j = 0; j < ones; j++) {
        inco1_array[i][j] = 0;
      }
    }

    for (i = 0; i < ones - 1; i++) {
      i_col = argument1(i, arg_array[0], arg_array[2]);
      for (j = i + 1; j < ones; j++) {
        j_col = argument1(j, arg_array[1], arg_array[3]);
        INCOMPATIBLE = FALSE;
        for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {
          if (new->output[i_col] == ZERO || new->output[j_col] == ZERO) {
            if ((strcmp(new->pstate, arg_array[2]) == 0) ||
                (strcmp(new->pstate, arg_array[3]) == 0) ||
                (strcmp(new->pstate, "ANY") == 0)) {
              if (ISYMB) {
                if ((strcmp(new->input, arg_array[0]) == 0) ||
                    (strcmp(new->input, arg_array[1]) == 0)) {
                  INCOMPATIBLE = TRUE;
                  break;
                }
              } else {
                if (inp_test(arg_array[0], arg_array[1], new->input) == 1) {
                  INCOMPATIBLE = TRUE;
                  break;
                }
              }
            }
          }
        }
        if (INCOMPATIBLE == TRUE) {
          inco1_array[i][j] = 1;
          inco1_array[j][i] = 1;
        }
        /*printf("%d %d %d\n", i, j, inco1_array[i][j]);*/
      }
    }
    for (i = 0; i < ones; i++) {
      for (j = 0; j < ones; j++) {
        printf("%d", inco1_array[i][j]);
      }
      printf("\n");
    }

    ones_color = coloring(ones, inco1_array);
  }

  zeroes = 0;
  for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {
    for (i = 0; i < strlen(new->output) - 1; i++) {
      if (new->output[i] == ZERO) {
        zeroes++;
      }
    }
  }
  printf("\nIncompatibility graph (size = %d):\n", zeroes);

  if (zeroes > 0) {
    if ((inco0_array = (int **)calloc((unsigned)zeroes, sizeof(int *))) ==
        (int **)0) {
      fprintf(stderr, "Insufficient memory for 'inco0_array'");
      exit(-1);
    }
    for (i = 0; i < zeroes; i++) {
      if ((inco0_array[i] = (int *)calloc((unsigned)zeroes, sizeof(int))) ==
          (int *)0) {
        fprintf(stderr, "Insufficient memory for inco0_array[i]");
        exit(-1);
      }
    }

    for (i = 0; i < zeroes; i++) {
      for (j = 0; j < zeroes; j++) {
        inco0_array[i][j] = 0;
      }
    }

    for (i = 0; i < zeroes - 1; i++) {
      i_col = argument0(i, arg_array[0], arg_array[2]);
      for (j = i + 1; j < zeroes; j++) {
        j_col = argument0(j, arg_array[1], arg_array[3]);
        INCOMPATIBLE = FALSE;
        for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {
          if (new->output[i_col] == ONE || new->output[j_col] == ONE) {
            if ((strcmp(new->pstate, arg_array[2]) == 0) ||
                (strcmp(new->pstate, arg_array[3]) == 0) ||
                (strcmp(new->pstate, "ANY") == 0)) {
              if (ISYMB) {
                if ((strcmp(new->input, arg_array[0]) == 0) ||
                    (strcmp(new->input, arg_array[1]) == 0)) {
                  INCOMPATIBLE = TRUE;
                  break;
                }
              } else {
                if (inp_test(arg_array[0], arg_array[1], new->input) == 1) {
                  INCOMPATIBLE = TRUE;
                  break;
                }
              }
            }
          }
        }
        if (INCOMPATIBLE == TRUE) {
          inco0_array[i][j] = 1;
          inco0_array[j][i] = 1;
        }
        /*printf("%d %d %d\n", i, j, inco0_array[i][j]);*/
      }
    }
    for (i = 0; i < zeroes; i++) {
      for (j = 0; j < zeroes; j++) {
        printf("%d", inco0_array[i][j]);
      }
      printf("\n");
    }

    zero_color = coloring(zeroes, inco0_array);
  }

  bound = max(ones_color, zero_color);
  printf("\nLower bound on the # of product-terms = %d\n", bound);
}

int argument1(index, arg_array0, arg_array2) int index;
char *arg_array0;
char *arg_array2;

{

  int i, i_count;
  INPUTTABLE *new;

  i_count = -1;
  for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {
    for (i = 0; i < strlen(new->output) - 1; i++) {
      if (new->output[i] == ONE) {
        i_count++;
      }
      if (index == i_count) {
        strcpy(arg_array0, new->input);
        strcpy(arg_array2, new->pstate);
        /*printf("%s", arg_array0);
        printf("%s\n", arg_array2);*/
        return (i);
      }
    }
  }

  return (0);
}

int argument0(index, arg_array0, arg_array2) int index;
char *arg_array0;
char *arg_array2;

{

  int i, i_count;
  INPUTTABLE *new;

  i_count = -1;
  for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {
    for (i = 0; i < strlen(new->output) - 1; i++) {
      if (new->output[i] == ZERO) {
        i_count++;
      }
      if (index == i_count) {
        strcpy(arg_array0, new->input);
        strcpy(arg_array2, new->pstate);
        /*printf("%s", arg_array0);
        printf("%s\n", arg_array2);*/
        return (i);
      }
    }
  }

  return (0);
}

inp_test(cube1, cube2, cube3) char *cube1;
char *cube2;
char *cube3;

{

  int i, cube_length;

  cube_length = strlen(cube1);
  for (i = 0; i < cube_length; i++) {
    if (cube1[i] != '-' || cube2[i] != '-') {
      if (cube1[i] == cube2[i] && cube1[i] != cube3[i])
        return (0);
    }
  }
  return (1);
}
