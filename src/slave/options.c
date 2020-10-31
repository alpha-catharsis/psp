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
  opts_ptr->obs_win = 120;
  opts_ptr->synch_method = synch_freq;
  opts_ptr->freq_estim_slots = 10;
  opts_ptr->time_step_thr = 10000;
  opts_ptr->time_corr_damp = 0;
  opts_ptr->freq_corr_damp = 50;
  opts_ptr->time_corr_clamp = LONG_MAX;
  opts_ptr->freq_corr_clamp = LONG_MAX;
  opts_ptr->qs_rounds = 0;
  opts_ptr->key_filename = NULL;
  opts_ptr->debug = 0;

  const int action_precalibr_val = action_precalibr;
  const int action_calibr_val = action_calibr;
  const int action_synch_val = action_synch;
  const struct num_bounds win_bounds = {1, 10000000000L};
  const struct num_bounds pkt_cnt_bounds = {1, LONG_MAX};
  const struct num_bounds synch_method_bounds = {0, 2};
  const struct num_bounds freq_estim_slots_bounds = {2,1000};
  const struct num_bounds damp_bounds = {0, 99};
  const struct num_bounds clamp_bounds = {0, LONG_MAX};
  const struct num_bounds time_step_thr_bounds = {1, 3600000000L};
  const struct num_bounds qs_rounds_bounds = {1, 10};

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
     BND_INT_OPT('m', "<integer>, specifies the synchronization method (0=STEP, 1=SMOOTH, 2=FREQ)",
                 &opts_ptr->synch_method, &synch_method_bounds, "s", ""),
     BND_LONG_OPT('f', "<integer>, set the number of slots used for frequency estimation",
                  &opts_ptr->freq_estim_slots, &freq_estim_slots_bounds, "s", ""),
     BND_LONG_OPT('t', "<integer>, set the time correction step threshold in us",
		  &opts_ptr->time_step_thr, &time_step_thr_bounds, "s", ""),
     BND_LONG_OPT('T', "<integer>, set time correction dampening factor in percent",
                  &opts_ptr->time_corr_damp, &damp_bounds, "s", ""),
     BND_LONG_OPT('F', "<integer>, set frequecy correction dampening factor in percent",
                  &opts_ptr->freq_corr_damp, &damp_bounds, "s", ""),
     BND_LONG_OPT('C', "<integer>, set time correction clamping in ns",
                  &opts_ptr->time_corr_clamp, &clamp_bounds, "s", ""),
     BND_LONG_OPT('D', "<integer>, set frequecy correction clamping in ppb",
                  &opts_ptr->freq_corr_clamp, &clamp_bounds, "s", ""),
     BND_LONG_OPT('q', "<integer>, enables quickstart and specifies the quickstart rounds",
		  &opts_ptr->qs_rounds, &qs_rounds_bounds, "s", ""),
     
     /* secure protocol options */
     STR_OPT('k', "<filename>, specifies the cryptographic key for timestamp authentication", &opts_ptr->key_filename, "", ""),

     /* debugging options */
     FLAG_OPT('d', "enables the generation of debug files", &opts_ptr->debug, "", ""),

     /* end of options */
     END_OPTS
    };

  struct opt_group optg[] = {GEN_OPTS_GROUP,
                             OPTS_GROUP("action options", "acs"),
                             OPTS_GROUP("common options", "pnw"),
                             OPTS_GROUP("synchronization options", "mftTFCDq"),
                             OPTS_GROUP("secure protocol options", "k"),
                             OPTS_GROUP("debugging options", "d"),
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
    output(info_lvl, "  action                 = pre-calibrate");
  }else if(opts_ptr->action == action_calibr){
    output(info_lvl, "  action                 = calibrate");
  }else{
    output(info_lvl, "  action                 = synchronize");
    if(opts_ptr->synch_method == synch_step){
      output(info_lvl, "  synchronization method = step");
    }else if(opts_ptr->synch_method == synch_smooth){
      output(info_lvl, "  synchronization method = smooth");
    }else{
      output(info_lvl, "  synchronization method = frequency");
    }
    if(opts_ptr->synch_method != synch_step){
      output(info_lvl, "  time step threshold    = %ld", opts_ptr->time_step_thr);
      output(info_lvl, "  time correction damp.  = %ld%%", opts_ptr->time_corr_damp);
      output(info_lvl, "  time correction clamp. = %ld", opts_ptr->time_corr_clamp);
      if(opts_ptr->synch_method != synch_smooth){
        output(info_lvl, "  frequency estim. slots = %ld", opts_ptr->freq_estim_slots);
        output(info_lvl, "  freq. correction damp. = %ld%%", opts_ptr->freq_corr_damp);
        output(info_lvl, "  freq. correction clamp.= %ld", opts_ptr->freq_corr_clamp);
      }
    }
    if(opts_ptr->qs_rounds){
      if(opts_ptr->qs_rounds == 1){
        output(info_lvl, "  quickstart             = 1 round");
      }else{
        output(info_lvl, "  quickstart             = %ld rounds", opts_ptr->qs_rounds);
      }
    }else{
      output(info_lvl, "  quickstart             = disabled");
    }
  }
  output(info_lvl, "  slave UDP port         = %hu", ntohs(opts_ptr->slave_port));
  if(opts_ptr->max_pkt_cnt > 0){
    output(info_lvl, "  max packet count       = %ld", opts_ptr->max_pkt_cnt);
  }else{
    output(info_lvl,"  max packet count       = infinite");
  }
  output(info_lvl, "  observation window     = %ld", opts_ptr->obs_win);
  if(opts_ptr->key_filename){
    output(info_lvl,"  key filename           = %s", opts_ptr->key_filename);
  }else{
    output(info_lvl,"  key filename           = not set");
  }
  if(opts_ptr->debug){
    output(info_lvl,"  debug files            = enabled");
  }else{
    output(info_lvl,"  debug files            = disabled");
  }
}
