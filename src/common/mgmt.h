#ifndef PSP_COMMON_MGMT_H
#define PSP_COMMON_MGMT_H

/* typedefs */
typedef void (*managed_main_t)(void *);
typedef void (*main_finalizer_t)(void *);

/* state management functions */
int run_managed(managed_main_t, main_finalizer_t, void *);
void clean_exit(void);
void fatal_exit(void);

#endif /* PSP_COMMON_MGMT_H */
