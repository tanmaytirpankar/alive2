define <4 x i32> @f7(<4 x i32> %0) {
  %2 = shufflevector <4 x i32> %0, <4 x i32> undef, <4 x i32> zeroinitializer
  ret <4 x i32> %2
}
