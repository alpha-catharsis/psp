/* C standard library headers */
#include "limits.h"
#include "stdio.h"

/* PSP Common headers */
#include "../common/output.h"

/* PSP Master headers */
#include "options.h"

/* functions forward declarations */
static int custom_option_checks(struct option_descriptor *);

/* parse_command_line */
int parse_command_line(int argc, char **argv, struct options *opts_ptr)
{
  init_general_options(&opts_ptr->gen_opts);
  opts_ptr->slave_addr.sin_family = AF_INET;
  opts_ptr->slave_addr.sin_port = htons(4242);
  opts_ptr->bcast_enabled = 0;
  opts_ptr->period = 1000;
  opts_ptr->stagger = 250;
  opts_ptr->max_pkt_cnt = -1;
  opts_ptr->tos = -1;
  opts_ptr->key_filename = NULL;
  opts_ptr->nonce_filename = NULL;

  const struct num_bounds period_bounds = {0, 86400000};
  const struct num_bounds stagger_bounds = {0, 86399999};
  const struct num_bounds pkt_cnt_bounds = {1, LONG_MAX};
  const struct num_bounds tos_bounds = {0, 255};

  struct option_descriptor optreg[] =
    { /* general options */
     GEN_OPTS(opts_ptr->gen_opts),
     
     /* destination options */
     IN_ADDR_OPT('a', "<IP address>, specifies the slave IP address", &opts_ptr->slave_addr.sin_addr, "*", ""),
     FLAG_OPT('b', "enables broadcast of timestamp packets", &opts_ptr->bcast_enabled, "", ""),
     IN_PORT_OPT('p', "<port number>, specifies the slave UDP port", &opts_ptr->slave_addr.sin_port, "", ""),

     /* timestamp transmission options */
     BND_LONG_OPT('d', "<integer>, specifies timestamp transmission "
		  "period in ms (value between 1 and 86400000)", &opts_ptr->period, &period_bounds,
		  "", ""),
     BND_LONG_OPT('s', "<integer>, specifies timestamp transmission "
		  "stagger is ms (value between 0 and 86399999)", &opts_ptr->stagger, &stagger_bounds,
		  "", ""),
     BND_LONG_OPT('n', "<integer>, specifies the number of timestamp packets to emit before stopping",
		  &opts_ptr->max_pkt_cnt, &pkt_cnt_bounds, "", ""), 

     /* QoS options */
     BND_INT_OPT('t', "<integer>, specifies timestamp packets TOS field", &opts_ptr->tos, &tos_bounds, "", ""),

     /* secure protocol options */
     STR_OPT('k', "<filename>, specifies the cryptographic key for timestamp authentication", &opts_ptr->key_filename, "o", ""),
     STR_OPT('o', "<filename>, specifies the nonce file name", &opts_ptr->nonce_filename, "k", ""),

     /* end of options */
     END_OPTS};

  struct opt_group optg[] = {GEN_OPTS_GROUP,
			     OPTS_GROUP("destination options", "abp"),
			     OPTS_GROUP("timestamp transmission options", "dsnt"),
			     OPTS_GROUP("secure protocol options", "ko"),
			     END_OPTS_GROUP};

  if(parse_opts(optreg, argc, argv) &&
     !is_opt_set(optreg, 'h') &&
     check_opts(optreg) &&
     custom_option_checks(optreg)){
    return 1;
  }else{
    print_help_msg("Packet Synchronization Protocol (PSP) Master",
		   "usage: pspm [action] [options]\n",
		   optreg, optg);
    return 0;
  }
}

/* custom opttion checks */
static int custom_option_checks(struct option_descriptor *optreg)
{
  struct option_descriptor *period_opt = find_opt_desc(optreg, 'd');
  struct option_descriptor *stagger_opt = find_opt_desc(optreg, 's');
  if(*((long *) period_opt->trgt) <= *((long *) stagger_opt->trgt)){
    printf("stagger shall be strictly smaller than period\n");
    return 0;
  }
  return 1;
}

/* options reporting */
void print_selected_options(const struct options *opts_ptr)
{
  output(info_lvl, "Packet Synchronization Master started...");
  output(info_lvl, "Parameters:");
  output(info_lvl, "  slave address        = %s", inet_ntoa(opts_ptr->slave_addr.sin_addr));
  output(info_lvl, "  slave UDP port       = %hu", ntohs(opts_ptr->slave_addr.sin_port));
  output(info_lvl, "  broadcast            = %s", opts_ptr->bcast_enabled ? "enabled" : "disabled");
  output(info_lvl, "  transmission period  = %ld", opts_ptr->period);
  output(info_lvl, "  transmission stagger = %ld", opts_ptr->stagger);
  if(opts_ptr->max_pkt_cnt > 0){
    output(info_lvl, "  max packet count     = %ld", opts_ptr->max_pkt_cnt);
  }else{
    output(info_lvl,"  max packet count     = infinite");
  }
  if(opts_ptr->tos != -1){
    output(info_lvl,"  UDP packet TOS field = 0x%02x", opts_ptr->tos);
  }else{
    output(info_lvl,"  UDP packet TOS field = not set");
  }
  if(opts_ptr->key_filename){
    output(info_lvl,"  key filename         = %s", opts_ptr->key_filename);
  }else{
    output(info_lvl,"  key filename         = not set");
  }
  if(opts_ptr->nonce_filename){
    output(info_lvl,"  nonce filename       = %s", opts_ptr->nonce_filename);
  }else{
    output(info_lvl,"  nonce filename       = not set");
  }
}
