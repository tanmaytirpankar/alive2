define <4 x i32> @f(<4 x i32> %0, <4 x i32> %1, <4 x i32> %2) {
  %4 = shufflevector <4 x i32> %2, <4 x i32> undef, <4 x i32> zeroinitializer
  %5 = mul <4 x i32> %4, %1
  %6 = sub <4 x i32> %0, %5
  ret <4 x i32> %6
}
