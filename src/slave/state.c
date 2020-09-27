/* C standard library headers */
#include <errno.h>
#include <memory.h>
#include <stdlib.h>

/* POSIX library headers */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
#include "calibr.h"
#include "precalibr.h"
#include "state.h"

/* slave state management functions */
void init_state_from_options(struct slave_state *state_ptr, const struct options *opt_ptr)
{
  /* trivial state initializaton */
  state_ptr->pkt_cnt = opt_ptr->max_pkt_cnt;
  state_ptr->pkt_idx = 0;
  state_ptr->pkt_buff = NULL;
  state_ptr->first_ts.tv_sec = 0;
  state_ptr->first_ts.tv_nsec = 0;
  state_ptr->offset_corr = (double)opt_ptr->offset * 1e-6;
  state_ptr->freq_corr = 0.;
  state_ptr->action = opt_ptr->action;
  state_ptr->simulate = opt_ptr->simulate;
  state_ptr->debug = opt_ptr->debug;
  state_ptr->out_file = NULL;
  state_ptr->debug_lat_file = NULL;
  state_ptr->debug_lat_cdf_file = NULL;
  state_ptr->debug_freq_off_file = NULL;
  state_ptr->debug_time_corr_file = NULL;
  state_ptr->debug_freq_corr_file = NULL;

  /* socket initialization */
  struct sockaddr_in host_addr;
  host_addr.sin_family = AF_INET;
  host_addr.sin_port = opt_ptr->slave_port;
  host_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  state_ptr->socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
  if(state_ptr->socket_desc == -1){
    output(erro_lvl, "failure creating UDP socket");
  }else if(bind(state_ptr->socket_desc, (struct sockaddr *)&host_addr, sizeof(host_addr)) == -1){
    output(erro_lvl, "failure binding UDP socket");
  }

  /* security functions initialization */
  if(opt_ptr->key_filename){
    state_ptr->secure = 1;

    /* secure key loading */
    FILE *key_file = fopen(opt_ptr->key_filename, "r");
    if(!key_file){
      output(erro_lvl, "cannot open key file '%s' for reading", opt_ptr->key_filename);
    }else if(fread(&state_ptr->key, 32, 1, key_file) != 1){
	output(erro_lvl, "failure reading key from key file '%s'", opt_ptr->key_filename);
    }else if(fclose(key_file) == EOF){
      output(erro_lvl, "failure closing key file '%s'", opt_ptr->key_filename);
    }
  }else{
    state_ptr->secure = 0;
  }

  /* statistics initialization */
  reset_basic_stats(&state_ptr->bs);
  init_perc_stats(&state_ptr->ps, opt_ptr->obs_win);
  init_least_squares(&state_ptr->ls, 1000);

  /* packet buffer initialization */
  state_ptr->pkt_size = ts_pkt_size(state_ptr->secure);
  state_ptr->pkt_buff = malloc(state_ptr->pkt_size + 1); /* +1 is needed to detect packets 
                                                            longer than valid ones */
  if(!state_ptr->pkt_buff){
    output(erro_lvl, "cannot allocate buffer for timestamp packets transmission");
  }

  /* file initialization */
  FILE * in_file;
  switch(state_ptr->action){
  case action_precalibr:
    init_precalibr(state_ptr);
    break;
  case action_calibr:
    init_calibr(state_ptr);
    break;
  case action_synch:
    in_file = fopen("calibr_results.txt", "r");
    if(!in_file){
      output(erro_lvl, "cannot open calibration output file");
    }
    fclose(in_file);

    if(state_ptr->debug){
      state_ptr->debug_lat_file = fopen("synch_latency.txt", "w");
      if(!state_ptr->debug_lat_file){
	output(erro_lvl, "cannot open synchronization latency file");
      }
      state_ptr->debug_time_corr_file = fopen("synch_time_corr.txt", "w");
      if(!state_ptr->debug_time_corr_file){
	output(erro_lvl, "cannot open synchronization time corrections file");
      }
      state_ptr->debug_freq_corr_file = fopen("synch_freq_corr.txt", "w");
      if(!state_ptr->debug_freq_corr_file){
	output(erro_lvl, "cannot open synchronization freq corrections file");
      }
    }
    break;
  }

  // initialization finished
  output(debg_lvl, "Slave state created");
}

void fini_state(void *ptr)
{
  struct slave_data *data_ptr = (struct slave_data *) ptr;
  struct slave_state *state_ptr = (struct slave_state *) &data_ptr->state;

  switch(state_ptr->action){
    case action_precalibr:
      fini_precalibr(state_ptr);
      break;
    case action_calibr:
      fini_calibr(state_ptr);
      break;
    case action_synch:
      break;
  }

  free(state_ptr->pkt_buff);
  if(state_ptr->out_file){
    fclose(state_ptr->out_file);
  }
  if(state_ptr->debug_lat_file){
    fclose(state_ptr->debug_lat_file);
  }
  if(state_ptr->debug_lat_cdf_file){
    fclose(state_ptr->debug_lat_cdf_file);
  }
  if(state_ptr->debug_freq_off_file){
    fclose(state_ptr->debug_freq_off_file);
  }
  if(state_ptr->debug_time_corr_file){
    fclose(state_ptr->debug_time_corr_file);
  }
  if(state_ptr->debug_freq_corr_file){
    fclose(state_ptr->debug_freq_corr_file);
  }

  fini_perc_stats(&state_ptr->ps);
  fini_least_squares(&state_ptr->ls);
}
