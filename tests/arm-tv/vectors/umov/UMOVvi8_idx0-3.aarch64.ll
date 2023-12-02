define <2 x i64> @f(<16 x i8> %0) {
  %2 = shufflevector <16 x i8> %0, <16 x i8> poison, <2 x i32> <i32 0, i32 1>
  %3 = sext <2 x i8> %2 to <2 x i64>
  ret <2 x i64> %3
}
