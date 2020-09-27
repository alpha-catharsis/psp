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
    if(fscanf(in_file, "%lf", &state_ptr->freq_corr) != 1){
      fclose(in_file);
      output(erro_lvl, "cannot read frequency offset in  pre-calibration output file.");
    }
    fclose(in_file);
  }

  state_ptr->out_file = fopen("calibr_results.txt", "w");
  if(!state_ptr->out_file){
    output(erro_lvl, "cannot open calibration output file");
  }
  if(state_ptr->debug){
    state_ptr->debug_lat_file = fopen("calibr_latency.txt", "w");
    if(!state_ptr->debug_lat_file){
      output(erro_lvl, "cannot open calibration latency file");
    }
    state_ptr->debug_lat_cdf_file = fopen("calibr_latency_cdf.txt", "w");
    if(!state_ptr->debug_lat_cdf_file){
      output(erro_lvl, "cannot open calibration latency CDF file");
    }
  }
}

void fini_calibr(struct slave_state *state_ptr)
{
  if(fprintf(state_ptr->out_file, "%.9f\n", perc_stats_perc(&state_ptr->ps, 0.5)) < 0){
    output(erro_lvl, "cannot write calibration results to file");
  }
  if(state_ptr->debug){
    double prev_x = 0.;
    for(int i = 0; i <= 100; i++){
      double y = i * 0.01;
      double x = perc_stats_perc(&state_ptr->ps, y);
      if(fprintf(state_ptr->debug_lat_cdf_file, "%.9f %.9f\n%.9f %.9f\n", prev_x, y, x, y) < 0) {
	output(erro_lvl, "cannot write to latency histogram file");
      }
      prev_x = x;
    }
  }
}

/* timestamp handling */
void calibr_handle_ts(struct slave_state *state_ptr, double time, double delta)
{
  (void) time;
  add_perc_stats_sample(&state_ptr->ps, delta);
    output(info_lvl, "median latency: %.9f", perc_stats_perc(&state_ptr->ps, 0.5));
  if(perc_stats_count(&state_ptr->ps) == perc_stats_max_samples(&state_ptr->ps)){
    clean_exit();
  }
}  
