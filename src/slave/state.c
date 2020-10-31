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
#include "synch.h"

/* slave state management functions */
void init_state_from_options(struct slave_state *state_ptr, const struct options *opt_ptr)
{
  /* trivial state initializaton */
  state_ptr->pkt_cnt = opt_ptr->max_pkt_cnt;
  state_ptr->pkt_idx = 0;
  state_ptr->pkt_buff = NULL;
  state_ptr->clk_freq_ofs = 0.;
  state_ptr->action = opt_ptr->action;
  state_ptr->debug = opt_ptr->debug;
  state_ptr->obs_win_start_time = -1.;
  state_ptr->first_clk_time = -1.;
  state_ptr->synch_method = opt_ptr->synch_method;
  state_ptr->freq_estim_slots = opt_ptr->freq_estim_slots;
  state_ptr->time_step_thr = (double)opt_ptr->time_step_thr / 1e6;
  state_ptr->time_corr_gain = 1. - (double)opt_ptr->time_corr_damp / 100.;
  state_ptr->freq_corr_gain = 1. - (double)opt_ptr->freq_corr_damp / 100.;
  state_ptr->time_corr_max = (double)opt_ptr->time_corr_clamp * 1e-9;
  state_ptr->freq_corr_max = (double)opt_ptr->freq_corr_clamp * 1e-9;
  state_ptr->qs_rounds = opt_ptr->qs_rounds;
  state_ptr->time_cumul_corr = 0.;
  state_ptr->freq_cumul_corr = 0.;
  state_ptr->obs_win = opt_ptr->obs_win;
  state_ptr->out_file = NULL;
  state_ptr->debug_timestamp_file = NULL;
  state_ptr->debug_corr_time_delta_file = NULL;
  state_ptr->debug_time_delta_cdf_file = NULL;
  state_ptr->debug_freq_delta_file = NULL;
  state_ptr->debug_time_error_file = NULL;
  state_ptr->debug_time_corr_file = NULL;
  state_ptr->debug_time_cumul_corr_file = NULL;
  state_ptr->debug_freq_error_file = NULL;
  state_ptr->debug_freq_corr_file = NULL;
  state_ptr->debug_freq_cumul_corr_file = NULL;

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
  long max_obs_win = state_ptr->obs_win;
  for(long i = 0; i < state_ptr->qs_rounds; i++){
    max_obs_win *= 2;
  }
  reset_basic_stats(&state_ptr->bs);
  init_perc_stats(&state_ptr->ps, max_obs_win);
  init_least_squares(&state_ptr->ls, 1000);

  /* packet buffer initialization */
  state_ptr->pkt_size = ts_pkt_size(state_ptr->secure);
  state_ptr->pkt_buff = malloc(state_ptr->pkt_size + 1); /* +1 is needed to detect packets 
                                                            longer than valid ones */
  if(!state_ptr->pkt_buff){
    output(erro_lvl, "cannot allocate buffer for timestamp packets transmission");
  }

  /* file initialization */
  switch(state_ptr->action){
  case action_precalibr:
    init_precalibr(state_ptr);
    break;
  case action_calibr:
    init_calibr(state_ptr);
    break;
  case action_synch:
    init_synch(state_ptr);
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
      fini_synch(state_ptr);
      break;
  }

  free(state_ptr->pkt_buff);
  if(state_ptr->out_file){
    fclose(state_ptr->out_file);
  }
  if(state_ptr->debug_timestamp_file){
    fclose(state_ptr->debug_timestamp_file);
  }
  if(state_ptr->debug_corr_time_delta_file){
    fclose(state_ptr->debug_corr_time_delta_file);
  }
  if(state_ptr->debug_time_delta_cdf_file){
    fclose(state_ptr->debug_time_delta_cdf_file);
  }
  if(state_ptr->debug_freq_delta_file){
    fclose(state_ptr->debug_freq_delta_file);
  }
  if(state_ptr->debug_time_error_file){
    fclose(state_ptr->debug_time_error_file);
  }
  if(state_ptr->debug_time_corr_file){
    fclose(state_ptr->debug_time_corr_file);
  }
  if(state_ptr->debug_time_cumul_corr_file){
    fclose(state_ptr->debug_time_cumul_corr_file);
  }
  if(state_ptr->debug_freq_error_file){
    fclose(state_ptr->debug_freq_error_file);
  }
  if(state_ptr->debug_freq_corr_file){
    fclose(state_ptr->debug_freq_corr_file);
  }
  if(state_ptr->debug_freq_cumul_corr_file){
    fclose(state_ptr->debug_freq_cumul_corr_file);
  }

  fini_perc_stats(&state_ptr->ps);
  fini_least_squares(&state_ptr->ls);
}
