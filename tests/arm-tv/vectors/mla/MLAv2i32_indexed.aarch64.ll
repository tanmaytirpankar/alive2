define <2 x i32> @f(<2 x i32> %0, <2 x i32> %1, <2 x i32> %2) {
  %4 = shufflevector <2 x i32> %2, <2 x i32> undef, <2 x i32> <i32 1, i32 1>
  %5 = mul <2 x i32> %4, %1
  %6 = add <2 x i32> %5, %0
  ret <2 x i32> %6
}
