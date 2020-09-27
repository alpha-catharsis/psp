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
#include "calibr.h"
#include "options.h"
#include "precalibr.h"
#include "state.h"
#include "ts_handler.h"

/* functions forward declarations */
static void mngd_main(void *);
static void receive_timestamp(struct slave_state *, ts_handler);

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
  switch(state_ptr->action)
  {
  case action_precalibr:
    receive_timestamp(state_ptr, precalibr_handle_ts);
    break;
  case action_calibr:
    receive_timestamp(state_ptr, calibr_handle_ts);
    break;
  case action_synch:
    /* perform_synch(state_ptr); */
    break;
  }
}

static void receive_timestamp(struct slave_state *state_ptr,
			      ts_handler handle_timestamp)
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
	  double delta;
	  double time;

	  if(state_ptr->first_ts.tv_sec == 0){
	    state_ptr->first_ts.tv_sec = ts.tv_sec;
	    state_ptr->first_ts.tv_nsec = ts.tv_nsec;
	  }

	  state_ptr->pkt_idx = idx;
	  output(debg_lvl, "idx %09lu secs: %09lu nsecs: %09lu", idx,
		 sec, nsec);

	  time = (double)ts.tv_sec + ((double)ts.tv_nsec) * 1e-9;
	  delta = ((double) (ts.tv_sec - sec)) + ((double) (ts.tv_nsec - nsec)) * 1e-9;
	  delta += state_ptr->offset_corr;
	  delta -= ((double) (ts.tv_sec - state_ptr->first_ts.tv_sec) +
		    (double) (ts.tv_nsec - state_ptr->first_ts.tv_nsec) * 1e-9) *
	    state_ptr->freq_corr;

	  add_basic_stats_sample(&state_ptr->bs, delta);
	  output(debg_lvl, "delta: %.6f", delta);
	  print_basic_stats(&state_ptr->bs, debg_lvl);
	  if(state_ptr->debug){
	    if(fprintf(state_ptr->debug_lat_file, "%.9f %.9f\n", time, delta) < 0){
	      output(erro_lvl, "cannot write latency sample to file");
	    }
	  }

	  handle_timestamp(state_ptr, time, delta);

          if(state_ptr->pkt_cnt >= 0){
	    state_ptr->pkt_cnt--;
	  }
	  if(state_ptr->pkt_cnt == 0){
	    clean_exit();
	  }
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


/* void synchronize(struct slave_state *state_ptr) */
/* { */
/*   (void) state_ptr; */

  /* struct timespec ts; */
  /* double corr = 0; */
  /* switch(state_ptr->synch_algo){ */
  /* case algo_mean: */
  /*   corr = state_ptr->lat_summ_stats.mean - stats_mean(&state_ptr->lat_stats); */
  /*   break; */
  /* case algo_p10: */
  /*   corr = state_ptr->lat_summ_stats.perc[0] - stats_percentile(&state_ptr->lat_stats, 0.10); */
  /*   break; */
  /* case algo_p25: */
  /*   corr = state_ptr->lat_summ_stats.perc[1] - stats_percentile(&state_ptr->lat_stats, 0.25); */
  /*   break; */
  /* case algo_median: */
  /*   corr = state_ptr->lat_summ_stats.perc[2] - stats_percentile(&state_ptr->lat_stats, 0.50); */
  /*   break; */
  /* } */
  /* output(info_lvl, "applying correction: %.6f", corr); */
  /* if(state_ptr->corr_file && (fprintf(state_ptr->corr_file, "%.15f\n", corr) < 0)){ */
  /*   output(erro_lvl, "cannot write clock correction to file"); */
  /* } */
  /* if(state_ptr->simulate){ */
  /*   state_ptr->sim_offset += (long)(corr * 1e9); */
  /* }else{ */
  /*   if(clock_gettime(CLOCK_REALTIME, &ts) == -1){ */
  /*     output(erro_lvl, "failure reading realtime clock"); */
  /*   }else{ */
  /*     double time = (double) ts.tv_sec + (double) ts.tv_nsec * 1e-9; */
  /*     time += corr; */
  /*     ts.tv_sec  = (time_t) floor(time); */
  /*     ts.tv_nsec = (long) ((time - floor(time)) * 1e9); */
  /*     if(clock_settime(CLOCK_REALTIME, &ts) == -1){ */
  /* 	output(erro_lvl, "failure setting realtime clock"); */
  /*     } */
  /*   } */
  /* } */
  /* reset_stats(&state_ptr->lat_stats); */
/* } */
