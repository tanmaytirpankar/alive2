define <8 x i8> @f(<8 x i16> %0, i8 %1) {
  %3 = insertelement <8 x i8> poison, i8 %1, i8 0
  %4 = shufflevector <8 x i8> %3, <8 x i8> poison, <8 x i32> zeroinitializer
  %5 = zext <8 x i8> %4 to <8 x i16>
  %6 = ashr <8 x i16> %0, %5
  %7 = trunc <8 x i16> %6 to <8 x i8>
  ret <8 x i8> %7
}
