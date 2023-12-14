define <2 x i64> @f(<2 x i32> %0) {
  %2 = zext <2 x i32> %0 to <2 x i64>
  %3 = shl <2 x i64> %2, <i64 1, i64 1>
  ret <2 x i64> %3
}
