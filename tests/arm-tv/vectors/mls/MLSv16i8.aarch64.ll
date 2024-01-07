define <16 x i8> @f(<16 x i8> %0, <16 x i8> %1, <16 x i8> %2) {
  %4 = mul <16 x i8> %1, %2
  %5 = sub <16 x i8> %0, %4
  ret <16 x i8> %5
}
