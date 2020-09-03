/* C standard library headers */
#include <memory.h>

/* POSIX headers */
#include <arpa/inet.h>

/* PSP Common headers */
#include "hmac.h"
#include "timestamp.h"

/* timestamp packet offset macros*/
#define TIMESTAMP_IDX_OFFSET (0)
#define TIMESTAMP_SEC_OFFSET (sizeof(ts_pkt_idx_t))
#define TIMESTAMP_NSEC_OFFSET (TIMESTAMP_SEC_OFFSET + sizeof(ts_sec_t))
#define TIMESTAMP_HMAC_OFFSET (TIMESTAMP_NSEC_OFFSET + sizeof(ts_nsec_t))

/* timestamp management functions */
size_t ts_pkt_size(int secure)
{
  return sizeof(ts_pkt_idx_t) + sizeof(ts_sec_t) +
    sizeof(ts_nsec_t) + (secure ? 32 : 0);
}

void write_ts_pkt(uint8_t *dest_ptr, int secure, ts_pkt_idx_t idx, time_t sec,
		  long nsec, uint8_t *key_ptr)
{
  *((ts_pkt_idx_t *) (dest_ptr + TIMESTAMP_IDX_OFFSET)) = htonl((ts_pkt_idx_t) idx);
  *((ts_sec_t *) (dest_ptr + TIMESTAMP_SEC_OFFSET)) = htonl((ts_sec_t)sec);
  *((ts_nsec_t *) (dest_ptr + TIMESTAMP_NSEC_OFFSET)) = htonl((ts_nsec_t)nsec);
  if(secure){
    generate_hmac(TIMESTAMP_HMAC_OFFSET, dest_ptr + TIMESTAMP_HMAC_OFFSET,
		  dest_ptr, key_ptr);
  }
}

int read_ts_pkt(uint8_t *src_ptr, int secure, ts_pkt_idx_t *idx_ptr,
		time_t *sec_ptr, long *nsec_ptr, uint8_t *key_ptr)
{
  if(secure && !verify_hmac(TIMESTAMP_HMAC_OFFSET,
			    src_ptr + TIMESTAMP_HMAC_OFFSET,
			    src_ptr, key_ptr)){
    return 0;
  }
  *idx_ptr = (ts_pkt_idx_t) ntohl(*((ts_pkt_idx_t *) (src_ptr + TIMESTAMP_IDX_OFFSET)));
  *sec_ptr = (time_t) ntohl(*((ts_sec_t *) (src_ptr + TIMESTAMP_SEC_OFFSET)));
  *nsec_ptr = (long) ntohl(*((ts_nsec_t *) (src_ptr + TIMESTAMP_NSEC_OFFSET)));
  return 1;  
}
