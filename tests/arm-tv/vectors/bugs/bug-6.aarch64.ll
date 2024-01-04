define <16 x i8> @f(<16 x i8> %0) {
  %2 = shufflevector <16 x i8> %0, <16 x i8> undef, <16 x i32> zeroinitializer
  ret <16 x i8> %2
}
