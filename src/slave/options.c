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
  opts_ptr->slave_port = htons(4242);
  opts_ptr->max_pkt_cnt = -1;
  opts_ptr->obs_win = 100;
  opts_ptr->simulate = 0;
  opts_ptr->key_filename = NULL;
  opts_ptr->debug = 0;
  opts_ptr->offset = 0;

  const int action_precalibr_val = action_precalibr;
  const int action_calibr_val = action_calibr;
  const int action_synch_val = action_synch;
  const struct num_bounds win_bounds = {1, 1000000};
  const struct num_bounds pkt_cnt_bounds = {1, LONG_MAX};

  struct option_descriptor optreg[] =
    { /* general options */
     GEN_OPTS(opts_ptr->gen_opts),
     
     /* action options */
     INT_SET_OPT('a', "perform pre-calibration", &opts_ptr->action, &action_precalibr_val, "", "csh"),
     INT_SET_OPT('c', "perform calibration", &opts_ptr->action, &action_calibr_val, "", "ash"),
     INT_SET_OPT('s', "perform synchronization", &opts_ptr->action, &action_synch_val, "", "ach"),

     /* general options */
     IN_PORT_OPT('p', "<port number>, specifies the slave UDP port", &opts_ptr->slave_port, "", ""),
     BND_LONG_OPT('n', "<integer>, specifies the number of timestamp packets to receive before stopping",
		  &opts_ptr->max_pkt_cnt, &pkt_cnt_bounds, "", ""), 
     BND_LONG_OPT('w', "<integer>, specifies the observation window in samples", &opts_ptr->obs_win, &win_bounds, "", ""),

     /* synchronization options */
     FLAG_OPT('f', "simulate synchronization (does not change the machine clock)", &opts_ptr->simulate, "s", ""),
     
     /* secure protocol options */
     STR_OPT('k', "<filename>, specifies the cryptographic key for timestamp authentication", &opts_ptr->key_filename, "", ""),

     /* debugging options */
     FLAG_OPT('d', "enables the generation of debug files", &opts_ptr->debug, "", ""),

     /* tweaking options */
     LONG_OPT('o', "<integer>, specifies an offset to apply to latency computation in us", &opts_ptr->offset, "", ""), 

     /* end of options */
     END_OPTS
    };

  struct opt_group optg[] = {GEN_OPTS_GROUP,
			     OPTS_GROUP("action options", "acs"),
			     OPTS_GROUP("general options", "pnw"),
			     OPTS_GROUP("synchronization options", "f"),
			     OPTS_GROUP("secure protocol options", "k"),
			     OPTS_GROUP("debugging options", "d"),
			     OPTS_GROUP("tweaking options", "o"),
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
  if(!is_opt_set(optreg, 'a') && !is_opt_set(optreg, 'c') && !is_opt_set(optreg, 's')){
    printf("no action specified\n");
    return 0;
  }
  return 1;
}

/* options reporting */
void print_selected_options(const struct options *opts_ptr)
{
  output(info_lvl, "Packet Synchronization Slave started...");
  output(info_lvl, "Parameters:");
  if(opts_ptr->action == action_precalibr){
    output(info_lvl, "  action               = pre-calibrate");
  }else if(opts_ptr->action == action_calibr){
    output(info_lvl, "  action               = calibrate");
  }else{
    if(opts_ptr->simulate){
      output(info_lvl, "  action               = synchronize (simulated)");
    }else{
      output(info_lvl, "  action               = synchronize");
    }
  }
  output(info_lvl, "  slave UDP port       = %hu", ntohs(opts_ptr->slave_port));
  if(opts_ptr->max_pkt_cnt > 0){
    output(info_lvl, "  max packet count     = %ld", opts_ptr->max_pkt_cnt);
  }else{
    output(info_lvl,"  max packet count     = infinite");
  }
  output(info_lvl, "  observation window   = %ld", opts_ptr->obs_win);
  if(opts_ptr->key_filename){
    output(info_lvl,"  key filename         = %s", opts_ptr->key_filename);
  }else{
    output(info_lvl,"  key filename         = not set");
  }
  if(opts_ptr->debug){
    output(info_lvl,"  debug files          = enabled");
  }else{
    output(info_lvl,"  debug files          = disabled");
  }
}
