#ifndef PSP_COMMON_OPTIONS_H
#define PSP_COMMON_OPTIONS_H

/* C standard library headers */
#include <stddef.h>

/* option_descriptor structure */
struct option_descriptor
{
  char letter;
  int has_param;
  const char *desc;
  int set;
  void *trgt;
  int (*apply_option)(struct option_descriptor *, const char *, const void *);
  const void *apply_data;
  const char *required_opts;
  const char *forbidden_opts;
};

/* support structures */
struct general_options
{
  int verb_lvl;
  char *log_fname;
};

struct num_bounds
{
  long low_bound;
  long hi_bound;
};

struct num_ubounds
{
  unsigned long low_bound;
  unsigned long hi_bound;
};

struct num_dbounds
{
  double low_bound;
  double hi_bound;
};

struct opt_group
{
  const char *group_name;
  const char *opts;
};

/* globals declarations */
extern const struct num_ubounds verb_bounds;

/* option register management */
int parse_opts(struct option_descriptor *, int, char **);
int check_opts(struct option_descriptor *);
int print_help_msg(const char *, const char *,
		   struct option_descriptor *, struct opt_group *);
int is_opt_set(struct option_descriptor *, char);
struct option_descriptor *find_opt_desc(struct option_descriptor *, char);

/* general options management */
void init_general_options(struct general_options *);
void apply_general_options(const struct general_options *);

/* options apply callbacks */
int opt_flag_apply(struct option_descriptor *, const char *, const void *);
int opt_int_set_apply(struct option_descriptor *, const char *, const void *);
int opt_int_apply(struct option_descriptor *, const char *, const void *);
int opt_uint_apply(struct option_descriptor *, const char *, const void *);
int opt_long_apply(struct option_descriptor *, const char *, const void *);
int opt_ulong_apply(struct option_descriptor *, const char *, const void *);
int opt_double_apply(struct option_descriptor *, const char *, const void *);
int opt_str_apply(struct option_descriptor *, const char *, const void *);
int opt_in_addr_apply(struct option_descriptor *, const char *, const void *);
int opt_in_port_apply(struct option_descriptor *, const char *, const void *);

/* option groups definition macros */
#define END_OPTS_GROUP {NULL, NULL}
#define OPTS_GROUP(G, O) {G, O}

/* options definition macros */
#define END_OPTS {'\0', 0, NULL, 0, NULL, NULL, NULL, NULL, NULL}
#define SIMPLE_OPT(L, D, R, F) {L, 0, D, 0, NULL, NULL, NULL, R, F}
#define FLAG_OPT(L, D, T, R, F) {L, 0, D, 0, T, &opt_flag_apply, NULL, R, F}
#define INT_SET_OPT(L, D, T, V, R, F) {L, 0, D, 0, T, &opt_int_set_apply, V, R, F}
#define INT_OPT(L, D, T, R, F) {L, 1, D, 0, T, &opt_int_apply, NULL, R, F}
#define UINT_OPT(L, D, T, R, F) {L, 1, D, 0, T, &opt_uint_apply, NULL, R, F}
#define BND_INT_OPT(L, D, T, B, R, F) {L, 1, D, 0, T, &opt_int_apply, B, R, F}
#define BND_UINT_OPT(L, D, T, B, R, F) {L, 1, D, 0, T, &opt_uint_apply, B, R, F}
#define LONG_OPT(L, D, T, R, F) {L, 1, D, 0, T, &opt_long_apply, NULL, R, F}
#define ULONG_OPT(L, D, T, R, F) {L, 1, D, 0, T, &opt_ulong_apply, NULL, R, F}
#define BND_LONG_OPT(L, D, T, B, R, F) {L, 1, D, 0, T, &opt_long_apply, B, R, F}
#define BND_ULONG_OPT(L, D, T, B, R, F) {L, 1, D, 0, T, &opt_ulong_apply, B, R, F}
#define DOUBLE_OPT(L, D, T, R, F) {L, 1, D, 0, T, &opt_double_apply, NULL, R, F}
#define BND_DOUBLE_OPT(L, D, T, B, R, F) {L, 1, D, 0, T, &opt_double_apply, B, R, F}
#define STR_OPT(L, D, T, R, F) {L, 1, D, 0, T, &opt_str_apply, NULL, R, F}
#define IN_ADDR_OPT(L, D, T, R, F) {L, 1, D, 0, T, &opt_in_addr_apply, NULL, R, F}
#define IN_PORT_OPT(L, D, T, R, F) {L, 1, D, 0, T, &opt_in_port_apply, NULL, R, F}

/* general options macros */
#define GEN_OPTS(DATA) SIMPLE_OPT('h', "displays this help message", "", ""), \
    STR_OPT('l', "<filename>, specifies the log file", &DATA.log_fname, "", "h"),\
    BND_INT_OPT('v', "<integer>, set verbosity level (0=ERRO, 1=WARN, 2=INFO, 3=DEBG)",\
		&DATA.verb_lvl, &verb_bounds, "", "h")

#define GEN_OPTS_GROUP OPTS_GROUP("general options", "hlv")

#endif /* PSP_COMMON_OPTIONS_H */
