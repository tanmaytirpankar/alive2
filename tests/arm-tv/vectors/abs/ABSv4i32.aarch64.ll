define <4 x i32> @f1(<4 x i32> %0) {
  %2 = icmp slt <4 x i32> %0, zeroinitializer
  %3 = sub <4 x i32> zeroinitializer, %0
  %4 = select <4 x i1> %2, <4 x i32> %3, <4 x i32> %0
  ret <4 x i32> %4
}
