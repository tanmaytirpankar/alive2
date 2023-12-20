define <8 x i16> @vtrnQi16(ptr %0, ptr %1) {
  %3 = load <8 x i16>, ptr %0, align 16
  %4 = load <8 x i16>, ptr %1, align 16
  %5 = shufflevector <8 x i16> %3, <8 x i16> %4, <8 x i32> <i32 0, i32 8, i32 2, i32 10, i32 4, i32 12, i32 6, i32 14>
  %6 = shufflevector <8 x i16> %3, <8 x i16> %4, <8 x i32> <i32 1, i32 9, i32 3, i32 11, i32 5, i32 13, i32 7, i32 15>
  %7 = add <8 x i16> %5, %6
  ret <8 x i16> %7
}
