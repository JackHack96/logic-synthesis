

#define MAX_STEP 20

struct u {
    struct {
        int verbose;    /* verbose level */
        int hmap;        /* mapping heuristics */
        int solution;    /* solution heuristics */
        int heuristics;    /* binate covering heuristics */
    }    opt;
    struct {
        int   n_compatible;    /* number of compatible pair */
        int   n_iso;            /* number of base isomorphic states */
        int   n_max;            /* number of maximal compatibles */
        int   base_max;        /* number of base maximal compatibles */
        int   n_prime;        /* number of prime compatibles */
        int   nstates;        /* number of original states */
        int   rstates;        /* number of states which are compatible */
        int   ostates;        /* number of incompatible states */
        int   product;        /* number of product terms */
        int   high;            /* high bound */
        int   low;            /* low bound */
        int   shrink;            /* number of shrunk states */
        int   xinput;
        int   disjoint;
        int   map_alternative;    /* mapping alternative */
        int   map_total;            /* total number of choice */
        int   reset;            /* Is there reset state */
        float quality;            /* mapping quality */
    }    stat;
    struct {
        int shrink;
        int merge;
        int trans;
    }    cmd;
    long ltime[MAX_STEP];        /* for timing information */
    int  iso;
    int  level;
    char *fname;        /* input file name */
    char *oname;        /* output file name */
};

extern struct u user;

#define HEU1 1
