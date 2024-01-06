define <16 x i8> @f(<16 x i8> %0) {
  %2 = shufflevector <16 x i8> %0, <16 x i8> undef, <16 x i32> <i32 1, i32 17, i32 3, i32 19, i32 5, i32 21, i32 7, i32 23, i32 9, i32 25, i32 11, i32 27, i32 13, i32 29, i32 15, i32 31>
  ret <16 x i8> %2
}
