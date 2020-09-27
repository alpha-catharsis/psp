/* C standard library headers */
#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
#include "least_squares.h"

/* statistics management functions */
void init_least_squares(struct least_squares *st_ptr, long size)
{
  st_ptr->size = size;
  st_ptr->xi = malloc((size_t)size * sizeof(double));
  st_ptr->yi = malloc((size_t)size * sizeof(double));
  if(!st_ptr->xi || !st_ptr->yi){
    output(erro_lvl, "failure allocating memory for least squares");
  }
  reset_least_squares(st_ptr);
}

void fini_least_squares(struct least_squares *st_ptr)
{
  free(st_ptr->xi);
  free(st_ptr->yi);
}

void reset_least_squares(struct least_squares *st_ptr)
{
  st_ptr->count = 0;
}

void least_squares_add_xy(struct least_squares *st_ptr, double x, double y)
{
  if(st_ptr->count < st_ptr->size){
    st_ptr->count++;
  }else{
    memmove(st_ptr->xi, st_ptr->xi + 1, st_ptr->count - 1);
    memmove(st_ptr->yi, st_ptr->yi + 1, st_ptr->count - 1);
  }
  st_ptr->xi[st_ptr->count - 1] = x;
  st_ptr->yi[st_ptr->count - 1] = y;
}

/* stats */
long least_squares_count(const struct least_squares *st_ptr)
{
  return st_ptr->count;
}

double least_squares_dy(const struct least_squares *st_ptr)
{
  if(st_ptr->count < 2){
    return 0.;
  }else{
    double mean_x = 0;
    double mean_y = 0;
    for(long i = 0; i < st_ptr->count; i++){
      mean_x += st_ptr->xi[i];
      mean_y += st_ptr->yi[i];
    }
    mean_x /= (double)st_ptr->count;
    mean_y /= (double)st_ptr->count;

    double num = 0;
    double denum = 0;
    for(long i = 0; i < st_ptr->count; i++){
      double dx = st_ptr->xi[i] - mean_x;
      num += dx * (st_ptr->yi[i] - mean_y);
      denum += dx * dx;
    }
    return num / denum;
  }
}
