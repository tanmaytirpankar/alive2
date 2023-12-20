define <8 x i8> @vtrni16_viabitcast(ptr %0, ptr %1) {
  %3 = load <4 x i16>, ptr %0, align 8
  %4 = load <4 x i16>, ptr %1, align 8
  %5 = bitcast <4 x i16> %3 to <8 x i8>
  %6 = bitcast <4 x i16> %4 to <8 x i8>
  %7 = shufflevector <8 x i8> %5, <8 x i8> %6, <8 x i32> <i32 0, i32 1, i32 8, i32 9, i32 4, i32 5, i32 12, i32 13>
  ret <8 x i8> %7
}
