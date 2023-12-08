define <2 x i64> @f8(<2 x i64> %0, <2 x i64> %1) {
  %3 = icmp uge <2 x i64> %1, %0
  %4 = select <2 x i1> %3, <2 x i64> %0, <2 x i64> %1
  ret <2 x i64> %4
}
