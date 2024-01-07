define <8 x i8> @f(<8 x i8> %0) {
  %2 = urem <8 x i8> %0, <i8 20, i8 20, i8 20, i8 20, i8 20, i8 20, i8 20, i8 20>
  ret <8 x i8> %2
}
