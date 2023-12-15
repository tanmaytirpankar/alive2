define <4 x i32> @t3(<4 x i32> %0, <4 x i32> %1) {
  %3 = icmp ugt <4 x i32> %0, %1
  %4 = select <4 x i1> %3, <4 x i32> %0, <4 x i32> %1
  ret <4 x i32> %4
}
