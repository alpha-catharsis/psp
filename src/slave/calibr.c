/* C standard library headers */
#include <stdio.h>

/* PSP Common headers */
#include "../common/mgmt.h"
#include "../common/output.h"

/* PSP Slave headers */
#include "least_squares.h"
#include "perc_stats.h"
#include "calibr.h"
#include "ts_handler.h"

/* calibration initialization */
void init_calibr(struct slave_state *state_ptr)
{
  FILE *in_file = fopen("precalibr_results.txt", "r");
  if(!in_file){
      output(warn_lvl, "cannot open pre-calibration output file. Assuming zero frequency offset.");
  }else{
    double freq_off;
    if(fscanf(in_file, "%lf", &freq_off) != 1){
      fclose(in_file);
      output(erro_lvl, "cannot read frequency offset in pre-calibration output file.");
    }
    state_ptr->clk_freq_ofs = -freq_off;
    fclose(in_file);
  }

  state_ptr->out_file = fopen("calibr_results.txt", "w");
  if(!state_ptr->out_file){
    output(erro_lvl, "cannot open calibration output file");
  }
  if(state_ptr->debug){
    state_ptr->debug_timestamp_file = fopen("calibr_timestamp.txt", "w");
    if(!state_ptr->debug_timestamp_file){
      output(erro_lvl, "cannot open calibration timestamp file");
    }
    state_ptr->debug_corr_time_delta_file = fopen("calibr_corr_time_delta.txt", "w");
    if(!state_ptr->debug_corr_time_delta_file){
      output(erro_lvl, "cannot open calibration corrected time delta file");
    }
    state_ptr->debug_time_delta_cdf_file = fopen("calibr_time_delta_cdf.txt", "w");
    if(!state_ptr->debug_time_delta_cdf_file){
      output(erro_lvl, "cannot open calibration time delta CDF file");
    }
  }
}

void fini_calibr(struct slave_state *state_ptr)
{
  if(fprintf(state_ptr->out_file, "%.9f\n", perc_stats_perc(&state_ptr->ps, 0.5)) < 0){
    output(erro_lvl, "cannot write calibration results to file");
  }
  if(state_ptr->debug){
    for(int i = 0; i <= 100; i++){
      double y = i * 0.01;
      double x = perc_stats_perc(&state_ptr->ps, y);
      if(fprintf(state_ptr->debug_time_delta_cdf_file, "%.9f %.9f\n", x, y) < 0) {
	output(erro_lvl, "cannot write to time offset CDF file");
      }
    }
  }
}

/* timestamp handling */
void calibr_handle_ts(struct slave_state *state_ptr, double clk_time, double time_delta)
{
  if(state_ptr->first_clk_time < 0.){
    state_ptr->first_clk_time = clk_time;
  }
  double corrected_delta = time_delta + state_ptr->clk_freq_ofs *
    (clk_time - state_ptr->first_clk_time);
  if(state_ptr->debug){
    if(fprintf(state_ptr->debug_corr_time_delta_file, "%lu %.9f\n",
	       basic_stats_count(&state_ptr->bs) - 1, corrected_delta) < 0){
      output(erro_lvl, "cannot write corrected time delta sample to file");
    }
  }
  add_perc_stats_sample(&state_ptr->ps, corrected_delta);
  output(info_lvl, "median time delta: %.9f", perc_stats_perc(&state_ptr->ps, 0.5));
  if(perc_stats_count(&state_ptr->ps) == perc_stats_max_samples(&state_ptr->ps)){
    clean_exit();
  }
}  
