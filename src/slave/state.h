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
  double clk_freq_ofs;

  /* action */
  int action;
  int debug;
  
  /* statistics */
  struct basic_stats bs;
  struct perc_stats ps;
  struct least_squares ls;
  double median_time_off;

  /* dynamic data */
  double obs_win_start_time;
  double first_clk_time;
  double first_delta;
  double time_step_thr;
  long qs_rounds;
  double time_cumul_corr;
  long obs_win;

  /* files */
  FILE *out_file;
  FILE *debug_timestamp_file;
  FILE *debug_corr_time_delta_file;
  FILE *debug_time_delta_cdf_file;
  FILE *debug_freq_delta_file;
  FILE *debug_time_corr_file;
  FILE *debug_time_cumul_corr_file;
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
