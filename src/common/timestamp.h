#ifndef PSP_COMMON_TIMESTAMP_H
#define PSP_COMMON_TIMESTAMP_H

/* C standard library headers */
#include <stdint.h>
#include <time.h>

/* timestamp packet typedefs */
typedef uint32_t ts_pkt_idx_t;
typedef uint32_t ts_sec_t;
typedef uint32_t ts_nsec_t;

/* timestamp management functions */
size_t ts_pkt_size(int);
void write_ts_pkt(uint8_t *, int, ts_pkt_idx_t,
		  time_t, long, uint8_t *);
int read_ts_pkt(uint8_t *, int, ts_pkt_idx_t *,
		time_t *, long *, uint8_t *);

#endif /* PSP_COMMON_TIMESTAMP_H */
