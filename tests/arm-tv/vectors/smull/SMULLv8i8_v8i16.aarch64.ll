define <8 x i16> @vmulls8(<8 x i8> %0, <8 x i8> %1) {
  %3 = sext <8 x i8> %0 to <8 x i16>
  %4 = sext <8 x i8> %1 to <8 x i16>
  %5 = mul <8 x i16> %3, %4
  ret <8 x i16> %5
}
