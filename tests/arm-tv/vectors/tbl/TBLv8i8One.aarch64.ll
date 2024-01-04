define <8 x i8> @f(<8 x i8> %0, <8 x i8> %1) {
  %3 = shufflevector <8 x i8> %0, <8 x i8> %1, <8 x i32> <i32 2, i32 8, i32 4, i32 2, i32 2, i32 2, i32 8, i32 2>
  ret <8 x i8> %3
}
