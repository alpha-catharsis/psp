#ifndef PSPS_STATE_H
#define PSPS_STATE_H

/* C standard library headers */
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/* PSP Common headers */
#include "../common/timestamp.h"

/* PSP Slave headers */
#include "options.h"
#include "stats.h"
#include "summ_stats.h"

/* slave state structure */
struct slave_state
{
  /* action options */
  int action;
  int synch_algo;
  int perc;
  long drift_win;
  long obs_win;
  int simulate;
  FILE *stats_file;

  /* latency data */
  struct stats lat_stats;
  struct summ_stats lat_summ_stats;
  long offset;
  long sim_offset;

  /* slave socket and port */
  int socket_desc;
  in_port_t slave_port;

  /* timestamp reception data */
  long pkt_cnt;
  ts_pkt_idx_t pkt_idx;
  long drift;
  size_t pkt_size;
  uint8_t *pkt_buff;
  struct timespec first_ts;

  /* secure protocol data */
  int secure;
  uint8_t key[32];

  /* debugging files */
  FILE *lat_file;
  FILE *corr_file;
};

/* master data structure */
struct slave_data
{
  struct options opts;
  struct slave_state state;
};

/* slave state management functions */
void init_state_from_options(struct slave_state *, const struct options *);
void fini_state(void *);

#endif /* PSPS_STATE_H */
