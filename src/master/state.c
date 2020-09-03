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

/* PSP Master headers */
#include "nonce.h"
#include "state.h"

/* master state management functions */
void init_state_from_options(struct master_state *state_ptr, const struct options *opt_ptr)
{
  /* trivial state initializaton */
  state_ptr->socket_desc = 0;
  memcpy(&state_ptr->slave_addr, &opt_ptr->slave_addr, sizeof(struct sockaddr_in));
  state_ptr->period = opt_ptr->period;
  state_ptr->stagger = opt_ptr->stagger;
  state_ptr->pkt_cnt = opt_ptr->max_pkt_cnt;
  state_ptr->pkt_idx = 1;
  state_ptr->pkt_buff = NULL;
  state_ptr->nonce_file = NULL;

  /* socket initialization */
  state_ptr->socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
  if(state_ptr->socket_desc == -1){
    output(erro_lvl, "failure creating UDP socket");
  }else if(opt_ptr->bcast_enabled && (setsockopt(state_ptr->socket_desc, SOL_SOCKET,
						 SO_BROADCAST, &opt_ptr->bcast_enabled,
						 sizeof(opt_ptr->bcast_enabled)) == -1)){
    output(erro_lvl, "failure enabling broadcast on UDP socket");
  }else if((opt_ptr->tos != -1) && (setsockopt(state_ptr->socket_desc, IPPROTO_IP,
					       IP_TOS, &opt_ptr->tos,
					       sizeof(opt_ptr->tos)) == -1)){
    output(erro_lvl, "failure setting UDP socket TOS field");
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

    /* nonce file opening */
    state_ptr->nonce_file = fopen(opt_ptr->nonce_filename, "r+");
    if(state_ptr->nonce_file){
      state_ptr->pkt_idx = read_nonce(state_ptr->nonce_file);
    }else{
      state_ptr->nonce_file = fopen(opt_ptr->nonce_filename, "w");
      if(!state_ptr->nonce_file){
	output(erro_lvl, "cannot open nonce file for writing");
      }else{
	write_nonce(state_ptr->nonce_file, state_ptr->pkt_idx);
      }
    }
  }else{
    state_ptr->secure = 0;
    state_ptr->nonce_file = NULL;
  }

  /* packet buffer initialization */
  state_ptr->pkt_size = ts_pkt_size(state_ptr->secure);
  state_ptr->pkt_buff = malloc(state_ptr->pkt_size);
  if(!state_ptr->pkt_buff){
    output(erro_lvl, "cannot allocate buffer for timestamp packets transmission");
  }

  output(debg_lvl, "Master state created");
}

void fini_state(void *ptr)
{
  struct master_data *data_ptr = (struct master_data *) ptr;
  struct master_state *state_ptr = (struct master_state *) &data_ptr->state;

  free(state_ptr->pkt_buff);
  if(state_ptr->nonce_file && (fclose(state_ptr->nonce_file) == EOF)){
    output(erro_lvl, "failure closing nonce file");
  }
  if(close(state_ptr->socket_desc) == -1){
    output(erro_lvl, "failure closing UDP socket");
  }
}
