#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// Minimal, selvstaendig SHA-256 og HMAC-SHA256 til SAMD21 (bruges til 60s TOTP PIN og Sprint 3 HMAC)
namespace SafetyCrypto {

static const uint32_t K[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline uint32_t rotr(uint32_t x, uint32_t n) {
  return (x >> n) | (x << (32 - n));
}

struct Sha256Ctx {
  uint32_t state[8];
  uint8_t buffer[64];
  uint64_t bitCount;
  size_t bufferLen;
};

inline void sha256Transform(Sha256Ctx& ctx, const uint8_t block[64]) {
  uint32_t w[64];
  for (int i = 0; i < 16; i++) {
    w[i] = ((uint32_t)block[i * 4] << 24) |
           ((uint32_t)block[i * 4 + 1] << 16) |
           ((uint32_t)block[i * 4 + 2] << 8) |
           ((uint32_t)block[i * 4 + 3]);
  }
  for (int i = 16; i < 64; i++) {
    uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }

  uint32_t a = ctx.state[0], b = ctx.state[1], c = ctx.state[2], d = ctx.state[3];
  uint32_t e = ctx.state[4], f = ctx.state[5], g = ctx.state[6], h = ctx.state[7];

  for (int i = 0; i < 64; i++) {
    uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
    uint32_t ch = (e & f) ^ ((~e) & g);
    uint32_t temp1 = h + S1 + ch + K[i] + w[i];
    uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
    uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    uint32_t temp2 = S0 + maj;

    h = g; g = f; f = e; e = d + temp1;
    d = c; c = b; b = a; a = temp1 + temp2;
  }

  ctx.state[0] += a; ctx.state[1] += b; ctx.state[2] += c; ctx.state[3] += d;
  ctx.state[4] += e; ctx.state[5] += f; ctx.state[6] += g; ctx.state[7] += h;
}

inline void sha256Init(Sha256Ctx& ctx) {
  ctx.state[0] = 0x6a09e667; ctx.state[1] = 0xbb67ae85;
  ctx.state[2] = 0x3c6ef372; ctx.state[3] = 0xa54ff53a;
  ctx.state[4] = 0x510e527f; ctx.state[5] = 0x9b05688c;
  ctx.state[6] = 0x1f83d9ab; ctx.state[7] = 0x5be0cd19;
  ctx.bitCount = 0;
  ctx.bufferLen = 0;
}

inline void sha256Update(Sha256Ctx& ctx, const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    ctx.buffer[ctx.bufferLen++] = data[i];
    if (ctx.bufferLen == 64) {
      sha256Transform(ctx, ctx.buffer);
      ctx.bitCount += 512;
      ctx.bufferLen = 0;
    }
  }
}

inline void sha256Final(Sha256Ctx& ctx, uint8_t outHash[32]) {
  ctx.bitCount += (uint64_t)ctx.bufferLen * 8;
  ctx.buffer[ctx.bufferLen++] = 0x80;
  if (ctx.bufferLen > 56) {
    while (ctx.bufferLen < 64) ctx.buffer[ctx.bufferLen++] = 0x00;
    sha256Transform(ctx, ctx.buffer);
    ctx.bufferLen = 0;
  }
  while (ctx.bufferLen < 56) ctx.buffer[ctx.bufferLen++] = 0x00;
  for (int i = 7; i >= 0; i--) {
    ctx.buffer[ctx.bufferLen++] = (uint8_t)(ctx.bitCount >> (i * 8));
  }
  sha256Transform(ctx, ctx.buffer);

  for (int i = 0; i < 8; i++) {
    outHash[i * 4]     = (uint8_t)(ctx.state[i] >> 24);
    outHash[i * 4 + 1] = (uint8_t)(ctx.state[i] >> 16);
    outHash[i * 4 + 2] = (uint8_t)(ctx.state[i] >> 8);
    outHash[i * 4 + 3] = (uint8_t)(ctx.state[i]);
  }
}

inline void hmacSha256(const uint8_t* key, size_t keyLen, const uint8_t* msg, size_t msgLen, uint8_t outMac[32]) {
  uint8_t kPad[64];
  memset(kPad, 0, sizeof(kPad));

  if (keyLen > 64) {
    Sha256Ctx kCtx;
    sha256Init(kCtx);
    sha256Update(kCtx, key, keyLen);
    sha256Final(kCtx, kPad);
  } else {
    memcpy(kPad, key, keyLen);
  }

  uint8_t iPad[64], oPad[64];
  for (int i = 0; i < 64; i++) {
    iPad[i] = kPad[i] ^ 0x36;
    oPad[i] = kPad[i] ^ 0x5c;
  }

  uint8_t innerHash[32];
  Sha256Ctx inner;
  sha256Init(inner);
  sha256Update(inner, iPad, 64);
  sha256Update(inner, msg, msgLen);
  sha256Final(inner, innerHash);

  Sha256Ctx outer;
  sha256Init(outer);
  sha256Update(outer, oPad, 64);
  sha256Update(outer, innerHash, 32);
  sha256Final(outer, outMac);
}

// Beregner 4-cifret 60s dynamisk PIN (1000-9999) ud fra delt hemmelighed og NTP Unix-tid
inline int computeTotpPin(const char* secret, unsigned long unixEpochSeconds) {
  unsigned long timeWindow = unixEpochSeconds / 60UL;
  char windowText[24];
  snprintf(windowText, sizeof(windowText), "%lu", timeWindow);

  uint8_t mac[32];
  hmacSha256(
    (const uint8_t*)secret, strlen(secret),
    (const uint8_t*)windowText, strlen(windowText),
    mac
  );

  uint32_t value = ((uint32_t)mac[0] << 24) |
                   ((uint32_t)mac[1] << 16) |
                   ((uint32_t)mac[2] << 8)  |
                   ((uint32_t)mac[3]);

  return (int)((value % 9000UL) + 1000UL);
}

} // namespace SafetyCrypto
