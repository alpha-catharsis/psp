/* C standard library headers */
#include <stdio.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
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
    state_ptr->debug_lat_file = fopen("precalibr_latency.txt", "w");
    if(!state_ptr->debug_lat_file){
      output(erro_lvl, "cannot open pre-calibration latency file");
    }
    state_ptr->debug_freq_off_file = fopen("precalibr_freq_offset.txt", "w");
    if(!state_ptr->debug_freq_off_file){
      output(erro_lvl, "cannot open pre-calibration frequecy offset file");
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
void precalibr_handle_ts(struct slave_state *state_ptr, double time, double delta)
{
  if(perc_stats_count(&state_ptr->ps) == 0){
    state_ptr->first_ts_time = time;
  }
  add_perc_stats_sample(&state_ptr->ps, delta);
  if(perc_stats_count(&state_ptr->ps) == perc_stats_max_samples(&state_ptr->ps)){
    double avg_x = state_ptr->first_ts_time + (time - state_ptr->first_ts_time) / 2.;
    double median_y = perc_stats_perc(&state_ptr->ps, 0.5);
    least_squares_add_xy(&state_ptr->ls, avg_x, median_y);
    output(info_lvl, "median delta: %.9f", median_y);

    double freq_off = least_squares_dy(&state_ptr->ls);
    output(info_lvl, "frequency offset: %.9f", freq_off);
    if(state_ptr->debug){
      if(fprintf(state_ptr->debug_freq_off_file, "%.9f %.9f\n", time, freq_off) < 0){
	output(erro_lvl, "cannot write frequecy offset sample to file");
      }
    }

    reset_perc_stats(&state_ptr->ps);
  }
}  
