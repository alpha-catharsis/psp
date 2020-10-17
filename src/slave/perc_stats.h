#ifndef PSPS_PERC_STATS_H
#define PSPS_PERC_STATS_H

/* percentile statistics data structure */
struct perc_stats
{
  long count;
  long max_samples;
  double *sorted_samples;
};

/* percentile statistics management functions */
void init_perc_stats(struct perc_stats *, long);
void fini_perc_stats(struct perc_stats *);
void reset_perc_stats(struct perc_stats *);
void add_perc_stats_sample(struct perc_stats *, double);

/* stats */
long perc_stats_count(const struct perc_stats *);
long perc_stats_max_samples(const struct perc_stats *);
double perc_stats_perc(const struct perc_stats *, double);

/* printing */
void print_perc_stats(const struct perc_stats *, int);

#endif /* PSPS_PERC_STATS_H */
