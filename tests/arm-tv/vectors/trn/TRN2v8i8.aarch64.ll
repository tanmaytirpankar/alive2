define <16 x i8> @vtrni8_undef_Qres(ptr %0, ptr %1) {
  %3 = load <8 x i8>, ptr %0, align 8
  %4 = load <8 x i8>, ptr %1, align 8
  %5 = shufflevector <8 x i8> %3, <8 x i8> %4, <16 x i32> <i32 0, i32 poison, i32 2, i32 10, i32 poison, i32 12, i32 6, i32 14, i32 1, i32 9, i32 3, i32 11, i32 5, i32 poison, i32 poison, i32 15>
  ret <16 x i8> %5
}
