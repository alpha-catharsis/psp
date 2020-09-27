#ifndef PSPS_LEAST_SQUARES_H
#define PSPS_LEAST_SQUARES_H

/* least squares data structure */
struct least_squares
{
  long count;
  long size;
  double *xi;
  double *yi;
};

/* statistics management functions */
void init_least_squares(struct least_squares *, long);
void fini_least_squares(struct least_squares *);
void reset_least_squares(struct least_squares *);
void least_squares_add_xy(struct least_squares *, double, double);

/* least squares */
long least_squares_count(const struct least_squares *);
double least_squares_dy(const struct least_squares *);

#endif /* PSPS_STATS_H */
