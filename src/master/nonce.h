#ifndef PSPM_NONCE_H
#define PSPM_NONCE_H

/* C standard library headers */
#include <stdio.h>

/* PSP Common headers */
#include "../common/timestamp.h"

/* nonce management functions */
ts_pkt_idx_t read_nonce(FILE *);
void write_nonce(FILE *, ts_pkt_idx_t);

#endif /* PSPM_NONCE_H */
