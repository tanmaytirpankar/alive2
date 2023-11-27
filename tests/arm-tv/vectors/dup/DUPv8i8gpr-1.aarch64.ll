define <8 x i8> @splat_v8i8(i8 %0) {
  %2 = insertelement <8 x i8> undef, i8 %0, i64 0
  %3 = shufflevector <8 x i8> %2, <8 x i8> undef, <8 x i32> zeroinitializer
  ret <8 x i8> %3
}
