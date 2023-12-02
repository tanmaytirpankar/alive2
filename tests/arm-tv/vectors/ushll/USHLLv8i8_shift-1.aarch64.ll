define <8 x i16> @f(<8 x i8> %0) {
  %2 = zext <8 x i8> %0 to <8 x i16>
  ret <8 x i16> %2
}

