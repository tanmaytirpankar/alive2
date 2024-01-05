define <4 x i32> @f(<4 x i32> %0, <4 x i32> %1, <4 x i32> %2) {
  %4 = shufflevector <4 x i32> %2, <4 x i32> undef, <2 x i32> <i32 2, i32 3>
  %5 = shufflevector <2 x i32> %4, <2 x i32> undef, <4 x i32> <i32 1, i32 1, i32 1, i32 1>
  %6 = mul <4 x i32> %5, %1
  %7 = add <4 x i32> %6, %0
  ret <4 x i32> %7
}
