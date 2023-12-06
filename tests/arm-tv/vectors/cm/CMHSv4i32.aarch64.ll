define <4 x i32> @compare_sext_ule_v4i32(<4 x i32> %0, <4 x i32> %1) {
  %3 = icmp ule <4 x i32> %0, %1
  %4 = sext <4 x i1> %3 to <4 x i32>
  ret <4 x i32> %4
}
