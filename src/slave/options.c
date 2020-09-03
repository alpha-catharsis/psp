/* C standard library headers */
#include <limits.h>
#include <stdio.h>

/* PSP Common headers */
#include "../common/output.h"

/* PSP Slave headers */
#include "options.h"

/* functions forward declarations */
static int custom_option_checks(struct option_descriptor *);

/* parse_command_line */
int parse_command_line(int argc, char **argv, struct options *opts_ptr)
{
  init_general_options(&opts_ptr->gen_opts);
  opts_ptr->synch_algo = algo_median;
  opts_ptr->no_perc = 0;
  opts_ptr->drift_win = 0;
  opts_ptr->obs_win = 100;
  opts_ptr->simulate = 0;
  opts_ptr->max_pkt_cnt = -1;
  opts_ptr->slave_port = htons(4242);
  opts_ptr->key_filename = NULL;
  opts_ptr->lat_filename = NULL;
  opts_ptr->corr_filename = NULL;
  opts_ptr->offset = 0;
  opts_ptr->drift = 0;

  const int action_calibr_val = action_calibr;
  const int action_synch_val = action_synch;
  const struct num_bounds algo_bounds = {algo_mean, algo_median};
  const struct num_bounds win_bounds = {1, 1000000};
  const struct num_bounds pkt_cnt_bounds = {1, LONG_MAX};

  struct option_descriptor optreg[] =
    { /* general options */
     GEN_OPTS(opts_ptr->gen_opts),
     
     /* action options */
     INT_SET_OPT('c', "perform calibration", &opts_ptr->action, &action_calibr_val, "n", "sh"),
     INT_SET_OPT('s', "perform synchronization", &opts_ptr->action, &action_synch_val, "", "ch"),
     BND_INT_OPT('a', "<integer>, synchronization algorithm (0=mean, 1=p10, 2=p25, 3=median)", \
		 &opts_ptr->synch_algo, &algo_bounds, "s", ""),
     FLAG_OPT('x', "do not compute latency percentile (saves memory and cpu load)", &opts_ptr->no_perc, "", ""),
     BND_LONG_OPT('d', "<integer> specifies the drift evaluation window in samples and enables drift evaluation",
		  &opts_ptr->drift_win, &win_bounds, "c", ""),
     BND_LONG_OPT('w', "<integer>, specifies the observation window in samples", &opts_ptr->obs_win, &win_bounds, "s", ""),
     FLAG_OPT('f', "simulate synchronization (does not change the machine clock)", &opts_ptr->simulate, "s", ""),
     BND_LONG_OPT('n', "<integer>, specifies the number of timestamp packets to receive before stopping",
		  &opts_ptr->max_pkt_cnt, &pkt_cnt_bounds, "", ""), 
     STR_OPT('u', "<filename>, specifies the latency statistics file", &opts_ptr->stats_filename, "*", ""),
     IN_PORT_OPT('p', "<port number>, specifies the slave UDP port", &opts_ptr->slave_port, "", ""),
     
     /* secure protocol options */
     STR_OPT('k', "<filename>, specifies the cryptographic key for timestamp authentication", &opts_ptr->key_filename, "", ""),

     /* debugging options */
     STR_OPT('t', "<filename>, records the latency sample on the specified file", &opts_ptr->lat_filename, "", ""),
     STR_OPT('r', "<filename>, records the time corrections on the specified file", &opts_ptr->corr_filename, "s", ""),

     /* tweaking options */
     LONG_OPT('o', "<integer>, specifies the latency offest in us", &opts_ptr->offset, "", ""), 
     LONG_OPT('i', "<integer>, compensate clocks drift in ppb for calibration", &opts_ptr->drift, "", "s"),
    
     /* end of options */
     END_OPTS
    };

  struct opt_group optg[] = {GEN_OPTS_GROUP,
			     OPTS_GROUP("action options", "cs"),
			     OPTS_GROUP("calibration options", "d"),
			     OPTS_GROUP("syncrhonization options", "awf"),
			     OPTS_GROUP("syncrhonization options", "awf"),
			     OPTS_GROUP("secure protocol options", "k"),
			     OPTS_GROUP("debugging options", "tr"),
			     OPTS_GROUP("tweaking options", "oi"),
			     END_OPTS_GROUP};

  if(parse_opts(optreg, argc, argv) &&
     !is_opt_set(optreg, 'h') &&
     check_opts(optreg) &&
     custom_option_checks(optreg)){
    return 1;
  }else{
    print_help_msg("Packet Synchronization Protocol (PSP) Slave",
		   "usage: pspm [action] [options]\n",
		   optreg, optg);
    return 0;
  }
}

/* custom opttion checks */
static int custom_option_checks(struct option_descriptor *optreg)
{
  struct option_descriptor *algo_opt = find_opt_desc(optreg, 'a');
  if(!is_opt_set(optreg, 'c') && !is_opt_set(optreg, 's')){
    printf("no action specified\n");
    return 0;
  }else if((is_opt_set(optreg, 'x')) && (is_opt_set(optreg, 's')) &&
	   *((int *)algo_opt->trgt) != algo_mean){
    printf("only 'mean' algorithm can be used if latency percentile are not computed\n");
    return 0;
  }
  return 1;
}

/* options reporting */
void print_selected_options(const struct options *opts_ptr)
{
  output(info_lvl, "Slave Synchronization Master started...");
  output(info_lvl, "Parameters:");
  if(opts_ptr->action == action_calibr){
    output(info_lvl, "  action               = calibrate");
    if(opts_ptr->drift_win) {
      output(info_lvl, "  drift window         = %ld", opts_ptr->drift_win);
    }else{
      output(info_lvl, "  drift window         = not set");
    }
  }else{
    if(opts_ptr->simulate){
      output(info_lvl, "  action               = synchronize (simulated)");
    }else{
      output(info_lvl, "  action               = synchronize");
    }
    switch(opts_ptr->synch_algo){
    case algo_mean:
      output(info_lvl, "  algorithm            = mean");
      break;
    case algo_p10:
      output(info_lvl, "  algorithm            = 10th percentile");
      break;
    case algo_p25:
      output(info_lvl, "  algorithm            = 25th percentile");
      break;
    case algo_median:
      output(info_lvl, "  algorithm            = median");
      break;
    }
    output(info_lvl, "  observation window   = %ld", opts_ptr->obs_win);
  }
  if(opts_ptr->no_perc){
    output(info_lvl, "  compute percentile   = no");
  }else{
    output(info_lvl, "  compute percentile   = yes");
  }
  if(opts_ptr->max_pkt_cnt > 0){
    output(info_lvl, "  max packet count     = %ld", opts_ptr->max_pkt_cnt);
  }else{
    output(info_lvl,"  max packet count     = infinite");
  }
  output(info_lvl,"  stats filename       = %s", opts_ptr->stats_filename);
  output(info_lvl, "  slave UDP port       = %hu", ntohs(opts_ptr->slave_port));
  if(opts_ptr->key_filename){
    output(info_lvl,"  key filename         = %s", opts_ptr->key_filename);
  }else{
    output(info_lvl,"  key filename         = not set");
  }
  if(opts_ptr->lat_filename){
    output(info_lvl,"  latency filename     = %s", opts_ptr->lat_filename);
  }else{
    output(info_lvl,"  latency filename     = not set");
  }
  if(opts_ptr->corr_filename){
    output(info_lvl,"  corrections filename = %s", opts_ptr->corr_filename);
  }else{
    output(info_lvl,"  corrections filename = not set");
  }
  output(info_lvl, "  latency offset       = %ld", opts_ptr->offset);
  output(info_lvl, "  drift compensation   = %ld", opts_ptr->drift);
}
