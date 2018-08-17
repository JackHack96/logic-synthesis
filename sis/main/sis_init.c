
#include "sis.h"

void
init_sis(graphics_flag)
        int graphics_flag;
{
#if defined(bsd4_2) || defined(sun)    /* hack, but its nice to have ... */
    setlinebuf(stdout);
    setlinebuf(stderr);
#endif
#ifdef OCT
#ifdef SIS
    errProgramName("SIS - Version 1.2");
#else
    errProgramName("MIS - Version 2.2");
#endif
    errCore(1);
#endif

    sisout  = stdout;
    siserr  = stderr;
    sishist = NIL(FILE);
    init_command(graphics_flag);    /* must be first */
    init_node();
    init_network();
    init_io();
    init_extract();
    init_factor();
    init_decomp();
    init_resub();
    init_delay();
    init_map();
    init_genlib();
    init_phase();
    init_pld();
    init_sim();
    init_simplify();
    init_gcd();
#ifdef OCT
    init_octio();
#endif
    init_ntbdd();
    init_maxflow();
    init_speed();
    init_atpg();
    init_graphics();
#ifdef SIS
    init_latch();
    init_power();
    init_retime();
    init_graph();
    init_seqbdd();
    init_stg();
    init_clock();
    init_astg();
    init_timing();
#endif
    init_test();

    com_graphics_help();
}


void
end_sis() {
    end_test();
#ifdef SIS
    end_timing();
    end_astg();		/* Has daemons and uses node package.	*/
    end_clock();
    end_stg();
    end_seqbdd();
    end_graph();
    end_retime();
    end_power();
    end_latch();
#endif /* SIS */
    end_graphics();
    end_atpg();
    end_speed();
    end_maxflow();
    end_ntbdd();
#ifdef OCT
    end_octio();
#endif
    end_gcd();
    end_simplify();
    end_sim();
    end_phase();
    end_pld();
    end_genlib();
    end_map();
    end_delay();
    end_resub();
    end_decomp();
    end_factor();
    end_extract();
    end_io();
    end_network();
    end_command();
    end_node();        /* Should be last (to discard daemons).	*/
    if (sisout != stdout) (void) fclose(sisout);
    if (siserr != stderr) (void) fclose(siserr);
    if (sishist != NIL(FILE)) (void) fclose(sishist);
    sisout  = stdout;
    siserr  = stderr;
    sishist = NIL(FILE);
    sf_cleanup();
}
