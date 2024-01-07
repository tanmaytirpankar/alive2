define <4 x i32> @f(<4 x i32> %0, <4 x i32> %1, <4 x i32> %2) {
  %4 = mul <4 x i32> %0, %1
  %5 = sub <4 x i32> %2, %4
  ret <4 x i32> %5
}
