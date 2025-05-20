#include "backend_tv/bitutils.h"

// Some of these are from https://github.com/agustingianni/retools

uint64_t replicate8to64(uint64_t v) {
  uint64_t ret = 0;
  for (int i = 0; i < 8; ++i) {
    bool b = (v & 128) != 0;
    ret <<= 8;
    if (b)
      ret |= 0xff;
    v <<= 1;
  }
  return ret;
}

uint64_t Replicate(uint64_t bit, int N) {
  if (!bit)
    return 0;
  if (N == 64)
    return 0xffffffffffffffffLL;
  return (1ULL << N) - 1;
}

uint64_t Replicate32x2(uint64_t bits32) {
  return (bits32 << 32) | bits32;
}

uint64_t Replicate16x4(uint64_t bits16) {
  return Replicate32x2((bits16 << 16) | bits16);
}

uint64_t Replicate8x8(uint64_t bits8) {
  return Replicate16x4((bits8 << 8) | bits8);
}
