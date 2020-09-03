/* C standard library headers */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* PSP Common headers */
#include "../common/mgmt.h"
#include "../common/output.h"

/* PSP Mater headers */
#include "nonce.h"    
#include "options.h"
#include "state.h"
#include "timer.h"

/* functions forward declarations */
static void mngd_main(void *);
static void emit_timestamp(void *);

/* main function */
int main(int argc, char **argv)
{
  struct master_data data;
  if(parse_command_line(argc, argv, &data.opts)){
    return run_managed(&mngd_main, &fini_state, &data);
  }else{
    return EXIT_FAILURE;
  }
}

/* managed main function */
static void mngd_main(void *ptr)
{
  struct master_data *data_ptr = (struct master_data *) ptr;
  struct options *opts_ptr = (struct options *) &data_ptr->opts;
  struct master_state *state_ptr = (struct master_state *) &data_ptr->state;
  apply_general_options(&opts_ptr->gen_opts);
  print_selected_options(opts_ptr);
  init_state_from_options(state_ptr, opts_ptr);
  emit_timestamp(state_ptr);
  wait_signals();
}

/* emit timestamp function */
static void emit_timestamp(void *data_ptr)
{
  struct master_state *state_ptr = (struct master_state *) data_ptr;
  struct timespec ts;
  long delay = state_ptr->period - state_ptr->stagger +
    (long) (((double) (state_ptr->stagger * 2)) *
	    (((double) rand()) / ((double) RAND_MAX + 1.0)));
  errno = 0;
  if(clock_gettime(CLOCK_REALTIME, &ts) == -1){
    output(erro_lvl, "failure reading realtime clock: %s", strerror(errno));
  }else{
    write_ts_pkt(state_ptr->pkt_buff, state_ptr->secure, state_ptr->pkt_idx,
		 ts.tv_sec, ts.tv_nsec, state_ptr->key);
    errno = 0;
    if(sendto(state_ptr->socket_desc, state_ptr->pkt_buff, state_ptr->pkt_size, 0,
	      (struct sockaddr *)&state_ptr->slave_addr,
	      sizeof(state_ptr->slave_addr)) == -1){
      if((errno != EINTR) && (errno != EAGAIN)){
	output(erro_lvl, "sendto failure: %s", strerror(errno));
      }
    }else{
      output(info_lvl, "sending packet to %s:%hu",
      	     inet_ntoa(state_ptr->slave_addr.sin_addr),
      	     ntohs(state_ptr->slave_addr.sin_port));
      output(debg_lvl, "idx %09lu secs: %09lu nsecs: %09lu", state_ptr->pkt_idx,
	     ts.tv_sec, ts.tv_nsec);
      state_ptr->pkt_idx++;
      if(state_ptr->secure){
	write_nonce(state_ptr->nonce_file, state_ptr->pkt_idx);
      }
      if(state_ptr->pkt_cnt >= 0){
	state_ptr->pkt_cnt--;
      }
      if(state_ptr->pkt_cnt == 0){
	output(info_lvl, "finished emitting timestamps. Exiting...");
	clean_exit();
      }
      output(debg_lvl, "waiting for % 6ld.%03ld seconds", delay / 1000l, delay % 1000l);
      set_timer(delay, &emit_timestamp, state_ptr);
    }
  }
}
