/* POSIX library headers */
#include <signal.h>
#include <time.h>

/* LSP Common headers */
#include "../common/output.h"

/* LSP Master headers */
#include "timer.h"

/* globals */
timer_t timer_id;
int timer_prepared = 0;
timer_callback_t timer_callback;
void *timer_data_ptr;

/* functions forward declarations */
static void signal_handler(int);

/* set_timer function */
void set_timer(long delay, timer_callback_t cb, void* td_ptr)
{
  struct sigevent sigevent_data;
  struct itimerspec timer_spec;

  timer_callback = cb;
  timer_data_ptr = td_ptr;

  if(!timer_prepared){
    sigevent_data.sigev_notify = SIGEV_SIGNAL;
    sigevent_data.sigev_signo = SIGUSR1;

    if(timer_create(CLOCK_REALTIME, &sigevent_data, &timer_id) == -1){
      output(erro_lvl, "failure creating the timer");
    }else{
      timer_prepared = 1;
    }
  }

  if(timer_prepared){
    if(signal(SIGUSR1,&signal_handler) == SIG_ERR){
      output(erro_lvl, "failure installing SIGUSR1 signal handler");
    }else{
      timer_spec.it_interval.tv_sec = 0;
      timer_spec.it_interval.tv_nsec = 0;
      timer_spec.it_value.tv_sec = delay / 1000l;
      timer_spec.it_value.tv_nsec = (delay % 1000l) * 1000000;
      if(timer_settime(timer_id, 0, &timer_spec, NULL) == -1){
	output(erro_lvl, "failure arming the timer");
      }
    }
  }

}

void wait_signals()
{
  sigset_t set;
  int signo;

  if(sigfillset(&set) == -1){
    output(erro_lvl, "sigfillset failure");
  }else{
    while(1){
      sigwait(&set, &signo);
      raise(signo);
    }
  }
}

/* signal_handler function */
static void signal_handler(int signo)
{
  (void) signo;
  (*timer_callback)(timer_data_ptr);
}
