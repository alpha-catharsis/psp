/* C standard library headers */
#include <stdio.h>
#include <string.h>

/* LSP Common headers */
#include "hmac.h"

/* SHA256 macros */
#define SHA_Ch(x,y,z)       (((x) & (y)) ^ ((~(x)) & (z)))
#define SHA_Maj(x,y,z)      (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SHA_Parity(x, y, z) ((x) ^ (y) ^ (z))

#define SHA256_SHR(bits,word)  ((word) >> (bits))
#define SHA256_ROTL(bits,word) (((word) << (bits)) | ((word) >> (32-(bits))))
#define SHA256_ROTR(bits,word) (((word) >> (bits)) | ((word) << (32-(bits))))

#define SHA256_SIGMA0(word) (SHA256_ROTR( 2,word) ^ SHA256_ROTR(13,word) ^ SHA256_ROTR(22,word))
#define SHA256_SIGMA1(word) (SHA256_ROTR( 6,word) ^ SHA256_ROTR(11,word) ^ SHA256_ROTR(25,word))
#define SHA256_sigma0(word) (SHA256_ROTR( 7,word) ^ SHA256_ROTR(18,word) ^ SHA256_SHR( 3,word))
#define SHA256_sigma1(word) (SHA256_ROTR(17,word) ^ SHA256_ROTR(19,word) ^ SHA256_SHR(10,word))

/* SHA256 constants */
static const uint32_t SHA256_H0[8] = {
  0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
  0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

static const uint32_t K[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
  0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
  0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
  0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
  0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
  0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
  0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
  0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
  0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
  0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* SHA256 context data structure */
struct sha256_context
{
  uint32_t ihash[8];
  uint32_t len_low; 
  uint32_t len_hi;
  int16_t block_idx;
  uint8_t block[64];
};

/* functions prototypes */
static void digest_to_str(char *, const uint8_t *);
static void sha256_reset(struct sha256_context *);
static void sha256_input(struct sha256_context *, const uint8_t *, size_t);
static void sha256_result(struct sha256_context *, uint8_t *);
static void sha256_process_block(struct sha256_context *);
static void hmac(size_t, uint8_t *, const uint8_t *, const uint8_t *);

/* HMAC management functions */
void generate_hmac(size_t length, uint8_t *digest_ptr, const uint8_t *data_ptr, const uint8_t *key_ptr)
{
  char ref_str[65];
  hmac(length, digest_ptr, data_ptr, key_ptr);
  digest_to_str(ref_str, digest_ptr);
}

int verify_hmac(size_t length, const uint8_t *digest_ptr, const uint8_t *data_ptr, const uint8_t *key_ptr)
{
  uint8_t digest[32];
  hmac(length, digest, data_ptr, key_ptr);
  for(int i = 0; i < 32; i++){
    if(digest[i] != digest_ptr[i]){
      char ref_str[65], test_str[65];
      digest_to_str(ref_str, digest_ptr);
      digest_to_str(test_str, (const uint8_t *)&digest);
      return 0;
    }
  }
  return 1;
}

/* helper functions */
static void digest_to_str(char *dest_ptr, const uint8_t *digest_ptr)
{
  for(int i = 0; i < 32; i++){
    sprintf(dest_ptr + 2 * i, "%02x", digest_ptr[i]);
  }
}

void sha256_reset(struct sha256_context *ctx)
{
  ctx->len_low = 0;
  ctx->len_hi = 0;
  ctx->block_idx = 0;
  memcpy(ctx->ihash, SHA256_H0, 8 * sizeof(uint32_t));
}

void sha256_input(struct sha256_context *ctx, const uint8_t *data_ptr, size_t length)
{
  while(length--)
  {
    ctx->block[ctx->block_idx++] = *data_ptr;
    ctx->len_low += 8;
    ctx->len_hi++;
    if(ctx->block_idx == 64){
      sha256_process_block(ctx);
    }
    data_ptr++;
  }
}

void sha256_result(struct sha256_context *ctx, uint8_t *digest_ptr)
{
  if(ctx->block_idx >= (64 - 8)){
    ctx->block[ctx->block_idx++] = 0x80;
    while (ctx->block_idx < 64){
      ctx->block[ctx->block_idx++] = 0;
    }
    sha256_process_block(ctx);
  }else{
    ctx->block[ctx->block_idx++] = 0x80;
  }
  while (ctx->block_idx < (64 - 8))
    ctx->block[ctx->block_idx++] = 0;
  ctx->block[56] = (uint8_t) (ctx->len_hi >> 24);
  ctx->block[57] = (uint8_t) (ctx->len_hi >> 16);
  ctx->block[58] = (uint8_t) (ctx->len_hi >> 8);
  ctx->block[59] = (uint8_t) (ctx->len_hi);
  ctx->block[60] = (uint8_t) (ctx->len_low >> 24);
  ctx->block[61] = (uint8_t) (ctx->len_low >> 16);
  ctx->block[62] = (uint8_t) (ctx->len_low >> 8);
  ctx->block[63] = (uint8_t) (ctx->len_low);
  sha256_process_block(ctx);
  memset(ctx->block, 0, 64);
  ctx->len_low = 0;
  ctx->len_hi = 0;
  for(int i = 0; i < 32; ++i)
    digest_ptr[i] = (uint8_t)(ctx->ihash[i >> 2] >> 8 * (3 - (i & 0x03)));
}

void sha256_process_block(struct sha256_context *ctx)
{
  int t4;
  uint32_t temp1, temp2;
  uint32_t W[64];
  uint32_t A, B, C, D, E, F, G, H;

  for(int t = t4 = 0; t < 16; t++, t4 += 4)
    W[t] = (((uint32_t) ctx->block[t4]) << 24) |
      (((uint32_t) ctx->block[t4 + 1]) << 16) |
      (((uint32_t) ctx->block[t4 + 2]) << 8) |
      (((uint32_t) ctx->block[t4 + 3]));
  for(int t = 16; t < 64; t++)
    W[t] = SHA256_sigma1(W[t - 2]) + W[t - 7] +
      SHA256_sigma0(W[t - 15]) + W[t - 16];
  A = ctx->ihash[0];
  B = ctx->ihash[1];
  C = ctx->ihash[2];
  D = ctx->ihash[3];
  E = ctx->ihash[4];
  F = ctx->ihash[5];
  G = ctx->ihash[6];
  H = ctx->ihash[7];
  for(int t = 0; t < 64; t++){
    temp1 = H + SHA256_SIGMA1(E) + SHA_Ch (E, F, G) + K[t] + W[t];
    temp2 = SHA256_SIGMA0(A) + SHA_Maj (A, B, C);
    H = G;
    G = F;
    F = E;
    E = D + temp1;
    D = C;
    C = B;
    B = A;
    A = temp1 + temp2;
  }
  ctx->ihash[0] += A;
  ctx->ihash[1] += B;
  ctx->ihash[2] += C;
  ctx->ihash[3] += D;
  ctx->ihash[4] += E;
  ctx->ihash[5] += F;
  ctx->ihash[6] += G;
  ctx->ihash[7] += H;
  ctx->block_idx = 0;
}

void hmac(size_t length, uint8_t *digest_ptr, const uint8_t *data_ptr, const uint8_t *key_ptr)
{
  struct sha256_context ctx;
  uint8_t k_ipad[64];
  uint8_t k_opad[64];

  for(int i = 0; i < 32; i++){
    k_ipad[i] = key_ptr[i] ^ 0x36;
    k_opad[i] = key_ptr[i] ^ 0x5c;
  }
  for(int i = 32; i < 64; i++){
    k_ipad[i] = 0x36;
    k_opad[i] = 0x5c;
  }
  sha256_reset(&ctx);
  sha256_input(&ctx, k_ipad, 64);
  sha256_input(&ctx, data_ptr, length);
  sha256_result(&ctx, digest_ptr);
  sha256_reset(&ctx);
  sha256_input(&ctx, k_opad, 64);
  sha256_input(&ctx, digest_ptr, 32);
  sha256_result(&ctx, digest_ptr);
}
