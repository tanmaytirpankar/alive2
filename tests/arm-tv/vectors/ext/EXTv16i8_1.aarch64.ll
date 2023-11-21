define <2 x i64> @v2i64(<2 x i64> %0) {
  %2 = shufflevector <2 x i64> %0, <2 x i64> undef, <2 x i32> <i32 1, i32 0>
  ret <2 x i64> %2
}
