/* C standard library headers */
#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
#include "basic_stats.h"

/* basic statistics management functions */
void reset_basic_stats(struct basic_stats *st_ptr)
{
  st_ptr->count = 0;
  st_ptr->min = DBL_MAX;
  st_ptr->max = -DBL_MAX;
  st_ptr->mean = 0.;
  st_ptr->var_d = 0.;  
}

void add_basic_stats_sample(struct basic_stats *st_ptr, double sample)
{
  st_ptr->count++;
  if(sample < st_ptr->min){
    st_ptr->min = sample;
  }
  if(sample > st_ptr->max){
    st_ptr->max = sample;
  }
  if(st_ptr->count == 1){
    st_ptr->mean = sample;
    st_ptr->var_d = 0;
  }else{
    double old_mean = st_ptr->mean;
    st_ptr->mean += (sample - st_ptr->mean) / ((double) st_ptr->count);
    st_ptr->var_d += (sample - old_mean) * (sample - st_ptr->mean);
  }
}

/* stats */
long basic_stats_count(const struct basic_stats *st_ptr)
{
  return st_ptr->count;
}

double basic_stats_min(const struct basic_stats *st_ptr)
{
  return st_ptr->min;
}

double basic_stats_max(const struct basic_stats *st_ptr)
{
  return st_ptr->max;
}

double basic_stats_mean(const struct basic_stats *st_ptr)
{
  return st_ptr->mean;
}

double basic_stats_stddev(const struct basic_stats *st_ptr)
{
  if(st_ptr->count > 1){
    return sqrt(st_ptr->var_d / ((double) (st_ptr->count - 1)));
  }else{
    return 0.;
  }
}

/* printing */
void print_basic_stats(const struct basic_stats *st_ptr, int lvl)
{
  output(lvl, "count: % 9lu | min: %.6f | max: %.6f",
	 basic_stats_count(st_ptr), basic_stats_min(st_ptr),
	 basic_stats_max(st_ptr));
  output(lvl, "mean: %.6f | stddev: %.6f",
  	 basic_stats_mean(st_ptr),
  	 basic_stats_stddev(st_ptr));
}
