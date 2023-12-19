define <2 x i32> @vec_select_no_equivalence(<2 x i32> %0) {
  %2 = shufflevector <2 x i32> %0, <2 x i32> undef, <2 x i32> <i32 1, i32 0>
  %3 = icmp eq <2 x i32> %0, zeroinitializer
  %4 = select <2 x i1> %3, <2 x i32> %2, <2 x i32> %0
  ret <2 x i32> %4
}
