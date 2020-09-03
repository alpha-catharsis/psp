/* PSP Common headers */
#include "../common/output.h"

/* PSP Master headers */
#include "nonce.h"

/* nonce management functions */
ts_pkt_idx_t read_nonce(FILE *file_ptr)
{
  ts_pkt_idx_t res;
  if(fscanf(file_ptr, "%09u", &res) != 1){
    output(erro_lvl, "failure reading nonce from nonce file");
  }else{
    rewind(file_ptr);
  }
  return res;
}

void write_nonce(FILE *file_ptr, ts_pkt_idx_t idx)
{
  if(fprintf(file_ptr, "%09u", idx) < 0){
    output(erro_lvl, "failure writing nonce to file");
  }else if(fflush(file_ptr)){
    output(erro_lvl, "failure flushing nonce file");
  }else{
    rewind(file_ptr);
  }
}
