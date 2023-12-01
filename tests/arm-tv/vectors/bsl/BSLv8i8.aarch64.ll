define <16 x i8> @bsl(<4 x i16> noundef %0, <4 x i16> noundef %1, <4 x i16> noundef %2, <4 x i16> noundef %3) {
  %5 = and <4 x i16> %1, %0
  %6 = xor <4 x i16> %0, <i16 -1, i16 -1, i16 -1, i16 -1>
  %7 = and <4 x i16> %6, %2
  %8 = or <4 x i16> %7, %5
  %9 = bitcast <4 x i16> %8 to <8 x i8>
  %10 = shufflevector <8 x i8> %9, <8 x i8> zeroinitializer, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  ret <16 x i8> %10
}
