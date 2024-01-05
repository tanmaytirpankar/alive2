define <8 x i16> @f(<8 x i16> %0, <8 x i16> %1, <8 x i16> %2) {
  %4 = shufflevector <8 x i16> %2, <8 x i16> undef, <8 x i32> <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7>
  %5 = mul <8 x i16> %4, %1
  %6 = add <8 x i16> %5, %0
  ret <8 x i16> %6
}
