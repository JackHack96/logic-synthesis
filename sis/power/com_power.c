/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/power/com_power.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:05 $
 *
 */
/*---------------------------------------------------------------------------
|   Interface routines between SIS and the probabilistic power estimation
| package:
|
|       init_sis()
|       end_sis()
|
| Copyright (c) 1991 - Abhijit Ghosh
| University of California at Berkeley
|
| Jose' Monteiro, MIT, Jan/93            jcm@rle-vlsi.mit.edu
+--------------------------------------------------------------------------*/

#include "sis.h"
#include "power_int.h"

static int com_power_main();
static int free_power_info();
static int print_power_info();

static struct{
    char *name;
    int (*function)();
    boolean changes_network;
} table[] = {
    { "power_estimate", com_power_main, TRUE /* Adds prob info */},
    { "power_free_info", free_power_info, TRUE /* Frees prob info */},
    { "power_print", print_power_info, FALSE /* Doesn't change network */}
};

int init_power()
{
    int i;

    for(i = 0; i < sizeof_el(table); i++){
        com_add_command(table[i].name, table[i].function,
                        table[i].changes_network);
    }
}

end_power()
{
}


/* Main driver for deriving the power of a network */
static int com_power_main(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int status;

    status = power_command_line_interface(*network, argc, argv);

    return status;
}


static int free_power_info(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int status;

    if(argc != 1){
        fprintf(siserr, "Too many arguments. Usage: power_free_info\n");
        return 1;
    }

    status = power_free_info();

    return status;
}


static int print_power_info(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int status;

    if(argc != 1){
        fprintf(siserr, "Too many arguments. Usage: power_print\n");
        return 1;
    }

    status = power_print_info(*network);

    return status;
}


