define <8 x i16> @shuffle_v8i16_def01234(<8 x i16> %0, <8 x i16> %1) {
  %3 = shufflevector <8 x i16> %0, <8 x i16> %1, <8 x i32> <i32 13, i32 14, i32 15, i32 0, i32 1, i32 2, i32 3, i32 4>
  ret <8 x i16> %3
}
