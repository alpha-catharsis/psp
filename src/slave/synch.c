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

/* functions forward declarations */
static void perform_synch_step(struct slave_state *, double, double);
static void perform_synch_smooth(struct slave_state *, double, double);
static void perform_synch_freq(struct slave_state *, double, double);

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
    state_ptr->debug_freq_corr_file = fopen("synch_freq_correction.txt", "w");
    if(!state_ptr->debug_freq_corr_file){
      output(erro_lvl, "cannot open sychronization frequency correction file");
    }
    state_ptr->debug_freq_cumul_corr_file = fopen("synch_freq_cumul_correction.txt", "w");
    if(!state_ptr->debug_freq_cumul_corr_file){
      output(erro_lvl, "cannot open sychronization frequency cumulative correction file");
    }
  }

  struct timex tx;
  tx.modes = ADJ_OFFSET | ADJ_FREQUENCY | ADJ_STATUS;
  tx.offset = 0;
  tx.freq = 0;
  tx.status = STA_UNSYNC;
  if(adjtimex(&tx) == -1){
    output(erro_lvl, "failure resetting system clock");
  }else{
    output(info_lvl, "system clock reset");
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

  if(state_ptr->obs_win_start_time < 0.){
    state_ptr->obs_win_start_time = clk_time;
  }

  if(state_ptr->synch_method == synch_smooth){
    struct timeval tv;
    if(adjtime(NULL, &tv) == -1) {
      output(erro_lvl, "failure reading system clock adjustments");
    }else{
      uncorr_delta = (double)tv.tv_sec + ((double)tv.tv_usec) * 1e-6;
    }
  }else if(state_ptr->synch_method == synch_freq){
    struct timex tx;
    tx.modes = 0;
    if(adjtimex(&tx) == -1){
      output(erro_lvl, "failure reading system clock adjustments");
    }else{
      uncorr_delta = (double)tx.offset / 1e9;
    }
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
    double time_corr = state_ptr->median_time_off - median_delta + uncorr_delta;
    double freq_corr = 0.;

    if(state_ptr->synch_method == synch_freq){
      double avg_x = state_ptr->obs_win_start_time +
        (clk_time - state_ptr->obs_win_start_time) / 2.;
      double median_y = perc_stats_perc(&state_ptr->ps, 0.5) - state_ptr->time_cumul_corr;
      least_squares_add_xy(&state_ptr->ls, avg_x, median_y);
      if(least_squares_count(&state_ptr->ls) == state_ptr->freq_estim_slots) {
        freq_corr = -least_squares_dy(&state_ptr->ls) / 2.;
        reset_least_squares(&state_ptr->ls);
      }
    }

    switch(state_ptr->synch_method)
    {
      case synch_step:
        perform_synch_step(state_ptr, time_corr, freq_corr);
        break;
      case synch_smooth:
        perform_synch_smooth(state_ptr, time_corr, freq_corr);
        break;
      case synch_freq:
        perform_synch_freq(state_ptr, time_corr, freq_corr);
    }

    state_ptr->time_cumul_corr += time_corr;
    state_ptr->freq_cumul_corr += freq_corr;

    if(state_ptr->debug){
      if(fprintf(state_ptr->debug_time_corr_file, "%lu %.9f\n",
                 basic_stats_count(&state_ptr->bs) - 1, time_corr) < 0){
        output(erro_lvl, "cannot write time correction to file");
      }
      if(fprintf(state_ptr->debug_time_cumul_corr_file, "%lu %.9f\n",
                 basic_stats_count(&state_ptr->bs) - 1, state_ptr->time_cumul_corr) < 0){
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

/* functions for different synchronization methods */
void perform_synch_step(struct slave_state *state_ptr, double time_corr,
                        double freq_corr)
{
  (void)state_ptr;
  (void)freq_corr;
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

void perform_synch_smooth(struct slave_state *state_ptr, double time_corr,
                          double freq_corr)
{
  (void)freq_corr;
  struct timeval tv;
  int need_to_step = fabs(time_corr) >= state_ptr->time_step_thr;
  if(!need_to_step){
    tv.tv_sec = (time_t) floor(time_corr);
    tv.tv_usec = (suseconds_t) ((time_corr - floor(time_corr)) * 1e6);
    if(adjtime(&tv, NULL) == -1) {
      output(warn_lvl, "failure adjusting system clock");
      need_to_step = 1;
    }else{
      output(info_lvl, "time adjustment: %.9f", time_corr);
    }
  }
  if(need_to_step){
    perform_synch_step(state_ptr, time_corr, freq_corr);
  }
}

void perform_synch_freq(struct slave_state *state_ptr, double time_corr,
                        double freq_corr)
{
  int need_to_step = fabs(time_corr) >= state_ptr->time_step_thr;

  if(state_ptr->debug && (fabs(freq_corr) > 0.)){
    if(fprintf(state_ptr->debug_freq_corr_file, "%lu %.9f\n",
               basic_stats_count(&state_ptr->bs) - 1, freq_corr) < 0){
      output(erro_lvl, "cannot write frequency correction to file");
    }
    if(fprintf(state_ptr->debug_freq_cumul_corr_file, "%lu %.9f\n",
               basic_stats_count(&state_ptr->bs) - 1, state_ptr->freq_cumul_corr) < 0){
      output(erro_lvl, "cannot write cumulative frequency correction to file");
    }
  }

  if(!need_to_step){
    struct timex tx;
    double cumul_freq_corr = state_ptr->freq_cumul_corr + freq_corr;
    tx.modes = ADJ_OFFSET | ADJ_FREQUENCY | ADJ_STATUS | ADJ_TIMECONST | ADJ_NANO;
    tx.offset = (long)(time_corr * 1e9);
    tx.freq = (long)(cumul_freq_corr * 65536e6);
    tx.status = STA_PLL | STA_NANO | STA_UNSYNC | STA_FREQHOLD | STA_NANO;
    tx.constant = 1;
    if(adjtimex(&tx) == -1){
      output(warn_lvl, "failure adjusting system clock");
      need_to_step = 1;
    }else{
      output(info_lvl, "time adjustment: %.9f", time_corr);
      if(fabs(freq_corr) > 0.){
        output(info_lvl, "frequecy offset correction: %.9f", freq_corr);
      }
    }
  }

  if(need_to_step){
    perform_synch_step(state_ptr, time_corr, freq_corr);
  }
}
