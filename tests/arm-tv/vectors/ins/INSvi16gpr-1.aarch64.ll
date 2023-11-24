define <4 x i16> @low_index_same_length_basevec(i64 %0, <4 x i16> %1) {
  %3 = trunc i64 %0 to i16
  %4 = insertelement <4 x i16> %1, i16 %3, i64 0
  ret <4 x i16> %4
}
