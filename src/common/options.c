/* C standard library headers */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* POSIX library headers */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* PSP Common headers */
#include "options.h"
#include "output.h"

/* globals */
const struct num_ubounds verb_bounds = {erro_lvl, debg_lvl};

/* functions prototypes */
char *gen_opt_string(struct option_descriptor *);
int apply_option(struct option_descriptor *, char, char *);

/* option register management */
int parse_opts(struct option_descriptor* ods_ptr, int argc, char **argv)
{
  int c;
  char *optstring = gen_opt_string(ods_ptr);
  if(!optstring)
    return 0;
  
  int res = 1;
  while((c = getopt(argc, argv, optstring)) != -1){
    if((c == ':') || (c == '?')){
      printf("invalid options\n");
      res = 0;
    }else{
      if(!apply_option(ods_ptr, (char) c, optarg)){
	res = 0;
      }
    }
  }
  free(optstring);
  return res;
}

int check_opts(struct option_descriptor* ods_ptr)
{
  struct option_descriptor *it = ods_ptr;
  struct option_descriptor *od_ptr;
  const char *opt_letter;

  while(it->letter != '\0'){
    if(it->set){
      if(it->required_opts[0] != '*'){
	opt_letter = it->required_opts;
	while(*opt_letter != '\0'){
	  od_ptr = find_opt_desc(ods_ptr, *opt_letter);
	  if(!od_ptr){
	    printf("program bug\n");
	    return 0;
	  }else{
	    if(!od_ptr->set){
	      printf("option '-%c' requires option '-%c'\n", it->letter, *opt_letter);
	      return 0;
	    }
	  }
	  opt_letter++;
	}
      }

      opt_letter = it->forbidden_opts;
      while(*opt_letter != '\0'){
	od_ptr = find_opt_desc(ods_ptr, *opt_letter);
	if(!od_ptr){
	  printf("program bug\n");
	  return 0;
	}else{
	  if(od_ptr->set){
	    printf("option '-%c' is incompatible with option '-%c'\n", it->letter, *opt_letter);
	    return 0;
	  }
	}
	opt_letter++;
      }
    }else{
      if(it->required_opts[0] == '*'){
	printf("option '-%c' must be specified\n", it->letter);
	return 0;
      }
    }
    it++;
  }
  return 1;
}

int print_help_msg(const char *prolg1, const char *prolg2,
		   struct option_descriptor *ods_ptr, struct opt_group *ogs)
{
  struct opt_group *group_it;
  const char *opt_letter_it;
  struct option_descriptor *od_ptr;

  printf(prolg1);
  printf("\n");
  printf(prolg2);
  printf("\n");
  group_it = ogs;
  while(group_it->group_name){
    printf("  %s:\n", group_it->group_name);
    opt_letter_it = group_it->opts;
    while(*opt_letter_it != '\0'){
      od_ptr = find_opt_desc(ods_ptr, *opt_letter_it);
      if(!od_ptr){
	  printf("program bug\n");
	  return 0;
      }else{
	printf("    -%c %s\n", *opt_letter_it, od_ptr->desc);
      }
      opt_letter_it++;
    }
    puts("");
    group_it++;
  }
  return 1;
}

int is_opt_set(struct option_descriptor *ods_ptr, char letter)
{
  struct option_descriptor *od_ptr = find_opt_desc(ods_ptr, letter);
  if(!od_ptr){
    printf("program bug\n");
    return 0;
  }else{
    return od_ptr->set;
  }
}

struct option_descriptor *find_opt_desc(struct option_descriptor *ods_ptr, char letter)
{
  struct option_descriptor *it = ods_ptr;
  while(it->letter != '\0'){
    if(it->letter == letter){
      return it;
    }
    it++;
  }
  return NULL;
}

/* general options management */
void init_general_options(struct general_options *gen_opts_ptr)
{
  gen_opts_ptr->verb_lvl = info_lvl;
  gen_opts_ptr->log_fname = NULL;
}

void apply_general_options(const struct general_options *gen_opts_ptr)
{
  set_verbosity(gen_opts_ptr->verb_lvl);
  if(gen_opts_ptr->log_fname){
    set_logfile(gen_opts_ptr->log_fname);
  }
}

/* helper functions */
char *gen_opt_string(struct option_descriptor *ods_ptr)
{
  size_t chr_cnt = 3;
  struct option_descriptor *it = ods_ptr;
  char *res, *res_it;
  while(it->letter != '\0'){
    chr_cnt += it->has_param ? 2 : 1;
    it++;
  }
  res = malloc(chr_cnt);
  if(!res){
    return NULL;
  }else{
    it = ods_ptr;
    res_it = res;
    *res_it = ':';
    res_it++;
    while(it->letter != '\0'){
      *res_it = it->letter;
      res_it++;
      if(it->has_param){
        *res_it = ':';
	res_it++;
      }
      it++;
    }
  }
  return res;
}

int apply_option(struct option_descriptor *ods_ptr, char letter, char *arg)
{
  struct option_descriptor *od_ptr = find_opt_desc(ods_ptr, letter);
  if(!od_ptr){
    printf("unrecognized option '-%c'\n", letter);
    return 0;
  }else{
    if(od_ptr->has_param){
      if(!arg){
	printf("no parameter provided for option '-%c'\n", letter);
	return 0;
      }else{
	if(!od_ptr->apply_option(od_ptr, arg, od_ptr->apply_data)){
	  return 0;
	}
      }
    }else{
      if((od_ptr->apply_option) &&
	 (!od_ptr->apply_option(od_ptr, NULL, od_ptr->apply_data))){
	return 0;
      }
    }
    od_ptr->set = 1;
  }
  return 1;
}

/* options apply callbacks */
int opt_flag_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  int *trgt_int = (int *) opt_desc_ptr->trgt;
  (void) arg;
  (void) data_ptr;
  *trgt_int = 1;
  return 1;
}

int opt_int_set_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  int *trgt_int = (int *) opt_desc_ptr->trgt;
  (void) arg;
  *trgt_int = *((const int *) data_ptr);
  return 1;
}

int opt_int_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  char *endptr;
  int *trgt_int = (int *) opt_desc_ptr->trgt;
  const struct num_bounds *bounds_ptr = (const struct num_bounds *) data_ptr;
  long aux = strtol(arg, &endptr, 10);
  if(*endptr == '\0'){
    if((aux >= INT_MIN) && (aux <= INT_MAX) &&
       (!bounds_ptr || ((aux >= bounds_ptr->low_bound) &&
			(aux <= bounds_ptr->hi_bound)))){
      *trgt_int = (int) aux;
    }else{
      printf("option '-%c': integer %ld is out of bounds\n", opt_desc_ptr->letter, aux);
      return 0;
    }
  }else{
    printf("option '-%c': invalid integer \"%s\"\n", opt_desc_ptr->letter, arg);
    return 0;
  }
  return 1;
}

int opt_uint_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  char *endptr;
  unsigned int *trgt_int = (unsigned int *) opt_desc_ptr->trgt;
  const struct num_ubounds *bounds_ptr = (const struct num_ubounds *) data_ptr;
  unsigned long aux = strtoul(arg, &endptr, 10);
  if(*endptr == '\0'){
    if((aux <= UINT_MAX) &&
       (!bounds_ptr || ((aux >= bounds_ptr->low_bound) &&
			(aux <= bounds_ptr->hi_bound)))){
      *trgt_int = (unsigned int) aux;
    }else{
      printf("option '-%c': unsigned integer %ld is out of bounds\n", opt_desc_ptr->letter, aux);
      return 0;
    }
  }else{
    printf("option '-%c': invalid unsigned integer \"%s\"\n", opt_desc_ptr->letter, arg);
    return 0;
  }
  return 1;
}

int opt_long_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  char *endptr;
  long *trgt_int = (long *) opt_desc_ptr->trgt;
  const struct num_bounds *bounds_ptr = (const struct num_bounds *) data_ptr;
  long aux = strtol(arg, &endptr, 10);
  if(*endptr == '\0'){
    if((!bounds_ptr || ((aux >= bounds_ptr->low_bound) &&
			(aux <= bounds_ptr->hi_bound)))){
      *trgt_int = (long) aux;
    }else{
      printf("option '-%c': long integer %ld is out of bounds\n", opt_desc_ptr->letter, aux);
      return 0;
    }
  }else{
    printf("option '-%c': invalid long integer \"%s\"\n", opt_desc_ptr->letter, arg);
    return 0;
  }
  return 1;
}

int opt_ulong_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  char *endptr;
  unsigned long *trgt_int = (unsigned long *) opt_desc_ptr->trgt;
  const struct num_ubounds *bounds_ptr = (const struct num_ubounds *) data_ptr;
  unsigned long aux = strtoul(arg, &endptr, 10);
  if(*endptr == '\0'){
    if((!bounds_ptr || ((aux >= bounds_ptr->low_bound) &&
			(aux <= bounds_ptr->hi_bound)))){
      *trgt_int = (unsigned long) aux;
    }else{
      printf("option '-%c': unsigned long integer %ld is out of bounds\n", opt_desc_ptr->letter, aux);
      return 0;
    }
  }else{
    printf("option '-%c': invalid unsigned long integer \"%s\"\n", opt_desc_ptr->letter, arg);
    return 0;
  }
  return 1;
}

int opt_double_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  char *endptr;
  double *trgt_int = (double *) opt_desc_ptr->trgt;
  const struct num_dbounds *bounds_ptr = (const struct num_dbounds *) data_ptr;
  double aux = strtod(arg, &endptr);
  if(*endptr == '\0'){
    if((!bounds_ptr || ((aux >= bounds_ptr->low_bound) &&
			(aux <= bounds_ptr->hi_bound)))){
      *trgt_int = (double) aux;
    }else{
      printf("option '-%c': real %f is out of bounds\n", opt_desc_ptr->letter, aux);
      return 0;
    }
  }else{
    printf("option '-%c': invalid real \"%s\"\n", opt_desc_ptr->letter, arg);
    return 0;
  }
  return 1;
}

int opt_in_addr_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  in_addr_t *trgt_addr = (in_addr_t *) opt_desc_ptr->trgt;
  in_addr_t addr_aux = inet_addr(arg);
  (void) data_ptr;
  if(addr_aux != ((in_addr_t) (-1))){
    *trgt_addr = addr_aux;
  }else{
    printf("option '-%c': invalid IP adderss \"%s\"\n", opt_desc_ptr->letter, arg);
    return 0;
  }
  return 1;
}

int opt_in_port_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  in_port_t *trgt_port = (in_port_t *) opt_desc_ptr->trgt;
  long aux = atol(optarg);
  (void) data_ptr;
  if((aux > 0) && (aux <= USHRT_MAX)){
    *trgt_port = htons((in_port_t) aux);
  }else{
    printf("option '-%c': invalid internet port \"%s\"\n", opt_desc_ptr->letter, arg);
    return 0;
  }
  return 1;
}

int opt_str_apply(struct option_descriptor *opt_desc_ptr, const char *arg, const void *data_ptr)
{
  const char **trgt_str = (const char **) opt_desc_ptr->trgt;
  *trgt_str = arg;
  (void) data_ptr;
  return 1;
}
