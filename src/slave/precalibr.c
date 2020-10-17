/* C standard library headers */
#include <stdio.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
#include "basic_stats.h"
#include "least_squares.h"
#include "perc_stats.h"
#include "precalibr.h"
#include "ts_handler.h"

/* precalibration initialization */
void init_precalibr(struct slave_state *state_ptr)
{
  state_ptr->out_file = fopen("precalibr_results.txt", "w");
  if(!state_ptr->out_file){
    output(erro_lvl, "cannot open pre-calibration output file");
  }
  if(state_ptr->debug){
    state_ptr->debug_timestamp_file = fopen("precalibr_timestamp.txt", "w");
    if(!state_ptr->debug_timestamp_file){
      output(erro_lvl, "cannot open pre-calibration timestamp file");
    }
    state_ptr->debug_freq_delta_file = fopen("precalibr_freq_delta.txt", "w");
    if(!state_ptr->debug_freq_delta_file){
      output(erro_lvl, "cannot open pre-calibration frequecy delta file");
    }
  }
}

void fini_precalibr(struct slave_state *state_ptr)
{
  if(fprintf(state_ptr->out_file, "%.9f\n", least_squares_dy(&state_ptr->ls)) < 0){
    output(erro_lvl, "cannot write pre-calibration results to file");
  }
}

/* timestamp handling */
void precalibr_handle_ts(struct slave_state *state_ptr, double clk_time, double time_delta)
{
  if(state_ptr->first_clk_time < 0.){
    state_ptr->first_clk_time = clk_time;
    state_ptr->first_delta = time_delta;
  }

  if(state_ptr->obs_win_start_time < 0.){
    state_ptr->obs_win_start_time = clk_time;
  }

  add_perc_stats_sample(&state_ptr->ps, time_delta);
  if(perc_stats_count(&state_ptr->ps) == state_ptr->obs_win){
    double avg_x = state_ptr->obs_win_start_time +
      (clk_time - state_ptr->obs_win_start_time) / 2. -
      state_ptr->first_clk_time;
    double median_y = perc_stats_perc(&state_ptr->ps, 0.5) - state_ptr->first_delta;
    least_squares_add_xy(&state_ptr->ls, avg_x, median_y);

    if(least_squares_count(&state_ptr->ls) > 1) {
      double freq_off = least_squares_dy(&state_ptr->ls);
      output(info_lvl, "frequency delta: %.9f", freq_off);
      if(state_ptr->debug){
	if(fprintf(state_ptr->debug_freq_delta_file, "%lu %.9f\n", basic_stats_count(&state_ptr->bs), freq_off) < 0){
	  output(erro_lvl, "cannot write frequecy delta sample to file");
	}
      }
    }
    
    state_ptr->obs_win_start_time = -1.;
    reset_perc_stats(&state_ptr->ps);
  }
}  
