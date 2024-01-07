define <8 x i16> @f(<8 x i16> %0) {
  %2 = shufflevector <8 x i16> %0, <8 x i16> undef, <8 x i32> <i32 1, i32 9, i32 3, i32 11, i32 5, i32 13, i32 7, i32 15>
  ret <8 x i16> %2
}
