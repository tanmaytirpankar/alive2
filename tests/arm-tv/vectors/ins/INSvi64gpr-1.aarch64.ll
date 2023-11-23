define <2 x i64> @ins1_ins1_sdiv(i64 %0, i64 %1) {
  %3 = insertelement <2 x i64> <i64 42, i64 -42>, i64 %0, i64 1
  %4 = insertelement <2 x i64> <i64 -7, i64 128>, i64 %1, i32 1
  %5 = sdiv <2 x i64> %3, %4
  ret <2 x i64> %5
}
