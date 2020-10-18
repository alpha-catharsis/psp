/* C standard library headers */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Linux headers */
#include <sys/time.h>
#include <sys/timex.h>

/* PSP Common headers */
#include "../common/mgmt.h"
#include "../common/output.h"

/* PSP Slave headers */
#include "least_squares.h"
#include "perc_stats.h"
#include "synch.h"
#include "ts_handler.h"

/* sychronization initialization */
void init_synch(struct slave_state *state_ptr)
{
  FILE *in_file = fopen("calibr_results.txt", "r");
  if(!in_file){
      output(erro_lvl, "cannot open calibration output file.");
  }else{
    if(fscanf(in_file, "%lf", &state_ptr->median_time_off) != 1){
      fclose(in_file);
      output(erro_lvl, "cannot read median latency estimation in calibration output file.");
    }
    fclose(in_file);
  }

  if(state_ptr->debug){
    state_ptr->debug_timestamp_file = fopen("synch_timestamp.txt", "w");
    if(!state_ptr->debug_timestamp_file){
      output(erro_lvl, "cannot open sychronization timestamp file");
    }
    state_ptr->debug_corr_time_delta_file = fopen("synch_corr_time_delta.txt", "w");
    if(!state_ptr->debug_corr_time_delta_file){
      output(erro_lvl, "cannot open synchronization corrected time delta file");
    }
    state_ptr->debug_time_corr_file = fopen("synch_time_correction.txt", "w");
    if(!state_ptr->debug_time_corr_file){
      output(erro_lvl, "cannot open sychronization time correction file");
    }
    state_ptr->debug_time_cumul_corr_file = fopen("synch_time_cumul_correction.txt", "w");
    if(!state_ptr->debug_time_cumul_corr_file){
      output(erro_lvl, "cannot open sychronization time cumulative correction file");
    }
  }

  struct timex tx;
  tx.modes = ADJ_STATUS;
  tx.status = STA_PLL | STA_NANO;
  if(adjtimex(&tx) == -1){
    output(erro_lvl, "failure setting phase-locked loop disciple: %s", strerror(errno));
  }else{
    output(info_lvl, "enabled phase-locked loop disciple");
  }

  output(info_lvl, "setting observation window to %ld samples", state_ptr->obs_win);
}

void fini_synch(struct slave_state *state_ptr)
{
  (void) state_ptr;
}

/* timestamp handling */
void synch_handle_ts(struct slave_state *state_ptr, double clk_time, double time_delta)
{
  (void) clk_time;
  
  double corrected_delta = time_delta;
  double uncorr_delta = 0.;

  struct timex tx;
  tx.modes = 0;
  if(adjtimex(&tx) == -1){
    output(erro_lvl, "failure reading system clock adjustments");
  }else{
    uncorr_delta = (double)tx.offset / 1e9;
  }
  corrected_delta += uncorr_delta;

  if(state_ptr->debug){
    if(fprintf(state_ptr->debug_corr_time_delta_file, "%lu %.9f\n",
	       basic_stats_count(&state_ptr->bs) - 1, corrected_delta) < 0){
      output(erro_lvl, "cannot write corrected time delta sample to file");
    }
  }

  add_perc_stats_sample(&state_ptr->ps, corrected_delta);
  if(perc_stats_count(&state_ptr->ps) == state_ptr->obs_win){
    double median_delta = perc_stats_perc(&state_ptr->ps, 0.5);
    double time_corr = state_ptr->median_time_off - median_delta;
    int need_to_step = fabs(time_corr) >= state_ptr->time_step_thr;

    if(!need_to_step){
      tx.modes = ADJ_OFFSET | ADJ_STATUS | ADJ_TIMECONST | ADJ_NANO;
      tx.offset = (long)(time_corr * 1e9 + uncorr_delta);
      tx.status = STA_PLL | STA_NANO;
      tx.constant = 1;
      if(adjtimex(&tx) == -1){
        output(warn_lvl, "failure adjusting system clock");
        need_to_step = 1;
      }else{
        output(info_lvl, "time adjustment: %.9f", time_corr);
      }
    }

    if(need_to_step){
      struct timespec ts;
      if(clock_gettime(CLOCK_REALTIME, &ts) == -1){
	output(erro_lvl, "failure reading realtime clock");
      }else{
	double new_time = (double) ts.tv_sec + (double) ts.tv_nsec * 1e-9 + time_corr;
	ts.tv_sec  = (time_t) floor(new_time);
	ts.tv_nsec = (long) ((new_time - floor(new_time)) * 1e9);
	if(clock_settime(CLOCK_REALTIME, &ts) == 0){
	  output(info_lvl, "time correction step: %.9f", time_corr);
	}else{
	  output(erro_lvl, "failure setting realtime clock");
	}
      }
    }

    state_ptr->time_cumul_corr += time_corr;

    if(state_ptr->debug){
      if(fprintf(state_ptr->debug_time_corr_file, "%.9f\n", time_corr) < 0){
	output(erro_lvl, "cannot write time correction to file");
      }
      if(fprintf(state_ptr->debug_time_cumul_corr_file, "%.9f\n", state_ptr->time_cumul_corr) < 0){
	output(erro_lvl, "cannot write cumulative time correction to file");
      }
    }

    if(state_ptr->qs_rounds){
      state_ptr->qs_rounds--;
      state_ptr->obs_win *= 2;
      output(info_lvl, "setting observation window to %ld samples", state_ptr->obs_win);
    }

    reset_perc_stats(&state_ptr->ps);
  }
}
