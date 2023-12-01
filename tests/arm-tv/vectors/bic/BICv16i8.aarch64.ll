define <2 x i64> @andnot_v2i64(<2 x i64> %0, <2 x i64> %1) {
  %3 = xor <2 x i64> %1, <i64 -1, i64 -1>
  %4 = and <2 x i64> %0, %3
  ret <2 x i64> %4
}
