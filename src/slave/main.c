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
#include "synch.h"
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
    receive_timestamp(state_ptr, synch_handle_ts);
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
	  state_ptr->pkt_idx = idx;
	  output(debg_lvl, "idx %09lu secs: %09lu nsecs: %09lu", idx,
		 sec, nsec);

	  double clk_time = (double)ts.tv_sec + ((double)ts.tv_nsec) * 1e-9;
	  double ts_time = (double)sec + ((double)nsec) * 1e-9;
	  double time_delta = clk_time - ts_time;

	  if(state_ptr->debug_timestamp_file){
	    if(fprintf(state_ptr->debug_timestamp_file, "%lu %.9f %.9f %.9f\n",
		       basic_stats_count(&state_ptr->bs),
		       clk_time,
		       ts_time,
		       time_delta) < 0){
	      output(erro_lvl, "cannot write timestamp information to file");
	    }
	  }

	  output(debg_lvl, "time delta: %.9f", time_delta);
	  add_basic_stats_sample(&state_ptr->bs, time_delta);
	  print_basic_stats(&state_ptr->bs, debg_lvl);

	  handle_timestamp(state_ptr, clk_time, time_delta);

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
