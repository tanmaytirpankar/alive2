define <8 x i8> @f(<8 x i8> %0, <8 x i8> %1) {
  %3 = zext <8 x i8> %0 to <8 x i16>
  %4 = zext <8 x i8> %1 to <8 x i16>
  %5 = mul <8 x i16> %3, %4
  %6 = lshr <8 x i16> %5, <i16 8, i16 8, i16 8, i16 8, i16 8, i16 8, i16 8, i16 8>
  %7 = trunc <8 x i16> %6 to <8 x i8>
  ret <8 x i8> %7
}
