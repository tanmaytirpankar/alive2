define <4 x i32> @smull_extvec_v4i16_v4i32(<4 x i16> %0) {
  %2 = sext <4 x i16> %0 to <4 x i32>
  %3 = mul <4 x i32> %2, <i32 -12, i32 -12, i32 -12, i32 -12>
  ret <4 x i32> %3
}
