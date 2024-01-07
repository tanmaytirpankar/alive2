define <4 x i16> @f(<4 x i16> %0, <4 x i16> %1, <4 x i16> %2) {
  %4 = mul <4 x i16> %1, %2
  %5 = sub <4 x i16> %0, %4
  ret <4 x i16> %5
}
