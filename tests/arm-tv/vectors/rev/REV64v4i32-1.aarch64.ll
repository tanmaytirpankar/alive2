define <4 x i32> @v4i32(<4 x i32> %0) {
  %2 = shufflevector <4 x i32> %0, <4 x i32> undef, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
  ret <4 x i32> %2
}
