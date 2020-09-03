#ifndef PSPM_OPTIONS_H
#define PSPM_OPTIONS_H

/* POSIX library headers */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

/* PSP Common headers */
#include "../common/options.h"

/* option structure */
struct options
{
  /* general options */
  struct general_options gen_opts;

  /* destination options */
  struct sockaddr_in slave_addr;
  int bcast_enabled;

  /* timestamp transmission options */
  long period;
  long stagger;
  long max_pkt_cnt;
  
  /* QoS options */
  int tos;
  
  /* secure protocol options */
  const char *key_filename;
  const char *nonce_filename;
};

/* options parsing functions */
int parse_command_line(int, char **, struct options *);

/* options reporting */
void print_selected_options(const struct options *);

#endif /* PSPM_OPTIONS_H */
