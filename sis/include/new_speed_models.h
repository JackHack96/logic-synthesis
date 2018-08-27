

/*
 * To avoid an extra sorting operation, It is REQUIRED that the transforms
 * be listed in decreasing priority. Lower value of priority means that the
 * transform is preferred. If there is a saving from low valued
 * transforms, the higher ones are not evaluated
 */

static sp_xform_t local_transforms[] = {
    /* name, optimization fn, delay computing fn, priority, on-flag, netw-flag
     */

    {"noalg", sp_noalg_opt, new_delay_arrival, 0, 0, CLP},
    {"repower", sp_fanout_opt, new_delay_required, 0, 0, FAN},
    {"fanout", sp_fanout_opt, new_delay_required, 1, 0, FAN},
    {"duplicate", sp_duplicate_opt, new_delay_required, 1, 0, FAN},
    {"and_or", sp_and_or_opt, new_delay_arrival, 2, 0, CLP},
    {"divisor", sp_divisor_opt, new_delay_arrival, 2, 1, CLP},
    {"2c_kernel", sp_2c_kernel_opt, new_delay_arrival, 2, 0, CLP},
    {"comp_div", sp_comp_div_opt, new_delay_arrival, 2, 0, CLP},
    {"comp_2c", sp_comp_2c_opt, new_delay_arrival, 2, 0, CLP},
    {"cofactor", sp_cofactor_opt, new_delay_arrival, 2, 0, CLP},
    {"bypass", sp_bypass_opt, new_delay_arrival, 2, 0, CLP},
    {"dualize", sp_dual_opt, new_delay_slack, 2, 0, DUAL},
};
