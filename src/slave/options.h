#ifndef PSPS_OPTIONS_H
#define PSPS_OPTIONS_H

/* POSIX library headers */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

/* PSP Common headers */
#include "../common/options.h"

/* master action enumeration */
enum action_type
{
  action_precalibr = 0,
  action_calibr = 1,
  action_synch = 2
};

/* synchronization method values enumeration */
enum synch_method_value
{
  synch_step = 0,
  synch_smooth = 1,
  synch_freq = 2
};

/* option structure */
struct options
{
  /* general options */
  struct general_options gen_opts;

  /* action options */
  int action;

  /* general options */
  in_port_t slave_port;
  long max_pkt_cnt;
  long obs_win;

  /* synchronization options */
  int synch_method;
  long freq_estim_slots;
  long time_step_thr;
  long time_corr_damp;
  long freq_corr_damp;
  long time_corr_clamp;
  long freq_corr_clamp;
  long qs_rounds;

  /* secure protocol options */
  const char *key_filename;

  /* debugging options */
  int debug;
};

/* options parsing functions */
int parse_command_line(int, char **, struct options *);

/* options reporting */
void print_selected_options(const struct options *);

#endif /* PSPS_OPTIONS_H */
