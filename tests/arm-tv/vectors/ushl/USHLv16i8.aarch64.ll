define <16 x i8> @lshr_v16i8(<16 x i8> %0, <16 x i8> %1) {
  %3 = lshr <16 x i8> %0, %1
  ret <16 x i8> %3
}
