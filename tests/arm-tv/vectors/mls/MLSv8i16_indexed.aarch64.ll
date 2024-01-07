define <8 x i16> @f(<8 x i16> %0, <8 x i16> %1, <4 x i16> %2) {
  %4 = shufflevector <4 x i16> %2, <4 x i16> undef, <8 x i32> <i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3, i32 3>
  %5 = mul <8 x i16> %4, %1
  %6 = sub <8 x i16> %0, %5
  ret <8 x i16> %6
}
