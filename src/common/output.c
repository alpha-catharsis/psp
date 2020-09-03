/* C standard library headers */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* PSP Common headers */
#include "mgmt.h"
#include "output.h"

/* constants */
static const char *tags[5] = {"ERRO", "WARN", "INFO", "DEBG"};

/* globals */
enum output_lvl verb_lvl;
FILE *log_file;

/* function prototypes */
const char *verbosity_to_str(enum output_lvl);

/* verbosity management functions */
void set_verbosity(enum output_lvl lvl)
{
  verb_lvl = lvl;
}

enum output_lvl verbosity()
{
  return verb_lvl;
}

/* log management functions */
void set_logfile(const char *filename)
{
  if(log_file){
    output(warn_lvl, "log file already opened");
  }else{
    log_file = fopen(filename, "w");
    if(!log_file){
      output(warn_lvl, "failure openining log file \"%s\" for writing",
	     filename);
    }
  }
}

/* output management functions */
void init_output()
{
  verb_lvl = erro_lvl;
  log_file = NULL;
}

void fini_output(void)
{
  if(log_file && (fclose(log_file) == EOF)) {
    output(erro_lvl, "failure closing log file");
  }
}

void output(enum output_lvl lvl, const char *fmt, ...)
{
  va_list ap, ap2;
  FILE * file_ptr;
  size_t fmt_len;
  char *fmt_ext;

  if((lvl <= verb_lvl) || log_file){
    fmt_len = strlen(fmt);
    fmt_ext = malloc(fmt_len + 10);
    if(fmt_ext){
      va_start(ap, fmt);
      fmt_ext[0] = '[';
      memcpy(fmt_ext + 1, verbosity_to_str(lvl), 4);
      fmt_ext[5] = ']';
      fmt_ext[6] = ' ';
      memcpy(fmt_ext + 7, fmt, fmt_len);
      strcpy(fmt_ext + fmt_len + 7, "\n");
      if(log_file){
	va_copy(ap2, ap);
      	vfprintf(log_file, fmt_ext, ap2);
	va_end(ap2);
      }
      if(lvl <= verb_lvl){
	file_ptr = stdout;
	if(lvl <= warn_lvl){
	  file_ptr = stderr;
	}
	vfprintf(file_ptr, fmt_ext, ap);
      }
      va_end(ap);
      free(fmt_ext);
      if(lvl == erro_lvl){
	fatal_exit();
      }
    }
  }
}

/* helper functions */
const char *verbosity_to_str(enum output_lvl lvl)
{
  const char *res = "****";
  if(lvl < sizeof(tags)){
    res = tags[lvl];
  }
  return res;
}
