define <2 x i32> @f(<2 x i32> %0, <2 x i32> %1) {
  %3 = shufflevector <2 x i32> %0, <2 x i32> %1, <2 x i32> <i32 1, i32 2>
  ret <2 x i32> %3
}
