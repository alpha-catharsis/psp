#ifndef PSP_COMMON_OUTPUT_H
#define PSP_COMMON_OUTPUT_H

/* constants */
enum output_lvl {erro_lvl = 0,
		 warn_lvl = 1,
		 info_lvl = 2,
		 debg_lvl = 3};

/* verbosity management functions */
void set_verbosity(enum output_lvl);
enum output_lvl verbosity(void);

/* log management functions */
void set_logfile(const char *);

/* output management functions */
void init_output(void);
void fini_output(void);
void output(enum output_lvl, const char *, ...);

#endif /* PSP_COMMON_STATE_H */
