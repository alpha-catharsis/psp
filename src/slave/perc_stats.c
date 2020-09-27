/* C standard library headers */
#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
#include "perc_stats.h"

/* percetile statistics management functions */
void init_perc_stats(struct perc_stats *st_ptr, long max_samples)
{
  st_ptr->max_samples = max_samples;
  st_ptr->sorted_samples = malloc((size_t)max_samples * sizeof(double));
  if(!st_ptr->sorted_samples){
    output(erro_lvl, "failure allocating memory for percentile statistics");
  }
  reset_perc_stats(st_ptr);
}

void fini_perc_stats(struct perc_stats *st_ptr)
{
  free(st_ptr->sorted_samples);
}

void reset_perc_stats(struct perc_stats *st_ptr)
{
  st_ptr->count = 0;
  memset(st_ptr->sorted_samples, 0, (size_t)st_ptr->max_samples * sizeof(double));
}

void add_perc_stats_sample(struct perc_stats *st_ptr, double sample)
{
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
  st_ptr->count++;
}

/* stats */
long perc_stats_count(const struct perc_stats *st_ptr)
{
  return st_ptr->count;
}

long perc_stats_max_samples(const struct perc_stats *st_ptr)
{
  return st_ptr->max_samples;
}

double perc_stats_perc(const struct perc_stats *st_ptr, double perc)
{
  if(st_ptr->count == 0){
    return NAN;
  }else if(st_ptr->count == 1){
    return st_ptr->sorted_samples[0];
  }else{
    size_t posi = (size_t)(perc * (double)(st_ptr->count - 1));
    return st_ptr->sorted_samples[posi];
  }
}

/* printing */
void print_perc_stats(const struct perc_stats *st_ptr, int lvl)
{
  output(lvl, "p10: %.6f | p25: %.6f | p50: %.6f | p99: %.6f",
	 perc_stats_perc(st_ptr, 0.10),
	 perc_stats_perc(st_ptr, 0.25),
	 perc_stats_perc(st_ptr, 0.50),
	 perc_stats_perc(st_ptr, 0.99));
}
