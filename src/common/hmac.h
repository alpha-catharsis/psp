#ifndef PSP_COMMON_HMAC_H
#define PSP_COMMON_HMAC_H

/* C standard library headers */
#include <stdint.h>

/* HMAC management functions */
void generate_hmac(size_t, uint8_t *, const uint8_t *, const uint8_t *);
int verify_hmac(size_t, const uint8_t *, const uint8_t *, const uint8_t *);

#endif /* PSP_COMMON_HMAC_H */
