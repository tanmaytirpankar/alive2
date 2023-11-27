define <1 x i64> @lshr_v1i64(<1 x i64> %0, <1 x i64> %1) {
  %3 = lshr <1 x i64> %0, %1
  ret <1 x i64> %3
}
