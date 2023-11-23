define <16 x i8> @f1(<16 x i8> %0) {
  %2 = insertelement <16 x i8> %0, i8 0, i32 0
  ret <16 x i8> %2
}
