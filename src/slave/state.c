/* C standard library headers */
#include <memory.h>
#include <stdlib.h>

/* POSIX library headers */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
#include "state.h"
#include "summ_stats.h"

/* slave state management functions */
void init_state_from_options(struct slave_state *state_ptr, const struct options *opt_ptr)
{
  FILE *aux;

  /* trivial state initializaton */
  state_ptr->action = opt_ptr->action;
  state_ptr->synch_algo = opt_ptr->synch_algo;
  state_ptr->perc = !opt_ptr->no_perc;
  state_ptr->drift_win = opt_ptr->drift_win;
  state_ptr->obs_win = opt_ptr->obs_win;
  state_ptr->simulate = opt_ptr->simulate;
  state_ptr->stats_file = NULL;
  state_ptr->socket_desc = 0;
  state_ptr->pkt_cnt = opt_ptr->max_pkt_cnt;
  state_ptr->pkt_idx = 0;
  state_ptr->pkt_buff = NULL;
  state_ptr->first_ts.tv_nsec = -1;
  state_ptr->offset = opt_ptr->offset;
  state_ptr->drift = opt_ptr->drift;
  state_ptr->sim_offset = 0;
  state_ptr->lat_file = NULL;
  state_ptr->corr_file = NULL;
  
  /* stats file initialization */
  aux = fopen(opt_ptr->stats_filename,
	      opt_ptr->action == action_calibr ? "w" : "r");
  if(!aux){
      output(erro_lvl, "cannot open stats file '%s'", opt_ptr->stats_filename);
  }else{
    state_ptr->stats_file = aux;
  }

  /* latency statistics initialization */
  if(state_ptr->action == action_calibr){
    init_stats(&state_ptr->lat_stats, state_ptr->perc, state_ptr->pkt_cnt, state_ptr->drift_win);
  }else{
    init_stats(&state_ptr->lat_stats, state_ptr->perc, state_ptr->obs_win, 0);
    read_summ_stats(state_ptr->stats_file, &state_ptr->lat_summ_stats);
  }
  
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

  /* packet buffer initialization */
  state_ptr->pkt_size = ts_pkt_size(state_ptr->secure);
  state_ptr->pkt_buff = malloc(state_ptr->pkt_size + 1); /* +1 is needed to detect packets 
                                                            longer than valid ones */
  if(!state_ptr->pkt_buff){
    output(erro_lvl, "cannot allocate buffer for timestamp packets transmission");
  }

  /* debug files initialization */
  if(opt_ptr->lat_filename){
    aux = fopen(opt_ptr->lat_filename, "w");
    if(!aux){
      output(erro_lvl, "cannot open lat file '%s'", opt_ptr->lat_filename);
    }else{
      state_ptr->lat_file = aux;
    }
  }else{
    state_ptr->lat_file = NULL;
  }
  if(opt_ptr->corr_filename){
    aux = fopen(opt_ptr->corr_filename, "w");
    if(!aux){
      output(erro_lvl, "cannot open corr file '%s'", opt_ptr->corr_filename);
    }else{
      state_ptr->corr_file = aux;
    }
  }else{
    state_ptr->corr_file = NULL;
  }

  output(debg_lvl, "Slave state created");
}

void fini_state(void *ptr)
{
  struct slave_data *data_ptr = (struct slave_data *) ptr;
  struct slave_state *state_ptr = (struct slave_state *) &data_ptr->state;
  if(state_ptr->corr_file && (fclose(state_ptr->corr_file) == EOF)){
    output(erro_lvl, "failure closing corr file");
  }
  if(state_ptr->lat_file && (fclose(state_ptr->lat_file) == EOF)){
    output(erro_lvl, "failure closing lat file");
  }
  free(state_ptr->pkt_buff);
  if(close(state_ptr->socket_desc) == -1){
    output(erro_lvl, "failure closing UDP socket");
  }
  if(state_ptr->action == action_calibr){
    struct summ_stats ss;
    init_summ_stats(&ss, &state_ptr->lat_stats);
    write_summ_stats(state_ptr->stats_file, &ss);
  }
  if(state_ptr->stats_file){
    fini_stats(&state_ptr->lat_stats);
    if(fclose(state_ptr->stats_file) == EOF){
      output(erro_lvl, "failure closing stats file");
    }
  }
}
