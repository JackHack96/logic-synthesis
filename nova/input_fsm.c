
#include "inc/nova.h"

/*********************************************************************
 *                                                                    *
 *             This routine reads the input data file                 *
 *       and creates the data structure of the symbolic table         *
 *                                                                    *
 *********************************************************************/

input_fsm(infile) int infile;

{

  FILE *fp, *fopen();
  char line[MAXLINE], *fgets();
  int linelength;
  int sfd;

  productnum = 0;

  if ((infile == 1) && (fp = fopen(file_name, "r")) == NULL) {
    fprintf(stderr, "input: can't open %s\n", file_name);
    exit(-1);
  } else {
    if (infile == 0) {
      fp = stdin;
      strcpy(file_name, "/tmp/SISXXXXXX");
      sfd = mkstemp(file_name);
      if (sfd == -1) {
        fprintf(stderr, "can't create temp file\n");
        exit(-1);
      }
      getname(file_name);
    }

    temp_files();

    printf(".model %s\n", esp);
    printf(".start_kiss\n");
    while (fgets(line, MAXLINE, fp) != (char *)0) {
      linelength = strlen(line);
      /*printf("Linelength = %d\n", linelength);
      printf("Line = %s\n", line);*/

      /* echoes the input line */
      if (LIST)
        printf("%s", line);

      /* replaces all characters after a pound sign with blanks */
      comment(line, linelength);

      /* removes trailing blanks and tabs  -
      practical reason : shrinks blank lines to length 1 so that
      later they are skipped                                    */
      trailblanks(line, linelength);

      /* updates linelength for consistency with "trailblanks" */
      linelength = strlen(line);

      /* skip blank lines and comment lines -
         comment lines are introduced by "/*" or "#" */
      if ((linelength == 1) || (myindex(line, "/*") >= 0) ||
          (myindex(line, "#") >= 0))
        ;

      /* check for end of file flag */
      else if (myindex(line, ".end") >= 0)
        break;

      /* check for command line option */
      else if (line[0] == '.')
        commandline(line);

      /* otherwise assume symbolic implicant */
      else {
        productnum++;
        productline(line, linelength);
        /*printf("product term line memorized internally\n");
        printf("%s", lastable->input);
        printf("%s", lastable->pstate);
        printf("%s", lastable->nstate);
        printf("%s\n", lastable->output);*/
      }
    }
    printf(".end_kiss\n#\n");

    fclose(fp);
  }

  label();

  if (ISYMB && IBITS_DEF) {
    inp_codelength = special_log2(minpow2(inputnum));
  }
  if (ISYMB && !IBITS_DEF) {
    icheck_usercode(inputnum, inp_codelength);
  }

  if (SBITS_DEF) {
    st_codelength = special_log2(minpow2(statenum));
  }
  if (!SBITS_DEF) {
    scheck_usercode(statenum, st_codelength);
  }

  array_alloc();

  name();
}

/**************************************************************************
 *                  Reads a command line in the input file                 *
 *                                                                         *
 **************************************************************************/

commandline(line) char *line;

{

  /*if (LIST) printf("%s" , line);*/

  if (myindex(line, "list") >= 0)
    LIST = TRUE;

  if (myindex(line, "type") >= 0) {
    if (myindex(line, "fr") >= 0)
      TYPEFR = TRUE;
    if (myindex(line, "fd") >= 0)
      TYPEFR = FALSE;
  }

  if (myindex(line, "option") >= 0) {
    if (myindex(line, "short") >= 0)
      SHORT = TRUE;
    if (myindex(line, "prtall") >= 0)
      PRTALL = TRUE;
    if (myindex(line, "fr") >= 0)
      TYPEFR = TRUE;
    if (myindex(line, "fd") >= 0)
      TYPEFR = FALSE;
  }

  if (myindex(line, "symbolic") >= 0) {
    if (myindex(line, "input") >= 0)
      ISYMB = TRUE;
    if (myindex(line, "output") >= 0)
      OSYMB = TRUE;
  }

  if (myindex(line, ".r ") >= 0) {
    sscanf(line, ".r %s", init_state);
    INIT_FLAG = TRUE;
  }

  if (myindex(line, "ilb") >= 0) {
    strcpy(input_names, line);
    ILB = TRUE;
  }
  if (myindex(line, "ob") >= 0) {
    strcpy(output_names, line);
    OB = TRUE;
  }
}

/****************************************************************************
 *                       Reads a product term line                           *
 *                                                                           *
 ****************************************************************************/

productline(line, linelength) char *line;
int linelength;

{

  int i, j, first;

  char inputstr[MAXSTRING];
  char prstatestr[MAXSTRING];
  char nxstatestr[MAXSTRING];
  char outputstr[MAXSTRING];

  /*if (LIST) printf("%s" , line);*/

  /* replaces tabs with blanks */
  tabs(line, linelength);

  /* captures the proper input */
  for (i = 0; i < linelength; i++) {
    if (line[i] != ' ') {
      break;
    }
  }
  for (first = i; i < linelength; i++) {
    if (line[i] == ' ') {
      break;
    }
  }
  for (j = 0; j < (i - first); j++) {
    inputstr[j] = line[j + first];
  }
  inputstr[i - first] = '\0';
  inputfield = i - first;

  /* captures the present state */
  for (; i < linelength; i++) {
    if (line[i] != ' ') {
      break;
    }
  }
  for (first = i; i < linelength; i++) {
    if (line[i] == ' ') {
      break;
    }
  }
  for (j = 0; j < (i - first); j++) {
    prstatestr[j] = line[j + first];
  }
  prstatestr[i - first] = '\0';

  /* captures the next state */
  for (; i < linelength; i++) {
    if (line[i] != ' ') {
      break;
    }
  }
  for (first = i; i < linelength; i++) {
    if (line[i] == ' ') {
      break;
    }
  }
  for (j = 0; j < (i - first); j++) {
    nxstatestr[j] = line[j + first];
  }
  nxstatestr[i - first] = '\0';

  /* captures the proper output */
  for (; i < linelength; i++) {
    if (line[i] != ' ') {
      break;
    }
  }
  for (first = i; i < linelength; i++) {
    if (line[i] == ' ') {
      i++;
      break;
    }
  }
  for (j = 0; j < (i - first); j++) {
    outputstr[j] = line[j + first];
  }
  outputstr[i - first] = '\0';
  outputfield = i - first;

  newinputrow(inputstr, prstatestr, nxstatestr, outputstr);
}

/*******************************************************************************
 *  Labels every state string with an integer "plab" or "nlab" * Labels every
 *input (output) string with an integer "ilab" ("olab")          *
 *******************************************************************************/

label() {

  INPUTTABLE *new, *old;

  statenum = 0;
  inputnum = 0;
  outputnum = 0;

  /* labels present states */
  for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {
    /* if the current present state is a don't care state it gets the
       label "-1" - a don't care state may be denoted by a "-" in the
       first position of the string or by the string "ANY"            */
    if (new->pstate[0] == DASH || myindex(new->pstate, ANYSTATE) >= 0) {
      new->plab = DCLABEL;
    } else {
      for (old = firstable; old != new; old = old->next) {
        if (strcmp(new->pstate, old->pstate) == 0) {
          new->plab = old->plab;
          break;
        }
      }
      if (new != old)
        continue;
      new->plab = statenum++;
    }
  }

  /* labels next states */
  for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {
    /* if the current next state is a don't care state it gets the
       label "-1" - a don't care state may be denoted by a "-" in the
       first position of the string or by the string "ANY"            */
    if (new->nstate[0] == DASH || myindex(new->nstate, ANYSTATE) >= 0) {
      new->nlab = DCLABEL;
    } else {
      for (old = firstable; old != (INPUTTABLE *)0; old = old->next) {
        if (strcmp(new->nstate, old->pstate) == 0) {
          new->nlab = old->plab;
          break;
        }
      }
      if (old != (INPUTTABLE *)0)
        continue;
      for (old = firstable; old != new; old = old->next) {
        if (strcmp(new->nstate, old->nstate) == 0) {
          new->nlab = old->nlab;
          break;
        }
      }
      if (new != old)
        continue;
      new->nlab = statenum++;
    }
  }

  /* labels symbolic inputs */
  if (ISYMB) {
    for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {
      /* if the current symbolic input is a don't care it gets the label
         "-1" - a don't care symbolic input may be denoted by a "-" in
         the first position of the string or by the string "ANY"       */
      if (new->input[0] == DASH || myindex(new->input, ANYSTATE) >= 0) {
        new->ilab = DCLABEL;
      } else {
        for (old = firstable; old != new; old = old->next) {
          if (strcmp(new->input, old->input) == 0) {
            new->ilab = old->ilab;
            break;
          }
        }
        if (new != old)
          continue;
        new->ilab = inputnum++;
      }
    }
  }

  /* note : outputfield is larger than 1 than actual outputfield */
  outputnum = outputfield - 1;
  /*printf("\noutputnum = %d\n", outputnum);*/

  /*for (new=firstable; new!=(INPUTTABLE *) 0; new=new->next) {
      printf("new->input= %s , new->ilab= %d\n", new->input , new->ilab);
      printf("new->pstate= %s , new->plab= %d\n", new->pstate , new->plab);
      printf("new->nstate= %s , new->nlab= %d\n", new->nstate , new->nlab);
      printf("new->output= %s , new->olab= %d\n", new->output , new->olab);
  }


  if (ISYMB) printf("inputnum= %d\n", inputnum);
    else printf("inputfield= %d\n", inputfield);
  printf("inputfield= %d\n", inputfield);
  printf("statenum= %d\n", statenum);
  printf("outputfield= %d\n", outputfield);*/
}

/*******************************************************************************
 *      fills in the field "name" of the symblemes *
 *******************************************************************************/

name() {

  INPUTTABLE *new;

  int i;

  for (new = firstable; new != (INPUTTABLE *)0; new = new->next) {

    if (ISYMB && (new->ilab != DCLABEL)) {
      inputs[new->ilab].name = new->input;
    }
    if (new->plab != DCLABEL) {
      states[new->plab].name = new->pstate;
    }
    if (new->nlab != DCLABEL) {
      states[new->nlab].name = new->nstate;
    }
  }

  if (VERBOSE) {
    printf("\n");
    for (i = 0; i < statenum; i++) {
      printf("states[%d].name= %s \n", i, states[i].name);
    }
    printf("\n");
    if (ISYMB) {
      for (i = 0; i < inputnum; i++) {
        printf("inputs[%d].name= %s \n", i, inputs[i].name);
      }
      printf("\n");
    }
  }
}

scheck_usercode(symblemes_num, code_length) int symblemes_num;
int code_length;

{

  int codes_num;

  codes_num = power(2, code_length);

  if (symblemes_num > codes_num) {
    fprintf(stderr, "Argument of option -s too small\n");
    exit(-1);
  }
  if (code_length > symblemes_num) {
    fprintf(stderr, "Argument of option -s too large\n");
    exit(-1);
  }
}

icheck_usercode(symblemes_num, code_length) int symblemes_num;
int code_length;

{

  int codes_num;

  codes_num = power(2, code_length);

  if (symblemes_num > codes_num) {
    fprintf(stderr, "Argument of option -i too small\n");
    exit(-1);
  }
  if (code_length > symblemes_num) {
    fprintf(stderr, "Argument of option -i too large\n");
    exit(-1);
  }
}
