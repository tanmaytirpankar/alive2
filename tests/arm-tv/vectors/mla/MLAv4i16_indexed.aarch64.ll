define <4 x i16> @f(<4 x i16> %0, <4 x i16> %1, <8 x i16> %2) {
  %4 = shufflevector <8 x i16> %2, <8 x i16> undef, <4 x i32> <i32 6, i32 6, i32 6, i32 6>
  %5 = mul <4 x i16> %4, %1
  %6 = add <4 x i16> %5, %0
  ret <4 x i16> %6
}
