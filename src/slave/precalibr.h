#ifndef PSPS_PRECALIBR_H
#define PSPS_PRECALIBR_H

/* PSP Slave headers */
#include "state.h"

/* precalibration initialization and finalization */
void init_precalibr(struct slave_state *);
void fini_precalibr(struct slave_state *);

/* precalibration timestamp handler */
void precalibr_handle_ts(struct slave_state *, double, double);

#endif /* PSPS_PRECALIBR_H */
