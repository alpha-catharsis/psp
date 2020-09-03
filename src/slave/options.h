#ifndef PSPS_OPTIONS_H
#define PSPS_OPTIONS_H

/* POSIX library headers */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

/* PSP Common headers */
#include "../common/options.h"

/* master action enumeration */
enum action_type { action_calibr = 0,
                   action_synch = 1};

/* synchronization algorithm enumeration */
enum sync_algo { algo_mean = 0,
		 algo_p10 = 1,
		 algo_p25 = 2,
		 algo_median = 3};

/* option structure */
struct options
{
  /* general options */
  struct general_options gen_opts;

  /* action options */
  int action;
  int synch_algo;
  int no_perc;
  long drift_win;
  long obs_win;
  int simulate;
  long max_pkt_cnt;
  const char *stats_filename;
  in_port_t slave_port;

  /* secure protocol options */
  const char *key_filename;

  /* debugging options */
  const char *lat_filename;
  const char *corr_filename;

  /* tweaking options */
  long offset;
  long drift;
};

/* options parsing functions */
int parse_command_line(int, char **, struct options *);

/* options reporting */
void print_selected_options(const struct options *);

#endif /* PSPS_OPTIONS_H */
