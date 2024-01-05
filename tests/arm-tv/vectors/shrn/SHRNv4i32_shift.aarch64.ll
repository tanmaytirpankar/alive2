define <4 x i32> @f(<2 x i32> %0, <2 x i64> %1) {
  %3 = bitcast <2 x i32> %0 to <1 x i64>
  %4 = lshr <2 x i64> %1, <i64 19, i64 19>
  %5 = trunc <2 x i64> %4 to <2 x i32>
  %6 = bitcast <2 x i32> %5 to <1 x i64>
  %7 = shufflevector <1 x i64> %3, <1 x i64> %6, <2 x i32> <i32 0, i32 1>
  %8 = bitcast <2 x i64> %7 to <4 x i32>
  ret <4 x i32> %8
}
