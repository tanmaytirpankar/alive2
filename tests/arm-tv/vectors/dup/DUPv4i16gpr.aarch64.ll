define <4 x i16> @splat_v4i16(i16 %0) {
  %2 = insertelement <4 x i16> undef, i16 %0, i64 0
  %3 = shufflevector <4 x i16> %2, <4 x i16> undef, <4 x i32> zeroinitializer
  ret <4 x i16> %3
}
