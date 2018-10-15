/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/options/options.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:10 $
 *
 */
#ifndef OPT_H
#define OPT_H

#include "ansi.h"

#define OPT_PKG_NAME	"options"

typedef struct {
    char *optionLetters;
    char *argName;
    char *description;
} optionStruct;

/* dummy `optionLetters' values for arguments following options */
extern char OPT_RARG[];		/* required argument */
extern char OPT_OARG[];		/* optional argument */
extern char OPT_ELLIP[];	/* flag to put ellipsis in summary */
extern char OPT_DESC[];		/* general description */
extern char OPT_CONT[];		/* continues description */

extern optionStruct optionList[];
extern int optind;
extern char *optarg;
extern char *optProgName;

EXTERN int optGetOption
	ARGS((int argc, char **argv));
EXTERN void optUsage
	NULLARGS;
EXTERN void optInit
	ARGS((char *progName, optionStruct options[], int rtnBadFlag));
EXTERN void optAddOptions
	ARGS((optionStruct options[]));
EXTERN void optAddUnivOptions
	NULLARGS;
EXTERN char *optUsageString
	NULLARGS;

#endif /* OPT_H */
