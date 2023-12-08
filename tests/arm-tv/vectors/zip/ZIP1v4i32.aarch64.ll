define <4 x i32> @svop(<4 x i32> %0, <4 x i32> %1) {
  %3 = shufflevector <4 x i32> %0, <4 x i32> %1, <4 x i32> <i32 0, i32 4, i32 1, i32 5>
  ret <4 x i32> %3
}
