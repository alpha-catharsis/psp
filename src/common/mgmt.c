/* C standard library headers */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

/* POSIX library headers */
#include <signal.h>

/* PSP Common headers */
#include "mgmt.h"
#include "output.h"

/* constants */
enum exit_type {no_exit_yet = 0,
		clean_exit_code = 1,
		error_exit_code = -1};

/* functions forward declarations */
void install_signal_handler(void);
void signal_handler(int);

/* globals */
jmp_buf buf;

/* state management functions */
int run_managed(managed_main_t main_func, main_finalizer_t fini_func, void *data_ptr)
{
  int res;

  init_output();
  install_signal_handler();
  if((res = setjmp(buf)) == no_exit_yet){
    (*main_func)(data_ptr);
  }
  if(fini_func){
    (*fini_func)(data_ptr);
  }
  fini_output();
  return res == clean_exit_code ? EXIT_SUCCESS : EXIT_FAILURE;
}

void clean_exit()
{
  longjmp(buf, clean_exit_code);
}

void fatal_exit()
{
  longjmp(buf, error_exit_code);
}

void install_signal_handler()
{
  if(signal(SIGINT,&signal_handler) == SIG_ERR){
    output(erro_lvl, "failure installing signal handler");
  }
}

void signal_handler(int signo)
{
  (void) signo;
  puts("");
  clean_exit();
}

