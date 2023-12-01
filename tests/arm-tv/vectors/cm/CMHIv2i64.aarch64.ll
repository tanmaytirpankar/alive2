define <2 x i64> @cmhiz2xi64(<2 x i64> %0) {
  %2 = icmp ugt <2 x i64> %0, <i64 1, i64 1>
  %3 = sext <2 x i1> %2 to <2 x i64>
  ret <2 x i64> %3
}
