#ifndef PSPS_STATE_H
#define PSPS_STATE_H

/* C standard library headers */
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/* PSP Common headers */
#include "../common/timestamp.h"

/* PSP Slave headers */
#include "basic_stats.h"
#include "least_squares.h"
#include "options.h"
#include "perc_stats.h"

/* slave state structure */
struct slave_state
{
  /* slave socket and port */
  int socket_desc;
  in_port_t slave_port;

  /* secure protocol data */
  int secure;
  uint8_t key[32];

  /* timestamp reception data */
  long pkt_cnt;
  ts_pkt_idx_t pkt_idx;
  size_t pkt_size;
  uint8_t *pkt_buff;
  struct timespec first_ts;
  double offset_corr;
  double freq_corr;

  /* action */
  int action;
  int simulate;
  int debug;
  
  /* statistics */
  struct basic_stats bs;
  struct perc_stats ps;
  struct least_squares ls;

  /* dynamic data */
  double first_ts_time;
  
  /* files */
  FILE *out_file;
  FILE *debug_lat_file;
  FILE *debug_lat_cdf_file;
  FILE *debug_freq_off_file;
  FILE *debug_time_corr_file;
  FILE *debug_freq_corr_file;
};

/* slave data structure */
struct slave_data
{
  struct options opts;
  struct slave_state state;
};

/* slave state management functions */
void init_state_from_options(struct slave_state *, const struct options *);
void fini_state(void *);

#endif /* PSPS_STATE_H */
