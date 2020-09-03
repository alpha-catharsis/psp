/* C standard library headers */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* PSP Common headers */
#include "../common/mgmt.h"
#include "../common/output.h"

/* PSP Slave headers */
#include "options.h"
#include "state.h"

/* functions forward declarations */
static void mngd_main(void *);
static void receive_timestamp(struct slave_state *);
static void handle_timestamp(struct slave_state *, struct timespec *,
			     ts_pkt_idx_t, time_t, long);
static void print_lat_data(struct slave_state *, double);
static void synchronize(struct slave_state *);

/* main function */
int main(int argc, char **argv)
{
  struct slave_data data;
  if(parse_command_line(argc, argv, &data.opts)){
    return run_managed(&mngd_main, &fini_state, &data);
  }else{
    return EXIT_FAILURE;
  }
}

/* managed main function */
static void mngd_main(void *ptr)
{
  struct slave_data *data_ptr = (struct slave_data *) ptr;
  struct options *opts_ptr = (struct options *) &data_ptr->opts;
  struct slave_state *state_ptr = (struct slave_state *) &data_ptr->state;
  apply_general_options(&opts_ptr->gen_opts);
  print_selected_options(opts_ptr);
  init_state_from_options(state_ptr, opts_ptr);
  receive_timestamp(state_ptr);
}

/* receive timestamp function */
static void receive_timestamp(struct slave_state *state_ptr)
{
  struct timespec ts;
  ts_pkt_idx_t idx;
  time_t sec;
  long nsec;
  ssize_t bytes_read;
  struct sockaddr_in master_addr;
  socklen_t addrlen;
  while(1){
    addrlen = sizeof(master_addr);
    errno = 0;
    bytes_read = recvfrom(state_ptr->socket_desc, state_ptr->pkt_buff,
			  state_ptr->pkt_size + 1, 0,
			  (struct sockaddr *)&master_addr, &addrlen);
    if(clock_gettime(CLOCK_REALTIME, &ts) == -1){
      output(erro_lvl, "failure reading time: %s", strerror(errno));
    }else if(bytes_read == -1){
      if(errno != EINTR){
	output(erro_lvl, "recvfrom failure: %s", strerror(errno));
      }
    }else{
      output(debg_lvl, "received packet from %s:%hu",
	     inet_ntoa(master_addr.sin_addr),
	     ntohs(master_addr.sin_port));
      if(bytes_read == (ssize_t) state_ptr->pkt_size){
	if(!read_ts_pkt(state_ptr->pkt_buff, state_ptr->secure, &idx,
			&sec, &nsec, state_ptr->key)){
	  output(warn_lvl, "discarded packet due to hmac mismatch");
	}else if(idx > state_ptr->pkt_idx){
	  handle_timestamp(state_ptr, &ts, idx, sec, nsec);
	}else{
	  output(warn_lvl, "discarded packet due to idx %lu <= %lu",
		 idx, state_ptr->pkt_idx);
	}
      }else{
	output(warn_lvl, "discarded packet due to invalid size");
      }
    }
  }
}

void handle_timestamp(struct slave_state *state_ptr, struct timespec *ts,
		      ts_pkt_idx_t idx, time_t sec, long nsec)
{
  double delta;
  double time;

  state_ptr->pkt_idx = idx;
  output(debg_lvl, "idx %09lu secs: %09lu nsecs: %09lu", idx,
	 sec, nsec);

  time = (double)ts->tv_sec + ((double)ts->tv_nsec) * 1e-9;
  delta = ((double) (ts->tv_sec - sec)) + ((double) (ts->tv_nsec - nsec)) * 1e-9 +
    (double)state_ptr->offset * 1e-6 + (double)state_ptr->sim_offset * 1e-9;

  if(state_ptr->first_ts.tv_nsec == -1){
    state_ptr->first_ts.tv_sec = ts->tv_sec;
    state_ptr->first_ts.tv_nsec = ts->tv_nsec;
  }else{
    delta -= ((double) (ts->tv_sec - state_ptr->first_ts.tv_sec) +
	      (double) (ts->tv_nsec - state_ptr->first_ts.tv_nsec) * 1e-9) *
             ((double) state_ptr->drift * 1e-9);
  }
  
  add_sample(&state_ptr->lat_stats, time, delta);
  print_lat_data(state_ptr, delta);

  if(state_ptr->lat_file && (fprintf(state_ptr->lat_file, "%.15f\n", delta) < 0)){
    output(erro_lvl, "cannot write latency sample to file");
  }

  if((state_ptr->action == action_synch) &&
     (stats_count(&state_ptr->lat_stats) == state_ptr->obs_win)){
    synchronize(state_ptr);
  }

  if(state_ptr->pkt_cnt >= 0){
    state_ptr->pkt_cnt--;
  }
  if(state_ptr->pkt_cnt == 0){
    output(info_lvl, "finished receiving timestamps. Exiting...");
    clean_exit();
  }
}

void print_lat_data(struct slave_state *state_ptr, double delta)
{
  output(debg_lvl, "delta: %.6f | min: %.6f | max: %.6f", delta,
	 stats_min(&state_ptr->lat_stats), stats_max(&state_ptr->lat_stats));
  output(debg_lvl, "mean: %.6f | stddev: %.6f | mean stddev: %.6f",
	 stats_mean(&state_ptr->lat_stats), 
	 stats_stddev(&state_ptr->lat_stats), 
	 stats_mean_stddev(&state_ptr->lat_stats));
  if(state_ptr->perc){
    output(debg_lvl, "p10: %.6f | p25: %.6f | p50: %.6f | p99: %.6f",
	   stats_percentile(&state_ptr->lat_stats, 0.10),
	   stats_percentile(&state_ptr->lat_stats, 0.25),
	   stats_percentile(&state_ptr->lat_stats, 0.50),
	   stats_percentile(&state_ptr->lat_stats, 0.99));
  }
  if(state_ptr->drift_win){
    output(debg_lvl, "total drift: %.9f | drift: %.9f", stats_cumul_drift(&state_ptr->lat_stats),
	   stats_drift(&state_ptr->lat_stats));
  }
}

void synchronize(struct slave_state *state_ptr)
{
  struct timespec ts;
  double corr = 0;
  switch(state_ptr->synch_algo){
  case algo_mean:
    corr = state_ptr->lat_summ_stats.mean - stats_mean(&state_ptr->lat_stats);
    break;
  case algo_p10:
    corr = state_ptr->lat_summ_stats.perc[0] - stats_percentile(&state_ptr->lat_stats, 0.10);
    break;
  case algo_p25:
    corr = state_ptr->lat_summ_stats.perc[1] - stats_percentile(&state_ptr->lat_stats, 0.25);
    break;
  case algo_median:
    corr = state_ptr->lat_summ_stats.perc[2] - stats_percentile(&state_ptr->lat_stats, 0.50);
    break;
  }
  output(info_lvl, "applying correction: %.6f", corr);
  if(state_ptr->corr_file && (fprintf(state_ptr->corr_file, "%.15f\n", corr) < 0)){
    output(erro_lvl, "cannot write clock correction to file");
  }
  if(state_ptr->simulate){
    state_ptr->sim_offset += (long)(corr * 1e9);
  }else{
    if(clock_gettime(CLOCK_REALTIME, &ts) == -1){
      output(erro_lvl, "failure reading realtime clock");
    }else{
      double time = (double) ts.tv_sec + (double) ts.tv_nsec * 1e-9;
      time += corr;
      ts.tv_sec  = (time_t) floor(time);
      ts.tv_nsec = (long) ((time - floor(time)) * 1e9);
      if(clock_settime(CLOCK_REALTIME, &ts) == -1){
	output(erro_lvl, "failure setting realtime clock");
      }
    }
  }
  reset_stats(&state_ptr->lat_stats);
}
