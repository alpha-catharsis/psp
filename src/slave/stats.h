#ifndef PSPS_STATS_H
#define PSPS_STATS_H

/* latency statistics data structure */
struct stats
{
  /* settings */
  int perc;

  /* basic stats */
  long count;
  double min;
  double max;
  double mean;
  double var_d;

  /* sorted samples */
  long max_samples;
  double *sorted_samples;

  /* drift stats */
  long drift_win;
  double first_time;
  double win_start_time;
  double min_sample_in_win;
  double *drift_win_x;
  double *drift_win_y;
};

/* statistics management functions */
void init_stats(struct stats *, int, long, long);
void fini_stats(struct stats *);
void reset_stats(struct stats *);
void add_sample(struct stats *, double, double);

/* stats */
long stats_count(const struct stats *);
double stats_min(const struct stats *);
double stats_max(const struct stats *);
double stats_mean(const struct stats *);
double stats_stddev(const struct stats *);
double stats_mean_stddev(const struct stats *);
double stats_percentile(const struct stats *, double);
double stats_cumul_drift(const struct stats *);
double stats_drift(const struct stats *);

#endif /* PSPS_STATS_H */
