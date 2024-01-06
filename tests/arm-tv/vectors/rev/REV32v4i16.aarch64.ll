define <4 x i16> @sel.v4i16(<4 x i16> %0, <4 x i16> %1) {
  %3 = shufflevector <4 x i16> %0, <4 x i16> %1, <4 x i32> <i32 0, i32 5, i32 2, i32 7>
  ret <4 x i16> %3
}
