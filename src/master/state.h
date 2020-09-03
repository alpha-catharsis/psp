#ifndef PSPM_STATE_H
#define PSPM_STATE_H

/* C standard library headers */
#include <stdint.h>
#include <stdio.h>

/* PSP Common headers */
#include "../common/timestamp.h"

/* PSP Master headers */
#include "options.h"

/* master state structure */
struct master_state
{
  /* socket and slave address/port */
  int socket_desc;
  struct sockaddr_in slave_addr;

  /* timestamp transmission data */
  long period;
  long stagger;
  long pkt_cnt;
  ts_pkt_idx_t pkt_idx;
  size_t pkt_size;
  uint8_t *pkt_buff;

  /* secure protocol data */
  int secure;
  uint8_t key[32];
  FILE *nonce_file;
};

/* master data structure */
struct master_data
{
  struct options opts;
  struct master_state state;
};

/* master state management functions */
void init_state_from_options(struct master_state *, const struct options *);
void fini_state(void *);

#endif /* PSPM_STATE_H */
