
#ifndef OPT_H
#define OPT_H


#define OPT_PKG_NAME    "options"

typedef struct {
    char *optionLetters;
    char *argName;
    char *description;
}                   optionStruct;

/* dummy `optionLetters' values for arguments following options */
extern char OPT_RARG[];        /* required argument */
extern char OPT_OARG[];        /* optional argument */
extern char OPT_ELLIP[];    /* flag to put ellipsis in summary */
extern char OPT_DESC[];        /* general description */
extern char OPT_CONT[];        /* continues description */

extern optionStruct optionList[];
extern int          optind;
extern char         *optarg;
extern char         *optProgName;

extern int optGetOption
        (int argc, char **argv);

extern void optUsage();

extern void optInit
        (char *progName, optionStruct options[], int rtnBadFlag);

extern void optAddOptions
        (optionStruct options[]);

extern void optAddUnivOptions();

extern char *optUsageString();

#endif /* OPT_H */
