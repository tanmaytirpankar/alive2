define <2 x i64> @shr_s_vec_v2i64(<2 x i64> %0, <2 x i64> %1) {
  %3 = ashr <2 x i64> %0, %1
  ret <2 x i64> %3
}
