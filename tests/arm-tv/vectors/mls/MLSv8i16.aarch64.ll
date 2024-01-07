define <8 x i16> @f(<8 x i16> %0, <8 x i16> %1, ptr %2) {
  %4 = udiv <8 x i16> %0, %1
  store <8 x i16> %4, ptr %2, align 16
  %5 = mul <8 x i16> %4, %1
  %6 = sub <8 x i16> %0, %5
  ret <8 x i16> %6
}
