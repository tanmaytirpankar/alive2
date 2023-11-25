define <8 x i16> @ins4h8(<4 x i16> %0, <8 x i16> %1) {
  %3 = extractelement <4 x i16> %0, i32 2
  %4 = insertelement <8 x i16> %1, i16 %3, i32 7
  ret <8 x i16> %4
}
