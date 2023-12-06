define <2 x i64> @cmtst2xi64(<2 x i64> %0, <2 x i64> %1) {
  %3 = and <2 x i64> %0, %1
  %4 = icmp ne <2 x i64> %3, zeroinitializer
  %5 = sext <2 x i1> %4 to <2 x i64>
  ret <2 x i64> %5
}
