define <4 x i16> @ashr_v4i16(<4 x i16> %0, <4 x i16> %1) {
  %3 = ashr <4 x i16> %0, %1
  ret <4 x i16> %3
}
