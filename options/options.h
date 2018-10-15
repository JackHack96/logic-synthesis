
#ifndef OPT_H
#define OPT_H

#define OPT_PKG_NAME "options"

typedef struct {
  char *optionLetters;
  char *argName;
  char *description;
} optionStruct;

/* dummy `optionLetters' values for arguments following options */
char OPT_RARG[];  /* required argument */
char OPT_OARG[];  /* optional argument */
char OPT_ELLIP[]; /* flag to put ellipsis in summary */
char OPT_DESC[];  /* general description */
char OPT_CONT[];  /* continues description */

optionStruct optionList[];
int optind;
char *optarg;
char *optProgName;

int optGetOption(int argc, char **argv);

void optUsage();

void optInit(char *progName, optionStruct options[], int rtnBadFlag);

void optAddOptions(optionStruct options[]);

void optAddUnivOptions();

char *optUsageString();

#endif /* OPT_H */
