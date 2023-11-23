define <8 x i16> @fun5(<8 x i16> %0, <8 x i16> %1) {
  %3 = sdiv <8 x i16> %0, %1
  ret <8 x i16> %3
}
