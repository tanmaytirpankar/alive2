define <4 x i32> @all_sign_bit_ashr_vec1(<4 x i32> %0) {
  %2 = and <4 x i32> %0, <i32 1, i32 1, i32 1, i32 1>
  %3 = sub <4 x i32> <i32 0, i32 1, i32 2, i32 3>, %2
  %4 = shufflevector <4 x i32> %3, <4 x i32> undef, <4 x i32> zeroinitializer
  %5 = ashr <4 x i32> %4, <i32 1, i32 31, i32 5, i32 0>
  ret <4 x i32> %5
}
