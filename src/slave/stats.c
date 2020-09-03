/* C standard library headers */
#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
#include "stats.h"

/* statistics management functions */
void init_stats(struct stats *st_ptr, int perc, long max_samples, long drift_win)
{
  /* settings init */
  st_ptr->perc = perc;

  /* sorted samples init */
  if(st_ptr->perc){
    st_ptr->max_samples = max_samples;
    st_ptr->sorted_samples = malloc((size_t)max_samples * sizeof(double));
    if(!st_ptr->sorted_samples){
      output(erro_lvl, "failure allocating memory for latency samples");
    }
  }
  
  /* drift evaluation init */
  st_ptr->drift_win = drift_win;
  if(drift_win){
    long slots = max_samples / drift_win;
    st_ptr->drift_win_x = malloc((size_t)slots * sizeof(double));
    st_ptr->drift_win_y = malloc((size_t)slots * sizeof(double));
    if(!st_ptr->drift_win_x || !st_ptr->drift_win_y){
      output(erro_lvl, "failure allocating memory for drift evaluation");
    }
  }
  /* reset statistics */
  reset_stats(st_ptr);
}

void fini_stats(struct stats *st_ptr)
{
  if(st_ptr->perc){
    free(st_ptr->sorted_samples);
  }
  if(st_ptr->drift_win){
    free(st_ptr->drift_win_x);
    free(st_ptr->drift_win_y);
  }
}

void reset_stats(struct stats *st_ptr)
{
  /* base stats reset */
  st_ptr->count = 0;
  st_ptr->min = DBL_MAX;
  st_ptr->max = -DBL_MAX;
  st_ptr->mean = 0.;
  st_ptr->var_d = 0.;  

  /* sorted samples reset */
  if(st_ptr->perc) {
    memset(st_ptr->sorted_samples, 0, (size_t)st_ptr->max_samples * sizeof(double));
  }
}

void add_sample(struct stats *st_ptr, double time, double sample)
{
  /* sorted_samples updating */
  if(st_ptr->perc){
    long begin = 0, end = st_ptr->count;
    while(begin != end){
      if(sample > st_ptr->sorted_samples[end - 1]){
	begin = end;
      }else{
	long halfway = begin + (end - begin) / 2;
	if(sample < st_ptr->sorted_samples[halfway]){
	  if(end == halfway){
	    end = begin;
	  }else{
	    end = halfway;
	  }
	}else{
	  if(begin == halfway){
	    begin = end;
	  }else{
	    begin = halfway;
	  }
	}
      }
    }
    memmove(st_ptr->sorted_samples + begin + 1,
	    st_ptr->sorted_samples + begin,
	    (st_ptr->count - begin) * sizeof(double));
    st_ptr->sorted_samples[begin] = sample;
  }
  
  /* drift evaluation updating */
  if(st_ptr->drift_win){
    if(st_ptr->count == 0){
      st_ptr->first_time = time;
    }
    if((st_ptr->count % st_ptr->drift_win) == 0){
      st_ptr->win_start_time = time;
      st_ptr->min_sample_in_win = sample;
    }else{
      if(sample < st_ptr->min_sample_in_win){
	st_ptr->min_sample_in_win = sample;
      }
      if(((st_ptr->count + 1) % st_ptr->drift_win) == 0){
	long slot = st_ptr->count / st_ptr->drift_win;
	st_ptr->drift_win_x[slot] = (time - st_ptr->win_start_time) / 2 + st_ptr->win_start_time - st_ptr->first_time;
	st_ptr->drift_win_y[slot] = st_ptr->min_sample_in_win;
      }
    }
  }
    
  /* base stats updating */
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
long stats_count(const struct stats *st_ptr)
{
  return st_ptr->count;
}

double stats_min(const struct stats *st_ptr)
{
  return st_ptr->min;
}

double stats_max(const struct stats *st_ptr)
{
  return st_ptr->max;
}

double stats_mean(const struct stats *st_ptr)
{
  return st_ptr->mean;
}

double stats_stddev(const struct stats *st_ptr)
{
  if(st_ptr->count > 1){
    return sqrt(st_ptr->var_d / ((double) (st_ptr->count - 1)));
  }else{
    return 0.;
  }
}

double stats_mean_stddev(const struct stats *st_ptr)
{
  return stats_stddev(st_ptr) / sqrt((double) st_ptr->count);
}

double stats_percentile(const struct stats *st_ptr, double perc)
{
  if(st_ptr->perc){
    if(st_ptr->count == 0){
      return NAN;
    }else if(st_ptr->count == 1){
      return st_ptr->sorted_samples[0];
    }else{
      size_t posi = (size_t)(perc * (double)(st_ptr->count - 1));
      return st_ptr->sorted_samples[posi];
    }
  }else{
    return NAN;
  }
}

double stats_cumul_drift(const struct stats *st_ptr)
{
  if(st_ptr->drift_win){
    long slots = st_ptr->count / st_ptr->drift_win;
    if(slots < 2){
      return 0.;
    }else{
      return st_ptr->drift_win_y[slots-1] - st_ptr->drift_win_y[0];
    }
  }else{
    return 0.;
  }
}

double stats_drift(const struct stats *st_ptr)
{
  if(st_ptr->drift_win){
    long slots = st_ptr->count / st_ptr->drift_win;
    if(slots < 2){
      return 0.;
    }else{
      double mean_x = 0;
      double mean_y = 0;
      for(long i = 0; i < slots; i++){
	mean_x += st_ptr->drift_win_x[i];
	mean_y += st_ptr->drift_win_y[i];
      }
      mean_x /= (double)slots;
      mean_y /= (double)slots;

      double num = 0;
      double denum = 0;
      for(long i = 0; i < slots; i++){
	double dx = st_ptr->drift_win_x[i] - mean_x;
	num += dx * (st_ptr->drift_win_y[i] - mean_y);
	denum += dx * dx;
      }
      return num / denum;
    }
  }else{
    return 0.;
  }
}
