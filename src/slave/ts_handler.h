#ifndef PSPS_TS_HANDLER_H
#define PSPS_TS_HANDLER_H

/* PSP Slave headers */
#include "state.h"

/* typedefs */
typedef void (*ts_handler)(struct slave_state *, double, double);

#endif /* PSPS_TS_HANDLER_H */
