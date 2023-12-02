define <2 x i64> @f(<8 x i16> %0) {
  %2 = shufflevector <8 x i16> %0, <8 x i16> undef, <2 x i32> <i32 0, i32 1>
  %3 = zext <2 x i16> %2 to <2 x i64>
  ret <2 x i64> %3
}
