define <1 x i64> @f(<1 x i64> %0, <1 x i64> %1) {
  %3 = icmp eq <1 x i64> %0, %1
  %4 = sext <1 x i1> %3 to <1 x i64>
  ret <1 x i64> %4
}
