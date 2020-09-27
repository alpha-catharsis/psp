#ifndef PSPS_BASIC_STATS_H
#define PSPS_BASIC_STATS_H

/* basic statistics data structure */
struct basic_stats
{
  long count;
  double min;
  double max;
  double mean;
  double var_d;
};

/* basic statistics management functions */
void reset_basic_stats(struct basic_stats *);
void add_basic_stats_sample(struct basic_stats *, double);

/* stats */
long basic_stats_count(const struct basic_stats *);
double basic_stats_min(const struct basic_stats *);
double basic_stats_max(const struct basic_stats *);
double basic_stats_mean(const struct basic_stats *);
double basic_stats_stddev(const struct basic_stats *);

/* printing */
void print_basic_stats(const struct basic_stats *, int);

#endif /* PSPS_BASIC_STATS_H */
