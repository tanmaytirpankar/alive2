define <4 x i32> @combine_vec_udiv_by_pow2a(<4 x i32> %0) {
  %2 = udiv <4 x i32> %0, <i32 4, i32 4, i32 4, i32 4>
  ret <4 x i32> %2
}
