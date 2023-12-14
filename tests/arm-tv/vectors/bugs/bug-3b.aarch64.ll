define <2 x i64> @f(<4 x i32> %0) {
  %2 = shufflevector <4 x i32> %0, <4 x i32> undef, <2 x i32> <i32 0, i32 1>
  %3 = zext <2 x i32> %2 to <2 x i64>
  ret <2 x i64> %3
}
