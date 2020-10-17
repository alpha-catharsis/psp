#ifndef PSPS_CALIBR_H
#define PSPS_CALIBR_H

/* PSP Slave headers */
#include "state.h"

/* calibration initialization and finalization */
void init_calibr(struct slave_state *);
void fini_calibr(struct slave_state *);

/* calibration timestamp handler */
void calibr_handle_ts(struct slave_state *, double, double);

#endif /* PSPS_CALIBR_H */
