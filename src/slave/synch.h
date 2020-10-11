#ifndef PSPS_SYNCH_H
#define PSPS_SYNCH_H

/* PSP Slave headers */
#include "state.h"

/* synchation initialization and finalization */
void init_synch(struct slave_state *);
void fini_synch(struct slave_state *);

/* synchation timestamp handler */
void synch_handle_ts(struct slave_state *, double, double);

#endif /* PSPS_SYNCH_H */
