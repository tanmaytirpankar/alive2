define <4 x i32> @f(<4 x i32> %0) {
  %2 = srem <4 x i32> %0, <i32 1, i32 1, i32 2, i32 3>
  %3 = icmp eq <4 x i32> %2, zeroinitializer
  %4 = zext <4 x i1> %3 to <4 x i32>
  ret <4 x i32> %4
}
