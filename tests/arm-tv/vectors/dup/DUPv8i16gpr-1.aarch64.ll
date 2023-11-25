define <8 x i16> @f5(i16 %0) {
  %2 = insertelement <8 x i16> undef, i16 %0, i32 3
  ret <8 x i16> %2
}
