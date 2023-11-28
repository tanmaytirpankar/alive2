define <1 x i64> @shl_v1i64(<1 x i64> %0, <1 x i64> %1) {
  %3 = shl <1 x i64> %0, %1
  ret <1 x i64> %3
}
