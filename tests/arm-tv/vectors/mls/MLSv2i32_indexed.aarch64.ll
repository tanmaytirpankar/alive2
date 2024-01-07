define <2 x i32> @f(<2 x i32> %0, <2 x i32> %1, <2 x i32> %2) {
  %4 = shufflevector <2 x i32> %2, <2 x i32> undef, <2 x i32> zeroinitializer
  %5 = mul <2 x i32> %4, %1
  %6 = sub <2 x i32> %0, %5
  ret <2 x i32> %6
}
