
#include "io_int.h"
#include "sis.h"

void write_pla(FILE *fp, network_t *network) {
    pPLA PLA;

    PLA = network_to_pla(network);
    if (PLA == 0)
        return;

    /* Let espresso do the dirty work */
    if (PLA->D) {
        fprint_pla(fp, PLA, FD_type);
    } else {
        fprint_pla(fp, PLA, F_type);
    }
    discard_pla(PLA);
}
