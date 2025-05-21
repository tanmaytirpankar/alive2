#pragma once

#include <cstdint>

namespace lifter {

uint64_t replicate8to64(uint64_t v);
uint64_t Replicate(uint64_t bit, int N);
uint64_t Replicate32x2(uint64_t bits32);
uint64_t Replicate16x4(uint64_t bits16);
uint64_t Replicate8x8(uint64_t bits8);

} // end namespace lifter
