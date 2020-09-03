#ifndef PSP_COMMON_TIMER_H
#define PSP_COMMON_TIMER_H

/* typedefs */
typedef void (*timer_callback_t)(void *);

/* timer setting function */
void set_timer(long delay, timer_callback_t cb, void* td_ptr);
void wait_signals(void);

#endif /* PSP_COMMON_TIMER_H */
