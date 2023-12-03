define <2 x i64> @f(<2 x i64> %0) {
  %2 = lshr <2 x i64> %0, <i64 1, i64 1>
  ret <2 x i64> %2
}
