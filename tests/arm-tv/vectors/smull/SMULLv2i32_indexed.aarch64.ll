define <2 x i32> @srem_splat_constant1(<2 x i32> %0) {
  %2 = shufflevector <2 x i32> %0, <2 x i32> undef, <2 x i32> zeroinitializer
  %3 = srem <2 x i32> %2, <i32 42, i32 42>
  ret <2 x i32> %3
}
