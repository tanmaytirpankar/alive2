define <4 x i32> @f10(<4 x i32> %0, <4 x i32> %1) {
  %3 = shufflevector <4 x i32> %0, <4 x i32> %1, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  ret <4 x i32> %3
}
