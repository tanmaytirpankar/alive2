define <4 x i16> @f(<4 x i16> %0) {
  %2 = mul <4 x i16> %0, <i16 3, i16 3, i16 3, i16 3>
  ret <4 x i16> %2
}
