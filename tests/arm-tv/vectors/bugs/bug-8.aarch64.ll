define <8 x i16> @f(<8 x i16> %0, ptr %1) {
  %3 = load <8 x i16>, ptr %1, align 16
  %4 = shufflevector <8 x i16> %3, <8 x i16> %0, <8 x i32> <i32 8, i32 1, i32 2, i32 3, i32 12, i32 5, i32 6, i32 7>
  ret <8 x i16> %4
}
