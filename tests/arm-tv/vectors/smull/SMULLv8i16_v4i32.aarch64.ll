define <8 x i16> @sdiv16xi8(<8 x i16> %0) {
  %2 = sdiv <8 x i16> %0, <i16 9, i16 9, i16 9, i16 9, i16 9, i16 9, i16 9, i16 9>
  ret <8 x i16> %2
}
