define <8 x i16> @f(<8 x i16> %0) {
  %2 = shufflevector <8 x i16> %0, <8 x i16> %0, <8 x i32> <i32 3, i32 2, i32 1, i32 0, i32 4, i32 5, i32 6, i32 7>
  ret <8 x i16> %2
}
