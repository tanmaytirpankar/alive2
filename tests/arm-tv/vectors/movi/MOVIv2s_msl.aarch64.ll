define <2 x i64> @f(<2 x i32> %0) {
  %2 = ashr <2 x i32> %0, <i32 16, i32 16>
  %3 = sext <2 x i32> %2 to <2 x i64>
  %4 = mul <2 x i64> %3, <i64 32767, i64 32767>
  ret <2 x i64> %4
}
