define <16 x i8> @f(<8 x i8> %0, <8 x i16> %1) {
  %3 = ashr <8 x i16> %1, <i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3>
  %4 = trunc <8 x i16> %3 to <8 x i8>
  %5 = bitcast <8 x i8> %0 to <1 x i64>
  %6 = bitcast <8 x i8> %4 to <1 x i64>
  %7 = shufflevector <1 x i64> %5, <1 x i64> %6, <2 x i32> <i32 0, i32 1>
  %8 = bitcast <2 x i64> %7 to <16 x i8>
  ret <16 x i8> %8
}
