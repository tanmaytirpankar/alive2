define <8 x i16> @f(<8 x i8> %0, <8 x i8> %1) {
  %3 = zext <8 x i8> %0 to <8 x i16>
  %4 = zext <8 x i8> %1 to <8 x i16>
  %5 = sub <8 x i16> %3, %4
  ret <8 x i16> %5
}
