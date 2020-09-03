/* C standard library headers */
#include <math.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
#include "summ_stats.h"

/* latency summary statistics management */
void init_summ_stats(struct summ_stats *summ_ptr, const struct stats *st_ptr)
{
  summ_ptr->count = st_ptr->count;
  summ_ptr->min = stats_min(st_ptr);
  summ_ptr->max = stats_max(st_ptr);
  summ_ptr->mean = stats_mean(st_ptr);
  summ_ptr->stddev = stats_stddev(st_ptr);
  summ_ptr->mean_stddev = stats_mean_stddev(st_ptr);
  summ_ptr->perc[0] = stats_percentile(st_ptr, 0.10);
  summ_ptr->perc[1] = stats_percentile(st_ptr, 0.25);
  summ_ptr->perc[2] = stats_percentile(st_ptr, 0.50);
  summ_ptr->perc[3] = stats_percentile(st_ptr, 0.99);
  summ_ptr->cumul_drift = stats_cumul_drift(st_ptr);
  summ_ptr->drift_ppb = (long)(round(stats_drift(st_ptr) * 1e9));
}

void write_summ_stats(FILE *file, const struct summ_stats *summ_ptr)
{
  int res = fprintf(file,
		    "count: %ld\n"
		    "min: %.15f\n"
		    "max: %.15f\n"
		    "mean: %.15f\n"
		    "stddev: %.15f\n"
		    "mean_stddev: %.15f\n"
		    "p10: %.15f\n"
		    "p25: %.15f\n"
		    "p50: %.15f\n"
		    "p99: %.15f\n"
		    "cumul_drift: %.15f\n"
		    "drift_ppb: %ld\n",
		    summ_ptr->count,
		    summ_ptr->min,
		    summ_ptr->max,
		    summ_ptr->mean,
		    summ_ptr->stddev,
		    summ_ptr->mean_stddev,
		    summ_ptr->perc[0],
		    summ_ptr->perc[1],
		    summ_ptr->perc[2],
		    summ_ptr->perc[3],
		    summ_ptr->cumul_drift,
		    summ_ptr->drift_ppb
		    );
  if(res < 0) {
    output(erro_lvl, "cannot write statistics file");
  }
}

void read_summ_stats(FILE *file, struct summ_stats * summ_ptr)
{
 int res = fscanf(file,
		  "count: %ld\n"
		  "min: %lf\n"
		  "max: %lf\n"
		  "mean: %lf\n"
		  "stddev: %lf\n"
		  "mean_stddev: %lf\n"
		  "p10: %lf\n"
		  "p25: %lf\n"
		  "p50: %lf\n"
		  "p99: %lf\n"
		  "cumul_drift: %lf\n"
		  "drift_ppb: %ld\n",
		  &summ_ptr->count,
		  &summ_ptr->min,
		  &summ_ptr->max,
		  &summ_ptr->mean,
		  &summ_ptr->stddev,
		  &summ_ptr->mean_stddev,
		  &summ_ptr->perc[0],
		  &summ_ptr->perc[1],
		  &summ_ptr->perc[2],
		  &summ_ptr->perc[3],
		  &summ_ptr->cumul_drift,
		  &summ_ptr->drift_ppb
		  );
  if(res != 12) {
    output(erro_lvl, "error reading statistics file");
  }
}
