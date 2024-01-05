define <8 x i16> @f(ptr %0, ptr %1) {
  %3 = load <4 x i16>, ptr %0, align 8
  %4 = load <4 x i32>, ptr %1, align 16
  %5 = lshr <4 x i32> %4, <i32 1, i32 1, i32 1, i32 1>
  %6 = trunc <4 x i32> %5 to <4 x i16>
  %7 = shufflevector <4 x i16> %3, <4 x i16> %6, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  ret <8 x i16> %7
}
