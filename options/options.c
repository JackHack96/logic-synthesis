
#include "options.h"
#include "../port/copyright.h"
#include "../port/port.h"
#include "../utility/utility.h"

static int doUnivOption();

static void usageSummary();

static void usageDetailed();

static void formatInit();

static void formatFlush();

static void formatString();

static void formatChar();

static void formatTab();

static void formatOptChoice();

#ifndef MM_TRACE
#define MM_PAD 0
#define MM_TRACE 1
#define MM_TRASH 2
#endif

/* dummy `optionLetters' values for arguments following options */
/* make sure the strings are different so smart compilers don't alias them */
char OPT_RARG[]  = "r";  /* required argument */
char OPT_OARG[]  = "o";  /* optional argument */
char OPT_ELLIP[] = "e"; /* flag to put ellipsis at end of summary */
char OPT_DESC[]  = "d";  /* overall description */
char OPT_CONT[]  = "c";  /* continues description */

char        *optProgName = "unknown program";
char        *optarg;
int         optind       = 0;
static char *scanPtr;

#define HARD_SP '\0' /* A space we don't allow to split lines */

#define LINEWIDTH 79      /* Maximum number of characters in a line */
#define DETAIL_INDENT 15  /* indent of detailed option descriptions */
#define SUMMARY_INDENT 20 /* indent of continuation of summary line */

static char usageProlog[LINEWIDTH] = "";
static char *usageString           = NIL(char);
static int  usageStrlen            = 0;

/* The linked list of optionStruct arrays */
struct optGroup {
    optionStruct    *array;
    struct optGroup *next;
};

static struct optGroup *allOptions = NIL(struct optGroup);
static int             exitBad     = 1;

#define OPT_UNIV_CHAR '='
#define OPT_UNIV_FLAG (0200)

static char *expandOptChar();

int optGetOption(int argc, char *argv[]) {
    int             optChar;
    struct optGroup *optGrpPtr;
    optionStruct    *optPtr;

    int doUnivOption();

    /* initialize, if necessary */
    if (allOptions == NIL(struct optGroup)) {
        optInit(argv[0], optionList, 0);

        optAddUnivOptions();
    }

    optarg = NIL(char);
    usageProlog[0] = '\0';

    if (scanPtr == NIL(char) || *scanPtr == '\0') {
        if (optind >= argc || argv[optind][0] != '-' || strlen(argv[optind]) < 2 ||
            strcmp(argv[optind], "--") == 0) {
            return (EOF);
        }
        scanPtr = &argv[optind++][1];
    }

    optChar = *scanPtr++;
    if (optChar == OPT_UNIV_CHAR && *scanPtr) {
        optChar = *scanPtr++ | OPT_UNIV_FLAG;
    }

    for (optGrpPtr = allOptions; optGrpPtr; optGrpPtr = optGrpPtr->next) {
        for (optPtr = optGrpPtr->array; optPtr->optionLetters; optPtr++) {
            if (strchr(optPtr->optionLetters, optChar)) {
                /* we found the option */
                /* Make  sure that it is not one of those dummy options. */
                if (optPtr->optionLetters == OPT_RARG ||
                    optPtr->optionLetters == OPT_OARG ||
                    optPtr->optionLetters == OPT_ELLIP ||
                    optPtr->optionLetters == OPT_DESC ||
                    optPtr->optionLetters == OPT_CONT)
                    continue; /* Skip dummy opt. */
                /* OK, it is a regular option. */
                if (optPtr->argName && optPtr->argName[0] != '\0') {
                    /* it takes an argumment */
                    optarg = (*scanPtr == '\0') ? argv[optind++] : scanPtr;
                    if (optarg == NIL(char)) {
                        (void) sprintf(usageProlog, "%s: option %s requires an argument",
                                       optProgName, expandOptChar(optChar));
                        if (exitBad)

                            optUsage();

                        return ('?');
                    }
                    scanPtr = NIL(char);
                }
                if (optChar & OPT_UNIV_FLAG) {
                    if (doUnivOption(optChar)) {
                        return (optGetOption(argc, argv));
                    } else {
                        if (exitBad)

                            optUsage();

                        return ('?');
                    }
                } else {
                    return (optChar);
                }
            }
        }
    }

    (void) sprintf(usageProlog, "%s: unknown option -- %s", optProgName,
                   expandOptChar(optChar));
    if (exitBad)

        optUsage();

    return ('?');
}

void optInit(progName, options, rtnBadFlag)char *progName;
                                           optionStruct options[];
                                           int rtnBadFlag;
{
    struct optGroup *optGrpPtr;

    while (allOptions) {
        optGrpPtr = allOptions->next;
        FREE(allOptions);
        allOptions = optGrpPtr;
    }

    /* errProgramName(optProgName = progName); */
    optAddOptions(options);
    exitBad = !rtnBadFlag;

    optind  = 1;
    scanPtr = NIL(char);
}

void optAddOptions(options)optionStruct options[];
{
    struct optGroup *optGrpPtr;

    optGrpPtr = ALLOC(struct optGroup, 1);
    optGrpPtr->next  = allOptions;
    optGrpPtr->array = options;
    allOptions = optGrpPtr;
}

void optAddUnivOptions() {
#ifndef _IBMR2
    static optionStruct univOptions[] = {
            {"T", "tech_root_dir", "set the technology root directory"},
            {"E", "on_error",
             "cause fatal errors to core dump (on_error = \"core\") or exit "
             "(on_error = \"exit\")"},
#ifdef notdef
    {"M", "mem_flags",
     "adjust memory manager behavior:  mem_flags is comma-separated list of"},
    {OPT_CONT, 0, "\n    [no]trace   turn tracing on/off"},
    {OPT_CONT, 0, "\n    [no]trash   do/don't trash freed memory"},
    {OPT_CONT, 0, "\n    pad=n       pad each block with `n' extra words"},
#endif
            {0, 0, 0}};
    optionStruct        *optPtr;
    char                *cPtr;
    char *tapRootDirectory();

    for (optPtr = univOptions; optPtr->optionLetters; optPtr++) {
        for (cPtr = optPtr->optionLetters; *cPtr; cPtr++) {
            *cPtr |= OPT_UNIV_FLAG;
        }
    }

    if (tapRootDirectory(NIL(char)) == NIL(char)) {
        /* tap isn't really loaded */
        optAddOptions(&univOptions[1]);
    } else {
        optAddOptions(univOptions);
    }
#endif
}

static int doUnivOption(optChar)int optChar;
{
    char *tapRootDirectory();
#ifdef notdef
    int parseMMoptions();
#endif

    switch (optChar) {
        case 'T' | OPT_UNIV_FLAG:(void) tapRootDirectory(optarg);
            return (1);
        case 'E' | OPT_UNIV_FLAG:
            if (strcmp(optarg, "core") == 0) {
                /* errCore(1); */
                return (1);
            } else if (strcmp(optarg, "exit") == 0) {
                /* errCore(0); */
                return (1);
            } else {
                (void) sprintf(usageProlog, "%s: bad argument for option %s -- %s",
                               optProgName, expandOptChar(optChar), optarg);
                return (0);
            }
#ifdef notdef
        case 'M' | OPT_UNIV_FLAG:
          return (parseMMoptions());
#endif
        default:fprintf(stderr, "unknown universal option `%c'", optChar);
            exit(-1);
            /*NOTREACHED*/
    }
}

#ifdef notdef
static int parseMMoptions() {
  char *oneOpt;
  int val;

  for (oneOpt = strtok(optarg, ","); oneOpt; oneOpt = strtok(NIL(char), ",")) {
    if (strcmp(oneOpt, "trace") == 0) {
      mm_set(MM_TRACE, 1);
    } else if (strcmp(oneOpt, "notrace") == 0) {
      mm_set(MM_TRACE, 0);
    } else if (strcmp(oneOpt, "trash") == 0) {
      mm_set(MM_TRASH, 1);
    } else if (strcmp(oneOpt, "notrash") == 0) {
      mm_set(MM_TRASH, 0);
    } else if (sscanf(oneOpt, " pad = %d", &val) == 1) {
      mm_set(MM_PAD, val);
    } else {
      (void)sprintf(usageProlog, "%s: bad memory manager flag -- %s",
                    optProgName, oneOpt);
    }
  }
  return (1);
}
#endif

void optUsage() {
    (void) fprintf(stderr, "%s", optUsageString());
    exit(2);
}

char *optUsageString() {
    usageStrlen = 0;

    if (usageProlog[0] != '\0') {
        formatString(usageProlog);
        formatFlush();
    }

    usageSummary();
    usageDetailed();
    formatFlush();

    return (usageString);
}

#define START_OPTION_GROUP()                                                   \
  if (inGroup++ == 0) {                                                        \
    formatString(" [-");                                                       \
  }
#define FINISH_OPTION_GROUP()                                                  \
  if (inGroup != 0) {                                                          \
    formatChar(']');                                                           \
    inGroup = 0;                                                               \
  }

static void usageSummary() {
    struct optGroup *optGrpPtr;
    optionStruct    *optPtr;
    int             inGroup = 0;

    formatInit(SUMMARY_INDENT);
    formatString("usage: ");
    formatString(optProgName);
    for (optGrpPtr = allOptions; optGrpPtr; optGrpPtr = optGrpPtr->next) {
        for (optPtr = optGrpPtr->array; optPtr->optionLetters; optPtr++) {
            if (optPtr->optionLetters == OPT_DESC ||
                optPtr->optionLetters == OPT_CONT) {
                /* descriptions don't appear in summary line */
                ;
            } else if (optPtr->optionLetters == OPT_ELLIP) {
                FINISH_OPTION_GROUP();
                formatString(" ...");
            } else if (optPtr->optionLetters == OPT_RARG) {
                FINISH_OPTION_GROUP();
                formatChar(' ');
                formatString(optPtr->argName);
            } else if (optPtr->optionLetters == OPT_OARG) {
                FINISH_OPTION_GROUP();
                formatString(" [");
                formatString(optPtr->argName);
                formatChar(']');
            } else if (optPtr->argName && optPtr->argName[0] != '\0') {
                FINISH_OPTION_GROUP();
                formatString(" [");
                formatOptChoice(optPtr->optionLetters);
                formatChar(HARD_SP);
                formatString(optPtr->argName);
                formatChar(']');
            } else {
                START_OPTION_GROUP();
                formatString(optPtr->optionLetters);
            }
        }
    }
    FINISH_OPTION_GROUP();
}

static void usageDetailed() {
    struct optGroup *optGrpPtr;
    optionStruct    *optPtr;

    for (optGrpPtr = allOptions; optGrpPtr; optGrpPtr = optGrpPtr->next) {
        for (optPtr = optGrpPtr->array; optPtr->optionLetters; optPtr++) {
            if (optPtr->optionLetters == OPT_CONT) {
                formatString(optPtr->description);
            } else if (optPtr->optionLetters == OPT_DESC) {
                formatInit(0);
                formatString(optPtr->description);
            } else if (optPtr->optionLetters == OPT_ELLIP) {
                /* ellipses don't appear in the detailed info */
                ;
            } else {
                formatInit(DETAIL_INDENT);
                formatString("   ");
                if (optPtr->optionLetters == OPT_RARG) {
                    formatString(optPtr->argName);
                } else if (optPtr->optionLetters == OPT_OARG) {
                    formatChar('[');
                    formatString(optPtr->argName);
                    formatChar(']');
                } else {
                    formatOptChoice(optPtr->optionLetters);
                }
                formatString(": ");
                formatTab(DETAIL_INDENT);
                formatString(optPtr->description);
            }
        }
    }
}

static int  curColumn = 0;
static int  indent;
static char lineBuffer[LINEWIDTH];

static void formatInit(contIndent)int contIndent;
{
    if (curColumn > 0)
        formatFlush();
    indent = contIndent;
}

static void formatFlush() {
    int i;

    usageString = REALLOC(char, usageString, usageStrlen + curColumn + 2);
    for (i      = 0; i < curColumn; i++) {
        usageString[usageStrlen++] =
                (lineBuffer[i] == HARD_SP) ? ' ' : lineBuffer[i];
    }
    usageString[usageStrlen++] = '\n';
    usageString[usageStrlen]   = '\0';
    curColumn = 0;
}

static void formatString(string)char *string;
{
    while (*string != '\0')
        formatChar(*string++);
}

static void formatChar(ch)int ch;
{
    if (ch == '\n') {
        formatFlush();
        formatTab(indent);
    } else {
        if (curColumn >= LINEWIDTH) {
            int i;

            /* find and mark a space for splitting the line */
            for (i = LINEWIDTH - 1; i > 0; i--) {
                if (lineBuffer[i] == ' ') {
                    curColumn = i;
                    break;
                }
            }
            formatFlush();

            /* space out for the indent */
            formatTab(indent);

            /* copy anything that was after the line break */
            while (++i < LINEWIDTH) {
                lineBuffer[curColumn++] = lineBuffer[i];
            }
        }

        lineBuffer[curColumn++] = ch;
    }
}

static void formatTab(position)int position;
{
    if (position >= LINEWIDTH)
        position = LINEWIDTH - 1;

    while (curColumn < position)
        formatChar(' ');
}

static void formatOptChoice(optChars)char *optChars;
{
    int firstFlag = 1;

    while (*optChars != '\0') {
        if (firstFlag) {
            firstFlag = 0;
        } else {
            formatChar('|');
        }
        formatChar('-');
        formatString(expandOptChar(*optChars++));
    }
}

static char *expandOptChar(optChar)int optChar;
{
    static char string[3];
    char        *cPtr;

    cPtr    = string;
    if (optChar & OPT_UNIV_FLAG) {
        optChar &= ~OPT_UNIV_FLAG;
        *cPtr++ = OPT_UNIV_CHAR;
    }
    *cPtr++ = optChar;
    *cPtr   = '\0';
    return (string);
}
