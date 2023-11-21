define <16 x i8> @shuffle_v16i8_vpalignr(<16 x i8> %0, <16 x i8> %1) {
  %3 = shufflevector <16 x i8> %0, <16 x i8> %1, <16 x i32> <i32 31, i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14>
  ret <16 x i8> %3
}
