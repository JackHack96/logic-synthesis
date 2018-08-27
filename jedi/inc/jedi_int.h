
/*
 * Symbolic encoding program for compiling a symbolic
 * description into a binary representation.  Target
 * is multi-level logic synthesis
 *
 * History:
 *
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

/*
 * constants
 */
#define INFINITY 100000 /* constant for infinity */
#define BUFSIZE 512     /* buffer size */
#define INPUT 0         /* input weighting */
#define OUTPUT 1        /* output weighting */
#define COUPLED 2       /* coupled weighting */
#define MAXSPACE 8      /* maximum Boolean dimensions */

#define RELEASE "official release 1.2"
#define DATE "June 5, 1991"
#define HEADER "JEDI, official release 1.2 (compiled: June 5, 1991)"

/*
 * flags
 */
Boolean addDontCareFlag; /* fully specify the machine */
Boolean bitsFlag;        /* specify a kiss-style input */
Boolean kissFlag;        /* specify a kiss-style input */
Boolean verboseFlag;     /* display verbose information */
Boolean sequentialFlag;  /* run sequential encoding */
Boolean clusterFlag;     /* run cluster encoding */
Boolean srandomFlag;     /* run static random */
Boolean drandomFlag;     /* run dynamic random */
Boolean variationFlag;   /* variation of weight calculation */
Boolean oneplaneFlag;    /* compute weights only on one plane */
Boolean hotFlag;         /* one hot encoding */
Boolean expandFlag;      /* code expansion flag */
Boolean plaFlag;         /* output PLA format */

/*
 * parameters
 */
double beginningStates;     /* number of beginning states for SA */
double endingStates;        /* number of ending states for SA */
double startingTemperature; /* starting temperature for SA */
double maximumTemperature;  /* maximum temperature for SA */
int weightType;             /* options for weighting type */

char *reset_state; /* reset state for state assignment */
int code_length;   /* code length for state assignemnt */
