define <8 x i16> @lshr_v8i16(<8 x i16> %0, <8 x i16> %1) {
  %3 = lshr <8 x i16> %0, %1
  ret <8 x i16> %3
}
