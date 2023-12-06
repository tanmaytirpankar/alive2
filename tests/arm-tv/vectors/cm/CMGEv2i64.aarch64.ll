define <2 x i64> @compare_sext_sle_v2i64(<2 x i64> %0, <2 x i64> %1) {
  %3 = icmp sle <2 x i64> %0, %1
  %4 = sext <2 x i1> %3 to <2 x i64>
  ret <2 x i64> %4
}
