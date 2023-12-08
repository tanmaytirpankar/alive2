define <4 x i32> @select_or_vec(<4 x i32> %0, <4 x i32> %1) {
  %3 = or <4 x i32> %1, %0
  %4 = icmp ne <4 x i32> %3, zeroinitializer
  %5 = select <4 x i1> %4, <4 x i32> %3, <4 x i32> %1
  ret <4 x i32> %5
}
