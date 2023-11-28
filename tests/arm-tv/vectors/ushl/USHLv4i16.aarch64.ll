define <4 x i16> @shl16x4(<4 x i16> %0, i16 %1) {
  %3 = sub i16 0, %1
  %4 = insertelement <4 x i16> undef, i16 %3, i32 0
  %5 = shufflevector <4 x i16> %4, <4 x i16> undef, <4 x i32> zeroinitializer
  %6 = shl <4 x i16> %0, %5
  ret <4 x i16> %6
}
